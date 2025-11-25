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
    
    bool deviceOpened = false;
    int attempts = 0;
    const int maxAttempts = 3;
    
    // Пытаемся открыть устройство с экспоненциальной backoff задержкой
    while (!deviceOpened && attempts < maxAttempts && m_running) {
        attempts++;
        
#ifdef _WIN32
        try {
            qDebug() << "Attempt" << attempts << "opening device with DSHOW backend";
            m_capture.open(m_deviceIndex, cv::CAP_DSHOW);
        } catch (const cv::Exception &e) {
            qWarning() << "OpenCV DSHOW exception:" << e.what();
            QThread::msleep(100 * attempts); // 100ms, 200ms, 300ms
            continue;
        }
#elif __linux__
        try {
            qDebug() << "Attempt" << attempts << "opening device with V4L2 backend";
            m_capture.open(m_deviceIndex, cv::CAP_V4L2);
        } catch (const cv::Exception &e) {
            qWarning() << "OpenCV V4L2 exception:" << e.what();
            QThread::msleep(100 * attempts);
            continue;
        }
#elif __APPLE__
        try {
            qDebug() << "Attempt" << attempts << "opening device with AVFOUNDATION backend";
            m_capture.open(m_deviceIndex, cv::CAP_AVFOUNDATION);
        } catch (const cv::Exception &e) {
            qWarning() << "OpenCV AVFOUNDATION exception:" << e.what();
            QThread::msleep(100 * attempts);
            continue;
        }
#else
        if (!m_capture.open(m_deviceIndex)) {
            qWarning() << "Failed to open device with default backend";
            QThread::msleep(100 * attempts);
            continue;
        }
#endif

        if (m_capture.isOpened()) {
            deviceOpened = true;
            qDebug() << "Successfully opened device" << m_deviceIndex << "on attempt" << attempts;
        } else {
            qWarning() << "Failed to open device" << m_deviceIndex << "on attempt" << attempts;
            QThread::msleep(100 * attempts);
        }
    }

    if (!deviceOpened || !m_capture.isOpened()) {
        emit errorOccurred(QString("Cannot open video device %1 after %2 attempts").arg(m_deviceIndex).arg(attempts));
        return;
    }

    // Оптимизированные настройки для низкой задержки
    m_capture.set(cv::CAP_PROP_FRAME_WIDTH, DEFAULT_WIDTH);
    m_capture.set(cv::CAP_PROP_FRAME_HEIGHT, DEFAULT_HEIGHT);
    m_capture.set(cv::CAP_PROP_FPS, m_fps);
    
    // КРИТИЧЕСКИ ВАЖНО: устанавливаем минимальный размер буфера
    m_capture.set(cv::CAP_PROP_BUFFERSIZE, 1);
    
    // Пробуем установить формат MJPEG для лучшей производительности
    m_capture.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M','J','P','G'));
    
    // Автоматическая фокусировка и экспозиция (если доступно)
    m_capture.set(cv::CAP_PROP_AUTOFOCUS, 0);
    m_capture.set(cv::CAP_PROP_AUTO_EXPOSURE, 1);

    // Даем камере время на инициализацию
    QThread::msleep(50);
    
    // Быстрый прогрев - захватываем несколько кадров без обработки
    qDebug() << "Warming up camera...";
    cv::Mat warmupFrame;
    for (int i = 0; i < 3; i++) {
        if (m_capture.read(warmupFrame) && !warmupFrame.empty()) {
            break; // Успешно получили кадр
        }
        QThread::msleep(10);
    }

    cv::Mat frame;
    QElapsedTimer timer;
    QElapsedTimer fpsTimer;
    int frameCount = 0;
    int consecutiveFailures = 0;
    const int maxConsecutiveFailures = 5;

    fpsTimer.start();

    qDebug() << "Starting video capture loop for device" << m_deviceIndex;

    while (m_running) {
        timer.restart();
        
        if (!m_capture.read(frame) || frame.empty()) {
            consecutiveFailures++;
            qWarning() << "Failed to read frame from device" << m_deviceIndex << "consecutive failures:" << consecutiveFailures;
            
            if (consecutiveFailures >= maxConsecutiveFailures) {
                emit errorOccurred(QString("Too many consecutive capture failures (%1)").arg(consecutiveFailures));
                break;
            }
            
            QThread::msleep(5);
            continue;
        }
        
        consecutiveFailures = 0; // Сброс счетчика ошибок при успешном захвате
        frameCount++;

        // Периодический вывод FPS (каждые 60 кадров)
        if (frameCount % 60 == 0) {
            double elapsed = fpsTimer.restart() / 1000.0;
            double currentFps = 60.0 / elapsed;
            qDebug() << "Device" << m_deviceIndex << "FPS:" << currentFps;
        }

        // Быстрая конвертация для прямого показа
        cv::Mat rgbFrame;
        cv::cvtColor(frame, rgbFrame, cv::COLOR_BGR2RGB);
        QImage image(rgbFrame.data, rgbFrame.cols, rgbFrame.rows, 
                    rgbFrame.step, QImage::Format_RGB888);
        
        // Отправляем для прямого показа
        emit rawFrameReady(image.copy());
        
        // Отправляем для кодирования (оригинальный BGR кадр)
        emit frameForEncodingReady(frame.clone());

        // Адаптивный контроль FPS - не блокируем если отстаем
        int elapsed = timer.elapsed();
        int targetFrameTime = 1000 / m_fps;
        
        if (elapsed < targetFrameTime) {
            int remaining = targetFrameTime - elapsed;
            if (remaining > 0 && remaining < 50) { // Защита от слишком долгого сна
                QThread::msleep(remaining);
            }
        } else {
            // Если не успеваем - просто продолжаем
            qDebug() << "Device" << m_deviceIndex << "can't keep up with target FPS. Frame time:" << elapsed << "ms";
        }
    }

    m_capture.release();
    qDebug() << "VideoCapture: stopped device" << m_deviceIndex << "total frames:" << frameCount;
}

QList<int> VideoCapture::getAvailableDevices()
{
    QList<int> devices;
    
    qDebug() << "Scanning for video devices...";
    for (int i = 0; i < 10; ++i) {
        cv::VideoCapture cap;
#ifdef _WIN32
        try {
            cap.open(i, cv::CAP_DSHOW);
        } catch (...) {
            continue;
        }
#else
#ifdef __linux__
        if (!cap.open(i, cv::CAP_V4L2)) continue;
#else
#ifdef __APPLE__
        if (!cap.open(i, cv::CAP_AVFOUNDATION)) continue;
#else
        if (!cap.open(i)) continue;
#endif
#endif
#endif
        if (cap.isOpened()) {
            devices.append(i);
            qDebug() << "Found device:" << i;
            cap.release();
        }
    }

    qDebug() << "Total devices found:" << devices.size();
    return devices;
}