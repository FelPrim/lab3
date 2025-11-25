// streamerwidget.cpp
#include "streamerwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QDebug>

const QString StreamerWidget::STATUS_NO_VIEWERS = "● No viewers";
const QString StreamerWidget::STATUS_HAS_VIEWERS = "● Viewers connected";
const QString StreamerWidget::STATUS_STOPPED = "● Stream stopped";
const QString StreamerWidget::PLACEHOLDER_TEXT = "Video device not connected";

StreamerWidget::StreamerWidget(int deviceIndex, QWidget *parent)
    : QWidget(parent)
    , m_streamId(0)
    , m_displayId("---")
    , m_deviceIndex(deviceIndex)
    , m_isStreaming(false)
    , m_hasViewers(false)
    , m_streamingEnabled(false)
    , m_videoDisplay(nullptr)
    , m_controlPanel(nullptr)
    , m_mainLayout(nullptr)
    , m_videoCapture(nullptr)
    , m_videoEncoder(nullptr)
{
    setupUI();
    setupConnections();
}

StreamerWidget::~StreamerWidget()
{
    cleanup();
}

void StreamerWidget::setupUI()
{
    setStyleSheet(R"(
        StreamerWidget {
            background: #1e1e1e;
            border: 1px solid #444;
            border-radius: 8px;
            margin: 2px;
        }
    )");

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(0);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);

    // Video display
    m_videoDisplay = new VideoDisplay(this);
    m_videoDisplay->setPlaceholderText(PLACEHOLDER_TEXT);
    m_videoDisplay->setStyleSheet(R"(
        VideoDisplay {
            background: #000000;
            border: none;
            border-radius: 6px;
            margin: 4px;
        }
    )");

    // Control panel
    m_controlPanel = new StreamControlPanel(StreamControlPanel::StreamerMode, this);
    m_controlPanel->setStreamId(m_displayId);
    m_controlPanel->setActive(false);
    m_controlPanel->setStreaming(false);
    m_controlPanel->setViewersCount(0);

    m_mainLayout->addWidget(m_videoDisplay, 1);
    m_mainLayout->addWidget(m_controlPanel);
}

void StreamerWidget::setupConnections()
{
    // Control panel signals
    connect(m_controlPanel, &StreamControlPanel::startStreamRequested,
            this, &StreamerWidget::onStartStreamRequested);
    connect(m_controlPanel, &StreamControlPanel::stopStreamRequested,
            this, &StreamerWidget::onStopStreamRequested);
    connect(m_controlPanel, &StreamControlPanel::disconnectRequested,
            this, &StreamerWidget::onDisconnectRequested);
}

void StreamerWidget::initialize()
{
    qDebug() << "Initializing StreamerWidget for device:" << m_deviceIndex;

    try {
        initializeVideoCapture();
        
        // Запускаем захват для превью (но не отправляем данные пока нет streamId)
        if (m_videoCapture) {
            m_videoCapture->startCapture();
        }
        
        // Encoder будет инициализирован когда получим реальный streamId
        setStreamingEnabled(true);
        updateStatus();
        
        qDebug() << "StreamerWidget initialized successfully for device:" << m_deviceIndex;
        
        // Уведомляем MainWindow что устройство готово к получению streamId
        // emit deviceReadyForStream(m_deviceIndex);
        
    } catch (const std::exception& e) {
        qCritical() << "Failed to initialize StreamerWidget for device" << m_deviceIndex << ":" << e.what();
        showError(QString("Failed to initialize video device: %1").arg(e.what()));
    }
}

void StreamerWidget::cleanup()
{
    qDebug() << "Cleaning up StreamerWidget for device:" << m_deviceIndex;

    // Останавливаем стриминг
    stopStream();
    setStreamingEnabled(false);

    // Чистим кодировщик
    cleanupVideoEncoder();
    
    // Чистим видеозахват
    cleanupVideoCapture();

    // Очищаем дисплей
    if (m_videoDisplay) {
        m_videoDisplay->clear();
        m_videoDisplay->setPlaceholderText("Device disconnected");
    }
    
    // Сбрасываем состояние
    m_streamId = 0;
    m_displayId = "---";
    
    qDebug() << "StreamerWidget cleanup completed for device:" << m_deviceIndex;
}


void StreamerWidget::initializeVideoCapture()
{
    cleanupVideoCapture();

    m_videoCapture = new VideoCapture(m_deviceIndex, this);
    
    connect(m_videoCapture, &VideoCapture::rawFrameReady,
            this, &StreamerWidget::onRawFrameReady);
    connect(m_videoCapture, &VideoCapture::frameForEncodingReady,
            this, &StreamerWidget::onFrameForEncoding);
    connect(m_videoCapture, &VideoCapture::errorOccurred,
            this, &StreamerWidget::onVideoError);

    // Note: VideoCapture initialization happens in startCapture()
    qDebug() << "VideoCapture created for device:" << m_deviceIndex;
}

