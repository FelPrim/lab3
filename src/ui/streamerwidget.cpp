// streamerwidget.cpp
#include "streamerwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QDebug>
#include "id_utils.h"
#include "../network/streammanager.h"

const QString StreamerWidget::STATUS_NO_VIEWERS = "● No viewers";
const QString StreamerWidget::STATUS_HAS_VIEWERS = "● Viewers connected";
const QString StreamerWidget::STATUS_STOPPED = "● Stream stopped";
const QString StreamerWidget::PLACEHOLDER_TEXT = "Video device not connected";

QString streamIdToDisplayString(uint32_t streamId) {
    char str[7];
    id_to_string(streamId, str);
    return QString::fromLatin1(str, 6);
}

// В конструкторе ИНИЦИАЛИЗИРОВАТЬ m_streamManager:
StreamerWidget::StreamerWidget(int deviceIndex, QWidget *parent)
    : QWidget(parent)
    , m_streamId(0)
    , m_streamState(State_NoStream) // ОДНО определение
    , m_encoderInitialized(false)
    , m_sendingPackets(false)
    , m_streamManager(nullptr) // ДОБАВИТЬ инициализацию
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
    
    if (m_controlPanel) {
        m_controlPanel->setActive(true);
        m_controlPanel->setStreamState(StreamControlPanel::StateNoStream);
        m_controlPanel->setViewersStatus(false);
    }
}

void StreamerWidget::setStreamManager(StreamManager* streamManager) {
    m_streamManager = streamManager;
    
    if (m_streamManager) {
        connect(m_streamManager, &StreamManager::serverStreamCreated, 
                this, &StreamerWidget::onServerStreamCreated);
        connect(m_streamManager, &StreamManager::serverStreamStart, 
                this, &StreamerWidget::onServerStreamStart);
        connect(m_streamManager, &StreamManager::serverStreamEnd, 
                this, &StreamerWidget::onServerStreamEnd);
        connect(m_streamManager, &StreamManager::serverStreamDeleted, 
                this, &StreamerWidget::onServerStreamDeleted);
        connect(m_streamManager, &StreamManager::errorOccurred, 
                this, &StreamerWidget::onNetworkError);
    }
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
    m_controlPanel->setViewersStatus(0);

    m_mainLayout->addWidget(m_videoDisplay, 1);
    m_mainLayout->addWidget(m_controlPanel);
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
#ifndef TEST_DECODER 
    connect(m_videoCapture, &VideoCapture::rawFrameReady,
            this, &StreamerWidget::onRawFrameReady);
#endif
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

    if (!m_videoEncoder) {
        initializeVideoEncoder();
    }
    else{
        m_videoEncoder->setStreamId(streamId);
    }

    updateStatus();
}

void StreamerWidget::initializeWithRealId(uint32_t streamId, const QString &displayId)
{
    setStreamId(streamId, displayId);
    initialize();
}







