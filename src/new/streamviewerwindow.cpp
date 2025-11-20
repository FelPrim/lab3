#include "streamviewerwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QDebug>

StreamViewerWindow::StreamViewerWindow(QWidget *parent)
    : StreamWindow(parent)
    , m_streamId(0)
    , m_isActive(false)
    , m_displayBuffer(nullptr)
{
    setupUI();
    setWindowTitle("Stream Viewer");
    setMinimumSize(700, 600);
    resize(800, 650);
}

StreamViewerWindow::~StreamViewerWindow()
{
    qDebug() << "StreamViewerWindow destroyed, ID:" << m_streamId;
}

void StreamViewerWindow::initialize()
{
    // Инициализируем буфер отображения
    if (!m_displayBuffer) {
        m_displayBuffer = new NetworkDisplayBuffer(m_streamId, DEFAULT_WIDTH, DEFAULT_HEIGHT, DEFAULT_FPS, this);
        connect(m_displayBuffer, &NetworkDisplayBuffer::frameReady,
                m_videoDisplay, &StreamVideoDisplayPanel::displayFrame);
        connect(m_displayBuffer, &NetworkDisplayBuffer::errorOccurred,
                this, [this](const QString &error) {
                    showError(QString("Display error: %1").arg(error));
                });
        
        m_displayBuffer->initialize();
    }
    
    m_isActive = true;
    m_controlPanel->setActive(true);
    updateStatusLabel();
    
    qDebug() << "StreamViewerWindow initialized for stream:" << m_streamId;
}

void StreamViewerWindow::cleanup()
{
    qDebug() << "StreamViewerWindow cleanup for stream:" << m_streamId;
    
    m_isActive = false;
    m_controlPanel->setActive(false);
    
    if (m_displayBuffer) {
        m_displayBuffer->cleanup();
        m_displayBuffer->deleteLater();
        m_displayBuffer = nullptr;
    }
    
    StreamWindow::cleanup();
}

void StreamViewerWindow::setupUI()
{
    setupCommonUI();

    setStyleSheet(
        "QWidget {"
        "   background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1,"
        "                               stop: 0 #1e1e2e, stop: 1 #252536);"
        "   color: #ffffff;"
        "}"
    );

    // Stream info section
    auto infoSection = new QFrame(this);
    infoSection->setStyleSheet(
        "QFrame {"
        "   background: rgba(255, 255, 255, 0.05);"
        "   border: 1px solid #444;"
        "   border-radius: 8px;"
        "   padding: 10px;"
        "}"
    );
    auto infoLayout = new QVBoxLayout(infoSection); // ИСПРАВЛЕНО: infoLayout

    auto infoHeader = new QLabel("Stream Information", this);
    infoHeader->setStyleSheet(
        "font-size: 16px;"
        "font-weight: bold;"
        "color: #e9ecef;"
        "padding: 5px 0;"
    );
    infoLayout->addWidget(infoHeader);

    auto streamInfoLabel = new QLabel("Waiting for stream data...", this);
    streamInfoLabel->setStyleSheet("color: #cfd8dc; font-size: 13px;");
    infoLayout->addWidget(streamInfoLabel); // ИСПРАВЛЕНО: infoLayout

    m_mainLayout->addWidget(infoSection);

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
    m_videoDisplay->setPlaceholderText("Waiting for video stream...\n\nVideo will appear here when the stream starts");
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

    m_controlPanel = new StreamControlPanel(StreamControlPanel::Viewer, this);
    controlLayout->addWidget(m_controlPanel);

    m_mainLayout->addWidget(controlSection);

    setupConnections();
}

void StreamViewerWindow::setupConnections()
{
    connect(m_controlPanel, &StreamControlPanel::leaveRequested, 
            this, &StreamViewerWindow::onLeaveRequested);
}

void StreamViewerWindow::setStreamId(int streamId, const QString &displayId)
{
    m_streamId = streamId;
    m_displayId = displayId;
    setStreamInfo(QString("Stream ID: %1").arg(displayId));
    m_controlPanel->setStreamId(displayId);
    setWindowTitle(QString("Stream Viewer - %1").arg(displayId));
}

void StreamViewerWindow::setActive(bool active)
{
    m_isActive = active;
    m_controlPanel->setActive(active);
    updateStatusLabel();
}

void StreamViewerWindow::onFrameAssembled(int streamId, int frameNumber, const QByteArray &frameData)
{
    if (streamId == m_streamId && m_displayBuffer && m_isActive) {
        m_displayBuffer->addFrame(frameNumber, frameData);
        
        // Обновляем статистику
        m_statsWidget->setFps(m_displayBuffer->getCurrentFps());
        m_statsWidget->setBitrate(frameData.size() * 8 / 1024); // Примерный расчет битрейта
    }
}

void StreamViewerWindow::onLeaveRequested()
{
    qDebug() << "Leave stream requested for:" << m_streamId;

    auto reply = QMessageBox::question(this, "Leave Stream",
                                       "Are you sure you want to leave this stream?",
                                       QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        m_isActive = false;
        updateStatusLabel();
        emit streamLeft(m_streamId);
        close();
    }
}

// ДОБАВЛЕНА РЕАЛИЗАЦИЯ МЕТОДА
void StreamViewerWindow::updateStatusLabel()
{
    if (m_isActive) {
        setStatus("Connected", "#28a745");
    } else {
        setStatus("Disconnected", "#dc3545");
    }
}