void StreamerWidget::cleanupVideoCapture()
{
    if (m_videoCapture) {
        qDebug() << "Cleaning up video capture for device:" << m_deviceIndex;
        
        m_videoCapture->stopCapture();
        
        // Даем время на корректное завершение
        if (!m_videoCapture->wait(2000)) { // Ждем до 2 секунд
            qWarning() << "Video capture thread didn't finish properly for device:" << m_deviceIndex;
            m_videoCapture->terminate(); // Принудительное завершение
            m_videoCapture->wait();
        }
        
        delete m_videoCapture;
        m_videoCapture = nullptr;
        
        qDebug() << "Video capture cleaned up for device:" << m_deviceIndex;
    }
}

void StreamerWidget::initializeVideoEncoder()
{
    cleanupVideoEncoder();

    if (m_streamId == 0) {
        qWarning() << "Cannot initialize encoder without valid stream ID";
        return;
    }

    m_videoEncoder = new VideoEncoder(m_streamId, this);
    
    connect(m_videoEncoder, &VideoEncoder::encodedPacketReady,
            this, &StreamerWidget::onFrameEncoded);
    connect(m_videoEncoder, &VideoEncoder::errorOccurred,
            this, [this](const QString& error) {
                qCritical() << "VideoEncoder error:" << error;
                showError(QString("Encoder error: %1").arg(error));
            });

    // Initialize encoder with default parameters
    m_videoEncoder->initialize(DEFAULT_WIDTH, DEFAULT_HEIGHT, DEFAULT_FPS);

    qDebug() << "VideoEncoder initialized for stream:" << m_streamId;
}

void StreamerWidget::cleanupVideoEncoder()
{
    if (m_videoEncoder) {
        m_videoEncoder->cleanup();
        delete m_videoEncoder;
        m_videoEncoder = nullptr;
    }
}

void StreamerWidget::setStreamId(uint32_t streamId, const QString &displayId)
{
    m_streamId = streamId;
    m_displayId = displayId;

    if (m_controlPanel) {
        m_controlPanel->setStreamId(displayId);
    }

    // Reinitialize encoder with new stream ID if needed
    if (streamId != 0) {
        initializeVideoEncoder();
    }

    updateStatus();
}

void StreamerWidget::initializeWithRealId(uint32_t streamId, const QString &displayId)
{
    setStreamId(streamId, displayId);
    initialize();
}

void StreamerWidget::startStream()
{
    if (m_isStreaming || !m_streamingEnabled) {
        return;
    }

    if (!m_videoCapture) {
        qWarning() << "Cannot start stream - video capture not initialized";
        return;
    }

    if (m_streamId == 0) {
        qWarning() << "Cannot start stream - no valid stream ID";
        showError("Cannot start stream: No valid stream ID assigned");
        return;
    }

    try {
        // Start video capture
        m_videoCapture->startCapture();
        
        m_isStreaming = true;
        
        if (m_controlPanel) {
            m_controlPanel->setStreaming(true);
        }

        updateStatus();
        
        qDebug() << "Stream started successfully for device:" << m_deviceIndex << "Stream ID:" << m_displayId;
        
    } catch (const std::exception& e) {
        qCritical() << "Failed to start stream:" << e.what();
        stopStream();
        showError(QString("Failed to start stream: %1").arg(e.what()));
    }
}

void StreamerWidget::stopStream()
{
    if (!m_isStreaming) {
        return;
    }

    qDebug() << "Stopping stream for device:" << m_deviceIndex;

    // Останавливаем видеозахват
    if (m_videoCapture) {
        m_videoCapture->stopCapture();
        // Даем время на остановку
        QThread::msleep(100);
    }

    m_isStreaming = false;
    
    // Обновляем UI
    if (m_controlPanel) {
        m_controlPanel->setStreaming(false);
        m_controlPanel->setViewersCount(0);
    }

    updateStatus();
    
    // Испускаем сигнал об остановке стрима
    if (m_streamId != 0) {
        emit streamStopped(m_streamId);
    }
    
    qDebug() << "Stream stopped for device:" << m_deviceIndex;
}

void StreamerWidget::setStreamingEnabled(bool enabled)
{
    if (m_streamingEnabled == enabled) return;

    m_streamingEnabled = enabled;
    
    if (m_controlPanel) {
        m_controlPanel->setActive(enabled);
    }

    if (!enabled) {
        stopStream();
    }

    updateStatus();
}

