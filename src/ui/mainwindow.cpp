// mainwindow.cpp
#include "mainwindow.h"
#include "darktheme.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QDebug>
#include "id_utils.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_mainSplitter(nullptr)
    , m_controlPanel(nullptr)
    , m_videoGrid(nullptr)
    , m_videoSelectionDialog(nullptr)
    , m_streamManager(nullptr)
    , m_initialized(false)
    , m_connectedToServer(false)
    , m_nextDeviceIndex(0)
{
    setupUI();
    setupConnections();
    
    // Инициализируем списки устройств
    refreshAvailableDevices();
    
    initialize();
}

MainWindow::~MainWindow()
{
    cleanup();
}

void MainWindow::setupUI()
{
    DarkTheme::applyToApplication();
    
    setWindowTitle("Video Streaming Client - New Protocol");
    setMinimumSize(800, 600);

    // Central widget
    auto centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // Main layout
    auto mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Create main splitter
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    m_mainSplitter->setChildrenCollapsible(false);
    m_mainSplitter->setHandleWidth(2);
    m_mainSplitter->setStyleSheet(R"(
        QSplitter::handle {
            background: #444;
        }
        QSplitter::handle:hover {
            background: #666;
        }
    )");

    // Create control panel (left side)
    m_controlPanel = new MainControlPanel(this);
    m_controlPanel->setMinimumWidth(300);
    m_controlPanel->setMaximumWidth(400);

    // Create video grid (right side)
    m_videoGrid = new VideoGridWidget(this);

    // Add widgets to splitter
    m_mainSplitter->addWidget(m_controlPanel);
    m_mainSplitter->addWidget(m_videoGrid);

    // Set initial splitter sizes (30% control panel, 70% video grid)
    m_mainSplitter->setSizes({300, 700});

    mainLayout->addWidget(m_mainSplitter);

    // Create video selection dialog (will be shown when needed)
    m_videoSelectionDialog = new VideoSelectionDialog(this); // Example devices
    m_videoSelectionDialog->setWindowTitle("Select Video Device");
}

void MainWindow::setupConnections()
{
    // MainControlPanel signals
    connect(m_controlPanel, &MainControlPanel::addDeviceRequested,
            this, &MainWindow::onAddDeviceRequested);
    connect(m_controlPanel, &MainControlPanel::createConferenceRequested,
            this, &MainWindow::onCreateConferenceRequested);
    connect(m_controlPanel, &MainControlPanel::joinConferenceRequested,
            this, &MainWindow::onJoinConferenceRequested);
    connect(m_controlPanel, &MainControlPanel::joinPublicStreamRequested,
            this, &MainWindow::onJoinPublicStreamRequested);

    // VideoGridWidget signals
    connect(m_videoGrid, &VideoGridWidget::streamerDisconnectRequested,
            this, &MainWindow::onStreamerDisconnectRequested);
    connect(m_videoGrid, &VideoGridWidget::viewerLeaveRequested,
            this, &MainWindow::onViewerLeaveRequested);
    connect(m_videoGrid, &VideoGridWidget::streamStartRequested,
            this, &MainWindow::onStreamStartRequested);
    connect(m_videoGrid, &VideoGridWidget::streamStopRequested,
            this, &MainWindow::onStreamStopRequested);
    connect(m_videoGrid, &VideoGridWidget::encodedPacketReady,
            this, &MainWindow::onEncodedPacketReady);

    // VideoSelectionDialog signal
    connect(m_videoSelectionDialog, &VideoSelectionDialog::deviceSelected,
            this, &MainWindow::onDeviceSelected);

    // StreamManager signals (when implemented)
    if (m_streamManager) {
        connect(m_streamManager, &StreamManager::connectionStatusChanged,
                this, &MainWindow::onConnectionStatusChanged);
        connect(m_streamManager, &StreamManager::streamWindowCreated,
                this, &MainWindow::onStreamWindowCreated);
        connect(m_streamManager, &StreamManager::streamWindowClosed,
                this, &MainWindow::onStreamWindowClosed);
        connect(m_streamManager, &StreamManager::errorOccurred,
                this, &MainWindow::onErrorOccurred);
    }
}

void MainWindow::initialize()
{
    qDebug() << "Initializing MainWindow";

    // Initialize StreamManager
    m_streamManager = new StreamManager(this);
    m_streamManager->initialize();
    
    // TODO: Set actual server address from configuration
    m_streamManager->setServerAddress("localhost", 8080);
    m_streamManager->connectToServer();

    m_initialized = true;
    qDebug() << "MainWindow initialized successfully";
}

void MainWindow::cleanup()
{
    qDebug() << "Cleaning up MainWindow";

    if (m_streamManager) {
        m_streamManager->disconnectFromServer();
        m_streamManager->cleanup();
        delete m_streamManager;
        m_streamManager = nullptr;
    }

    m_initialized = false;
}

// ===== MainControlPanel Slots =====

