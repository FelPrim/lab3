#include "videodecoder.h"
#include <QThread>
#include <QDebug>

extern "C" {
#include <libavutil/imgutils.h>
#include <libavutil/error.h>
#include <libavcodec/avcodec.h>
}

static QString ffmpegErrStr(int errnum) {
    char buf[256] = {0};
    av_strerror(errnum, buf, sizeof(buf));
    return QString::fromUtf8(buf);
}

VideoDecoder::VideoDecoder(int targetWidth, int targetHeight, QObject *parent)
    : QObject(parent), m_targetWidth(targetWidth), m_targetHeight(targetHeight)
{
}

VideoDecoder::~VideoDecoder()
{
    cleanupFFmpeg();
}

void VideoDecoder::initialize()
{
    qDebug() << "VideoDecoder::initialize() in thread" << QThread::currentThread();
    initFFmpeg();
    if (!m_dec_ctx || !m_dec_frame) {
        emit errorOccurred("VideoDecoder: initFFmpeg failed");
    } else {
        m_initialized = true;
        qDebug() << "VideoDecoder initialized";
    }
}

void VideoDecoder::cleanup()
{
    cleanupFFmpeg();
    m_initialized = false;
}

void VideoDecoder::initFFmpeg()
{
    cleanupFFmpeg();

    const AVCodec *dec_codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!dec_codec) {
        emit errorOccurred("H.264 decoder not found");
        return;
    }

    m_dec_ctx = avcodec_alloc_context3(dec_codec);
    if (!m_dec_ctx) {
        emit errorOccurred("avcodec_alloc_context3 (dec) failed");
        return;
    }

    // ОПТИМИЗАЦИИ ДЛЯ НИЗКОЙ ЗАДЕРЖКИ
    m_dec_ctx->flags |= AV_CODEC_FLAG_LOW_DELAY;
	m_dec_ctx->flags |= AV_CODEC_FLAG_OUTPUT_CORRUPT;
	m_dec_ctx->flags |= AV_CODEC_FLAG2_SHOW_ALL;
    m_dec_ctx->flags2 |= AV_CODEC_FLAG2_FAST;

	m_dec_ctx->error_concealment = FF_EC_GUESS_MVS | FF_EC_DEBLOCK;
    
    // Уменьшаем размер буфера
    m_dec_ctx->delay = 0;
    
    // Используем меньше потоков для снижения накладных расходов
    m_dec_ctx->thread_count = 1;

    int ret = avcodec_open2(m_dec_ctx, dec_codec, nullptr);
    if (ret < 0) {
        QString err = ffmpegErrStr(ret);
        avcodec_free_context(&m_dec_ctx);
        m_dec_ctx = nullptr;
        emit errorOccurred(QString("avcodec_open2(dec) failed: %1").arg(err));
        return;
    }

    m_dec_frame = av_frame_alloc();
    if (!m_dec_frame) {
        emit errorOccurred("av_frame_alloc (dec) failed");
        cleanupFFmpeg();
        return;
    }

    qDebug() << "✅ VideoDecoder optimized for low latency";
}

void VideoDecoder::cleanupFFmpeg()
{
    if (m_dec_ctx) {
        avcodec_send_packet(m_dec_ctx, nullptr);
        while (avcodec_receive_frame(m_dec_ctx, m_dec_frame) == 0) {
            av_frame_unref(m_dec_frame);
        }
        avcodec_free_context(&m_dec_ctx);
        m_dec_ctx = nullptr;
    }

    if (m_dec_frame) {
        av_frame_free(&m_dec_frame);
        m_dec_frame = nullptr;
    }

    if (m_sws_dec) {
        sws_freeContext(m_sws_dec);
        m_sws_dec = nullptr;
    }

    m_busy = false;
}

