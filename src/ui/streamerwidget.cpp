#define DEBUG_FRAMEBUFFER

#ifdef DEBUG_FRAMEBUFFER
#define FB_DEBUG qDebug() << "[FrameBuffer]" << Q_FUNC_INFO << "device:" << m_deviceIndex
#else
#define FB_DEBUG if(false) qDebug()
#endif

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

// ИСПРАВЛЕННЫЙ конструктор - наследуем от QWidget
// streamerwidget.cpp - конструктор
StreamerWidget::StreamerWidget(int deviceIndex, QWidget *parent)
    : QWidget(parent)
    , m_streamId(0)
    , m_streamState(State_NoStream)
    , m_encoderInitialized(false)
    , m_sendingPackets(false)
    , m_streamManager(nullptr)
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
#ifdef TEST_DECODER
    , m_testDecoder(nullptr)
    , m_frameBuffer(new FrameBuffer(DEFAULT_BUFFERSZ))  // Сразу создаем буфер!
#endif
{
    setupUI();
    setupConnections();
    
    if (m_controlPanel) {
        m_controlPanel->setActive(true);
        m_controlPanel->setStreamState(StreamControlPanel::StateNoStream);
        m_controlPanel->setViewersStatus(false);
    }
    
    qDebug() << "StreamerWidget created for device:" << deviceIndex 
             << "FrameBuffer created at:" << m_frameBuffer;
}

StreamerWidget::~StreamerWidget()
{
    cleanup();
}

void StreamerWidget::cleanupTestObjects()
{
#ifdef TEST_DECODER
    qDebug() << "Cleaning up test objects for device:" << m_deviceIndex;
    
    // Отключаем все соединения декодера
    if (m_testDecoder) {
        m_testDecoder->disconnect();
        m_testDecoder->cleanup();
        delete m_testDecoder;
        m_testDecoder = nullptr;
    }
    
    // Удаляем буфер
    if (m_frameBuffer) {
        delete m_frameBuffer;
        m_frameBuffer = nullptr;
        qDebug() << "FrameBuffer deleted";
    } else {
        qDebug() << "FrameBuffer was already null";
    }
#endif
}

void StreamerWidget::setupUI()
{
    // Теперь setStyleSheet доступен, так как мы наследуем от QWidget
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

    // Video display - this теперь QWidget*, что правильно
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

    // Control panel - this теперь QWidget*, что правильно
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

    // Чистим тестовые объекты
    cleanupTestObjects();
    
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
    
#ifdef TEST_DECODER
    // Прямое соединение для тестирования
    connect(m_videoCapture, &VideoCapture::frameForEncodingReady,
            this, [this](const cv::Mat &frame) {
                // Ждем, пока энкодер будет инициализирован
                if (m_videoEncoder && m_encoderInitialized) {
                    m_videoEncoder->encodeFrame(frame);
                } else {
                    // Если энкодер не готов, просто игнорируем кадр
                    static int warningCount = 0;
                    if (warningCount++ < 5) { // Ограничим количество предупреждений
                        qDebug() << "Encoder not ready yet, skipping frame";
                    }
                }
            });
#else
    connect(m_videoCapture, &VideoCapture::rawFrameReady,
            this, &StreamerWidget::onRawFrameReady);
    connect(m_videoCapture, &VideoCapture::frameForEncodingReady,
            this, &StreamerWidget::onFrameForEncoding);
#endif
    
    connect(m_videoCapture, &VideoCapture::errorOccurred,
            this, &StreamerWidget::onVideoError);

    qDebug() << "VideoCapture created for device:" << m_deviceIndex;
}

