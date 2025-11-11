#include "mainwindow.h"
#include "darktheme.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QInputDialog>
#include <QLineEdit>
#include <QFrame>
#include <QFont>
#include <QRegularExpression>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_btnStartStream(nullptr)
    , m_btnJoinStream(nullptr)
    , m_connectionStatusLabel(nullptr)
    , m_infoLabel(nullptr)
    , m_streamManager(new StreamManager(this))
{
    setupUI();
    setupConnections();
    
    // Initialize stream manager
    m_streamManager->initialize();
    m_streamManager->setServerAddress("127.0.0.1", 8080);
    m_streamManager->connectToServer();
    
    setMinimumSize(500, 400);
}

MainWindow::~MainWindow()
{
    m_streamManager->disconnectFromServer();
    m_streamManager->cleanup();
}

void MainWindow::setupUI()
{
    DarkTheme::applyToApplication();
    
    setWindowTitle("Video Streaming Client");
    
    // Central widget with same style as stream windows
    auto central = new QWidget(this);
    central->setObjectName("centralWidget");
    central->setStyleSheet(
        "#centralWidget {"
        "   background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1,"
        "                               stop: 0 #1e1e2e, stop: 1 #252536);"
        "}"
    );
    
    auto mainLayout = new QVBoxLayout(central);
    mainLayout->setSpacing(25);
    mainLayout->setContentsMargins(40, 40, 40, 40);
    
    // Header only
    auto headerLabel = new QLabel("Video Streaming", this);
    headerLabel->setAlignment(Qt::AlignCenter);
    headerLabel->setStyleSheet(
        "font-size: 24px;"
        "font-weight: 300;"
        "color: #e0e0e0;"
        "padding: 20px 0;"
        "margin-bottom: 10px;"
    );
    mainLayout->addWidget(headerLabel);
    
    // Connection status
    m_connectionStatusLabel = new QLabel("● Disconnected", this);
    m_connectionStatusLabel->setAlignment(Qt::AlignCenter);
    m_connectionStatusLabel->setStyleSheet(
        "font-size: 13px;"
        "font-weight: 500;"
        "padding: 8px 20px;"
        "background: #333;"
        "color: #999;"
        "border: 1px solid #444;"
        "border-radius: 12px;"
        "margin: 10px 0;"
    );
    mainLayout->addWidget(m_connectionStatusLabel);
    
    // Info label with same style as stream windows
    m_infoLabel = new QLabel("Select an action to begin streaming", this);
    m_infoLabel->setAlignment(Qt::AlignCenter);
    m_infoLabel->setStyleSheet(
        "color: #adb5bd;"
        "font-size: 13px;"
        "padding: 12px;"
        "background: rgba(255, 255, 255, 0.05);"
        "border: 1px solid #444;"
        "border-radius: 6px;"
        "margin: 10px 0;"
    );
    mainLayout->addWidget(m_infoLabel);
    
    mainLayout->addStretch();
    
    // Action buttons container
    auto buttonContainer = new QWidget(this);
    buttonContainer->setStyleSheet("background: transparent;");
    auto buttonLayout = new QVBoxLayout(buttonContainer);
    buttonLayout->setSpacing(12);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    
    // Start Stream button
    m_btnStartStream = new QPushButton("Start Streaming", this);
    m_btnStartStream->setMinimumHeight(50);
    m_btnStartStream->setStyleSheet(
        "QPushButton {"
        "   background: #2a2a2a;"
        "   color: #e0e0e0;"
        "   font-size: 14px;"
        "   font-weight: 500;"
        "   border: 1px solid #444;"
        "   border-radius: 8px;"
        "   padding: 12px;"
        "}"
        "QPushButton:hover {"
        "   background: #333;"
        "   border: 1px solid #555;"
        "}"
        "QPushButton:pressed {"
        "   background: #3a3a3a;"
        "   border: 1px solid #666;"
        "}"
        "QPushButton:disabled {"
        "   background: #1a1a1a;"
        "   color: #555;"
        "   border: 1px solid #333;"
        "}"
    );
    
    // Join Stream button
    m_btnJoinStream = new QPushButton("Join Stream", this);
    m_btnJoinStream->setMinimumHeight(50);
    m_btnJoinStream->setStyleSheet(
        "QPushButton {"
        "   background: #2a2a2a;"
        "   color: #e0e0e0;"
        "   font-size: 14px;"
        "   font-weight: 500;"
        "   border: 1px solid #444;"
        "   border-radius: 8px;"
        "   padding: 12px;"
        "}"
        "QPushButton:hover {"
        "   background: #333;"
        "   border: 1px solid #555;"
        "}"
        "QPushButton:pressed {"
        "   background: #3a3a3a;"
        "   border: 1px solid #666;"
        "}"
        "QPushButton:disabled {"
        "   background: #1a1a1a;"
        "   color: #555;"
        "   border: 1px solid #333;"
        "}"
    );
    
    buttonLayout->addWidget(m_btnStartStream);
    buttonLayout->addWidget(m_btnJoinStream);
    
    mainLayout->addWidget(buttonContainer);
    mainLayout->addStretch();
    
    setCentralWidget(central);
}

