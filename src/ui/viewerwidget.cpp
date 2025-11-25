// viewerwidget.cpp
#include "viewerwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QDebug>

const QString ViewerWidget::PLACEHOLDER_TEXT = "Waiting for video stream...";
const QString ViewerWidget::STATUS_ACTIVE = "● Connected";
const QString ViewerWidget::STATUS_INACTIVE = "● Disconnected";

ViewerWidget::ViewerWidget(uint32_t streamId, const QString &displayId, QWidget *parent)
    : QWidget(parent)
    , m_videoDisplay(nullptr)
    , m_controlPanel(nullptr)
    , m_mainLayout(nullptr)
    , m_displayBuffer(nullptr)
    , m_streamId(streamId)
    , m_displayId(displayId)
    , m_active(false)
{
    setupUI();
    setupConnections();
    initialize();
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
   // m_videoDisplay->setPlaceholderText(PLACEHOLDER_TEXT);
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

    // Note: NetworkDisplayBuffer will be provided by the network layer
    // For now, we'll handle frame assembly directly from network packets
    
    setActive(true);
    updateStatus();
}

void ViewerWidget::cleanup()
{
    qDebug() << "Cleaning up ViewerWidget for stream:" << m_displayId;

    setActive(false);

    // NetworkDisplayBuffer will be cleaned up by network layer
    m_displayBuffer = nullptr;

    clearDisplay();
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
    // Status is primarily handled by the control panel in ViewerMode
    if (m_controlPanel) {
        m_controlPanel->setConnectionStatus(m_active);
    }
}

void ViewerWidget::onFrameAssembled(int, int, const QByteArray&) {
    // ... существующий код до проблемной части ...
    
    // ЗАКОММЕНТИРОВАТЬ проблемные строки:
    /*
    if (m_framebuffer->isEmpty()) {
        QPixmap placeholder(m_videoDisplay->size());
        placeholder.fill(Qt::black);
        
        QPainter painter(&placeholder);
        painter.setPen(Qt::white);
        painter.drawText(placeholder.rect(), Qt::AlignCenter, PLACEHOLDER_TEXT);
        m_videoDisplay->setPixmap(placeholder);
    }
    */
    
    // ВРЕМЕННАЯ ЗАГЛУШКА:
    qDebug() << "Frame assembled received";
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