void MainWindow::onCreateConferenceRequested()
{
    qDebug() << "Create conference requested";

    // Отправляем запрос на сервер для создания конференции
    // ID будет сгенерирован на сервере
    if (m_streamManager) {
        // TODO: Реализовать отправку CLIENT_CALL_CREATE через StreamManager
        qDebug() << "Sending CLIENT_CALL_CREATE to server";
        
        // Временная заглушка - показываем сообщение
        QMessageBox::information(this, "Conference Creation", 
            "Conference creation request sent to server. Waiting for conference ID...");
    } else {
        showError("Stream manager not available");
    }
}


void MainWindow::onJoinConferenceRequested(const QString& conferenceId)
{
    qDebug() << "Join conference requested:" << conferenceId;

    if (conferenceId.length() != 6) {
        showError("Conference ID must be 6 characters");
        return;
    }

    // Отправляем запрос на сервер для присоединения к конференции
    if (m_streamManager) {
        // TODO: Реализовать отправку CLIENT_CALL_CONN_JOIN через StreamManager
        qDebug() << "Sending CLIENT_CALL_CONN_JOIN to server for conference:" << conferenceId;
        
        // Временная заглушка
        QMessageBox::information(this, "Join Conference", 
            QString("Join request sent for conference: %1").arg(conferenceId));
    } else {
        showError("Stream manager not available");
    }
}

void MainWindow::onJoinPublicStreamRequested(const QString& streamId)
{
    qDebug() << "Join public stream requested:" << streamId;

    if (streamId.length() != 6) {
        showError("Stream ID must be 6 characters");
        return;
    }

    if (m_streamManager) {
        m_streamManager->joinStream(streamId);
    } else {
        showError("Stream manager not available");
    }
}

// ===== VideoGridWidget Slots =====

void MainWindow::onViewerLeaveRequested(uint32_t streamId)
{
    qDebug() << "Viewer leave requested for stream:" << streamId;

    // Remove viewer widget from grid
    m_videoGrid->removeViewerWidget(streamId);

    // Notify StreamManager
    if (m_streamManager) {
        m_streamManager->leaveStream(streamId);
    }
}

void MainWindow::onStreamStartRequested(int deviceIndex)
{
    qDebug() << "Stream start requested for device:" << deviceIndex;

    // This is handled by StreamManager when creating the stream
    // The actual start happens after stream creation
}

void MainWindow::onStreamStopRequested(uint32_t streamId)
{
    qDebug() << "Stream stop requested for stream:" << streamId;

    // Notify StreamManager to stop the stream
    if (m_streamManager) {
        m_streamManager->deleteStream(streamId);
    }
}

void MainWindow::onEncodedPacketReady(uint32_t streamId, int frameNumber, const QByteArray& packet)
{
    // Forward encoded packets to StreamManager for network transmission
    if (m_streamManager) {
        // TODO: StreamManager needs interface for sending video packets
        qDebug() << "Encoded packet ready for stream:" << streamId << "frame:" << frameNumber << "size:" << packet.size();
    }
}

// ===== VideoSelectionDialog Slot =====

// ===== StreamManager Slots =====

void MainWindow::onConnectionStatusChanged(bool connected)
{
    qDebug() << "Connection status changed:" << connected;

    m_connectedToServer = connected;
    
    // Update control panel status
    if (m_controlPanel) {
        m_controlPanel->setConnectionStatus(connected);
    }

    updateStatus();
}



void MainWindow::onErrorOccurred(const QString& message)
{
    qCritical() << "Error occurred:" << message;
    showError(message);
}

// ===== Helper Methods =====

void MainWindow::createConferenceWindow(uint32_t callId, const QString& displayId)
{
    qDebug() << "Creating conference window - ID:" << callId << "Display:" << displayId;

    // TODO: Implement ConferenceWindow creation
    // ConferenceWindow *conferenceWindow = new ConferenceWindow(callId, displayId, this);
    // conferenceWindow->initialize();
    // conferenceWindow->show();
    
    showError("Conference windows not yet implemented");
}

void MainWindow::showError(const QString& message)
{
    QMessageBox::warning(this, "Error", message);
}

void MainWindow::updateStatus()
{
    // Update window title with connection status
    QString status = m_connectedToServer ? "Connected" : "Disconnected";
    setWindowTitle(QString("Video Streaming Client - %1").arg(status));
}

// ===== MainControlPanel Slots =====

