#include "capturethread.h"
#include <QDebug>

CaptureThread::CaptureThread(QObject *parent)
    : QThread(parent)
{
}

CaptureThread::~CaptureThread()
{
    stopCapture();
    wait();
}

void CaptureThread::startCapture(int deviceIndex)
{
    if (deviceIndex < 0) {
        emit errorOccurred("Неверный индекс устройства");
        return;
    }

    if (isRunning()) {
        m_running = false;
        wait();
    }

    m_deviceIndex = deviceIndex;
    m_running = true;
    start();
}

void CaptureThread::stopCapture()
{
    m_running = false;
}

void CaptureThread::run()
{
    qDebug() << "CaptureThread: старт, устройство =" << m_deviceIndex;

    cv::VideoCapture cap;
#ifdef _WIN32
    try {
        cap.open(m_deviceIndex, cv::CAP_DSHOW);
    } catch (const cv::Exception &e) {
        emit errorOccurred(QString("OpenCV exception: %1").arg(e.what()));
        m_running = false;
        return;
    }
#elif __linux__
    try {
    	cap.open(m_deviceIndex, cv::CAP_V4L2);
    } catch (const cv::Exception &e) {
        emit errorOccurred(QString("OpenCV exception: %1").arg(e.what()));
        m_running = false;
        return;
    }
#elif __APPLE__
    try {
        cap.open(m_deviceIndex, cv::CAP_AVFOUNDATION);
    } catch (const cv::Exception &e) {
        emit errorOccurred(QString("OpenCV exception: %1").arg(e.what()));
        m_running = false;
        return;
    }
#else

    if (!cap.open(m_deviceIndex)) {
        emit errorOccurred(QString("Не удалось открыть устройство: %1").arg(m_deviceIndex));
        m_running = false;
        return;
    }
#endif

    if (!cap.isOpened()) {
        emit errorOccurred(QString("Не удалось открыть устройство: %1").arg(m_deviceIndex));
        m_running = false;
        return;
    }

    cap.set(cv::CAP_PROP_FPS, m_fps);
    cv::Mat frame;

    while (m_running) {
        if (!cap.read(frame) || frame.empty()) {
            QThread::msleep(5);
            continue;
        }

        QImage img;
        if (frame.channels() == 3) {
            cv::Mat rgb;
            cv::cvtColor(frame, rgb, cv::COLOR_BGR2RGB);
            img = QImage(rgb.data, rgb.cols, rgb.rows, int(rgb.step), QImage::Format_RGB888).copy();
        } else if (frame.channels() == 1) {
            img = QImage(frame.data, frame.cols, frame.rows, int(frame.step), QImage::Format_Grayscale8).copy();
        } else {
            QThread::msleep(5);
            continue;
        }

        emit frameReady(img);
        QThread::msleep(1);
    }

    cap.release();
    qDebug() << "CaptureThread: завершён";
}