void StreamerWidget::cleanupVideoCapture()
{
    if (m_videoCapture) {
        qDebug() << "Cleaning up video capture for device:" << m_deviceIndex;
        
        m_videoCapture->stopCapture();
        
        if (!m_videoCapture->wait(2000)) {
            qWarning() << "Video capture thread didn't finish properly for device:" << m_deviceIndex;
            m_videoCapture->terminate();
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

    // В тестовом режиме используем фиктивный streamId если нет реального
#ifdef TEST_DECODER
    if (m_streamId == 0) {
        m_streamId = 1; // Фиктивный ID для тестирования
        m_displayId = streamIdToDisplayString(m_streamId);
        qDebug() << "Using test streamId:" << m_streamId << "for device:" << m_deviceIndex;
    }
#endif

    if (m_streamId == 0) {
        qWarning() << "Cannot initialize encoder without valid stream ID";
        return;
    }

    m_videoEncoder = new VideoEncoder(m_streamId, this);
    if (!m_videoEncoder) {
        qCritical() << "Failed to create VideoEncoder";
        return;
    }

#ifdef TEST_DECODER
    // Encoder -> FrameBuffer -> Decoder
    connect(m_videoEncoder, &VideoEncoder::encodedPacketReady,
            this, [this](int streamId, int frameNumber, const QByteArray &packet) {
                qDebug() << "StreamerWidget: Encoded frame" << frameNumber 
                         << "for stream" << streamId << "size:" << packet.size() << "bytes";
                
                // 1. Сохраняем в FrameBuffer
                if (m_frameBuffer) {
                    m_frameBuffer->insertFrame(frameNumber, packet);
                    qDebug() << "Frame saved to buffer";
                }
                
                // 2. Немедленно декодируем (для тестирования)
                if (m_testDecoder) {
                    m_testDecoder->decodeFrame(packet, frameNumber);
                }
            });
#else
    connect(m_videoEncoder, &VideoEncoder::encodedPacketReady,
            this, &StreamerWidget::onFrameEncoded);
#endif
    
    connect(m_videoEncoder, &VideoEncoder::errorOccurred,
            this, [this](const QString& error) {
                qCritical() << "VideoEncoder error:" << error;
                showError(QString("Encoder error: %1").arg(error));
            });

    // Инициализируем энкодер
    m_videoEncoder->initialize(DEFAULT_WIDTH, DEFAULT_HEIGHT, DEFAULT_FPS);
    m_encoderInitialized = true;

    qDebug() << "VideoEncoder initialized for stream:" << m_streamId;
}

void StreamerWidget::cleanupVideoEncoder()
{
    if (m_videoEncoder) {
        m_videoEncoder->cleanup();
        delete m_videoEncoder;
        m_videoEncoder = nullptr;
        m_encoderInitialized = false;
        qDebug() << "VideoEncoder cleaned up";
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
    } else {
        m_videoEncoder->setStreamId(streamId);
    }

    updateStatus();
}

void StreamerWidget::initializeWithRealId(uint32_t streamId, const QString &displayId)
{
    setStreamId(streamId, displayId);
    initialize();
}

void StreamerWidget::setStreamingEnabled(bool enabled)
{
    if (m_streamingEnabled == enabled) return;

    m_streamingEnabled = enabled;
    
    if (m_controlPanel) {
        m_controlPanel->setActive(true);
    }

    if (!enabled) {
        stopStream();
    }

    updateStatus();
}

void StreamerWidget::initialize()
{
    qDebug() << "Initializing StreamerWidget for device:" << m_deviceIndex;

    try {
#ifdef TEST_DECODER
        // 1. Создаем FrameBuffer если его нет
        if (!m_frameBuffer) {
            m_frameBuffer = new FrameBuffer(DEFAULT_BUFFERSZ);
            qDebug() << "FrameBuffer created with capacity:" << DEFAULT_BUFFERSZ;
        } else {
            qDebug() << "FrameBuffer already exists, reusing...";
        }
        
        // 2. Создаем декодер если его нет
        if (!m_testDecoder) {
            m_testDecoder = new VideoDecoder(DEFAULT_WIDTH, DEFAULT_HEIGHT, this);
            
            // Инициализируем декодер (метод initialize() возвращает void)
            try {
                m_testDecoder->initialize();
                qDebug() << "Test decoder initialized successfully";
                
                // Подключаем декодер к дисплею
                connect(m_testDecoder, &VideoDecoder::frameDecoded,
                        m_videoDisplay, &VideoDisplay::displayFrame, Qt::QueuedConnection);
                
                connect(m_testDecoder, &VideoDecoder::errorOccurred,
                        [this](const QString& err) { 
                            qWarning() << "Decoder error:" << err;
                            this->showError(QString("Decoder error: %1").arg(err));
                        });
                        
            } catch (const std::exception& e) {
                qCritical() << "Failed to initialize decoder:" << e.what();
                delete m_testDecoder;
                m_testDecoder = nullptr;
                showError(QString("Decoder init failed: %1").arg(e.what()));
            }
        } else {
            qDebug() << "Decoder already exists, reusing...";
        }
#endif
        
        // 3. Инициализируем видеозахват
        initializeVideoCapture();
        
        // 4. Включаем стриминг и обновляем статус
        setStreamingEnabled(true);
        updateStatus();
        
        // 5. Запускаем видеозахват (для превью)
        if (m_videoCapture) {
            m_videoCapture->startCapture();
        }
        
    } catch (const std::exception& e) {
        qCritical() << "Failed to initialize StreamerWidget:" << e.what();
        showError(QString("Failed to initialize: %1").arg(e.what()));
    }
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

    // Прямое соединение для локального показа превью
    if (m_videoCapture) {
        connect(m_videoCapture, &VideoCapture::rawFrameReady,
                this, &StreamerWidget::onRawFrameReady);
    }
}

void StreamerWidget::onRawFrameReady(const QImage &image)
{
    if (m_videoDisplay && m_streamingEnabled) {
        m_videoDisplay->displayFrame(image);
    }
}
void StreamerWidget::startStream()
{
    qDebug() << "StreamerWidget::startStream() called for device:" << m_deviceIndex;
    
    if (m_streamState != State_NoStream) {
        qWarning() << "Cannot start stream - wrong state:" << m_streamState;
        return;
    }

    if (!m_videoCapture) {
        qWarning() << "Cannot start stream - video capture not initialized";
        return;
    }

    // Очищаем буфер при начале нового стрима
#ifdef TEST_DECODER
    qDebug() << "Clearing FrameBuffer for new stream...";
    if (m_frameBuffer) {
        qDebug() << "FrameBuffer exists at address:" << m_frameBuffer 
                 << "capacity:" << (m_frameBuffer ? m_frameBuffer->capacity() : 0);
        m_frameBuffer->clear();
        qDebug() << "FrameBuffer cleared for new stream";
    } else {
        qCritical() << "FrameBuffer is null! Cannot clear.";
        return;  // Выходим, если буфера нет!
    }
#endif
    
    // Инициализируем кодировщик если еще не инициализирован
    if (!m_encoderInitialized) {
        qDebug() << "Initializing encoder for startStream...";
        initializeVideoEncoder();
    } else {
        qDebug() << "Encoder already initialized, reusing...";
    }
    
    // Проверяем, что энкодер успешно инициализирован
    if (!m_videoEncoder || !m_encoderInitialized) {
        qCritical() << "Failed to initialize video encoder for stream";
        return;
    }
    
    setStreamState(State_StreamCreated);
    
    // В тестовом режиме сразу переходим в активное состояние
#ifdef TEST_DECODER
    qDebug() << "TEST MODE: Immediately activating stream";
    setStreamState(State_StreamActive);
#else
    // Отправляем запрос на создание стрима
    if (m_streamManager) {
        m_streamManager->createStream(m_deviceIndex);
        qDebug() << "Stream creation requested for device:" << m_deviceIndex;
    } else {
        qWarning() << "StreamManager not available";
        setStreamState(State_StreamActive);
    }
#endif
    
    qDebug() << "Stream setup completed for device:" << m_deviceIndex;
}
// Исправленный метод setControlPanel - удаляем вызов setParent
void StreamerWidget::setControlPanel(StreamControlPanel* panel)
{
    if (m_controlPanel) {
        m_mainLayout->removeWidget(m_controlPanel);
        m_controlPanel->deleteLater();
    }

    m_controlPanel = panel;
    if (m_controlPanel) {
        // УБИРАЕМ: m_controlPanel->setParent(this); - это лишнее
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

// ... остальные методы оставляем без изменений (из предыдущей версии) ...

void StreamerWidget::setStreamState(StreamState newState)
{
    if (m_streamState == newState) return;
    
    StreamState oldState = m_streamState;
    m_streamState = newState;
    
    qDebug() << "Stream state changed for device" << m_deviceIndex 
             << ":" << oldState << "->" << newState;
    
    if (m_controlPanel) {
        m_controlPanel->setStreamState(static_cast<StreamControlPanel::StreamState>(newState));
        m_controlPanel->setViewersStatus(m_hasViewers);
    }
    
    updateStatus();
}

void StreamerWidget::stopStream()
{
    if (m_streamState == State_NoStream) {
        return;
    }

    qDebug() << "Stopping stream for device:" << m_deviceIndex;

    // Отправляем запрос на удаление стрима
    if (m_streamId != 0 && m_streamManager) {
        m_streamManager->deleteStream(m_streamId);
        qDebug() << "Stream deletion requested for:" << m_streamId;
    }

    // Останавливаем видеозахват
    if (m_videoCapture) {
        m_videoCapture->stopCapture();
    }

    // Очищаем кодировщик
    cleanupVideoEncoder();
    
    // Сбрасываем флаги
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

// Остальные методы остаются без изменений...
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






void StreamerWidget::setViewersStatus(bool hasViewers)
{
    if (m_hasViewers == hasViewers) return;

    m_hasViewers = hasViewers;
    
    if (m_controlPanel) {
        m_controlPanel->setViewersStatus(hasViewers ? 1 : 0);
    }

    updateStatus();
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