void MainWindow::onAddDeviceRequested()
{
    qDebug() << "Add device requested";

    // Обновляем список доступных устройств
    refreshAvailableDevices();
    
    // Проверяем есть ли доступные устройства
    if (m_availableDevices.isEmpty()) {
        QMessageBox::information(this, "No Devices", 
            "No video devices found on your system.");
        return;
    }

    // Исключаем уже используемые устройства
    QList<int> availableForUse;
    for (int device : m_availableDevices) {
        if (!m_usedDevices.contains(device)) {
            availableForUse.append(device);
        }
    }
    
    if (availableForUse.isEmpty()) {
        QMessageBox::information(this, "No Available Devices", 
            "All video devices are already in use. Please disconnect a device first.");
        return;
    }

    // Создаем диалог выбора
    VideoSelectionDialog dialog(this);
    
    // Показываем диалог и обрабатываем результат
    if (dialog.exec() == QDialog::Accepted) {
        int selectedDevice = dialog.getSelectedDeviceIndex();
        if (selectedDevice != -1) {
            // Проверяем, что устройство все еще доступно (на случай параллельного использования)
            if (m_usedDevices.contains(selectedDevice)) {
                QMessageBox::warning(this, "Device Busy", 
                    QString("Camera %1 is already in use. Please select another device.").arg(selectedDevice));
                return;
            }
            
            if (!m_availableDevices.contains(selectedDevice)) {
                QMessageBox::warning(this, "Device Not Available", 
                    QString("Camera %1 is no longer available.").arg(selectedDevice));
                return;
            }
            
            // Добавляем устройство
            onDeviceSelected(selectedDevice);
        } else {
            qDebug() << "No device selected or selection invalid";
        }
    } else {
        qDebug() << "Device selection canceled by user";
    }
}

void MainWindow::onDeviceSelected(int deviceIndex)
{
    qDebug() << "Device selected:" << deviceIndex;

    if (m_usedDevices.contains(deviceIndex)) {
        qWarning() << "Device" << deviceIndex << "is already in use!";
        QMessageBox::warning(this, "Device Busy", 
            QString("Camera %1 is already in use. Please select another device.").arg(deviceIndex));
        return;
    }

    // Добавляем устройство в список используемых
    m_usedDevices.append(deviceIndex);
    
    // Добавляем streamer widget в video grid
    m_videoGrid->addStreamerWidget(deviceIndex);
    
    // Инициализируем стрим через StreamManager
    if (m_streamManager) {
        m_streamManager->createStream(deviceIndex);
    }
    
    qDebug() << "Successfully added device:" << deviceIndex << "Total used devices:" << m_usedDevices.size();
}

void MainWindow::onStreamerDisconnectRequested(int deviceIndex)
{
    qDebug() << "Streamer disconnect requested for device:" << deviceIndex;

    // Удаляем streamer widget из grid
    m_videoGrid->removeStreamerWidget(deviceIndex);

    // Освобождаем устройство
    m_usedDevices.removeAll(deviceIndex);
    
    // Уведомляем StreamManager об остановке стрима
    if (m_streamManager) {
        // Здесь нужно найти streamId по deviceIndex
        // Временная реализация - ищем через виджет
        StreamerWidget* widget = m_videoGrid->findStreamerWidget(deviceIndex);
        if (widget && widget->getStreamId() != 0) {
            uint32_t streamId = widget->getStreamId();
            qDebug() << "Notifying stream manager to delete stream:" << streamId;
            m_streamManager->deleteStream(streamId);
        }
    }

    qDebug() << "Device" << deviceIndex << "disconnected and removed. Total used devices:" << m_usedDevices.size();
    
    // Обновляем статус доступности кнопки добавления устройств
    updateDeviceAvailability();
}

// Добавим вспомогательный метод для обновления доступности устройств
void MainWindow::updateDeviceAvailability()
{
    // Обновляем список доступных устройств
    refreshAvailableDevices();
    
    // Можно добавить логику для обновления состояния кнопки "Add Device"
    // если в MainControlPanel есть соответствующий метод
    bool hasAvailableDevices = !m_availableDevices.isEmpty() && 
                              (m_availableDevices.size() > m_usedDevices.size());
    
    qDebug() << "Device availability - Total:" << m_availableDevices.size() 
             << "Used:" << m_usedDevices.size() 
             << "Available:" << hasAvailableDevices;
}

// ===== Вспомогательные методы =====

void MainWindow::refreshAvailableDevices()
{
    m_availableDevices = VideoCapture::getAvailableDevices();
    
    if (m_availableDevices.isEmpty()) {
        qDebug() << "No video devices found on system";
    } else {
        qDebug() << "Available devices:" << m_availableDevices;
    }
    
    // Обновляем состояние кнопок на основе доступности устройств
    if (m_controlPanel) {
        // Можно добавить индикатор количества доступных устройств
        bool hasAvailableDevices = !m_availableDevices.isEmpty();
        // m_controlPanel->setAddDeviceEnabled(hasAvailableDevices); // Если добавим такой метод
    }
}

void MainWindow::initializeStreamerForDevice(int deviceIndex)
{
    // Эта функция будет вызываться из StreamerWidget когда он будет готов к инициализации
    // с реальным streamId от сервера
    qDebug() << "Initializing streamer for device:" << deviceIndex;
    
    // Временная заглушка - в реальной реализации здесь будет инициализация
    // видеозахвата и кодировщика с полученным streamId
}