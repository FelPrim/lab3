#include "capturethread.h"
#include "packetbuffer.h"
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

    if (m_packetBuffer) {
        m_packetBuffer->clear();
        delete m_packetBuffer;
        m_packetBuffer = nullptr;
    }
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
        stopCapture(); 
        wait();
    }

    int bufferCapacity = qMax(1, static_cast<int>(m_fps * m_bufferSeconds * 2));
    if (!m_packetBuffer) {
        m_packetBuffer = new PacketBuffer(bufferCapacity);
    } else {
        m_packetBuffer->setCapacity(bufferCapacity);
        m_packetBuffer->clear();
    }

    qDebug() << "Buffer capacity:" << bufferCapacity 
             << "packets (" << m_bufferSeconds << "seconds at" << m_fps << "FPS)";
    
    m_deviceIndex = deviceIndex;
    m_encoderFrameCount = 0;  
    m_running = true;
    start();
}

void CaptureThread::stopCapture()
{
    m_running = false;
    wait();  
	
    if (m_packetBuffer) {
        m_packetBuffer->clear();
    }
}

void CaptureThread::run()
{
    qDebug() << "capturethread: старт, устройство =" << m_deviceIndex;

    cv::VideoCapture cap;
#ifdef _WIN32
    try {
        cap.open(m_deviceIndex, cv::CAP_DSHOW);
    } catch (const cv::Exception &e) {
        emit errorOccurred(QString("opencv exception: %1").arg(e.what()));
        m_running = false;
        return;
    }
#elif __linux__
    try {
        cap.open(m_deviceIndex, cv::CAP_V4L2);
    } catch (const cv::Exception &e) {
        emit errorOccurred(QString("opencv exception: %1").arg(e.what()));
        m_running = false;
        return;
    }
#elif __APPLE__
    try {
        cap.open(m_deviceIndex, cv::CAP_AVFOUNDATION);
    } catch (const cv::Exception &e) {
        emit errorOccurred(QString("opencv exception: %1").arg(e.what()));
        m_running = false;
        return;
    }
#else
    if (!cap.open(m_deviceIndex)) {
        emit errorOccurred(QString("не удалось открыть устройство: %1").arg(m_deviceIndex));
        m_running = false;
        return;
    }
#endif

    if (!cap.isOpened()) {
        emit errorOccurred(QString("не удалось открыть устройство: %1").arg(m_deviceIndex));
        m_running = false;
        return;
    }
   
    cap.set(cv::CAP_PROP_FRAME_WIDTH, DEFAULT_WIDTH);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, DEFAULT_HEIGHT);
    cap.set(cv::CAP_PROP_FPS, m_fps);
    
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
        qDebug() << "probe frame size:" << width << "x" << height;
    } else {
        // если не удалось получить кадр — используем дефолты, но логгируем это.
        qDebug() << "не удалось получить probe-кадр, используем значения по умолчанию:"
                 << DEFAULT_WIDTH << "x" << DEFAULT_HEIGHT;
    }

    // дополнительно: если размеры нечётные, привести к чётным (нужно для yuv420)
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


    // decoder -> capture (forward to gui)
    connect(dec, &DecoderWorker::frameReady, this, &CaptureThread::frameReady, Qt::QueuedConnection);

    // pass errors up
    connect(enc, &EncoderWorker::errorOccurred, this, &CaptureThread::errorOccurred, Qt::QueuedConnection);
    connect(dec, &DecoderWorker::errorOccurred, this, &CaptureThread::errorOccurred, Qt::QueuedConnection);

    m_encoderWorker = enc;
    m_decoderWorker = dec;
	
	float delaySeconds = m_bufferSeconds;  // это уже float
    m_bufferReaderThread = new BufferReaderThread(m_packetBuffer, m_fps, delaySeconds, this);

    qDebug() << "Delay seconds:" << delaySeconds << "-> frames:" << delaySeconds * m_fps;

    // Подключаем сигнал из потока к декодеру
    connect(m_bufferReaderThread, &BufferReaderThread::packetReady,
        dec, &DecoderWorker::processPacket,
        Qt::QueuedConnection);

    m_encoderThread->start();
    m_decoderThread->start();
		
    if(!QMetaObject::invokeMethod(enc, "initialize", Qt::BlockingQueuedConnection)) {
        qWarning() << "failed to invoke initialize on encoder";
    }
    if(!QMetaObject::invokeMethod(dec, "initialize", Qt::BlockingQueuedConnection)) {
        qWarning() << "failed to invoke initialize on decoder";
    }
	
	QThread::sleep(1);
	qDebug() << "Starting BufferReaderThread, current buffer size:" << m_packetBuffer->size();
    m_bufferReaderThread->start();

    connect(m_encoderWorker, &EncoderWorker::packetReady,
        this, [this](const QByteArray &packet){
            if (m_packetBuffer && m_running) {
                m_packetBuffer->insertFrame(m_encoderFrameCount, packet);
                m_encoderFrameCount++;
                
                if (m_encoderFrameCount % 30 == 0) {
                    qDebug() << "Encoder frame" << m_encoderFrameCount 
                             << "added. Buffer:" << m_packetBuffer->size() 
                             << "/" << m_packetBuffer->capacity()
                             << "frames, range:" << m_packetBuffer->getMinFrameNumber()
                             << "-" << m_packetBuffer->getMaxFrameNumber();
                }
            }
        }, Qt::QueuedConnection);
	    cv::Mat frame;
    QElapsedTimer frameTimer;
    
	qDebug() << "Settings - FPS:" << m_fps 
         << "Buffer seconds:" << m_bufferSeconds
         << "Expected delay:" << (m_bufferSeconds * 1000) << "ms";
	
    while (m_running) {
        frameTimer.restart();
        
        if (!cap.read(frame) || frame.empty()) {
            QThread::msleep(5);
            continue;
        }

        cv::Mat bgr;
        if (frame.channels() == 3)
            bgr = frame;
        else if (frame.channels() == 1)
            cv::cvtColor(frame, bgr, cv::COLOR_GRAY2BGR);
        else if (frame.channels() == 4)
            cv::cvtColor(frame, bgr, cv::COLOR_BGRA2BGR);
        else
            continue;

        // Отправляем кадр в энкодер
        emit frameCaptured(bgr);

        // Точный контроль FPS
        int elapsed = frameTimer.elapsed();
        int frameTime = 1000 / m_fps;
        int remaining = frameTime - elapsed;
        if (remaining > 0) {
            QThread::msleep(remaining);
        }
    }
    
    // ОСТАНАВЛИВАЕМ потоки в правильном порядке
    if (m_bufferReaderThread) {
        m_bufferReaderThread->stop();
        m_bufferReaderThread->wait();
        delete m_bufferReaderThread;
        m_bufferReaderThread = nullptr;
    }
    
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
    qDebug() << "capturethread: завершён";
}