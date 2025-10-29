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

class VideoEncoder : public QObject
{
    Q_OBJECT

public:
    explicit VideoEncoder(int streamId, QObject *parent = nullptr);
    ~VideoEncoder() override;

    void initialize(int width, int height, int fps);
    void cleanup();

public slots:
    void encodeFrame(const cv::Mat &frame);

signals:

    void encodedPacketReady(int streamId, int frameNumber, const QByteArray &packet);
    void errorOccurred(const QString &message);

private:
    void initFFmpeg(int width, int height, int fps);
    void cleanupFFmpeg();

    int m_streamId;  // ID видеопотока для идентификации
    int m_width;
    int m_height;
    int m_fps;
    int m_bitrate = DEFAULT_BITRATE;
    int m_currentFrameNumber = 0;  // Добавляем счетчик кадров

    // FFmpeg objects (сохраняем из EncoderWorker)
    AVCodecContext *m_enc_ctx = nullptr;
    SwsContext *m_sws_enc = nullptr;
    AVFrame *m_enc_frame = nullptr;
    AVPacket *m_pkt = nullptr;

    std::atomic<bool> m_busy{false};
    int64_t m_pts = 0;
    bool m_initialized = false;
};
