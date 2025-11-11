#include "streampublisherwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QTimer>
#include <QFrame>
#include <QDebug>

const QString StreamPublisherWindow::STATUS_NO_VIEWERS = "No viewers";
const QString StreamPublisherWindow::STATUS_HAS_VIEWERS = "Viewers connected";
const QString StreamPublisherWindow::STATUS_STOPPED = "Stream stopped";

StreamPublisherWindow::StreamPublisherWindow(int streamId, QWidget *parent)
    : StreamWindow(parent)
    , m_streamId(streamId)
    , m_isStreaming(false)
    , m_hasViewers(false)
{
    setupUI();
    setWindowTitle(QString("Stream Publisher - ID: %1").arg(streamId));
    setMinimumSize(700, 600);
    resize(800, 650);
    
    // Заглушка: имитируем запуск потока через 1 секунду
    QTimer::singleShot(1000, this, [this]() {
        m_isStreaming = true;
        updateStatusLabel();
        m_videoDisplay->setPlaceholderText("Live Camera Feed\n\nStreaming to server...");
    });
}

StreamPublisherWindow::~StreamPublisherWindow()
{
    qDebug() << "StreamPublisherWindow destroyed, ID:" << m_streamId;
}

void StreamPublisherWindow::initialize()  // Исправлено: убран параметр NetworkManager
{
    qDebug() << "StreamPublisherWindow initialized for stream:" << m_streamId;
}

void StreamPublisherWindow::setupUI()
{
    // Вызываем общую настройку UI из базового класса
    setupCommonUI();
    
    // Main container with better styling
    setStyleSheet(
        "QWidget {"
        "   background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1,"
        "                               stop: 0 #1e1e2e, stop: 1 #252536);"
        "   color: #ffffff;"
        "}"
    );
    
    // Device selector section
    auto deviceSection = new QFrame(this);
    deviceSection->setStyleSheet(
        "QFrame {"
        "   background: rgba(255, 255, 255, 0.05);"
        "   border: 1px solid #444;"
        "   border-radius: 8px;"
        "   padding: 10px;"
        "}"
    );
    auto deviceLayout = new QVBoxLayout(deviceSection);
    
    auto deviceHeader = new QLabel("Video Source", this);
    deviceHeader->setStyleSheet(
        "font-size: 16px;"
        "font-weight: bold;"
        "color: #e9ecef;"
        "padding: 5px 0;"
    );
    deviceLayout->addWidget(deviceHeader);
    
    m_deviceSelector = new DeviceSelectorWidget(this);
    deviceLayout->addWidget(m_deviceSelector);
    
    m_mainLayout->addWidget(deviceSection);
    
    // Video display section
    auto videoSection = new QFrame(this);
    videoSection->setStyleSheet(
        "QFrame {"
        "   background: rgba(0, 0, 0, 0.3);"
        "   border: 2px solid #555;"
        "   border-radius: 8px;"
        "   padding: 0px;"
        "}"
    );
    auto videoLayout = new QVBoxLayout(videoSection);
    
    m_videoDisplay = new StreamVideoDisplayPanel(this);
    m_videoDisplay->setPlaceholderText("Camera Preview\n\nStream will start automatically when viewers connect");
    videoLayout->addWidget(m_videoDisplay);
    
    m_mainLayout->addWidget(videoSection, 1);
    
    // Statistics section
    auto statsSection = new QFrame(this);
    statsSection->setStyleSheet(
        "QFrame {"
        "   background: rgba(255, 255, 255, 0.03);"
        "   border: 1px solid #444;"
        "   border-radius: 6px;"
        "   padding: 8px;"
        "}"
    );
    auto statsLayout = new QVBoxLayout(statsSection);
    
    auto statsHeader = new QLabel("Stream Statistics", this);
    statsHeader->setStyleSheet(
        "font-size: 14px;"
        "font-weight: bold;"
        "color: #adb5bd;"
        "padding: 2px 0;"
    );
    statsLayout->addWidget(statsHeader);
    
    m_statsWidget = new StreamStatsWidget(StreamStatsWidget::PublisherStats, this);
    statsLayout->addWidget(m_statsWidget);
    
    m_mainLayout->addWidget(statsSection);
    
    // Control panel section
    auto controlSection = new QFrame(this);
    controlSection->setStyleSheet(
        "QFrame {"
        "   background: rgba(255, 255, 255, 0.05);"
        "   border: 1px solid #555;"
        "   border-radius: 8px;"
        "   padding: 15px;"
        "}"
    );
    auto controlLayout = new QVBoxLayout(controlSection);
    
    m_controlPanel = new StreamControlPanel(StreamControlPanel::Publisher, this);
    controlLayout->addWidget(m_controlPanel);
    
    m_mainLayout->addWidget(controlSection);
    
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
    setStreamInfo(QString("Stream ID: %1").arg(displayId));  // Исправлено: используем базовый метод
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
    // Исправлено: используем базовый метод setStatus
    if (!m_isStreaming) {
        setStatus(STATUS_STOPPED, "#757575");
    } else if (m_hasViewers) {
        setStatus(STATUS_HAS_VIEWERS, "#388E3C");
    } else {
        setStatus(STATUS_NO_VIEWERS, "#F57C00");
    }
}

// Добавляем недостающий метод (заглушка)
void StreamPublisherWindow::onVideoError(const QString &message)
{
    showError(message);
}

void StreamPublisherWindow::cleanup()
{
    qDebug() << "StreamPublisherWindow cleanup for stream:" << m_streamId;
    
    // Останавливаем поток если он запущен
    stopStream();
    
    // Базовый cleanup
    StreamWindow::cleanup();
}