void StreamerWidget::setViewersStatus(bool hasViewers)
{
    if (m_hasViewers == hasViewers) return;

    m_hasViewers = hasViewers;
    
    if (m_controlPanel) {
        m_controlPanel->setViewersCount(hasViewers ? 1 : 0);
    }

    updateStatus();
}

void StreamerWidget::setControlPanel(StreamControlPanel* panel)
{
    if (m_controlPanel) {
        m_mainLayout->removeWidget(m_controlPanel);
        m_controlPanel->deleteLater();
    }

    m_controlPanel = panel;
    if (m_controlPanel) {
        m_controlPanel->setParent(this);
        m_mainLayout->addWidget(m_controlPanel);
        m_controlPanel->setStreamId(m_displayId);
        m_controlPanel->setActive(m_streamingEnabled);
        m_controlPanel->setStreaming(m_isStreaming);
        m_controlPanel->setViewersCount(m_hasViewers ? 1 : 0);

        // Reconnect signals
        connect(m_controlPanel, &StreamControlPanel::startStreamRequested,
                this, &StreamerWidget::onStartStreamRequested);
        connect(m_controlPanel, &StreamControlPanel::stopStreamRequested,
                this, &StreamerWidget::onStopStreamRequested);
        connect(m_controlPanel, &StreamControlPanel::disconnectRequested,
                this, &StreamerWidget::onDisconnectRequested);
    }
}

void StreamerWidget::updateStatus()
{
    if (m_videoDisplay) {
        if (!m_streamingEnabled) {
            m_videoDisplay->setPlaceholderText("Device disconnected");
        } else if (!m_isStreaming) {
            m_videoDisplay->setPlaceholderText("Ready to stream");
        }
        // When streaming, the video display shows actual frames
    }
}


// ===== SLOTS =====

void StreamerWidget::onVideoError(const QString &message)
{
    qCritical() << "Video error for device" << m_deviceIndex << ":" << message;
    showError(message);
    
    // Stop streaming on error
    if (m_isStreaming) {
        stopStream();
    }
}

void StreamerWidget::onRawFrameReady(const QImage &image)
{
    if (!m_isStreaming) {
        return;
    }

    // Display the frame for local preview
    if (m_videoDisplay) {
        m_videoDisplay->displayFrame(image);
    }
}

void StreamerWidget::onFrameForEncoding(const cv::Mat &frame)
{
    if (!m_isStreaming || !m_videoEncoder || frame.empty()) {
        return;
    }

    try {
        m_videoEncoder->encodeFrame(frame);
    } catch (const std::exception& e) {
        qCritical() << "Failed to encode frame:" << e.what();
    }
}

void StreamerWidget::onFrameEncoded(int streamId, int frameNumber, const QByteArray &packet)
{
    if (streamId != static_cast<int>(m_streamId) || !m_isStreaming) {
        return;
    }

    // Emit the encoded packet for network transmission
    emit encodedPacketReady(m_streamId, frameNumber, packet);
    
    // Debug logging (can be disabled in production)
    if (frameNumber % 30 == 0) { // Log every ~1 second at 30fps
        qDebug() << "Encoded frame" << frameNumber << "for stream" << m_displayId 
                 << "size:" << packet.size() << "bytes";
    }
}

void StreamerWidget::onStartStreamRequested()
{
    qDebug() << "Start stream requested for device:" << m_deviceIndex;
    startStream();
}

void StreamerWidget::onStopStreamRequested()
{
    qDebug() << "Stop stream requested for device:" << m_deviceIndex;
    stopStream();
}

void StreamerWidget::onDisconnectRequested()
{
    qDebug() << "Disconnect requested for device:" << m_deviceIndex;
    
    // Останавливаем стриминг если он активен
    if (m_isStreaming) {
        stopStream();
    }
    
    // Отключаем устройство
    setStreamingEnabled(false);
    
    // Чистим ресурсы
    cleanup();
    
    // Испускаем сигнал об отключении
    emit disconnectRequested(m_deviceIndex);
    
    qDebug() << "Device" << m_deviceIndex << "fully disconnected";
}

void StreamerWidget::showError(const QString &message)
{
    qDebug() << "StreamerWidget Error:" << message;
    // Временная реализация
}

void StreamerWidget::forceDisconnect()
{
    if (m_disconnecting) return;
    
    m_disconnecting = true;
    qDebug() << "Force disconnecting device:" << m_deviceIndex;
    
    // Немедленная остановка всего
    if (m_videoCapture) {
        m_videoCapture->stopCapture();
        m_videoCapture->wait(1000); // Ждем до 1 секунды
    }
    
    cleanup();
    emit disconnectRequested(m_deviceIndex);
}