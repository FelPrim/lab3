#include "videocapture.h"
#include <QDebug>
#include <QElapsedTimer> 

VideoCapture::VideoCapture(int deviceIndex, QObject *parent)
    : QThread(parent), m_deviceIndex(deviceIndex)
{

    m_frameIntervalMs = 1000 / DEFAULT_FPS; // 66 мс для 15 FPS
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
        if (!wait(1000)) { // 1 секунда таймаут
            terminate();
            wait();
        }
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
    qDebug() << "VideoCapture: quick starting device" << m_deviceIndex 
             << "with target FPS:" << m_targetFps;
    
    QThread::msleep(100);
    
    // Быстрое открытие устройства
    try {
#ifdef _WIN32
        m_capture.open(m_deviceIndex, cv::CAP_DSHOW);
#elif __linux__
        m_capture.open(m_deviceIndex, cv::CAP_V4L2);
#elif __APPLE__
        m_capture.open(m_deviceIndex, cv::CAP_AVFOUNDATION);
#else
        m_capture.open(m_deviceIndex);
#endif
    } catch (const cv::Exception& e) {
        emit errorOccurred(QString("Cannot open video device %1: %2").arg(m_deviceIndex).arg(e.what()));
        return;
    }

    if (!m_capture.isOpened()) {
        m_capture.open(m_deviceIndex);
        if (!m_capture.isOpened()) {
            emit errorOccurred(QString("Cannot open video device %1").arg(m_deviceIndex));
            return;
        }
    }

    // Быстрые базовые настройки
    m_capture.set(cv::CAP_PROP_FRAME_WIDTH, DEFAULT_WIDTH);
    m_capture.set(cv::CAP_PROP_FRAME_HEIGHT, DEFAULT_HEIGHT);
    m_capture.set(cv::CAP_PROP_FPS, m_targetFps);
    m_capture.set(cv::CAP_PROP_BUFFERSIZE, 1);

    double actualWidth = m_capture.get(cv::CAP_PROP_FRAME_WIDTH);
    double actualHeight = m_capture.get(cv::CAP_PROP_FRAME_HEIGHT);
    double actualFps = m_capture.get(cv::CAP_PROP_FPS);

    qDebug() << "Requested:" << DEFAULT_WIDTH << "x" << DEFAULT_HEIGHT << "@" << m_targetFps;
    qDebug() << "Actual:" << actualWidth << "x" << actualHeight << "@" << actualFps;

    // Если реальное разрешение маленькое, испустить предупреждение
    if (actualWidth < 320 || actualHeight < 240) {
        qWarning() << "Camera returned very small resolution:" 
                   << actualWidth << "x" << actualHeight
                   << "Camera may not support requested resolution.";
    }

    // Минимальный прогрев - 2 кадра
    cv::Mat warmupFrame;
    for (int i = 0; i < 2 && m_running; i++) {
        if (m_capture.read(warmupFrame) && !warmupFrame.empty()) {
            break;
        }
        QThread::msleep(10);
    }

    qDebug() << "Starting capture loop for device" << m_deviceIndex 
             << "with interval:" << m_frameIntervalMs << "ms";
    
    // Основной цикл захвата с контролем FPS
    cv::Mat frame;
    int frameCount = 0;
    QElapsedTimer fpsTimer;
    QElapsedTimer frameTimer;
    fpsTimer.start();
    
    while (m_running) {
        frameTimer.restart();
        
        // Захват кадра
        if (!m_capture.read(frame) || frame.empty()) {
            // Быстрая обработка ошибок
            QThread::msleep(5);
            continue;
        }
        
        frameCount++;

        // Создаем QImage напрямую из данных кадра (без копирования)
        QImage image(frame.data, frame.cols, frame.rows, 
                     frame.step, QImage::Format_BGR888);
        
        // Критически важно: создаем копию, так как данные rgbFrame временные
#ifndef TEST_DECODER
        emit rawFrameReady(image.copy());
#endif

        // Отправляем кадр для кодирования
        emit frameForEncodingReady(frame.clone());

        // Периодический лог FPS (каждые 100 кадров)
        if (frameCount % 100 == 0) {
            double elapsed = fpsTimer.restart() / 1000.0;
            double currentFps = 100.0 / elapsed;
            qDebug() << "Device" << m_deviceIndex 
                     << "FPS:" << currentFps 
                     << "(target:" << m_targetFps << ")";
        }

        // Контроль FPS: ждем, чтобы достичь целевого интервала
        qint64 elapsed = frameTimer.elapsed();
        if (elapsed < m_frameIntervalMs) {
            QThread::msleep(m_frameIntervalMs - elapsed);
        }
        // Если elapsed >= m_frameIntervalMs, значит мы не успеваем 
        // по таймингу, пропускаем паузу
    }

    m_capture.release();
    qDebug() << "VideoCapture: stopped device" << m_deviceIndex;
}


QList<int> VideoCapture::getAvailableDevices()
{
    QList<int> devices;
    
    qDebug() << "Quick scanning for video devices...";
    
    for (int i = 0; i < 10; ++i) {
        QElapsedTimer timer;
        timer.start();
        
        cv::VideoCapture cap;
        bool opened = false;
        
        // Быстрая попытка открытия без исключений
#ifdef _WIN32
        opened = cap.open(i, cv::CAP_DSHOW);
#elif __linux__
        opened = cap.open(i, cv::CAP_V4L2);
#elif __APPLE__
        opened = cap.open(i, cv::CAP_AVFOUNDATION);
#else
        opened = cap.open(i);
#endif

        if (opened && cap.isOpened()) {
            cv::Mat testFrame;
            int attempts = 0;
            bool deviceValid = false;
            
            while (attempts < 5 && timer.elapsed() < 1000) {
                if (cap.read(testFrame) && !testFrame.empty()) {
                    deviceValid = true;
                    qDebug() << "Found device:" << i << "after" << attempts + 1 << "attempts";
                    break;
                }
                attempts++;
                QThread::msleep(30);
            }
            
            // Важно: полностью освобождать устройство перед выходом
            cap.release();
            QThread::msleep(50);
            
            if (deviceValid) {
                devices.append(i);
            }
        } else {
            // Если не удалось открыть, убедимся что устройство закрыто
            if (cap.isOpened()) {
                cap.release();
            }
        }
    }

    qDebug() << "Quick scan completed. Found" << devices.size() << "devices";
    return devices;
}