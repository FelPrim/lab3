// mainwindow.cpp
#include "../video_defaults.h"
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

// В методе initialize() добавьте эти подключения:
void MainWindow::initialize()
{
    qDebug() << "Initializing MainWindow";

    // Initialize StreamManager
    m_streamManager = new StreamManager(this);
    m_streamManager->initialize();
    
    m_streamManager->setServerAddress(DEFAULT_ECHO_SERVER_ADDRESS, DEFAULT_ECHO_SERVER_PORT);
    // ПОДКЛЮЧАЕМ ВСЕ необходимые сигналы
    connect(m_streamManager, &StreamManager::connectionStatusChanged,
            this, &MainWindow::onConnectionStatusChanged);
    connect(m_streamManager, &StreamManager::errorOccurred,
            this, &MainWindow::onErrorOccurred);
    
    // Сигналы потоков
    connect(m_streamManager, &StreamManager::serverStreamCreated,
            this, &MainWindow::onServerStreamCreated);
    connect(m_streamManager, &StreamManager::serverStreamStart,
            this, &MainWindow::onServerStreamStart);
    connect(m_streamManager, &StreamManager::serverStreamEnd,
            this, &MainWindow::onServerStreamEnd);
    connect(m_streamManager, &StreamManager::serverStreamJoined,
            this, &MainWindow::onServerStreamJoined);
    connect(m_streamManager, &StreamManager::serverStreamDeleted,
            this, &MainWindow::onServerStreamDeleted);

    // Подключаемся к серверу ПОСЛЕ настройки всех соединений
    m_streamManager->connectToServer();

    m_initialized = true;
    qDebug() << "MainWindow initialized successfully";
}

// ЗАМЕНИТЕ onServerStreamLeft на onServerStreamDeleted:
void MainWindow::onServerStreamDeleted(uint32_t streamId)
{
    qDebug() << "MainWindow: server stream deleted - ID:" << streamId;
    
    // Находим ViewerWidget и удаляем его
    ViewerWidget* viewer = m_videoGrid->findViewerWidget(streamId);
    if (viewer) {
        viewer->onStreamLeft(streamId);
        m_videoGrid->removeViewerWidget(streamId);
    } else {
        qDebug() << "ViewerWidget not found for deleted stream:" << streamId;
    }
}

// УБЕРИТЕ старый слот onServerStreamLeft - он больше не нужен
/*
void MainWindow::onServerStreamLeft(uint32_t streamId)
{
    // Этот метод больше не используется - используем onServerStreamDeleted
}
*/


void MainWindow::onServerStreamCreated(uint32_t streamId)
{
    qDebug() << "MainWindow: server stream created - ID:" << streamId;
    
    // Находим StreamerWidget по deviceIndex (нужен mapping deviceIndex -> streamId)
    // Временная реализация - ищем по всем streamer widgets
    for (StreamerWidget* widget : m_videoGrid->getStreamerWidgets()) {
        if (widget->getStreamId() == 0) { // Находим виджет без назначенного streamId
            char displayId[7] = {0};
            id_to_string(streamId, displayId);
            QString streamDisplayId = QString::fromLatin1(displayId, 6);
            widget->onServerStreamCreated(streamId);
            break;
        }
    }
}

void MainWindow::onServerStreamStart(uint32_t streamId)
{
    qDebug() << "MainWindow: server stream start - ID:" << streamId;
    
    // Находим StreamerWidget по streamId
    for (StreamerWidget* widget : m_videoGrid->getStreamerWidgets()) {
        if (widget->getStreamId() == streamId) {
            widget->onServerStreamStart(streamId);
            break;
        }
    }
}

