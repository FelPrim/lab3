
#include "streamviewerwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QTimer>
#include <QFrame>
#include <QDebug>

const QString StreamViewerWindow::STATUS_DISCONNECTED = "Disconnected";
const QString StreamViewerWindow::STATUS_CONNECTING = "Connecting...";
const QString StreamViewerWindow::STATUS_CONNECTED = "Connected";
const QString StreamViewerWindow::STATUS_ENDED = "Stream ended";

StreamViewerWindow::StreamViewerWindow(QWidget *parent)
    : StreamWindow(parent)
    , m_streamId(-1)
    , m_isJoined(false)
{
    setupUI();
    setWindowTitle("Stream Viewer");
    setMinimumSize(700, 600);
    resize(800, 650);
}

void StreamViewerWindow::setupUI()
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
    
    // Stream ID input section
    auto inputSection = new QFrame(this);
    inputSection->setStyleSheet(
        "QFrame {"
        "   background: rgba(255, 255, 255, 0.05);"
        "   border: 1px solid #444;"
        "   border-radius: 8px;"
        "   padding: 15px;"
        "}"
    );
    auto inputLayout = new QVBoxLayout(inputSection);
    
    auto inputHeader = new QLabel("Join Stream", this);
    inputHeader->setStyleSheet(
        "font-size: 16px;"
        "font-weight: bold;"
        "color: #e9ecef;"
        "padding: 5px 0;"
    );
    inputLayout->addWidget(inputHeader);
    
    auto inputHelp = new QLabel("Enter the 6-character stream ID to join:", this);
    inputHelp->setStyleSheet(
        "color: #adb5bd;"
        "font-size: 12px;"
        "padding: 2px 0 10px 0;"
    );
    inputLayout->addWidget(inputHelp);
    
    m_idInputWidget = new StreamIdInputWidget(this);
    inputLayout->addWidget(m_idInputWidget);
    
    m_mainLayout->addWidget(inputSection);
    
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
    m_videoDisplay->setPlaceholderText("Stream Preview\n\nEnter Stream ID and click Join to start watching");
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
    
    m_statsWidget = new StreamStatsWidget(StreamStatsWidget::ViewerStats, this);
    statsLayout->addWidget(m_statsWidget);
    
    m_mainLayout->addWidget(statsSection);
    
    // Control panel section (скрыт до подключения)
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
    
    m_controlPanel = new StreamControlPanel(StreamControlPanel::Viewer, this);
    m_controlPanel->setVisible(false);
    controlLayout->addWidget(m_controlPanel);
    
    m_mainLayout->addWidget(controlSection);
    
    setupConnections();
}

StreamViewerWindow::~StreamViewerWindow()
{
    qDebug() << "StreamViewerWindow destroyed, ID:" << m_streamId;
}

void StreamViewerWindow::initialize()  // Исправлено: убран параметр NetworkManager
{
    qDebug() << "StreamViewerWindow initialized";
}



void StreamViewerWindow::setupConnections()
{
    connect(m_idInputWidget, &StreamIdInputWidget::joinRequested, this, &StreamViewerWindow::onJoinRequested);
    connect(m_controlPanel, &StreamControlPanel::leaveRequested, this, &StreamViewerWindow::onLeaveRequested);
}

void StreamViewerWindow::onJoinRequested(const QString &streamId)
{
    qDebug() << "Join stream requested:" << streamId;
    
    if (streamId.length() != 6) {
        showError("Stream ID must be exactly 6 capital letters");
        return;
    }
    
    setStatus(STATUS_CONNECTING, "#FF8F00");
    
    // Заглушка: имитируем подключение через 2 секунды
    QTimer::singleShot(2000, this, [this, streamId]() {
        int numericId = streamId.length(); // Простая заглушка
        setStreamId(numericId, streamId);
        m_isJoined = true;
        
        setStatus(STATUS_CONNECTED, "#388E3C");
        
        m_idInputWidget->setVisible(false);
        m_controlPanel->setVisible(true);
        m_controlPanel->setStreamId(streamId);
        m_controlPanel->setActive(true);
        
        m_videoDisplay->setPlaceholderText("📺 Receiving Stream...\n\nVideo data will appear here");
        
        emit streamJoined(m_streamId, m_displayId);
    });
}

void StreamViewerWindow::onLeaveRequested()
{
    qDebug() << "Leave stream requested for:" << m_streamId;
    
    auto reply = QMessageBox::question(this, "Leave Stream", 
                                     "Are you sure you want to leave this stream?",
                                     QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        m_isJoined = false;
        emit streamLeft(m_streamId);
        close();
    }
}

void StreamViewerWindow::setStreamId(int streamId, const QString &displayId)
{
    m_streamId = streamId;
    m_displayId = displayId;
    setStreamInfo(QString("Stream ID: %1").arg(displayId));  // Исправлено: используем базовый метод
    m_controlPanel->setStreamId(displayId);
    setWindowTitle(QString("Stream Viewer - %1").arg(displayId));
}

void StreamViewerWindow::joinStream(const QString &streamId)
{
    if (!streamId.isEmpty()) {
        m_idInputWidget->setStreamId(streamId);
        onJoinRequested(streamId);
    }
}

void StreamViewerWindow::leaveStream()
{
    onLeaveRequested();
}

void StreamViewerWindow::updateStatusLabel()
{
    // Теперь используем базовый метод setStatus
    if (m_isJoined) {
        setStatus(STATUS_CONNECTED, "#388E3C");
    } else {
        setStatus(STATUS_DISCONNECTED, "#2d2d2d");
    }
}

// Добавляем недостающие методы (заглушки)
void StreamViewerWindow::onFrameReady(const QImage &image, int streamId)
{
    Q_UNUSED(image)
    Q_UNUSED(streamId)
    // Заглушка для тестирования
}

void StreamViewerWindow::onDecoderError(const QString &message)
{
    showError(message);
}

void StreamViewerWindow::cleanup()
{
    qDebug() << "StreamViewerWindow cleanup for stream:" << m_streamId;
    
    // Отключаемся от потока если подключены
    if (m_isJoined) {
        leaveStream();
    }
    
    // Базовый cleanup
    StreamWindow::cleanup();
}
