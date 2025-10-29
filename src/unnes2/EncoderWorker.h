
#pragma once

#include <QObject>
#include <QByteArray>
#include <atomic>
#include <opencv2/opencv.hpp>
#include "video_defaults.h"

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
    void initialize();                 
    void processFrame(const cv::Mat &frame);

signals:
    void packetReady(const QByteArray &packet);
    void errorOccurred(const QString &msg);
    void initialized();                 

private:
    void initFFmpeg(int width, int height, int fps);
    void cleanupFFmpeg();

    int m_width;
    int m_height;
    int m_fps;
    int m_bitrate = DEFAULT_BITRATE;

    // FFmpeg objects
    AVCodecContext *m_enc_ctx = nullptr;
    SwsContext *m_sws_enc = nullptr;
    AVFrame *m_enc_frame = nullptr;
    AVPacket *m_pkt = nullptr;

    std::atomic<bool> busy{false};
    int64_t m_pts = 0;
};
