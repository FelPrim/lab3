#pragma once
#include "video_defaults.h"
#include <QThread>
#include <QImage>
#include <atomic>
#include <opencv2/opencv.hpp>

class CaptureThread : public QThread
{
    Q_OBJECT
public:
    explicit CaptureThread(QObject *parent = nullptr);
    ~CaptureThread() override;

    void startCapture(int deviceIndex);
    void stopCapture();

signals:
    void frameReady(const QImage &img);
    void errorOccurred(const QString &msg);

    // to encoder
    void frameCaptured(const cv::Mat &frame);

protected:
    void run() override;

private:
    std::atomic<bool> m_running{false};
    int m_deviceIndex = -1;
    int m_fps = DEFAULT_FPS;

    // threads and workers
    QThread *m_encoderThread = nullptr;
    QThread *m_decoderThread = nullptr;
    QObject *m_encoderWorker = nullptr; // actually EncoderWorker*
    QObject *m_decoderWorker = nullptr; // actually DecoderWorker*
};