void MainWindow::setupConnections()
{
    connect(m_btnStartStream, &QPushButton::clicked, this, &MainWindow::onStartStreamClicked);
    connect(m_btnJoinStream, &QPushButton::clicked, this, &MainWindow::onJoinStreamClicked);
    
    // Stream manager signals
    connect(m_streamManager, &StreamManager::streamWindowCreated, 
            this, &MainWindow::onStreamWindowCreated);
    connect(m_streamManager, &StreamManager::streamWindowClosed,
            this, &MainWindow::onStreamWindowClosed);
    connect(m_streamManager, &StreamManager::connectionStatusChanged,
            this, &MainWindow::onConnectionStatusChanged);
    connect(m_streamManager, &StreamManager::errorOccurred,
            this, [this](const QString& error) {
                QMessageBox::warning(this, "Error", error);
            });
}

void MainWindow::onStartStreamClicked()
{
    qDebug() << "Start stream button clicked";
    
    // Заглушка: всегда используем устройство 0
    int deviceIndex = 0;
    m_streamManager->createStream(deviceIndex);
    
    m_infoLabel->setText("Creating new stream...");
    m_infoLabel->setStyleSheet(
        "color: #888;"
        "font-size: 13px;"
        "padding: 12px;"
        "background: #252525;"
        "border: 1px solid #444;"
        "border-radius: 6px;"
        "margin: 10px 0;"
    );
}

void MainWindow::onJoinStreamClicked()
{
    qDebug() << "Join stream button clicked";
    
    bool ok;
    QString streamId = QInputDialog::getText(this, "Join Stream", 
                                           "Enter 6-character Stream ID:",
                                           QLineEdit::Normal, "", &ok);
    if (ok && !streamId.isEmpty()) {
        // Проверяем формат ID
        if (streamId.length() != 6) {
            QMessageBox::warning(this, "Invalid ID", "Stream ID must be exactly 6 characters");
            return;
        }
        
        // Проверяем что все символы - заглавные буквы
        QRegularExpression regex("^[A-Z]{6}$");
        if (!regex.match(streamId).hasMatch()) {
            QMessageBox::warning(this, "Invalid ID", "Stream ID must contain only uppercase letters (A-Z)");
            return;
        }
        
        m_streamManager->joinStream(streamId);
        m_infoLabel->setText("Joining stream: " + streamId);
        m_infoLabel->setStyleSheet(
            "color: #888;"
            "font-size: 13px;"
            "padding: 12px;"
            "background: #252525;"
            "border: 1px solid #444;"
            "border-radius: 6px;"
            "margin: 10px 0;"
        );
    }
}

void MainWindow::onStreamWindowCreated(StreamWindow *window)
{
    if (!window) return;
    
    // Определяем streamId из заголовка окна или другим способом
    int streamId = window->getStreamId();
    m_openWindows[streamId] = window;
    
    // Показываем окно
    window->show();
    window->raise();
    window->activateWindow();
    
    qDebug() << "Stream window created with ID:" << streamId;
    m_infoLabel->setText(QString("Active streams: %1").arg(m_openWindows.size()));
    m_infoLabel->setStyleSheet(
        "color: #aaa;"
        "font-size: 13px;"
        "padding: 12px;"
        "background: #252525;"
        "border: 1px solid #444;"
        "border-radius: 6px;"
        "margin: 10px 0;"
    );
}

void MainWindow::onStreamWindowClosed(int streamId)
{
    m_openWindows.remove(streamId);
    qDebug() << "Stream window closed, ID:" << streamId;
    m_infoLabel->setText(QString("Active streams: %1").arg(m_openWindows.size()));
    
    if (m_openWindows.isEmpty()) {
        m_infoLabel->setText("Select an action to begin streaming");
        m_infoLabel->setStyleSheet(
            "color: #666;"
            "font-size: 13px;"
            "padding: 12px;"
            "background: #222;"
            "border: 1px solid #333;"
            "border-radius: 6px;"
            "margin: 10px 0;"
        );
    }
}

void MainWindow::onConnectionStatusChanged(bool connected)
{
    if (connected) {
        m_connectionStatusLabel->setText("● Connected");
        m_connectionStatusLabel->setStyleSheet(
            "font-size: 13px;"
            "font-weight: 500;"
            "padding: 8px 20px;"
            "background: #1a3a1a;"
            "color: #8bc34a;"
            "border: 1px solid #2a5a2a;"
            "border-radius: 12px;"
            "margin: 10px 0;"
        );
        m_btnStartStream->setEnabled(true);
        m_btnJoinStream->setEnabled(true);
    } else {
        m_connectionStatusLabel->setText("● Disconnected");
        m_connectionStatusLabel->setStyleSheet(
            "font-size: 13px;"
            "font-weight: 500;"
            "padding: 8px 20px;"
            "background: #333;"
            "color: #999;"
            "border: 1px solid #444;"
            "border-radius: 12px;"
            "margin: 10px 0;"
        );
        m_btnStartStream->setEnabled(false);
        m_btnJoinStream->setEnabled(false);
    }
}
