// viewerwidget.cpp
#include "viewerwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QDebug>
#include "../network/udp/networkmanager.h" // ДОБАВИТЬ

const QString ViewerWidget::PLACEHOLDER_TEXT = "Waiting for video stream...";
const QString ViewerWidget::STATUS_ACTIVE = "● Connected";
const QString ViewerWidget::STATUS_INACTIVE = "● Disconnected";

ViewerWidget::ViewerWidget(uint32_t streamId, const QString &displayId, QWidget *parent)
    : QWidget(parent)
    , m_videoDisplay(nullptr)
    , m_controlPanel(nullptr)
    , m_mainLayout(nullptr)
    , m_displayBuffer(nullptr)
    , m_videoDecoder(nullptr)
    , m_networkManager(nullptr) // ДОБАВИТЬ инициализацию
    , m_streamId(streamId)
    , m_displayId(displayId)
    , m_active(false)
{
    setupUI();
    setupConnections();
    // УБРАТЬ вызов initialize() из конструктора - он будет вызван позже
}

ViewerWidget::~ViewerWidget()
{
    cleanup();
}

void ViewerWidget::setupUI()
{
    setStyleSheet(R"(
        ViewerWidget {
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
    m_videoDisplay->setStyleSheet(R"(
        VideoDisplay {
            background: #000000;
            border: none;
            border-radius: 6px;
            margin: 4px;
        }
    )");

    // Control panel
    m_controlPanel = new StreamControlPanel(StreamControlPanel::ViewerMode, this);
    m_controlPanel->setStreamId(m_displayId);
    m_controlPanel->setActive(false);

    m_mainLayout->addWidget(m_videoDisplay, 1);
    m_mainLayout->addWidget(m_controlPanel);
}

void ViewerWidget::setupConnections()
{
    // Control panel signals
    connect(m_controlPanel, &StreamControlPanel::leaveStreamRequested,
            this, &ViewerWidget::onLeaveButtonClicked);
}

// ПЕРЕРАБОТАТЬ initialize для работы с NetworkManager
void ViewerWidget::initialize()
{
    qDebug() << "Initializing ViewerWidget for stream:" << m_displayId << "ID:" << m_streamId;

    // ИСПРАВИТЬ создание VideoDecoder - правильные параметры
    m_videoDecoder = new VideoDecoder(DEFAULT_WIDTH, DEFAULT_HEIGHT, this);
    connect(m_videoDecoder, &VideoDecoder::frameDecoded,
            this, &ViewerWidget::onFrameReady);

    // УБРАТЬ NetworkDisplayBuffer - он не нужен в новой архитектуре
    // m_displayBuffer = new NetworkDisplayBuffer(m_streamId, this);

    // Подключаемся к NetworkManager для получения собранных кадров
    if (m_networkManager) {
        connect(m_networkManager, &NetworkManager::frameAssembled,
                this, [this](int streamId, int frameNumber, const QByteArray &frameData) {
                    if (streamId == static_cast<int>(m_streamId) && m_active) {
                        // Передаем собранный кадр в декодер
                        m_videoDecoder->decodeFrame(frameData, frameNumber);
                    }
                });
        qDebug() << "ViewerWidget connected to NetworkManager for stream:" << m_streamId;
    } else {
        qWarning() << "NetworkManager not set for ViewerWidget, stream:" << m_streamId;
        m_videoDisplay->setPlaceholderText("Waiting for network connection...");
    }

    setActive(true);
    updateStatus();
    
    qDebug() << "ViewerWidget initialized for stream:" << m_displayId;
}



void ViewerWidget::cleanup()
{
    qDebug() << "Cleaning up ViewerWidget for stream:" << m_displayId;

    setActive(false);

    // Отключаемся от NetworkManager
    if (m_networkManager) {
        m_networkManager->disconnect(this);
    }

    if (m_videoDecoder) {
        m_videoDecoder->cleanup();
        delete m_videoDecoder;
        m_videoDecoder = nullptr;
    }

    // УБРАТЬ cleanup для displayBuffer
    // if (m_displayBuffer) {
    //    delete m_displayBuffer;
    //    m_displayBuffer = nullptr;
    // }

    clearDisplay();
}

// ДОБАВИТЬ метод для установки NetworkManager

void ViewerWidget::setNetworkManager(NetworkManager* networkManager)
{
    if (m_networkManager == networkManager) return;

    // Отключаемся от старого NetworkManager
    if (m_networkManager) {
        m_networkManager->disconnect(this);
    }

    m_networkManager = networkManager;

    // Подключаемся к новому NetworkManager если мы уже инициализированы
    if (m_networkManager && m_videoDecoder) {
        connect(m_networkManager, &NetworkManager::frameAssembled,
                this, [this](int streamId, int frameNumber, const QByteArray &frameData) {
                    if (streamId == static_cast<int>(m_streamId) && m_active) {
                        m_videoDecoder->decodeFrame(frameData, frameNumber);
                    }
                });
        qDebug() << "ViewerWidget connected to NetworkManager for stream:" << m_streamId;
    }
}

void ViewerWidget::setActive(bool active)
{
    if (m_active == active) return;

    m_active = active;
    
    if (m_controlPanel) {
        m_controlPanel->setActive(active);
        m_controlPanel->setConnectionStatus(active);
    }

    updateStatus();
}

void ViewerWidget::setStreamId(uint32_t streamId, const QString &displayId)
{
    m_streamId = streamId;
    m_displayId = displayId;

    if (m_controlPanel) {
        m_controlPanel->setStreamId(displayId);
    }

    // Обновляем ID только в VideoDecoder
    if (m_videoDecoder) {
        // VideoDecoder может не иметь setStreamId - убрать если нет такого метода
        // m_videoDecoder->setStreamId(streamId);
    }

    updateStatus();
}

void ViewerWidget::displayFrame(const QImage &frame)
{
    if (!m_videoDisplay || !m_active) return;

    if (frame.isNull()) {
        qWarning() << "Received null frame for stream:" << m_displayId;
        return;
    }

    m_videoDisplay->displayFrame(frame);
}

void ViewerWidget::clearDisplay()
{
    if (m_videoDisplay) {
        m_videoDisplay->clear();  // ИСПРАВИТЬ: использовать clear() вместо clearDisplay()
        m_videoDisplay->setPlaceholderText(PLACEHOLDER_TEXT);
    }
}

void ViewerWidget::setControlPanel(StreamControlPanel* panel)
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
        m_controlPanel->setActive(m_active);

        // Reconnect signals
        connect(m_controlPanel, &StreamControlPanel::leaveStreamRequested,
                this, &ViewerWidget::onLeaveButtonClicked);
    }
}