void VideoDecoder::decodeFrameInternal(const QByteArray &frameData, int frameNumber)
{
    if (!m_initialized || !m_dec_ctx || !m_dec_frame) {
        qWarning() << "VideoDecoder not initialized";
        return;
    }

    if (frameData.isEmpty()) {
        qDebug() << "VideoDecoder: empty frame data";
        return;
    }

    bool expected = false;
    if (!m_busy.compare_exchange_strong(expected, true)) {
        qDebug() << "VideoDecoder: busy, skipping frame" << frameNumber;
        return;
    }

    // Автоматическое освобождение busy с защитой от исключений
    struct BusyGuard {
        std::atomic<bool>& busy;
        BusyGuard(std::atomic<bool>& b) : busy(b) {}
        ~BusyGuard() { 
            busy.store(false);
        }
    } guard(m_busy);

    try {
        AVPacket *pkt = av_packet_alloc();
        if (!pkt) {
            emit errorOccurred("av_packet_alloc failed");
            return;
        }

        // Выделяем память с защитой
        if (av_new_packet(pkt, frameData.size()) < 0) {
            av_packet_free(&pkt);
            emit errorOccurred("av_new_packet failed");
            return;
        }
        memcpy(pkt->data, frameData.constData(), frameData.size());

        int ret = avcodec_send_packet(m_dec_ctx, pkt);
        if (ret < 0) {
            qDebug() << "avcodec_send_packet failed:" << ffmpegErrStr(ret);
           // av_free(pkt->data);  // ДОБАВИТЬ ЭТУ СТРОКУ
            av_packet_free(&pkt);
            return;
        }

        while (ret >= 0) {
            ret = avcodec_receive_frame(m_dec_ctx, m_dec_frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
            if (ret < 0) {
                qDebug() << "avcodec_receive_frame failed:" << ffmpegErrStr(ret);
                break;
            }

            // Создаем контекст масштабирования если нужно
            if (!m_sws_dec) {
		 /*
                m_sws_dec = sws_getContext(m_dec_frame->width, m_dec_frame->height,
                                        (AVPixelFormat)m_dec_frame->format,
                                        m_targetWidth, m_targetHeight, AV_PIX_FMT_BGR24,
                                        SWS_BILINEAR, nullptr, nullptr, nullptr);
		*/
		    m_sws_dec = sws_getContext(m_dec_frame->width, m_dec_frame->height,
                            (AVPixelFormat)m_dec_frame->format,
                            m_dec_frame->width, m_dec_frame->height, AV_PIX_FMT_BGR24,  
                            SWS_BILINEAR, nullptr, nullptr, nullptr);
                if (!m_sws_dec) {
                    emit errorOccurred("sws_getContext(dec) failed");
                    av_frame_unref(m_dec_frame);
                    continue;
                }
            }

            uint8_t *dst_data[4] = { nullptr };
            int dst_linesize[4] = { 0 };
            int bufsize = av_image_alloc(dst_data, dst_linesize, m_targetWidth, m_targetHeight, AV_PIX_FMT_BGR24, 1);
            if (bufsize < 0) {
                qDebug() << "av_image_alloc failed";
                av_frame_unref(m_dec_frame);
                continue;
            }

            int got = sws_scale(m_sws_dec, m_dec_frame->data, m_dec_frame->linesize, 0,
                                m_dec_frame->height, dst_data, dst_linesize);
            if (got <= 0) {
                qDebug() << "sws_scale decode->bgr failed";
                av_freep(&dst_data[0]);
                av_frame_unref(m_dec_frame);
                continue;
            }

            // Создаем QImage и эмитируем сигнал
            // QImage img(dst_data[0], m_targetWidth, m_targetHeight, dst_linesize[0], QImage::Format_BGR888);
            QImage img(dst_data[0], m_dec_frame->width, m_dec_frame->height, dst_linesize[0], QImage::Format_BGR888);
	    if (!img.isNull()) {
                emit frameDecoded(img.copy(), frameNumber);
            } else {
                qWarning() << "Failed to create QImage from decoded data for frame" << frameNumber;
            }

            av_freep(&dst_data[0]);
            av_frame_unref(m_dec_frame);
        }

        av_free(pkt->data);
        av_packet_free(&pkt);
        
    } catch (const std::exception& e) {
        qCritical() << "Exception in VideoDecoder::decodeFrame:" << e.what();
        emit errorOccurred(QString("Decoder exception: %1").arg(e.what()));
    } catch (...) {
        qCritical() << "Unknown exception in VideoDecoder::decodeFrame";
        emit errorOccurred("Unknown decoder exception");
    }
}

void VideoDecoder::decodeFrame(const QByteArray &frameData, int frameNumber)
{
	/*
    if (!m_initialized || !m_dec_ctx || !m_dec_frame) {
        return;
    }

    // Проверяем данные перед декодированием
    if (frameData.isEmpty() || frameData.size() < 4) {
        qDebug() << "Invalid frame data for decoding - too small";
        return;
    }
    
    // Проверяем наличие NAL unit starters в данных H.264
    bool hasValidH264Data = false;
    const char* data = frameData.constData();
    for (int i = 0; i <= frameData.size() - 4; ++i) {
        if ((data[i] == 0x00 && data[i+1] == 0x00 && data[i+2] == 0x00 && data[i+3] == 0x01) ||
            (data[i] == 0x00 && data[i+1] == 0x00 && data[i+2] == 0x01)) {
            hasValidH264Data = true;
            break;
        }
    }
    
    if (!hasValidH264Data) {
        qDebug() << "Invalid H.264 data - no NAL unit starters found for frame" << frameNumber;
        return;
    }
    
    // Декодируем только если данные валидны
    decodeFrameInternal(frameData, frameNumber);
*/
	if (!m_initialized) return;

    // ПРОПУСК ПРОВЕРОК ДЛЯ ЛУЧШЕГО ВОССТАНОВЛЕНИЯ:
    // Убираем строгие проверки H.264 данных - пытаемся декодировать всё
    if (frameData.isEmpty()) return;
    
    // Пропускаем проверку на NAL unit starters
    // Даже поврежденные данные пытаемся декодировать
    
    decodeFrameInternal(frameData, frameNumber);
}
