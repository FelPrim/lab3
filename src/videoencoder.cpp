#include "videoencoder.h"
#include <QThread>
#include <QDebug>
#include <thread>
#include <algorithm>

extern "C" {
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavutil/error.h>
}

static QString ffmpegErrStr(int errnum) {
    char buf[256] = {0};
    av_strerror(errnum, buf, sizeof(buf));
    return QString::fromUtf8(buf);
}

VideoEncoder::VideoEncoder(int streamId, QObject *parent)
    : QObject(parent), m_streamId(streamId)
{
    qDebug() << "VideoEncoder created for stream:" << streamId;
}

VideoEncoder::~VideoEncoder()
{
    cleanupFFmpeg();
    qDebug() << "VideoEncoder destroyed for stream:" << m_streamId;
}

void VideoEncoder::initialize(int width, int height, int fps)
{
    qDebug() << "VideoEncoder::initialize() for stream" << m_streamId 
             << "in thread" << QThread::currentThread()
             << "dimensions:" << width << "x" << height << "fps:" << fps;
    
    m_width = width;
    m_height = height;
    m_fps = fps;
    
    initFFmpeg(width, height, fps);
    if (!m_enc_ctx) {
        emit errorOccurred(QString("VideoEncoder stream %1: initFFmpeg failed").arg(m_streamId));
    } else {
        m_initialized = true;
        qDebug() << "VideoEncoder initialized for stream:" << m_streamId
                 << "codec:" << (m_enc_ctx && m_enc_ctx->codec ? m_enc_ctx->codec->name : "<null>")
                 << "bitrate:" << m_enc_ctx->bit_rate
                 << "threads:" << m_enc_ctx->thread_count;
    }
}

void VideoEncoder::cleanup()
{
    cleanupFFmpeg();
    m_initialized = false;
}

void VideoEncoder::initFFmpeg(int width, int height, int fps)
{
    if (width <= 0 || height <= 0) {
        emit errorOccurred(QString("VideoEncoder stream %1: invalid dimensions %2x%3")
                          .arg(m_streamId).arg(width).arg(height));
        return;
    }

    cleanupFFmpeg();

    // Пробуем разные кодеки в порядке приоритета
    const AVCodec *enc_codec = avcodec_find_encoder_by_name("libx264");
    if (!enc_codec) enc_codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    
    if (!enc_codec) {
        emit errorOccurred(QString("VideoEncoder stream %1: H.264 encoder not found").arg(m_streamId));
        return;
    }

    m_enc_ctx = avcodec_alloc_context3(enc_codec);
    if (!m_enc_ctx) {
        emit errorOccurred(QString("VideoEncoder stream %1: failed to allocate encoder context").arg(m_streamId));
        return;
    }

    // ОСНОВНЫЕ НАСТРОЙКИ ДЛЯ БЫСТРОГО СТАРТА
    m_enc_ctx->width = width;
    m_enc_ctx->height = height;
    m_enc_ctx->time_base = AVRational{1, fps};
    m_enc_ctx->framerate = AVRational{fps, 1};
    m_enc_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    m_enc_ctx->gop_size = fps;          // Маленький GOP для быстрого восстановления
    m_enc_ctx->max_b_frames = 0;       // Без B-фреймов для низкой задержки
    m_enc_ctx->refs = 1;               // Минимальное количество reference фреймов
    m_enc_ctx->bit_rate = m_bitrate;
    m_enc_ctx->flags |= AV_CODEC_FLAG_LOW_DELAY;
    
    // ОПТИМИЗАЦИИ ДЛЯ НИЗКОЙ ЗАДЕРЖКИ
    m_enc_ctx->rc_buffer_size = 0;
    m_enc_ctx->rc_initial_buffer_occupancy = 0;
    
    // УМЕНЬШАЕМ количество потоков для стабильности
    m_enc_ctx->thread_count = 1;

    AVDictionary *enc_opts = nullptr;
    if (enc_codec && strstr(enc_codec->name, "x264")) {
        av_dict_set(&enc_opts, "preset", "ultrafast", 0);  // САМЫЙ БЫСТРЫЙ
        av_dict_set(&enc_opts, "tune", "zerolatency", 0);  // Нулевая задержка
        av_dict_set(&enc_opts, "crf", "25", 0);           // Немного выше CRF для скорости
    }

    int ret = avcodec_open2(m_enc_ctx, enc_codec, &enc_opts);
    av_dict_free(&enc_opts);

    if (ret < 0) {
        QString err = ffmpegErrStr(ret);
        avcodec_free_context(&m_enc_ctx);
        m_enc_ctx = nullptr;
        emit errorOccurred(QString("VideoEncoder stream %1: avcodec_open2 failed: %2").arg(m_streamId).arg(err));
        return;
    }

    // Остальная инициализация без изменений...
    m_sws_enc = sws_getContext(width, height, AV_PIX_FMT_BGR24,
                               width, height, m_enc_ctx->pix_fmt,
                               SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!m_sws_enc) {
        emit errorOccurred(QString("VideoEncoder stream %1: sws_getContext failed").arg(m_streamId));
        cleanupFFmpeg();
        return;
    }

    m_enc_frame = av_frame_alloc();
    if (!m_enc_frame) {
        emit errorOccurred(QString("VideoEncoder stream %1: av_frame_alloc failed").arg(m_streamId));
        cleanupFFmpeg();
        return;
    }
    m_enc_frame->format = m_enc_ctx->pix_fmt;
    m_enc_frame->width  = m_enc_ctx->width;
    m_enc_frame->height = m_enc_ctx->height;

    ret = av_frame_get_buffer(m_enc_frame, 32);
    if (ret < 0) {
        emit errorOccurred(QString("VideoEncoder stream %1: av_frame_get_buffer failed: %2")
                          .arg(m_streamId).arg(ffmpegErrStr(ret)));
        cleanupFFmpeg();
        return;
    }

    m_pkt = av_packet_alloc();
    if (!m_pkt) {
        emit errorOccurred(QString("VideoEncoder stream %1: av_packet_alloc failed").arg(m_streamId));
        cleanupFFmpeg();
        return;
    }

    m_pts = 0;
    m_busy = false;
    
    qDebug() << "✅ VideoEncoder ULTRAFAST preset for stream:" << m_streamId;
}

