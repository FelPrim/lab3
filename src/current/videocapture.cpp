#include "videocapture.h"
#include <QDebug>
#include <QElapsedTimer> 

VideoCapture::VideoCapture(int deviceIndex, QObject *parent)
    : QThread(parent), m_deviceIndex(deviceIndex)
{
}

VideoCapture::~VideoCapture()
{
    stopCapture();
    wait();
}

void VideoCapture::startCapture()
{
    if (isRunning()) {
        stopCapture();
        wait();
    }

    m_running = true;
    start();
}

void VideoCapture::stopCapture()
{
    m_running = false;
}

void VideoCapture::run()
{
    qDebug() << "VideoCapture: starting device" << m_deviceIndex;
    
#ifdef _WIN32
    try {
        m_capture.open(m_deviceIndex, cv::CAP_DSHOW);
    } catch (const cv::Exception &e) {
        emit errorOccurred(QString("OpenCV exception: %1").arg(e.what()));
        return;
    }
#elif __linux__
    try {
        m_capture.open(m_deviceIndex, cv::CAP_V4L2);
    } catch (const cv::Exception &e) {
        emit errorOccurred(QString("OpenCV exception: %1").arg(e.what()));
        return;
    }
#elif __APPLE__
    try {
        m_capture.open(m_deviceIndex, cv::CAP_AVFOUNDATION);
    } catch (const cv::Exception &e) {
        emit errorOccurred(QString("OpenCV exception: %1").arg(e.what()));
        return;
    }
#else
    if (!m_capture.open(m_deviceIndex)) {
        emit errorOccurred(QString("Cannot open video device %1").arg(m_deviceIndex));
        return;
    }
#endif

    if (!m_capture.isOpened()) {
        emit errorOccurred(QString("Cannot open video device %1").arg(m_deviceIndex));
        return;
    }

    // Настраиваем параметры захвата
    m_capture.set(cv::CAP_PROP_FRAME_WIDTH, DEFAULT_WIDTH);
    m_capture.set(cv::CAP_PROP_FRAME_HEIGHT, DEFAULT_HEIGHT);
    m_capture.set(cv::CAP_PROP_FPS, m_fps);

    cv::Mat frame;
    QElapsedTimer timer;

    while (m_running) {
        timer.restart();
        
        if (!m_capture.read(frame) || frame.empty()) {
            QThread::msleep(5);
            continue;
        }

        // Конвертируем для прямого показа
        cv::Mat rgbFrame;
        cv::cvtColor(frame, rgbFrame, cv::COLOR_BGR2RGB);
        QImage image(rgbFrame.data, rgbFrame.cols, rgbFrame.rows, 
                    rgbFrame.step, QImage::Format_RGB888);
        
        // Отправляем для прямого показа
        emit rawFrameReady(image.copy());
        
        // Отправляем для кодирования и сети (оригинальный BGR кадр)
        emit frameForEncodingReady(frame.clone());

        // Контроль FPS
        int elapsed = timer.elapsed();
        int frameTime = 1000 / m_fps;
        int remaining = frameTime - elapsed;
        if (remaining > 0) {
            QThread::msleep(remaining);
        }
    }

    m_capture.release();
    qDebug() << "VideoCapture: stopped device" << m_deviceIndex;
}
