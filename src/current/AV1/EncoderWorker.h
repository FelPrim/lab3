#pragma once

#include "video_defaults.h"
#include <QObject>
#include <QByteArray>
#include <atomic>
#include <opencv2/opencv.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

class EncoderWorker : public QObject
{
    Q_OBJECT
public:
    explicit EncoderWorker(int width, int height, int fps, QObject *parent = nullptr);
    ~EncoderWorker() override;

public slots:
    // called from CaptureThread (queued)
    void processFrame(const cv::Mat &frame);

signals:
    // send encoded packet bytes to decoder (queued)
    void packetReady(const QByteArray &packet);
    void errorOccurred(const QString &msg);

private:
    void initFFmpeg(int width, int height, int fps);
    void cleanupFFmpeg();

    int m_width;
    int m_height;
    int m_fps;
    int m_bitrate = DEFAULT_BITRATE;

    AVCodecContext *m_enc_ctx = nullptr;
    SwsContext *m_sws_enc = nullptr;
    AVFrame *m_enc_frame = nullptr;
    AVPacket *m_pkt = nullptr;

    std::atomic<bool> busy{false};
    int64_t m_pts = 0;
};