void MainWindow::onServerStreamEnd(uint32_t streamId)
{
    qDebug() << "MainWindow: server stream end - ID:" << streamId;
    
    // Находим StreamerWidget по streamId
    for (StreamerWidget* widget : m_videoGrid->getStreamerWidgets()) {
        if (widget->getStreamId() == streamId) {
            widget->onServerStreamEnd(streamId);
            break;
        }
    }
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

    if (m_streamManager) {
        // Генерируем временный ID для конференции (на сервере будет свой)
        uint32_t tempCallId = generate_id();
        char displayId[7] = {0};
        id_to_string(tempCallId, displayId);
        QString callDisplayId = QString::fromLatin1(displayId, 6);
        
        // Создаем окно конференции
        createConferenceWindow(tempCallId, callDisplayId);
        
        // TODO: Отправляем запрос на сервер для реального создания конференции
        // m_streamManager->createConference();
        
        qDebug() << "Conference window created - ID:" << tempCallId << "Display:" << callDisplayId;
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

    // Конвертируем строковый ID в числовой
    uint32_t callId = string_to_id(conferenceId.toLatin1().constData());

    // Создаем окно конференции
    createConferenceWindow(callId, conferenceId);

    // TODO: Отправляем запрос на сервер для присоединения к конференции
    // m_streamManager->joinConference(conferenceId);
    
    qDebug() << "Joining conference - ID:" << callId << "Display:" << conferenceId;
}

void MainWindow::createConferenceWindow(uint32_t callId, const QString& displayId)
{
    qDebug() << "Creating conference window - ID:" << callId << "Display:" << displayId;

    ConferenceWindow *conferenceWindow = new ConferenceWindow(callId, displayId, this);
    
    // Подключаем сигналы конференции
    connect(conferenceWindow, &ConferenceWindow::conferenceClosed,
            this, &MainWindow::onConferenceClosed);
    connect(conferenceWindow, &ConferenceWindow::conferenceJoined,
            this, &MainWindow::onConferenceJoined);
    
    conferenceWindow->show();
    
    // Сохраняем ссылку на активные конференции
    m_activeConferences[callId] = displayId;
    
    qDebug() << "Conference window created. Total active conferences:" << m_activeConferences.size();
}

void MainWindow::onConferenceClosed(uint32_t callId)
{
    if (m_activeConferences.remove(callId)) {
        qDebug() << "Conference closed and removed - ID:" << callId;
    } else {
        qWarning() << "Conference not found in active conferences - ID:" << callId;
    }
    qDebug() << "Total active conferences:" << m_activeConferences.size();
}

void MainWindow::onConferenceJoined(uint32_t callId, const QString& displayId)
{
    qDebug() << "Conference joined successfully - ID:" << callId << "Display:" << displayId;
    // Можно обновить UI или показать статус
}



void MainWindow::onJoinPublicStreamRequested(const QString& streamId) {
    qDebug() << "Join public stream requested:" << streamId;

    if (streamId.length() != 6) {
        showError("Stream ID must be 6 characters");
        return;
    }

    // Конвертируем display ID в server ID
    uint32_t streamIdNum = string_to_id(streamId.toLatin1().constData());

    qDebug() << "Joining stream, server ID:" << streamIdNum;

    // Создаем ViewerWidget, но НЕ инициализируем сразу
    m_videoGrid->addViewerWidget(streamIdNum, streamId);
    
    // Уведомляем StreamManager о присоединении
    if (m_streamManager) {
        m_streamManager->joinStream(streamId);
    } else {
        qWarning() << "StreamManager not available for joining stream";
        // Если StreamManager недоступен, удаляем виджет
        m_videoGrid->removeViewerWidget(streamIdNum);
        showError("Cannot join stream - not connected to server");
    }
}

void MainWindow::onServerStreamJoined(uint32_t streamId) {
    qDebug() << "MainWindow: server stream joined - ID:" << streamId;
    
    // Находим или создаем ViewerWidget
    ViewerWidget* viewer = m_videoGrid->findViewerWidget(streamId);
    if (!viewer) {
        // Создаем, если не существует
        char displayId[7] = {0};
        id_to_string(streamId, displayId);
        QString streamDisplayId = QString::fromLatin1(displayId, 6);
        m_videoGrid->addViewerWidget(streamId, streamDisplayId);
        viewer = m_videoGrid->findViewerWidget(streamId);
    }
    
    if (viewer) {
        // Устанавливаем NetworkManager и инициализируем
        if (m_streamManager) {
            NetworkManager* networkManager = m_streamManager->getNetworkManagerForStream(streamId);
            if (networkManager) {
                viewer->setNetworkManager(networkManager);
                viewer->initialize();
                viewer->onStreamJoined(streamId);
                qDebug() << "ViewerWidget activated for stream:" << streamId;
            } else {
                qWarning() << "NetworkManager not available for stream:" << streamId;
            }
        }
    } else {
        qWarning() << "Failed to create ViewerWidget for stream:" << streamId;
    }
}




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

void MainWindow::onDeviceSelected(int deviceIndex) {
    qDebug() << "Quick adding device:" << deviceIndex;

    if (m_usedDevices.contains(deviceIndex)) {
        showError(QString("Camera %1 is already in use").arg(deviceIndex));
        return;
    }

    // Добавляем в список используемых
    m_usedDevices.append(deviceIndex);
    
    // Добавляем виджет
    m_videoGrid->addStreamerWidget(deviceIndex);
    
    // Немедленная настройка
    StreamerWidget* widget = m_videoGrid->findStreamerWidget(deviceIndex);
    if (widget) {
        // УСТАНАВЛИВАЕМ StreamManager ПЕРЕД инициализацией
        widget->setStreamManager(m_streamManager);
        widget->initialize(); // Быстрая инициализация для превью
    }
    
    qDebug() << "Device" << deviceIndex << "added successfully";
}

void MainWindow::onStreamStartRequested(int deviceIndex) {
    qDebug() << "Stream start requested for device:" << deviceIndex;

    StreamerWidget* widget = m_videoGrid->findStreamerWidget(deviceIndex);
    if (widget) {
        widget->setStreamManager(m_streamManager);
        widget->startStream();
    } else {
        qWarning() << "StreamerWidget not found for device:" << deviceIndex;
    }
}

void MainWindow::onStreamStopRequested(uint32_t streamId)
{
    qDebug() << "Stream stop requested for stream:" << streamId;

    // Notify StreamManager to stop the stream
    if (m_streamManager) {
        m_streamManager->deleteStream(streamId);
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



void MainWindow::initializeStreamerForDevice(int deviceIndex)
{
    // Эта функция будет вызываться из StreamerWidget когда он будет готов к инициализации
    // с реальным streamId от сервера
    qDebug() << "Initializing streamer for device:" << deviceIndex;
    
    // Временная заглушка - в реальной реализации здесь будет инициализация
    // видеозахвата и кодировщика с полученным streamId
}



void MainWindow::refreshAvailableDevices()
{
    // Быстрое сканирование без лишних задержек
    m_availableDevices = VideoCapture::getAvailableDevices();
    
    if (m_availableDevices.isEmpty()) {
        qDebug() << "No video devices found";
    } else {
        qDebug() << "Available devices:" << m_availableDevices;
    }
}

