#include "videoencoder.h"
#include <QThread>
#include <QDebug>
#include <thread>
#include <algorithm>
#include "../video_defaults.h"

extern "C" {
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavutil/error.h>
}

static QByteArray avcExtradataToAnnexB(const uint8_t* extradata, int extradata_size) {
    QByteArray out;
    if (!extradata || extradata_size < 7) return out;
    // extradata format (AVCDecoderConfigurationRecord):
    // [0] configurationVersion == 1
    // [4] lengthSizeMinusOne (lower 2 bits)
    // [5] numOfSequenceParameterSets (lower 5 bits)
    int pos = 0;
    // sanity check
    if (extradata[0] != 1) return out;
    // lengthSizeMinusOne is at byte 4
    if (extradata_size < 7) return out;
    uint8_t lengthSizeMinusOne = extradata[4] & 0x03;
    int nalLenSize = static_cast<int>(lengthSizeMinusOne) + 1;

    pos = 5;
    uint8_t numSPS = extradata[pos++] & 0x1f; // lower 5 bits
    for (uint8_t i = 0; i < numSPS; ++i) {
        if (pos + 2 > extradata_size) return out;
        uint16_t sps_len = (extradata[pos] << 8) | extradata[pos + 1];
        pos += 2;
        if (pos + sps_len > extradata_size) return out;
        out.append("\x00\x00\x00\x01", 4);
        out.append(reinterpret_cast<const char*>(extradata + pos), sps_len);
        pos += sps_len;
    }
    if (pos >= extradata_size) return out;
    uint8_t numPPS = extradata[pos++];
    for (uint8_t i = 0; i < numPPS; ++i) {
        if (pos + 2 > extradata_size) return out;
        uint16_t pps_len = (extradata[pos] << 8) | extradata[pos + 1];
        pos += 2;
        if (pos + pps_len > extradata_size) return out;
        out.append("\x00\x00\x00\x01", 4);
        out.append(reinterpret_cast<const char*>(extradata + pos), pps_len);
        pos += pps_len;
    }
    return out;
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

    if (m_initialized) {
        qDebug() << "VideoEncoder: Already initialized, skipping reinitialization";
        return;
    }
    
    m_width = DEFAULT_WIDTH;
    m_height = DEFAULT_HEIGHT;
    m_fps = DEFAULT_FPS;
    
    initFFmpeg(DEFAULT_WIDTH, DEFAULT_HEIGHT, DEFAULT_FPS);
    if (!m_enc_ctx) {
        emit errorOccurred(QString("VideoEncoder stream %1: initFFmpeg failed").arg(m_streamId));
    } else {
        m_initialized = true;
        qDebug() << "VideoEncoder initialized for stream:" << m_streamId
                 << "codec:" << (m_enc_ctx && m_enc_ctx->codec ? m_enc_ctx->codec->name : "<null>")
                 << "bitrate:" << m_enc_ctx->bit_rate
                 << "threads:" << m_enc_ctx->thread_count;
    }

    m_encoderActive = true;
}

void VideoEncoder::cleanup()
{
    cleanupFFmpeg();
    m_initialized = false;
}

static QByteArray convertPacketToAnnexB(const AVPacket *pkt, int nalSizeBytes) {
    QByteArray out;
    if (!pkt || pkt->size <= 0 || !pkt->data) return out;

    const uint8_t *data = pkt->data;
    int size = pkt->size;

    // Если уже в Annex-B — просто скопировать
    if (size >= 4 && data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x00 && data[3] == 0x01) {
        out.append(reinterpret_cast<const char*>(data), size);
        return out;
    }
    if (size >= 3 && data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x01) {
        out.append(reinterpret_cast<const char*>(data), size);
        return out;
    }

    // Если у нас известен размер длины NAL (1..4) — парсим
    if (nalSizeBytes <= 0 || nalSizeBytes > 4) {
        // Невозможно корректно распарсить — fallback: копируем как есть
        out.append(reinterpret_cast<const char*>(data), size);
        return out;
    }

    int offset = 0;
    while (offset + nalSizeBytes <= size) {
        // big-endian length
        uint32_t nal_len = 0;
        for (int i = 0; i < nalSizeBytes; ++i) {
            nal_len = (nal_len << 8) | data[offset + i];
        }
        offset += nalSizeBytes;
        if (nal_len == 0) {
            // иногда встречаются нулевые длины — пропускаем
            continue;
        }
        if (offset + (int)nal_len > size) {
            // повреждённый пакет — останавливаемся
            break;
        }
        // Добавляем start code + payload
        out.append("\x00\x00\x00\x01", 4);
        out.append(reinterpret_cast<const char*>(data + offset), nal_len);
        offset += nal_len;
    }
    return out;
}

