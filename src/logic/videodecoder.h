#pragma once

#define TEST_DECODER

#include <QObject>
#include <QImage>
#include <QByteArray>
#include <atomic>
#include "../video_defaults.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

class VideoDecoder : public QObject
{
    Q_OBJECT

public:
    explicit VideoDecoder(int targetWidth, int targetHeight, QObject *parent = nullptr);
    ~VideoDecoder() override;

    void initialize();
    void cleanup();
    bool isBusy() const { return m_busy.load(); }  // Добавляем метод проверки состояния

public slots:
    void decodeFrame(const QByteArray &frameData, int frameNumber);
    void decodeFrameInternal(const QByteArray &frameData, int frameNumber);

signals:
    void frameDecoded(const QImage &image, int frameNumber);
    void errorOccurred(const QString &message);

private:
    void initFFmpeg();
    void cleanupFFmpeg();

    int m_targetWidth;
    int m_targetHeight;

    AVCodecContext *m_dec_ctx = nullptr;
    SwsContext *m_sws_dec = nullptr;
    AVFrame *m_dec_frame = nullptr;

    std::atomic<bool> m_busy{false};
    bool m_initialized = false;
};
