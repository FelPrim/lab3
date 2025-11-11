#include "streampublisherwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QDebug>

const QString StreamPublisherWindow::STATUS_NO_VIEWERS = "👁️ No viewers";
const QString StreamPublisherWindow::STATUS_HAS_VIEWERS = "👁️✅ Viewers connected";
const QString StreamPublisherWindow::STATUS_STOPPED = "⏹️ Stream stopped";

StreamPublisherWindow::StreamPublisherWindow(int streamId, QWidget *parent)
    : QWidget(parent)
    , m_streamId(streamId)
    , m_isStreaming(false)
    , m_hasViewers(false)
{
    setupUI();
    setWindowTitle(QString("Stream Publisher - ID: %1").arg(streamId));
    resize(600, 500);
    
    // Заглушка: имитируем запуск потока через 1 секунду
    QTimer::singleShot(1000, this, [this]() {
        m_isStreaming = true;
        updateStatusLabel();
        m_videoDisplay->setPlaceholderText("🎥 Live Camera Feed\n\nStreaming to server...");
    });
}

StreamPublisherWindow::~StreamPublisherWindow()
{
    qDebug() << "StreamPublisherWindow destroyed, ID:" << m_streamId;
}

void StreamPublisherWindow::initialize(NetworkManager *networkManager)
{
    qDebug() << "StreamPublisherWindow initialized for stream:" << m_streamId;
}

void StreamPublisherWindow::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(10);
    m_mainLayout->setContentsMargins(15, 15, 15, 15);
    
    // Stream ID display
    auto idLayout = new QHBoxLayout();
    m_streamIdLabel = new QLabel("Stream ID: ---", this);
    m_streamIdLabel->setStyleSheet("font-weight: bold; font-size: 16px; color: #4FC3F7;");
    idLayout->addWidget(m_streamIdLabel);
    idLayout->addStretch();
    m_mainLayout->addLayout(idLayout);
    
    // Status label
    m_statusLabel = new QLabel(STATUS_STOPPED, this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet("font-size: 14px; padding: 10px; background: #2d2d2d; border-radius: 5px;");
    m_mainLayout->addWidget(m_statusLabel);
    
    // Device selector
    m_deviceSelector = new DeviceSelectorWidget(this);
    m_mainLayout->addWidget(m_deviceSelector);
    
    // Video display placeholder
    m_videoDisplay = new StreamVideoDisplayPanel(this);
    m_videoDisplay->setPlaceholderText("🎥 Camera Preview\n\nStream will start automatically when viewers connect");
    m_mainLayout->addWidget(m_videoDisplay, 1);
    
    // Statistics
    m_statsWidget = new StreamStatsWidget(StreamStatsWidget::PublisherStats, this);
    m_mainLayout->addWidget(m_statsWidget);
    
    // Control panel
    m_controlPanel = new StreamControlPanel(StreamControlPanel::Publisher, this);
    m_mainLayout->addWidget(m_controlPanel);
    
    setupConnections();
}

void StreamPublisherWindow::setupConnections()
{
    connect(m_controlPanel, &StreamControlPanel::stopRequested, this, &StreamPublisherWindow::onStopRequested);
    connect(m_deviceSelector, &DeviceSelectorWidget::deviceSelected, this, &StreamPublisherWindow::onDeviceSelected);
}

void StreamPublisherWindow::onStopRequested()
{
    qDebug() << "Stop stream requested for:" << m_streamId;
    
    auto reply = QMessageBox::question(this, "Stop Stream", 
                                     "Are you sure you want to stop this stream?",
                                     QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        m_isStreaming = false;
        m_hasViewers = false;
        updateStatusLabel();
        emit streamStopped(m_streamId);
        close();
    }
}

void StreamPublisherWindow::onDeviceSelected(int deviceIndex)
{
    qDebug() << "Device selected:" << deviceIndex;
    m_videoDisplay->setPlaceholderText(QString("🎥 Camera #%1\n\nDevice changed, restarting stream...").arg(deviceIndex));
}

void StreamPublisherWindow::setViewersStatus(bool hasViewers)
{
    m_hasViewers = hasViewers;
    updateStatusLabel();
    qDebug() << "Viewers status changed for stream" << m_streamId << ":" << hasViewers;
}

void StreamPublisherWindow::setStreamId(int streamId, const QString &displayId)
{
    m_streamId = streamId;
    m_displayId = displayId;
    m_streamIdLabel->setText(QString("Stream ID: %1").arg(displayId));
    m_controlPanel->setStreamId(displayId);
    setWindowTitle(QString("Stream Publisher - %1").arg(displayId));
}

void StreamPublisherWindow::startStream()
{
    m_isStreaming = true;
    updateStatusLabel();
}

void StreamPublisherWindow::stopStream()
{
    m_isStreaming = false;
    updateStatusLabel();
}

void StreamPublisherWindow::updateStatusLabel()
{
    if (!m_isStreaming) {
        m_statusLabel->setText(STATUS_STOPPED);
        m_statusLabel->setStyleSheet("font-size: 14px; padding: 10px; background: #757575; color: white; border-radius: 5px;");
    } else if (m_hasViewers) {
        m_statusLabel->setText(STATUS_HAS_VIEWERS);
        m_statusLabel->setStyleSheet("font-size: 14px; padding: 10px; background: #388E3C; color: white; border-radius: 5px;");
    } else {
        m_statusLabel->setText(STATUS_NO_VIEWERS);
        m_statusLabel->setStyleSheet("font-size: 14px; padding: 10px; background: #F57C00; color: white; border-radius: 5px;");
    }
}