void VideoEncoder::initFFmpeg(int width, int height, int fps)
{
    qDebug() << "initFFMpeg";
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
    m_enc_ctx->bit_rate = DEFAULT_BITRATE;
    m_enc_ctx->flags |= AV_CODEC_FLAG_LOW_DELAY;
    
    // ОПТИМИЗАЦИИ ДЛЯ НИЗКОЙ ЗАДЕРЖКИ
    
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
    static bool first_time = true;
    if (first_time) {
        qDebug() << "VideoEncoder::encodeFrame was called for the first time\n";
        first_time = false;
    }

    // Сначала проверяем активность
    if (!m_encoderActive) {
        return;
    }

    // Затем проверяем инициализацию
    if (!m_initialized || !m_enc_ctx || !m_enc_frame || !m_pkt || !m_sws_enc) {
        qDebug() << "VideoEncoder: Not properly initialized";
        return;
    }

    // Затем захватываем busy flag
    bool expected = false;
    if (!m_busy.compare_exchange_strong(expected, true)) {
        // Уже кодируется другой кадр, пропускаем
        return;
    }

    // Guard ДОЛЖЕН быть объявлен сразу после захвата флага
    struct BusyGuard {
        std::atomic<bool>& busy;
        BusyGuard(std::atomic<bool>& b) : busy(b) {}
        ~BusyGuard() { busy.store( false, std::memory_order_release); }
    } guard(m_busy);

    // Теперь проверяем входной кадр
    cv::Mat frame = frame_in;
    if (frame.empty()) {
        qDebug() << "VideoEncoder: Empty frame received";
        return; // Guard освободит m_busy в деструкторе
    }

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
        cv::resize(bgr, resized, cv::Size(m_width, m_height), 0, 0, cv::INTER_LINEAR);
        bgr = resized;
    }

    if (bgr.cols < 16 || bgr.rows < 16) {
        qDebug() << "Bgr too small:" << bgr.cols << "x" << bgr.rows;
        return;
    }

    // Подготавливаем исходные указатели
    uint8_t *src_data[4] = { nullptr };
    int src_linesize[4] = { 0 };
    src_data[0] = bgr.data;
    src_linesize[0] = static_cast<int>(bgr.step);
    uint64_t sumBefore = 0;
    for (int y = 0; y < bgr.rows; ++y) {
        const uchar* row = bgr.ptr<uchar>(y);
        for (int x = 0; x < bgr.cols*3; ++x) sumBefore += row[x];
    }

    int got = sws_scale(m_sws_enc, src_data, src_linesize, 0, m_height,
                       m_enc_frame->data, m_enc_frame->linesize);
    if (got <= 0) {
        qDebug() << "VideoEncoder stream" << m_streamId << "sws_scale failed";
        return;
    }

    // Устанавливаем PTS и сохраняем текущий номер кадра (для совместимости)
    m_enc_frame->pts = m_pts++;
    int curFrameNumber = static_cast<int>(m_enc_frame->pts);

    AVFrame *sendFrame = av_frame_clone(m_enc_frame);
    int ret = avcodec_send_frame(m_enc_ctx, sendFrame);
    if (ret < 0) {
        qDebug() << "VideoEncoder stream" << m_streamId << "avcodec_send_frame failed:" << ffmpegErrStr(ret);
        av_frame_free(&sendFrame);
        return;
    }
    av_frame_free(&sendFrame);

    // Собираем все пакеты, относящиеся к кадрам, объединяя пакеты одного PTS
    QByteArray accum;                 // аккумулятор для текущего кадра (склеивает NAL units)
    int64_t accumPts = AV_NOPTS_VALUE; // PTS текущего аккумулятора
    bool accumIsKey = false;          // флаг, был ли в аккумуляторе ключевой NAL

    while (true) {
        ret = avcodec_receive_packet(m_enc_ctx, m_pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        }
        if (ret < 0) {
            qDebug() << "VideoEncoder stream" << m_streamId << "avcodec_receive_packet failed:" << ffmpegErrStr(ret);
            break;
        }

        // Игнорируем пустые пакеты
        if (m_pkt->size <= 0 || !m_pkt->data) {
            av_packet_unref(m_pkt);
            continue;
        }

        // Определяем PTS данного пакета (fallback на curFrameNumber)
        int64_t pktPts = (m_pkt->pts != AV_NOPTS_VALUE) ? m_pkt->pts : static_cast<int64_t>(curFrameNumber);

        // Если пакет относится к новому PTS и у нас есть накопленный кадр — эмитим его
        if (accumPts != AV_NOPTS_VALUE && pktPts != accumPts) {
            // Emit previous full frame
            QByteArray out = accum;
            if (!out.isEmpty()) {
                emit encodedPacketReady(m_streamId, m_currentFrameNumber, out);
                m_currentFrameNumber++;
            }

            // Сброс аккумулятора
            accum.clear();
            accumPts = AV_NOPTS_VALUE;
            accumIsKey = false;
        }

        // Если аккумулятор пуст — начинаем новый с текущим pktPts
        if (accumPts == AV_NOPTS_VALUE) {
            accumPts = pktPts;
        }

        // Добавляем данные пакета в аккумулятор.
        // Замечание: обычно avcodec возвращает NAL-ы уже в Annex-B или in-stream format;
        // мы просто конкатенируем как есть.
        if (m_pkt->size > 0 && m_pkt->data) {
            // Определяем размер поля длины NAL (AVCC) по extradata (если доступно)
            int nalSizeBytes = 4; // fallback
            if (m_enc_ctx && m_enc_ctx->extradata && m_enc_ctx->extradata_size >= 5) {
                // extradata[4] & 0x03 gives lengthSizeMinusOne
                nalSizeBytes = (m_enc_ctx->extradata[4] & 0x03) + 1;
            }

            QByteArray annex = convertPacketToAnnexB(m_pkt, nalSizeBytes);

            // Если это ключевой пакет и у нас есть extradata (SPS/PPS), — предоставить их перед IDR
            if ((m_pkt->flags & AV_PKT_FLAG_KEY) && m_enc_ctx && m_enc_ctx->extradata && m_enc_ctx->extradata_size > 0) {
                QByteArray header = avcExtradataToAnnexB(m_enc_ctx->extradata, m_enc_ctx->extradata_size);
                if (!header.isEmpty()) {
                    // Вставляем header перед данными кадра (если он ещё не вставлен в аккумулятор для этого PTS)
                    if (!accumIsKey) {
                        annex.prepend(header);
                    }
                }
            }

            accum.append(annex);
            if (m_pkt->flags & AV_PKT_FLAG_KEY) accumIsKey = true;
        }
        av_packet_unref(m_pkt);
    }

    // После выхода из цикла — если остаётся аккумулятор, отправляем его
if (accumPts != AV_NOPTS_VALUE && !accum.isEmpty()) {
    QByteArray out = std::move(accum);
    emit encodedPacketReady(m_streamId, m_currentFrameNumber, out);
    m_currentFrameNumber++;
}
}

void VideoEncoder::setStreamId(int streamId)
{
    m_streamId = streamId;
    qDebug() << "VideoEncoder: streamId updated to" << streamId;
}
