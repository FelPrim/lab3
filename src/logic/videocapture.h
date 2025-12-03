#pragma once

#include <QThread>
#include <QImage>
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
    float m_fps = DEFAULT_FPS;
public:
    // Статический метод для получения списка доступных устройств
public:
    bool isStable() const { return m_stable; }
    
private:
    std::atomic<bool> m_stable{false};
};
