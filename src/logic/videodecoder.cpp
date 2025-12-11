#include "videodecoder.h"
#include <QThread>
#include <QDebug>

extern "C" {
#include <libavutil/imgutils.h>
#include <libavutil/error.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}
#include <QCryptographicHash>

#define TESTING_NETCODE
#undef TESTING_NETCODE

static QString ffmpegErrStr(int errnum) {
    char buf[256] = {0};
    av_strerror(errnum, buf, sizeof(buf));
    return QString::fromUtf8(buf);
}

// --- Helper: try to detect & convert length-prefixed NALs to Annex-B ---
// If input already contains start codes, returns original QByteArray.
// Otherwise tries nalSize = 4,3,2,1 and returns converted Annex-B if parsing succeeds.
// If cannot parse, returns original input (fallback).
static QByteArray convertToAnnexBIfNeeded(const QByteArray &in)
{
    if (in.isEmpty()) return in;

    const uint8_t *data = reinterpret_cast<const uint8_t*>(in.constData());
    int size = in.size();

    // Quick check: if there's a start code near the beginning -> assume Annex-B already.
    if (size >= 4 && data[0] == 0x00 && data[1] == 0x00 &&
        (data[2] == 0x00 && data[3] == 0x01 || data[2] == 0x01)) {
        return in; // already Annex-B
    }

    // Also scan first 32 bytes for a start code anywhere (sometimes SPS/PPS at offset)
    int scanLimit = std::min(size, 32);
    for (int i = 0; i <= scanLimit - 3; ++i) {
        if (data[i] == 0x00 && data[i+1] == 0x00) {
            if ((i+2 < size && data[i+2] == 0x01) || (i+3 < size && data[i+2] == 0x00 && data[i+3] == 0x01)) {
                return in; // found start code -> treat as Annex-B
            }
        }
    }

    // Try different nal size interpretations (4..1)
    for (int nalSize = 4; nalSize >= 1; --nalSize) {
        int offset = 0;
        bool ok = true;
        QByteArray out;
        out.reserve(size + 64);
        while (offset + nalSize <= size) {
            // parse big-endian length
            uint32_t nal_len = 0;
            for (int i = 0; i < nalSize; ++i) {
                nal_len = (nal_len << 8) | data[offset + i];
            }
            offset += nalSize;

            // sanity checks
            if (nal_len == 0) { ok = false; break; }
            if (offset + (int)nal_len > size) { ok = false; break; }

            // append start code + payload
            out.append("\x00\x00\x00\x01", 4);
            out.append(reinterpret_cast<const char*>(data + offset), nal_len);
            offset += nal_len;
        }
        if (ok && offset == size) {
            return out; // successful conversion
        }
    }

    // Failed to parse as length-prefixed -> return original (best-effort)
    return in;
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

    // Оптимизации для низкой задержки (осторожно с флагами, они зависят от версии)
    m_dec_ctx->flags |= AV_CODEC_FLAG_LOW_DELAY;
    m_dec_ctx->flags |= AV_CODEC_FLAG_OUTPUT_CORRUPT;
    m_dec_ctx->flags2 |= AV_CODEC_FLAG2_FAST; 
    m_dec_ctx->error_concealment = FF_EC_GUESS_MVS | FF_EC_DEBLOCK;
    m_dec_ctx->delay = 0;   
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
        // flush decoder
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
#ifdef TESTING_NETCODE
    QByteArray sha = QCryptographicHash::hash(frameData, QCryptographicHash::Sha256);
    qDebug() << "VideoDecoder: frame" << frameNumber
           << "size:" << frameData.size() << "sha256:" << sha.toHex().left(64);
#endif
    struct BusyGuard {
        std::atomic<bool>& busy;
        BusyGuard(std::atomic<bool>& b) : busy(b) {}
        ~BusyGuard() { 
            busy.store(false, std::memory_order_release); 
        }
    } guard(m_busy);

    // Преобразуем данные в Annex-B формат если необходимо
    QByteArray packetData = convertToAnnexBIfNeeded(frameData);
    if (packetData.isEmpty()) {
        qDebug() << "VideoDecoder: packet data is empty after conversion";
        emit errorOccurred(QString("VideoDecoder: packet data empty for frame %1").arg(frameNumber));
        return;
    }

    // Создаем и заполняем AVPacket
    AVPacket *pkt = av_packet_alloc();
    if (!pkt) {
        emit errorOccurred("VideoDecoder: av_packet_alloc failed");
        return;
    }

    // Важно: используем av_packet_from_data для корректного управления памятью
    pkt->data = reinterpret_cast<uint8_t*>(av_malloc(packetData.size()));
    if (!pkt->data) {
        av_packet_free(&pkt);
        emit errorOccurred("VideoDecoder: av_malloc for packet data failed");
        return;
    }
    
    memcpy(pkt->data, packetData.constData(), packetData.size());
    pkt->size = packetData.size();

    // Отправляем пакет в декодер
    int ret = avcodec_send_packet(m_dec_ctx, pkt);
    if (ret < 0) {
        QString err = ffmpegErrStr(ret);
        qDebug() << "VideoDecoder: avcodec_send_packet failed for frame" << frameNumber 
                 << "error:" << err;
        emit errorOccurred(QString("VideoDecoder: avcodec_send_packet failed for frame %1: %2")
                           .arg(frameNumber).arg(err));
        // Освобождаем память пакета
        av_freep(&pkt->data);
        av_packet_free(&pkt);
        return;
    }


    // Освобождаем пакет после отправки (декодер делает внутреннюю копию)
    av_freep(&pkt->data);
    av_packet_free(&pkt);

    // Получаем декодированные фреймы
    while (true) {
        ret = avcodec_receive_frame(m_dec_ctx, m_dec_frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break; // Нет доступных фреймов или конец потока
        }
        
        if (ret < 0) {
            QString err = ffmpegErrStr(ret);
            qDebug() << "VideoDecoder: avcodec_receive_frame failed for frame" << frameNumber
                     << "error:" << err;
            emit errorOccurred(QString("VideoDecoder: avcodec_receive_frame failed for frame %1: %2")
                               .arg(frameNumber).arg(err));
            av_frame_unref(m_dec_frame);
            break;
        }


        // Получаем параметры декодированного фрейма
        int in_w = m_dec_frame->width;
        int in_h = m_dec_frame->height;
        AVPixelFormat in_fmt = static_cast<AVPixelFormat>(m_dec_frame->format);
        
        if (in_w <= 0 || in_h <= 0) {
            qDebug() << "VideoDecoder: invalid frame dimensions" << in_w << "x" << in_h;
            av_frame_unref(m_dec_frame);
            continue;
        }

        // Вычисляем выходные размеры
        int out_w = (m_targetWidth > 0) ? m_targetWidth : in_w;
        int out_h = (m_targetHeight > 0) ? m_targetHeight : in_h;

        // Проверяем нужно ли пересоздавать SwsContext
        bool needRecreate = false;
        if (!m_sws_dec) {
            needRecreate = true;
        } else if (m_sws_in_w != in_w || m_sws_in_h != in_h || 
                   m_sws_in_fmt != in_fmt || 
                   m_sws_out_w != out_w || m_sws_out_h != out_h) {
            needRecreate = true;
        }

        if (needRecreate) {
            if (m_sws_dec) {
                sws_freeContext(m_sws_dec);
                m_sws_dec = nullptr;
            }
            
            m_sws_dec = sws_getContext(
                in_w, in_h, in_fmt,
                out_w, out_h, AV_PIX_FMT_RGB24,
                SWS_BILINEAR, nullptr, nullptr, nullptr
            );
            
            if (!m_sws_dec) {
                emit errorOccurred(QString("VideoDecoder: sws_getContext failed for %1x%2")
                                   .arg(in_w).arg(in_h));
                av_frame_unref(m_dec_frame);
                continue;
            }
            
            // Кэшируем параметры
            m_sws_in_w = in_w;
            m_sws_in_h = in_h;
            m_sws_in_fmt = in_fmt;
            m_sws_out_w = out_w;
            m_sws_out_h = out_h;
            
            qDebug() << "VideoDecoder: created new SwsContext for" << in_w << "x" << in_h 
                     << "->" << out_w << "x" << out_h;
        }

        // Выделяем память для RGB изображения
        uint8_t *dst_data[4] = { nullptr };
        int dst_linesize[4] = { 0 };
        
        int buffer_size = av_image_alloc(dst_data, dst_linesize, 
                                         out_w, out_h, AV_PIX_FMT_RGB24, 1);
        if (buffer_size < 0) {
            emit errorOccurred(QString("VideoDecoder: av_image_alloc failed for %1x%2")
                               .arg(out_w).arg(out_h));
            av_frame_unref(m_dec_frame);
            continue;
        }

        // Конвертируем YUV -> RGB
        int got = sws_scale(m_sws_dec, 
                           m_dec_frame->data, m_dec_frame->linesize,
                           0, in_h,
                           dst_data, dst_linesize);
        
        if (got <= 0) {
            qDebug() << "VideoDecoder: sws_scale failed for frame" << frameNumber;
            av_freep(&dst_data[0]); // Освобождаем выделенную память
            av_frame_unref(m_dec_frame);
            continue;
        }

        // Создаем QImage
        QImage img(dst_data[0], out_w, out_h, dst_linesize[0], QImage::Format_RGB888);
        if (img.isNull()) {
            qWarning() << "VideoDecoder: Failed to create QImage for frame" << frameNumber;
        } else {
            // Создаем копию, так как img ссылается на данные, которые мы скоро освободим
            QImage imgCopy = img.copy();
            emit frameDecoded(imgCopy, frameNumber);
        }

        // Освобождаем выделенную память
        av_freep(&dst_data[0]);
        
        // Сбрасываем фрейм для следующей итерации
        av_frame_unref(m_dec_frame);
    }
}

void VideoDecoder::decodeFrame(const QByteArray &frameData, int frameNumber)
{
    if (!m_initialized) return;
    if (frameData.isEmpty()) return;

    // Попытаемся декодировать всё — без строгой предварительной проверки NAL'ов
    decodeFrameInternal(frameData, frameNumber);
}
