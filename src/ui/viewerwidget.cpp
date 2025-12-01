#include "viewerwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QDebug>

const QString ViewerWidget::PLACEHOLDER_TEXT = "Waiting for video stream...";
const QString ViewerWidget::STATUS_ACTIVE = "● Connected";
const QString ViewerWidget::STATUS_INACTIVE = "● Disconnected";

ViewerWidget::ViewerWidget(uint32_t streamId, const QString &displayId, uint32_t callId, QWidget *parent)
    : QWidget(parent)
    , m_videoDisplay(nullptr)
    , m_controlPanel(nullptr)
    , m_mainLayout(nullptr)
    , m_displayBuffer(nullptr)
    , m_videoDecoder(nullptr)
    , m_networkManager(nullptr)
    , m_streamId(streamId)
    , m_displayId(displayId)
    , m_callId(callId)
    , m_active(false)
{
    setupUI();
    setupConnections();
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

void ViewerWidget::initialize()
{
    qDebug() << "Initializing ViewerWidget for stream:" << m_displayId << "ID:" << m_streamId;

    if (!m_networkManager) {
        qWarning() << "NetworkManager not set before initialize()";
        m_videoDisplay->setPlaceholderText("Waiting for network connection...");
        return;
    }
    m_networkManager->setCallId(m_callId);
    // Create video decoder
    m_videoDecoder = new VideoDecoder(DEFAULT_WIDTH, DEFAULT_HEIGHT, this);
    connect(m_videoDecoder, &VideoDecoder::frameDecoded,
            this, &ViewerWidget::onFrameReady);

    // Connect to NetworkManager signals
    connect(m_networkManager, &NetworkManager::frameAssembled,
            this, &ViewerWidget::onFrameAssembled);
    
    // Note: NetworkManager doesn't have networkErrorOccurred signal, using errorOccurred if available
    // If NetworkManager has errorOccurred signal, connect it:
    // connect(m_networkManager, &NetworkManager::errorOccurred,
    //         this, &ViewerWidget::onNetworkError);

    setActive(true);
    updateStatus();
    
    qDebug() << "ViewerWidget initialized for stream:" << m_displayId;
}

void ViewerWidget::cleanup()
{
    qDebug() << "Cleaning up ViewerWidget for stream:" << m_displayId;

    setActive(false);

    // Disconnect from NetworkManager
    if (m_networkManager) {
        m_networkManager->disconnect(this);
    }

    if (m_videoDecoder) {
        m_videoDecoder->cleanup();
        delete m_videoDecoder;
        m_videoDecoder = nullptr;
    }

    clearDisplay();
}

void ViewerWidget::setNetworkManager(NetworkManager* networkManager)
{
    if (m_networkManager == networkManager) return;

    // Disconnect from old NetworkManager
    if (m_networkManager) {
        m_networkManager->disconnect(this);
    }

    m_networkManager = networkManager;

    // Connect to new NetworkManager if we're already initialized
    if (m_networkManager && m_videoDecoder) {
        connect(m_networkManager, &NetworkManager::frameAssembled,
                this, &ViewerWidget::onFrameAssembled);
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
        m_videoDisplay->clear();
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

    // Pass assembled frame to decoder with frameNumber
    if (m_videoDecoder) {
        m_videoDecoder->decodeFrame(frameData, frameNumber);
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

void ViewerWidget::onNetworkError(const QString& error)
{
    qCritical() << "Network error for viewer stream" << m_streamId << ":" << error;
    setActive(false);
    m_videoDisplay->setPlaceholderText(QString("Network error: %1").arg(error));
}