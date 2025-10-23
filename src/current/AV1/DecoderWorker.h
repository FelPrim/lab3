#pragma once

#include <QObject>
#include <QImage>
#include <QByteArray>
#include <atomic>
#include <opencv2/opencv.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

class DecoderWorker : public QObject
{
    Q_OBJECT
public:
    explicit DecoderWorker(int targetWidth, int targetHeight, QObject *parent = nullptr);
    ~DecoderWorker() override;

public slots:
    // receives encoded packet bytes from EncoderWorker (queued)
    void processPacket(const QByteArray &packet);

signals:
    // final image to GUI
    void frameReady(const QImage &img);
    void errorOccurred(const QString &msg);

private:
    void initFFmpeg();
    void cleanupFFmpeg();

    int m_targetWidth;
    int m_targetHeight;

    AVCodecContext *m_dec_ctx = nullptr;
    SwsContext *m_sws_dec = nullptr;
    AVFrame *m_dec_frame = nullptr;

    std::atomic<bool> busy{false};
};
