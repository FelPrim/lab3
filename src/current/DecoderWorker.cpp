#include "DecoderWorker.h"
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

DecoderWorker::DecoderWorker(int targetWidth, int targetHeight, QObject *parent)
    : QObject(parent), m_targetWidth(targetWidth), m_targetHeight(targetHeight)
{
}

void DecoderWorker::initialize()
{
    qDebug() << "DecoderWorker::initialize() in thread" << QThread::currentThread();
    initFFmpeg();
    if (!m_dec_ctx || !m_dec_frame) {
        emit errorOccurred("DecoderWorker: initFFmpeg failed");
    } else {
        qDebug() << "Decoder initialized: ctx=" << (void*)m_dec_ctx;
        emit initialized();
    }
}

DecoderWorker::~DecoderWorker()
{
    cleanupFFmpeg();
}

void DecoderWorker::initFFmpeg()
{
    // Defensive cleanup
    cleanupFFmpeg();

    const AVCodec *dec_codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!dec_codec) {
        emit errorOccurred("H.264 decoder not found in linked libavcodec.");
        return;
    }

    m_dec_ctx = avcodec_alloc_context3(dec_codec);
    if (!m_dec_ctx) {
        emit errorOccurred("avcodec_alloc_context3 (dec) failed");
        return;
    }

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

    qDebug() << "DecoderWorker initialized: codec=" << (m_dec_ctx && m_dec_ctx->codec ? m_dec_ctx->codec->name : "<null>");
}

void DecoderWorker::cleanupFFmpeg()
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

    busy = false;
}

void DecoderWorker::processPacket(const QByteArray &packet, int frameNumber) {
	qDebug() << "DecoderWorker::processPacket called, frame:" << frameNumber << "size:" << packet.size();
    
    if (!m_dec_ctx || !m_dec_frame) {
        qWarning() << "DecoderWorker: decoder not initialized";
        return;
    }

    if (packet.isEmpty()) {
        qDebug() << "DecoderWorker: empty packet";
        return;
    }

    bool expected = false;
    if (!busy.compare_exchange_strong(expected, true)) {
        // drop packet if busy
        return;
    }

    if (packet.isEmpty()) {
        busy = false;
        return;
    }

    AVPacket *pkt = av_packet_alloc();
    if (!pkt) {
        busy = false;
        emit errorOccurred("av_packet_alloc failed");
        return;
    }

    pkt->data = (uint8_t*)av_malloc(packet.size() + AV_INPUT_BUFFER_PADDING_SIZE);
    if (!pkt->data) {
        av_packet_free(&pkt);
        busy = false;
        emit errorOccurred("av_malloc failed for packet");
        return;
    }
    memcpy(pkt->data, packet.constData(), packet.size());
    pkt->size = packet.size();

    int ret = avcodec_send_packet(m_dec_ctx, pkt);
    if (ret < 0) {
        qDebug() << "avcodec_send_packet failed:" << ffmpegErrStr(ret);
        av_free(pkt->data);
        av_packet_free(&pkt);
        busy = false;
        return;
    }

    while (ret >= 0) {
        ret = avcodec_receive_frame(m_dec_ctx, m_dec_frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
        if (ret < 0) {
            qDebug() << "avcodec_receive_frame failed:" << ffmpegErrStr(ret);
            break;
        }

        if (!m_sws_dec) {
            m_sws_dec = sws_getContext(m_dec_frame->width, m_dec_frame->height,
                                       (AVPixelFormat)m_dec_frame->format,
                                       m_targetWidth, m_targetHeight, AV_PIX_FMT_RGB24,
                                       SWS_BILINEAR, nullptr, nullptr, nullptr);
            if (!m_sws_dec) {
                emit errorOccurred("sws_getContext(dec) failed");
                av_frame_unref(m_dec_frame);
                continue;
            }
        }

        uint8_t *dst_data[4] = { nullptr };
        int dst_linesize[4] = { 0 };
        int bufsize = av_image_alloc(dst_data, dst_linesize, m_targetWidth, m_targetHeight, AV_PIX_FMT_RGB24, 1);
        if (bufsize < 0) {
            qDebug() << "av_image_alloc failed";
            av_frame_unref(m_dec_frame);
            continue;
        }

        int got = sws_scale(m_sws_dec, m_dec_frame->data, m_dec_frame->linesize, 0,
                            m_dec_frame->height, dst_data, dst_linesize);
        if (got <= 0) {
            qDebug() << "sws_scale decode->rgb failed";
            av_freep(&dst_data[0]);
            av_frame_unref(m_dec_frame);
            continue;
        }

        QImage img(dst_data[0], m_targetWidth, m_targetHeight, dst_linesize[0], QImage::Format_RGB888);
        emit frameReady(img.copy());

        av_freep(&dst_data[0]);
        av_frame_unref(m_dec_frame);
    }

    av_free(pkt->data);
    av_packet_free(&pkt);

    busy = false;
	qDebug() << "DecoderWorker::processPacket finished for frame:" << frameNumber;
}