// В StreamerWidget::setStreamingEnabled убедимся, что правильно обновляем состояние:
void StreamerWidget::setStreamingEnabled(bool enabled)
{
    if (m_streamingEnabled == enabled) return;

    m_streamingEnabled = enabled;
    
    // ВАЖНО: Всегда устанавливаем активное состояние для control panel
    if (m_controlPanel) {
        m_controlPanel->setActive(true); // Всегда активно, чтобы кнопка Disconnect работала
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
        m_controlPanel->setViewersStatus(hasViewers ? 1 : 0);
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
        m_controlPanel->setViewersStatus(m_hasViewers ? 1 : 0);

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

// В StreamerWidget::initialize() - упрощенная инициализация
// В streamerwidget.cpp - исправленный initialize()
void StreamerWidget::initialize()
{
    qDebug() << "Fast initializing StreamerWidget for device:" << m_deviceIndex;

    try {
        // 1. Сначала инициализируем видеозахват
         QThread::msleep(50);
        initializeVideoCapture();
        
#ifdef TEST_DECODER
    if (!m_testDecoder) {
        m_testDecoder = new VideoDecoder(DEFAULT_WIDTH, DEFAULT_HEIGHT, this);
        m_testDecoder->initialize();
        connect(m_testDecoder, &VideoDecoder::frameDecoded,
                this, [this](const QImage& img, int){
                    if (m_videoDisplay) m_videoDisplay->displayFrame(img);
                });
    }
#endif
        // 2. Затем настраиваем соединения (чтобы подключиться к созданному VideoCapture)
        setupConnections();
        
        // 3. Включаем стриминг и обновляем статус
        setStreamingEnabled(true);
        updateStatus();
        
        // 4. Немедленный запуск захвата для превью
        if (m_videoCapture) {
            m_videoCapture->startCapture();
        }
        
    } catch (const std::exception& e) {
        qCritical() << "Failed to initialize StreamerWidget:" << e.what();
        showError(QString("Failed to initialize: %1").arg(e.what()));
    }
}

// В StreamerWidget::setupConnections() - прямые соединения для показа
void StreamerWidget::setupConnections()
{
    // Control panel signals
    connect(m_controlPanel, &StreamControlPanel::startStreamRequested,
            this, &StreamerWidget::onStartStreamRequested);
    connect(m_controlPanel, &StreamControlPanel::stopStreamRequested,
            this, &StreamerWidget::onStopStreamRequested);
    connect(m_controlPanel, &StreamControlPanel::disconnectRequested,
            this, &StreamerWidget::onDisconnectRequested);

    // ПРЯМОЕ соединение для локального показа - без кодировщика!
    if (m_videoCapture) {
        connect(m_videoCapture, &VideoCapture::rawFrameReady,
                this, &StreamerWidget::onRawFrameReady);
        connect(m_videoCapture, &VideoCapture::frameForEncodingReady,
            this, &StreamerWidget::onFrameForEncoding);
    }
}

// Упрощенный обработчик кадров для прямого показа
void StreamerWidget::onRawFrameReady(const QImage &image)
{
    if (m_videoDisplay && m_streamingEnabled) {
        m_videoDisplay->displayFrame(image);
    }
}

void StreamerWidget::onServerStreamDeleted(uint32_t streamId) {
    if (streamId != m_streamId) {
        qWarning() << "Stream deletion for wrong stream ID:" << streamId << "expected:" << m_streamId;
        return;
    }
    
    qDebug() << "Stream deleted by server:" << m_displayId;
    
    // Останавливаем стриминг и сбрасываем состояние
    stopStream();
    
    // Сбрасываем streamId
    m_streamId = 0;
    m_displayId = "---";
    
    if (m_controlPanel) {
        m_controlPanel->setStreamId("---");
    }
    
    updateStatus();
}


void StreamerWidget::startStream()
{
    if (m_streamState != State_NoStream) {
        qWarning() << "Cannot start stream - wrong state:" << m_streamState;
        return;
    }

    if (!m_videoCapture) {
        qWarning() << "Cannot start stream - video capture not initialized";
        return;
    }

    // Запускаем видеозахват для превью (если еще не запущен)
    if (m_videoCapture && !m_videoCapture->isRunning()) {
        m_videoCapture->startCapture();
    }
    
    // Инициализируем кодировщик ТОЛЬКО если он еще не инициализирован
    if (!m_encoderInitialized) {
        initializeVideoEncoder();
        m_encoderInitialized = true;
    }
    
    // Переходим в состояние "трансляция создана"
    setStreamState(State_StreamCreated);
    
    // ОТПРАВЛЯЕМ запрос на создание стрима через StreamManager
    if (m_streamManager) {
        m_streamManager->createStream(m_deviceIndex);
        qDebug() << "Stream creation requested for device:" << m_deviceIndex;
    } else {
        qWarning() << "StreamManager not available for device:" << m_deviceIndex;
        // Без StreamManager переходим сразу в активное состояние (для тестирования)
        setStreamState(State_StreamActive);
    }
    
    qDebug() << "Stream setup completed for device:" << m_deviceIndex;
}


void StreamerWidget::setStreamState(StreamState newState)
{
    if (m_streamState == newState) return;
    
    StreamState oldState = m_streamState;
    m_streamState = newState;
    
    qDebug() << "Stream state changed for device" << m_deviceIndex 
             << ":" << oldState << "->" << newState;
    
    // Обновляем ControlPanel
    if (m_controlPanel) {
        m_controlPanel->setStreamState(static_cast<StreamControlPanel::StreamState>(newState));
        m_controlPanel->setViewersStatus(m_hasViewers);
    }
    
    updateStatus();
    
    // Обработка переходов между состояниями - УБИРАЕМ вызов stopStream()
    switch (newState) {
    case State_NoStream:
        // Только сбрасываем флаги, не вызываем stopStream (чтобы избежать рекурсии)
        m_encoderInitialized = false;
        m_sendingPackets = false;
        m_isStreaming = false;
        break;
        
    case State_StreamCreated:
        if (!m_encoderInitialized) {
            initializeVideoEncoder();
            m_encoderInitialized = true;
        }
        m_sendingPackets = false;
        setViewersStatus(false);
        break;
        
    case State_StreamActive:
        m_sendingPackets = true;
        setViewersStatus(true);
        break;
        
    case State_StreamError:
        m_sendingPackets = false;
        setViewersStatus(false);
        showError("Stream error occurred");
        break;
    }
}

void StreamerWidget::stopStream()
{
    if (m_streamState == State_NoStream) {
        return;
    }

    qDebug() << "Stopping stream for device:" << m_deviceIndex;

    // ЕСЛИ есть streamId, отправляем запрос на удаление стрима
    if (m_streamId != 0 && m_streamManager) {
        m_streamManager->deleteStream(m_streamId);
        qDebug() << "Stream deletion requested for:" << m_streamId;
    }

    // Останавливаем видеозахват
    if (m_videoCapture) {
        m_videoCapture->stopCapture();
    }

    // Останавливаем кодировщик
    cleanupVideoEncoder();
    
    // Сбрасываем флаги напрямую
    m_isStreaming = false;
    m_sendingPackets = false;
    m_encoderInitialized = false;
    m_streamState = State_NoStream;
    
    // Обновляем UI
    if (m_controlPanel) {
        m_controlPanel->setStreamState(StreamControlPanel::StateNoStream);
        m_controlPanel->setViewersStatus(false);
    }
    
    updateStatus();
    
    qDebug() << "Stream stopped for device:" << m_deviceIndex;
}

// Обработчики серверных событий
void StreamerWidget::onServerStreamCreated(uint32_t streamId) {
    if (m_streamState != State_StreamCreated) {
        qWarning() << "Unexpected stream creation in state:" << m_streamState;
        return;
    }
    
    QString displayId = streamIdToDisplayString(streamId);
    setStreamId(streamId, displayId);
    qDebug() << "Stream officially created - ID:" << streamId << "Display:" << displayId;
    
}

void StreamerWidget::onServerStreamStart(uint32_t streamId)
{
    if (streamId != m_streamId) {
        qWarning() << "Stream start for wrong stream ID:" << streamId << "expected:" << m_streamId;
        return;
    }
    
    if (m_streamState == State_StreamCreated) {
        setStreamState(State_StreamActive);
        qDebug() << "Stream started - now sending packets for:" << m_displayId;
    } else {
        qWarning() << "Cannot start stream - wrong state:" << m_streamState;
    }
}

void StreamerWidget::onServerStreamEnd(uint32_t streamId)
{
    if (streamId != m_streamId) {
        qWarning() << "Stream end for wrong stream ID:" << streamId << "expected:" << m_streamId;
        return;
    }
    
    if (m_streamState == State_StreamActive) {
        setStreamState(State_StreamCreated); // Возвращаемся в состояние без отправки пакетов
        qDebug() << "Stream ended - stopped sending packets for:" << m_displayId;
    }
}

// Модифицируем обработчик кадров для кодирования
void StreamerWidget::onFrameForEncoding(const cv::Mat &frame)
{
    qDebug() << "StreamerWidget: Received frame for encoding, state:" << m_streamState;
    
    // Кодируем кадры только если трансляция создана или активна
    if (m_streamState == State_StreamCreated || m_streamState == State_StreamActive) {
        if (m_videoEncoder && m_encoderInitialized) {
            try {
                m_videoEncoder->encodeFrame(frame);
            } catch (const std::exception& e) {
                qCritical() << "Failed to encode frame:" << e.what();
                setStreamState(State_StreamError);
            }
        } else {
            qDebug() << "StreamerWidget: Encoder not ready - encoder:" << m_videoEncoder 
                     << "initialized:" << m_encoderInitialized;
        }
    }
}

// Модифицируем обработчик закодированных пакетов
void StreamerWidget::onFrameEncoded(int streamId, int frameNumber, const QByteArray &packet)
{
    if (streamId != static_cast<int>(m_streamId)) return;


#ifdef TEST_DECODER
    if (m_testDecoder) {
        connect(m_videoEncoder, &VideoEncoder::encodedPacketReady,
                this, [this](int /*streamId*/, int frameNumber, const QByteArray &packet){
                    if (m_testDecoder) {
                        m_testDecoder->decodeFrame(packet, frameNumber);
                    }
                });

        connect(m_testDecoder, &VideoDecoder::frameDecoded,
                this, [this](const QImage &image, int /*frameNumber*/){
                    if (m_videoDisplay) {
                        m_videoDisplay->displayFrame(image);
                    }
                });
    }
#endif
    // УПРОСТИТЬ: отправляем если стрим активен
    if (m_streamManager && m_streamState == State_StreamActive) {
        m_streamManager->sendVideoFrame(m_streamId, frameNumber, packet);
        
        if (frameNumber % 30 == 0) {
            qDebug() << "StreamerWidget: Sent frame" << frameNumber << "for stream" << m_displayId;
        }
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


void StreamerWidget::onNetworkError(const QString& error)
{
    qCritical() << "Network error for stream" << m_streamId << ":" << error;
    setStreamState(State_StreamError);
    showError(QString("Network error: %1").arg(error));
}
