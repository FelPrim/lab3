
#include "capturethread.h"
#include "video_defaults.h"
#include "EncoderWorker.h"
#include "DecoderWorker.h"
#include <QDebug>

CaptureThread::CaptureThread(QObject *parent)
    : QThread(parent)
{
    qRegisterMetaType<cv::Mat>("cv::Mat");
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
   
    cap.set(cv::CAP_PROP_FRAME_WIDTH, DEFAULT_WIDTH);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, DEFAULT_HEIGHT);
    cap.set(cv::CAP_PROP_FPS, m_fps);
    /*
    int width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
    int height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    if (width <= 0 || height <= 0) { width = DEFAULT_WIDTH; height = DEFAULT_HEIGHT; }
    */
    
    // Попробуем прочитать несколько кадров, чтобы драйвер "поднял" поток и вернул реальные размеры.
    cv::Mat probe;
    int attempts = 0;
    const int max_attempts = 10;
    while (attempts < max_attempts && (!cap.read(probe) || probe.empty())) {
        attempts++;
        QThread::msleep(50);
    }

    int width = DEFAULT_WIDTH;
    int height = DEFAULT_HEIGHT;
    if (!probe.empty()) {
        width = probe.cols;
        height = probe.rows;
        qDebug() << "Probe frame size:" << width << "x" << height;
    } else {
        // Если не удалось получить кадр — используем дефолты, но логгируем это.
        qDebug() << "Не удалось получить probe-кадр, используем значения по умолчанию:"
                 << DEFAULT_WIDTH << "x" << DEFAULT_HEIGHT;
    }

    // Дополнительно: если размеры нечётные, привести к чётным (нужно для YUV420)
    if (width % 2) --width;
    if (height % 2) --height;

    // setup encoder and decoder threads/workers
    m_encoderThread = new QThread();
    m_decoderThread = new QThread();

    EncoderWorker *enc = new EncoderWorker(width, height, m_fps);
    DecoderWorker *dec = new DecoderWorker(width, height);

    enc->moveToThread(m_encoderThread);
    dec->moveToThread(m_decoderThread);

    // cleanup on thread finish
    connect(m_encoderThread, &QThread::finished, enc, &QObject::deleteLater);
    connect(m_decoderThread, &QThread::finished, dec, &QObject::deleteLater);

    // capture -> encoder
    connect(this, &CaptureThread::frameCaptured, enc, &EncoderWorker::processFrame, Qt::QueuedConnection);

    // encoder -> decoder
    connect(enc, &EncoderWorker::packetReady, dec, &DecoderWorker::processPacket, Qt::QueuedConnection);

    // decoder -> capture (forward to GUI)
    connect(dec, &DecoderWorker::frameReady, this, &CaptureThread::frameReady, Qt::QueuedConnection);

    // pass errors up
    connect(enc, &EncoderWorker::errorOccurred, this, &CaptureThread::errorOccurred, Qt::QueuedConnection);
    connect(dec, &DecoderWorker::errorOccurred, this, &CaptureThread::errorOccurred, Qt::QueuedConnection);

    m_encoderWorker = enc;
    m_decoderWorker = dec;

    m_encoderThread->start();
    m_decoderThread->start();

    cv::Mat frame;
    while (m_running) {
        if (!cap.read(frame) || frame.empty()) {
            QThread::msleep(5);
            continue;
        }

        // ensure BGR 3 channels
        cv::Mat bgr;
        if (frame.channels() == 3) {
            bgr = frame;
        } else if (frame.channels() == 1) {
            cv::cvtColor(frame, bgr, cv::COLOR_GRAY2BGR);
        } else if (frame.channels() == 4) {
            cv::cvtColor(frame, bgr, cv::COLOR_BGRA2BGR);
        } else {
            QThread::msleep(5);
            continue;
        }

        // emit to encoder (queued copy of cv::Mat header)
        emit frameCaptured(bgr);

        QThread::msleep(1);
    }

    // stop threads
    if (m_encoderThread) {
        m_encoderThread->quit();
        m_encoderThread->wait();
        delete m_encoderThread;
        m_encoderThread = nullptr;
        m_encoderWorker = nullptr;
    }
    if (m_decoderThread) {
        m_decoderThread->quit();
        m_decoderThread->wait();
        delete m_decoderThread;
        m_decoderThread = nullptr;
        m_decoderWorker = nullptr;
    }

    cap.release();
    qDebug() << "CaptureThread: завершён";
}
