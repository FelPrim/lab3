// streamerwidget.cpp
#include "streamerwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QDebug>
#include <QTimer>
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

// ИСПРАВЛЕННЫЙ конструктор
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
    , m_bufferedDecoder(nullptr)  // ТОЛЬКО этот указатель
#endif
{
    setupUI();
    setupConnections();
    
    if (m_controlPanel) {
        m_controlPanel->setActive(true);
        m_controlPanel->setStreamState(StreamControlPanel::StateNoStream);
        m_controlPanel->setViewersStatus(false);
    }
    
    qDebug() << "StreamerWidget created for device:" << deviceIndex;
}

StreamerWidget::~StreamerWidget()
{
    cleanup();
}

void StreamerWidget::cleanupTestObjects()
{
#ifdef TEST_DECODER
    qDebug() << "Cleaning up test objects for device:" << m_deviceIndex;
    
    // Удаляем BufferedVideoDecoder
    if (m_bufferedDecoder) {
        m_bufferedDecoder->cleanup();
        delete m_bufferedDecoder;
        m_bufferedDecoder = nullptr;
        qDebug() << "BufferedVideoDecoder deleted";
    }
#endif
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

    stopStream();
    setStreamingEnabled(false);
    cleanupTestObjects();
    cleanupVideoEncoder();
    cleanupVideoCapture();

    if (m_videoDisplay) {
        m_videoDisplay->clear();
        m_videoDisplay->setPlaceholderText("Device disconnected");
    }
    
    m_streamId = 0;
    m_displayId = "---";
    
    qDebug() << "StreamerWidget cleanup completed for device:" << m_deviceIndex;
}

void StreamerWidget::initializeVideoCapture()
{
    cleanupVideoCapture();

    m_videoCapture = new VideoCapture(m_deviceIndex, this);
    
#ifdef TEST_DECODER
    connect(m_videoCapture, &VideoCapture::frameForEncodingReady,
            this, [this](const cv::Mat &frame) {                
                if (m_videoEncoder && m_encoderInitialized) {
                    m_videoEncoder->encodeFrame(frame);
                } else {
                    qDebug() << "[TEST ENCODING] Encoder not ready, dropping frame";
                }
            });
#else
    connect(m_videoCapture, &VideoCapture::frameForEncodingReady,
            this, &StreamerWidget::onFrameForEncoding);
#endif
    
    // Добавим также отладку для rawFrameReady
    connect(m_videoCapture, &VideoCapture::rawFrameReady,
            this, [this](const QImage &image) {
                static int rawFrameCounter = 0;
                rawFrameCounter++;
                if (m_videoDisplay && m_streamingEnabled) {
                    m_videoDisplay->displayFrame(image);
                }
            });
    
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
            qWarning() << "Video capture thread didn't finish properly";
            m_videoCapture->terminate();
            m_videoCapture->wait();
        }
        
        delete m_videoCapture;
        m_videoCapture = nullptr;
        qDebug() << "Video capture cleaned up";
    }
}

