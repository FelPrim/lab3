#pragma once
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

protected:
    void run() override;

private:
    std::atomic<bool> m_running{false};
    int m_deviceIndex = -1;
    int m_fps = 30;
};
