#include "EncoderWorker.h"
#include <QDebug>
#include <thread>   
#include <algorithm>
#include "video_defaults.h"

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

EncoderWorker::EncoderWorker(int width, int height, int fps, QObject *parent)
    : QObject(parent), m_width(width), m_height(height), m_fps(fps)
{
    initFFmpeg(width, height, fps);
}

EncoderWorker::~EncoderWorker()
{
    cleanupFFmpeg();
}

void EncoderWorker::initFFmpeg(int width, int height, int fps)
{
    const AVCodec *enc_codec = avcodec_find_encoder_by_name("libsvtav1");
    if (!enc_codec) enc_codec = avcodec_find_encoder(AV_CODEC_ID_AV1);
    if (!enc_codec) {
        emit errorOccurred("Encoder (libsvtav1/av1) not found in libavcodec.");
        return;
    }

    m_enc_ctx = avcodec_alloc_context3(enc_codec);
    if (!m_enc_ctx) {
        emit errorOccurred("Failed to allocate encoder context.");
        return;
    }

    m_enc_ctx->width = width;
    m_enc_ctx->height = height;
    m_enc_ctx->time_base = AVRational{1, fps};
    m_enc_ctx->framerate = AVRational{fps,1};
    m_enc_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    m_enc_ctx->gop_size = 12;
    m_enc_ctx->max_b_frames = 0;
    m_enc_ctx->bit_rate = m_bitrate; 

    int hw_threads = std::max(1u, std::thread::hardware_concurrency()-DEFAULT_HW_THREADS_SUBTRACT);
    m_enc_ctx->thread_count = hw_threads;

    // example options (may be ignored if unsupported)
    AVDictionary *opts = nullptr;
    if (strcmp(enc_codec->name, "libsvtav1") == 0) {
        av_dict_set(&opts, "preset", DEFAULT_SVT_PRESET, 0); 
    }

    int ret = avcodec_open2(m_enc_ctx, enc_codec, &opts);
    av_dict_free(&opts);
    if (ret < 0) {
        emit errorOccurred(QString("avcodec_open2 encoder failed: %1").arg(ffmpegErrStr(ret)));
        return;
    }

    m_sws_enc = sws_getContext(width, height, AV_PIX_FMT_BGR24,
                               width, height, m_enc_ctx->pix_fmt,
                               SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!m_sws_enc) {
        emit errorOccurred("sws_getContext (enc) failed");
        return;
    }

    m_enc_frame = av_frame_alloc();
    if (!m_enc_frame) {
        emit errorOccurred("av_frame_alloc (enc) failed");
        return;
    }
    m_enc_frame->format = m_enc_ctx->pix_fmt;
    m_enc_frame->width = m_enc_ctx->width;
    m_enc_frame->height = m_enc_ctx->height;
    ret = av_frame_get_buffer(m_enc_frame, 32);
    if (ret < 0) {
        emit errorOccurred(QString("av_frame_get_buffer failed: %1").arg(ffmpegErrStr(ret)));
        return;
    }

    m_pkt = av_packet_alloc();
    if (!m_pkt) {
        emit errorOccurred("av_packet_alloc failed");
        return;
    }

    qDebug() << "Encoder initialized:" << m_enc_ctx->codec->name
             << "bitrate=" << m_enc_ctx->bit_rate
             << "threads=" << m_enc_ctx->thread_count
             << "preset=" << DEFAULT_SVT_PRESET;
}

void EncoderWorker::cleanupFFmpeg()
{
    if (m_enc_ctx) {
        avcodec_send_frame(m_enc_ctx, nullptr);
        while (avcodec_receive_packet(m_enc_ctx, m_pkt) == 0) {
            // drop remaining packets (or you could send to decoder to flush)
            av_packet_unref(m_pkt);
        }
    }

    if (m_pkt) { av_packet_free(&m_pkt); m_pkt = nullptr; }
    if (m_enc_frame) { av_frame_free(&m_enc_frame); m_enc_frame = nullptr; }
    if (m_sws_enc) { sws_freeContext(m_sws_enc); m_sws_enc = nullptr; }
    if (m_enc_ctx) { avcodec_free_context(&m_enc_ctx); m_enc_ctx = nullptr; }
}

void EncoderWorker::processFrame(const cv::Mat &frame_in)
{
    bool expected = false;
    if (!busy.compare_exchange_strong(expected, true)) {
        // drop frame if busy
        return;
    }

    cv::Mat frame = frame_in;
    if (frame.empty()) { busy = false; return; }

    cv::Mat bgr;
    if (frame.channels() == 3) {
        bgr = frame;
    } else if (frame.channels() == 1) {
        cv::cvtColor(frame, bgr, cv::COLOR_GRAY2BGR);
    } else if (frame.channels() == 4) {
        cv::cvtColor(frame, bgr, cv::COLOR_BGRA2BGR);
    } else {
        busy = false;
        return;
    }

    if (bgr.cols != m_width || bgr.rows != m_height) {
        cv::Mat resized;
        cv::resize(bgr, resized, cv::Size(m_width, m_height));
        bgr = std::move(resized);
    }

    uint8_t *src_data[4] = { nullptr };
    int src_linesize[4] = { 0 };
    av_image_fill_arrays(src_data, src_linesize, bgr.data, AV_PIX_FMT_BGR24, m_width, m_height, 1);

    int got = sws_scale(m_sws_enc, src_data, src_linesize, 0, m_height, m_enc_frame->data, m_enc_frame->linesize);
    if (got <= 0) {
        qDebug() << "sws_scale failed in encoder";
        busy = false;
        return;
    }

    m_enc_frame->pts = m_pts++;

    int ret = avcodec_send_frame(m_enc_ctx, m_enc_frame);
    if (ret < 0) {
        qDebug() << "avcodec_send_frame failed:" << ffmpegErrStr(ret);
        busy = false;
        return;
    }

    // receive packets and emit them (as QByteArray)
    while (ret >= 0) {
        ret = avcodec_receive_packet(m_enc_ctx, m_pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
        if (ret < 0) {
            qDebug() << "avcodec_receive_packet failed:" << ffmpegErrStr(ret);
            break;
        }

        // copy packet data into QByteArray
        QByteArray bytes(reinterpret_cast<const char*>(m_pkt->data), m_pkt->size);
        emit packetReady(bytes);

        av_packet_unref(m_pkt);
    }

    busy = false;
}