void StreamerWidget::initializeVideoEncoder()
{
    cleanupVideoEncoder();

#ifdef TEST_DECODER
    if (m_streamId == 0) {
        m_streamId = 1;
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
    // Отправляем закодированные кадры в BufferedVideoDecoder
    connect(m_videoEncoder, &VideoEncoder::encodedPacketReady,
            this, [this](int streamId, int frameNumber, const QByteArray &packet) {
                qDebug() << "[ENCODER->BUFFERED_DECODER] Encoded frame" << frameNumber 
                         << "size:" << packet.size() << "bytes";
                
                // Отправляем в BufferedVideoDecoder
                if (m_bufferedDecoder) {
                    m_bufferedDecoder->addFrame(frameNumber, packet);
                } else {
                    qWarning() << "BufferedVideoDecoder is null!";
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
void StreamerWidget::initialize()
{
    qDebug() << "Initializing StreamerWidget for device:" << m_deviceIndex;

    try {
#ifdef TEST_DECODER
        // 1. Создаем BufferedVideoDecoder
        if (!m_bufferedDecoder) {
            // Используем автоматический расчет задержки (передаем -1)
            m_bufferedDecoder = new BufferedVideoDecoder(DEFAULT_WIDTH, DEFAULT_HEIGHT, 
                                                        DEFAULT_FPS, -1, this);
            m_bufferedDecoder->initialize();
            qDebug() << "BufferedVideoDecoder created and initialized";
            
            // ВАЖНО: Подключаем сигнал правильно
            connect(m_bufferedDecoder, &BufferedVideoDecoder::frameReady,
                    this, [this](const QImage &image, int frameNumber) {
                        qDebug() << "[BUFFERED_DECODER] Frame" << frameNumber << "ready, size:" 
                                 << image.size() << "format:" << image.format();
                        
                        if (m_videoDisplay) {
                            m_videoDisplay->displayFrame(image);
                        } else {
                            qWarning() << "VideoDisplay is null!";
                        }
                    }, Qt::QueuedConnection);
            
            // Обработка ошибок
            connect(m_bufferedDecoder, &BufferedVideoDecoder::errorOccurred,
                    this, [this](const QString& err) { 
                        qWarning() << "BufferedVideoDecoder error:" << err;
                        showError(QString("Decoder error: %1").arg(err));
                    });
        }
#endif
        
        // 2. Инициализируем видеозахват
        initializeVideoCapture();
        
        // 3. Включаем стриминг
        setStreamingEnabled(true);
        updateStatus();
        
        // 4. Запускаем видеозахват для превью
        if (m_videoCapture) {
            m_videoCapture->startCapture();
        }
        
    } catch (const std::exception& e) {
        qCritical() << "Failed to initialize StreamerWidget:" << e.what();
        showError(QString("Failed to initialize: %1").arg(e.what()));
    }
}

// Остальные методы без изменений...

void StreamerWidget::setupConnections()
{
    connect(m_controlPanel, &StreamControlPanel::startStreamRequested,
            this, &StreamerWidget::onStartStreamRequested);
    connect(m_controlPanel, &StreamControlPanel::stopStreamRequested,
            this, &StreamerWidget::onStopStreamRequested);
    connect(m_controlPanel, &StreamControlPanel::disconnectRequested,
            this, &StreamerWidget::onDisconnectRequested);

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
    qDebug() << "=== StreamerWidget::startStream() DEBUG ===";
    qDebug() << "Device index:" << m_deviceIndex;
    qDebug() << "Stream state:" << m_streamState;
    qDebug() << "StreamManager pointer:" << m_streamManager;
    
    if (m_streamState != State_NoStream) {
        qWarning() << "Cannot start stream - wrong state:" << m_streamState;
        return;
    }

    if (!m_videoCapture) {
        qWarning() << "Cannot start stream - video capture not initialized";
        return;
    }

#ifdef TEST_DECODER
    qDebug() << "Clearing BufferedVideoDecoder for new stream...";
    if (m_bufferedDecoder) {
        m_bufferedDecoder->clear();
    }
#endif
    
    // ИЗМЕНЕНИЕ 1: НЕ инициализируем энкодер здесь!
    // Вместо этого сначала создаем стрим на сервере
    
    setStreamState(State_StreamCreated);
    
    qDebug() << "DEBUG: Before calling StreamManager::createStream()";
    if (m_streamManager) {
        // Вызываем createStream БЕЗ предварительной инициализации энкодера
        m_streamManager->createStream(m_deviceIndex);
        qDebug() << "Stream creation requested for device:" << m_deviceIndex;
    } else {
        qCritical() << "ERROR: StreamManager is NULL!";
        return;
    }
    
    qDebug() << "=== StreamerWidget::startStream() END ===";
    
    // Энкодер будет инициализирован ПОСЛЕ получения streamId от сервера
    // в методе onServerStreamCreated()
}
// Остальные методы остаются без изменений (как в вашем исходном файле)...
// [Вставьте сюда оставшуюся часть вашего кода без изменений]

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
        m_sendingPackets = true;  // Теперь можно отправлять пакеты
        qDebug() << "Stream started - now sending packets for:" << m_displayId;
        
        setViewersStatus(true);  // Это обновит UI
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
        m_sendingPackets = false;
        qDebug() << "Stream ended - stopped sending packets for:" << m_displayId;
        
        // ОПОВЕЩАЕМ ОБ ОТКЛЮЧЕНИИ ПОСЛЕДНЕГО КЛИЕНТА
        setViewersStatus(false);  // Это обновит UI
    }
}

// Модифицируем обработчик кадров для кодирования
void StreamerWidget::onFrameForEncoding(const cv::Mat &frame)
{
    
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

// Вставьте этот код после метода processBufferedFrames() или в соответствующее место

void StreamerWidget::setStreamId(uint32_t streamId, const QString &displayId)
{
    m_streamId = streamId;
    m_displayId = displayId;

    if (m_controlPanel) {
        m_controlPanel->setStreamId(displayId);
    }

    // Если энкодер еще не создан - создаем его
    if (!m_videoEncoder && m_streamingEnabled) {
        initializeVideoEncoder();
    } else if (m_videoEncoder) {
        // Если энкодер уже существует - обновляем streamId
        m_videoEncoder->setStreamId(streamId);
    }

    updateStatus();
    qDebug() << "Stream ID set to:" << streamId << "(" << displayId << ")";
}

void StreamerWidget::setStreamingEnabled(bool enabled)
{
    if (m_streamingEnabled == enabled) {
        return;
    }

    m_streamingEnabled = enabled;
    
    if (m_controlPanel) {
        m_controlPanel->setActive(enabled);
    }

    if (enabled) {
        // Если включаем стриминг и есть streamId - инициализируем энкодер
        if (m_streamId != 0 && !m_videoEncoder) {
            initializeVideoEncoder();
        }
    } else {
        // Если выключаем стриминг - останавливаем все
        stopStream();
        
        // Также очищаем энкодер
        cleanupVideoEncoder();
    }

    updateStatus();
    qDebug() << "Streaming enabled:" << enabled << "for device:" << m_deviceIndex;
}

void StreamerWidget::initializeWithRealId(uint32_t streamId, const QString &displayId)
{
    setStreamId(streamId, displayId);
    initialize();
}




