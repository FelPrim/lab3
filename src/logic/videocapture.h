#pragma once

#include <QThread>
#include <QImage>
#include <QElapsedTimer>
#include <opencv2/opencv.hpp>
#include "../video_defaults.h"

class VideoCapture : public QThread
{
    Q_OBJECT

public:
    explicit VideoCapture(int deviceIndex, QObject *parent = nullptr);
    ~VideoCapture() override;

    static QList<int> getAvailableDevices();
    void startCapture();
    void stopCapture();
    int getDeviceIndex() const { return m_deviceIndex; }
    bool isStable() const { return m_stable; }
    
    // Установка целевого FPS
    void setTargetFps(int fps) { 
        if (fps > 0 && fps <= 60) {
            m_targetFps = fps;
            m_frameIntervalMs = 1000 / m_targetFps;
        }
    }

signals:
    // Сигнал с исходным кадром для прямого показа
    void rawFrameReady(const QImage &image);
    // Сигнал с кадром для кодирования и отправки по сети
    void frameForEncodingReady(const cv::Mat &frame);
    void errorOccurred(const QString &message);

protected:
    void run() override;

private:
    int m_deviceIndex;
    std::atomic<bool> m_running{false};
    cv::VideoCapture m_capture;
    int m_targetFps = DEFAULT_FPS;  // Целевой FPS
    qint64 m_frameIntervalMs = 1000/DEFAULT_FPS; // Интервал между кадрами (66 мс для 15 FPS)
    
    // Для контроля стабильности
    std::atomic<bool> m_stable{false};
};