void ViewerWidget::updateStatus()
{
    if (m_controlPanel) {
        m_controlPanel->setConnectionStatus(m_active);
    }
}

void ViewerWidget::onFrameAssembled(int streamId, int frameNumber, const QByteArray &frameData)
{
    if (streamId != static_cast<int>(m_streamId) || !m_active) {
        return;
    }

    // Передаем собранный кадр в декодер
    if (m_videoDecoder) {
        m_videoDecoder->decodeFrame(frameData);
    }
}

void ViewerWidget::onFrameReady(const QImage &frame, int frameNumber)
{
    Q_UNUSED(frameNumber)
    displayFrame(frame);
}

void ViewerWidget::onLeaveRequested()
{
    qDebug() << "Leave requested for stream:" << m_displayId;
    
    setActive(false);
    clearDisplay();
    
    emit streamLeft(m_streamId);
}

void ViewerWidget::onLeaveButtonClicked()
{
    qDebug() << "Leave button clicked for stream:" << m_displayId;
    onLeaveRequested();
}

void ViewerWidget::onStreamJoined(uint32_t streamId)
{
    if (streamId == m_streamId) {
        setActive(true);
        qDebug() << "ViewerWidget: successfully joined stream" << m_streamId;
    }
}

void ViewerWidget::onStreamLeft(uint32_t streamId)
{
    if (streamId == m_streamId) {
        setActive(false);
        clearDisplay();
        qDebug() << "ViewerWidget: left stream" << m_streamId;
    }
}

// УБРАТЬ старый обработчик - он больше не нужен
/*
void ViewerWidget::onVideoPacketReceived(uint32_t streamId, const QByteArray& packet)
{
    // Этот метод больше не используется - пакеты обрабатываются через NetworkManager
}
*/
void ViewerWidget::onNetworkError(const QString& error)
{
    qCritical() << "Network error for viewer stream" << m_streamId << ":" << error;
    setActive(false);
    m_videoDisplay->setPlaceholderText(QString("Network error: %1").arg(error));
}