void VideoEncoder::cleanupFFmpeg()
{
    if (m_enc_ctx) {
        avcodec_send_frame(m_enc_ctx, nullptr);
        if (m_pkt) {
            while (avcodec_receive_packet(m_enc_ctx, m_pkt) == 0) {
                av_packet_unref(m_pkt);
            }
        }
        avcodec_free_context(&m_enc_ctx);
        m_enc_ctx = nullptr;
    }

    if (m_pkt) {
        av_packet_free(&m_pkt);
        m_pkt = nullptr;
    }

    if (m_enc_frame) {
        av_frame_free(&m_enc_frame);
        m_enc_frame = nullptr;
    }

    if (m_sws_enc) {
        sws_freeContext(m_sws_enc);
        m_sws_enc = nullptr;
    }

    // Сбрасываем pts и busy флаг
    m_pts = 0;
    m_busy = false;
}

void VideoEncoder::encodeFrame(const cv::Mat &frame_in)
{
    if (!m_initialized || !m_enc_ctx || !m_enc_frame || !m_pkt || !m_sws_enc) {
        return;
    }

    bool expected = false;
    if (!m_busy.compare_exchange_strong(expected, true)) {
        return;
    }

    struct BusyGuard {
        std::atomic<bool>& busy;
        BusyGuard(std::atomic<bool>& b) : busy(b) {}
        ~BusyGuard() { busy = false; }
    } guard(m_busy);

    cv::Mat frame = frame_in;
    if (frame.empty()) return;

    cv::Mat bgr;
    if (frame.channels() == 3) {
        bgr = frame;
    } else if (frame.channels() == 1) {
        cv::cvtColor(frame, bgr, cv::COLOR_GRAY2BGR);
    } else if (frame.channels() == 4) {
        cv::cvtColor(frame, bgr, cv::COLOR_BGRA2BGR);
    } else {
        return;
    }

    // Ресайз если необходимо
    if (bgr.cols != m_width || bgr.rows != m_height) {
        cv::Mat resized;
        cv::resize(bgr, resized, cv::Size(m_width, m_height));
        bgr = std::move(resized);
    }

    // Подготавливаем исходные указатели
    uint8_t *src_data[4] = { nullptr };
    int src_linesize[4] = { 0 };
    av_image_fill_arrays(src_data, src_linesize, bgr.data, AV_PIX_FMT_BGR24, m_width, m_height, 1);

    int got = sws_scale(m_sws_enc, src_data, src_linesize, 0, m_height, 
                       m_enc_frame->data, m_enc_frame->linesize);
    if (got <= 0) {
        qDebug() << "VideoEncoder stream" << m_streamId << "sws_scale failed";
        return;
    }

    m_enc_frame->pts = m_pts++;

    int ret = avcodec_send_frame(m_enc_ctx, m_enc_frame);
    if (ret < 0) {
        qDebug() << "VideoEncoder stream" << m_streamId << "avcodec_send_frame failed:" << ffmpegErrStr(ret);
        return;
    }

    // Получаем пакеты произведенные энкодером и эмитируем их
    while (ret >= 0) {
        ret = avcodec_receive_packet(m_enc_ctx, m_pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
        if (ret < 0) {
            qDebug() << "VideoEncoder stream" << m_streamId << "avcodec_receive_packet failed:" << ffmpegErrStr(ret);
            break;
        }

        QByteArray encodedData(reinterpret_cast<const char*>(m_pkt->data), m_pkt->size);
        
        // Исправляем сигнал - правильный порядок параметров
        emit encodedPacketReady(m_streamId, m_currentFrameNumber, encodedData);
        m_currentFrameNumber++;  // Увеличиваем счетчик кадров
        
        av_packet_unref(m_pkt);
    }
}
