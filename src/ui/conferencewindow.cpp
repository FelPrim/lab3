// conferencewindow.cpp
#include "conferencewindow.h"
#include "darktheme.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QMessageBox>
#include <QDebug>

ConferenceWindow::ConferenceWindow(uint32_t callId, const QString& displayId, QWidget* parent)
    : QMainWindow(parent)
    , m_mainSplitter(nullptr)
    , m_controlPanel(nullptr)
    , m_videoGrid(nullptr)
    , m_videoSelectionDialog(nullptr)
    , m_streamManager(nullptr)
    , m_callId(callId)
    , m_displayId(displayId)
    , m_initialized(false)
    , m_nextDeviceIndex(0)
{
    setupUI();
    setupConnections();
    initialize();
}

ConferenceWindow::~ConferenceWindow()
{
    cleanup();
}

void ConferenceWindow::setupUI()
{
    DarkTheme::applyToApplication();
    
    setWindowTitle(QString("Conference: %1").arg(m_displayId));
    setMinimumSize(1000, 700);

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
    m_controlPanel = new ConferenceControlPanel(this);
    m_controlPanel->setMinimumWidth(300);
    m_controlPanel->setMaximumWidth(400);

    // Create video grid (right side)
    m_videoGrid = new VideoGridWidget(this);

    // Add widgets to splitter
    m_mainSplitter->addWidget(m_controlPanel);
    m_mainSplitter->addWidget(m_videoGrid);

    // Set initial splitter sizes (25% control panel, 75% video grid)
    m_mainSplitter->setSizes({250, 750});

    mainLayout->addWidget(m_mainSplitter);

    // Create video selection dialog
    m_videoSelectionDialog = new VideoSelectionDialog(  this);
    m_videoSelectionDialog->setWindowTitle("Select Video Device for Conference");

    // Set initial conference info
    m_controlPanel->setConferenceInfo(m_callId, m_displayId);
    m_controlPanel->setParticipantsCount(1); // Starting with ourselves
    m_controlPanel->setStreamsCount(0);
}

void ConferenceWindow::setupConnections()
{
    // ConferenceControlPanel signals
    connect(m_controlPanel, &ConferenceControlPanel::addDeviceRequested,
            this, &ConferenceWindow::onAddDeviceRequested);
    connect(m_controlPanel, &ConferenceControlPanel::leaveConferenceRequested,
            this, &ConferenceWindow::onLeaveConferenceRequested);
    connect(m_controlPanel, &ConferenceControlPanel::watchStreamRequested,
            this, &ConferenceWindow::onWatchStreamRequested);
    connect(m_controlPanel, &ConferenceControlPanel::stopWatchingRequested,
            this, &ConferenceWindow::onStopWatchingRequested);

    // VideoGridWidget signals
    connect(m_videoGrid, &VideoGridWidget::streamerDisconnectRequested,
            this, &ConferenceWindow::onStreamerDisconnectRequested);
    connect(m_videoGrid, &VideoGridWidget::viewerLeaveRequested,
            this, &ConferenceWindow::onViewerLeaveRequested);
    connect(m_videoGrid, &VideoGridWidget::streamStartRequested,
            this, &ConferenceWindow::onStreamStartRequested);
    connect(m_videoGrid, &VideoGridWidget::streamStopRequested,
            this, &ConferenceWindow::onStreamStopRequested);
    connect(m_videoGrid, &VideoGridWidget::encodedPacketReady,
            this, &ConferenceWindow::onEncodedPacketReady);

    // VideoSelectionDialog signal
    connect(m_videoSelectionDialog, &VideoSelectionDialog::deviceSelected,
            this, &ConferenceWindow::onDeviceSelected);

    // StreamManager signals (when implemented)
    if (m_streamManager) {
        connect(m_streamManager, &StreamManager::connectionStatusChanged,
                this, &ConferenceWindow::onConnectionStatusChanged);
        connect(m_streamManager, &StreamManager::streamWindowCreated,
                this, &ConferenceWindow::onStreamWindowCreated);
        connect(m_streamManager, &StreamManager::streamWindowClosed,
                this, &ConferenceWindow::onStreamWindowClosed);
        connect(m_streamManager, &StreamManager::errorOccurred,
                this, &ConferenceWindow::onErrorOccurred);
    }
}

// В conferencewindow.cpp - исправленный initialize()
void ConferenceWindow::initialize()
{
    qDebug() << "Initializing ConferenceWindow - ID:" << m_callId << "Display:" << m_displayId;

    // Инициализируем StreamManager для конференции
    m_streamManager = new StreamManager(this);
    m_streamManager->initialize();
    
    // TODO: Установить реальный адрес сервера из конфигурации
    m_streamManager->setServerAddress("localhost", 8080);
    m_streamManager->connectToServer();

    // ПОДКЛЮЧАЕМ СИГНАЛЫ ПОСЛЕ создания StreamManager
    connect(m_streamManager, &StreamManager::connectionStatusChanged,
            this, &ConferenceWindow::onConnectionStatusChanged);
    connect(m_streamManager, &StreamManager::streamWindowCreated,
            this, &ConferenceWindow::onStreamWindowCreated);
    connect(m_streamManager, &StreamManager::streamWindowClosed,
            this, &ConferenceWindow::onStreamWindowClosed);
    connect(m_streamManager, &StreamManager::errorOccurred,
            this, &ConferenceWindow::onErrorOccurred);

    // НЕ добавляем себя как участника - это сделает сервер
    // addParticipant(m_callId); // УДАЛИТЬ эту строку

    m_initialized = true;
    qDebug() << "ConferenceWindow initialized successfully";
}

void ConferenceWindow::cleanup()
{
    qDebug() << "Cleaning up ConferenceWindow - ID:" << m_callId;

    // Leave all streams we're watching
    for (auto it = m_watchedStreams.begin(); it != m_watchedStreams.end(); ++it) {
        if (m_streamManager) {
            m_streamManager->leaveStream(it.key());
        }
    }
    m_watchedStreams.clear();

    // Clean up StreamManager
    if (m_streamManager) {
        m_streamManager->disconnectFromServer();
        m_streamManager->cleanup();
        delete m_streamManager;
        m_streamManager = nullptr;
    }

    m_initialized = false;
}

void ConferenceWindow::setConferenceInfo(uint32_t callId, const QString& displayId)
{
    m_callId = callId;
    m_displayId = displayId;
    
    setWindowTitle(QString("Conference: %1").arg(m_displayId));
    m_controlPanel->setConferenceInfo(m_callId, m_displayId);
    
    qDebug() << "Conference info updated - ID:" << m_callId << "Display:" << m_displayId;
}

void ConferenceWindow::addParticipant(uint32_t participantId)
{
    if (m_participants.contains(participantId)) {
        qWarning() << "Participant already exists in conference:" << participantId;
        return;
    }

    m_participants.append(participantId);
    m_controlPanel->setParticipantsCount(m_participants.size());
    
    qDebug() << "Participant added to conference - ID:" << participantId 
             << "Total participants:" << m_participants.size();
}

void ConferenceWindow::removeParticipant(uint32_t participantId)
{
    if (m_participants.removeOne(participantId)) {
        m_controlPanel->setParticipantsCount(m_participants.size());
        qDebug() << "Participant removed from conference - ID:" << participantId
                 << "Total participants:" << m_participants.size();
    } else {
        qWarning() << "Participant not found in conference:" << participantId;
    }
}

void ConferenceWindow::addAvailableStream(uint32_t streamId, const QString& displayId)
{
    if (m_availableStreams.contains(streamId)) {
        qWarning() << "Stream already available in conference:" << streamId;
        return;
    }

    m_availableStreams[streamId] = displayId;
    m_controlPanel->addAvailableStream(streamId, displayId);
    
    qDebug() << "Available stream added to conference - ID:" << streamId << "Display:" << displayId;
}

void ConferenceWindow::removeAvailableStream(uint32_t streamId)
{
    if (m_availableStreams.remove(streamId)) {
        m_controlPanel->removeAvailableStream(streamId);
        
        // Also remove from watched streams if we were watching it
        if (m_watchedStreams.remove(streamId)) {
            m_videoGrid->removeViewerWidget(streamId);
        }
        
        qDebug() << "Available stream removed from conference - ID:" << streamId;
    } else {
        qWarning() << "Available stream not found in conference:" << streamId;
    }
}

// ===== ConferenceControlPanel Slots =====

void ConferenceWindow::onAddDeviceRequested()
{
    qDebug() << "Add device requested in conference";

    // Show device selection dialog
    if (m_videoSelectionDialog) {
        m_videoSelectionDialog->refreshDevices();
        if (m_videoSelectionDialog->exec() == QDialog::Accepted) {
            // Device will be handled in onDeviceSelected slot
        }
    } else {
        // Fallback: use default device (index 0)
        onDeviceSelected(0);
    }
}

void ConferenceWindow::onLeaveConferenceRequested()
{
    qDebug() << "Leave conference requested";
    
    // Close the window
    close();
}

void ConferenceWindow::onWatchStreamRequested(uint32_t streamId)
{
    qDebug() << "Watch stream requested in conference:" << streamId;
    joinStreamInConference(streamId);
}

void ConferenceWindow::onStopWatchingRequested(uint32_t streamId)
{
    qDebug() << "Stop watching requested in conference:" << streamId;
    
    if (m_watchedStreams.contains(streamId)) {
        // Remove viewer widget
        m_videoGrid->removeViewerWidget(streamId);
        m_watchedStreams.remove(streamId);
        
        // Notify StreamManager
        if (m_streamManager) {
            m_streamManager->leaveStream(streamId);
        }
        
        qDebug() << "Stopped watching stream in conference:" << streamId;
    } else {
        qWarning() << "Stream not being watched in conference:" << streamId;
    }
}

// ===== VideoGridWidget Slots =====

void ConferenceWindow::onStreamerDisconnectRequested(int deviceIndex)
{
    qDebug() << "Streamer disconnect requested in conference for device:" << deviceIndex;

    // Remove streamer widget from grid
    m_videoGrid->removeStreamerWidget(deviceIndex);

    // TODO: Notify conference server about device removal
}

void ConferenceWindow::onViewerLeaveRequested(uint32_t streamId)
{
    qDebug() << "Viewer leave requested in conference for stream:" << streamId;
    onStopWatchingRequested(streamId);
}

void ConferenceWindow::onStreamStartRequested(int deviceIndex)
{
    qDebug() << "Stream start requested in conference for device:" << deviceIndex;
    startStreamInConference(deviceIndex);
}

void ConferenceWindow::onStreamStopRequested(uint32_t streamId)
{
    qDebug() << "Stream stop requested in conference for stream:" << streamId;
    
    // This is handled by the stream owner, we just remove our viewer if we're watching
    if (m_watchedStreams.contains(streamId)) {
        onStopWatchingRequested(streamId);
    }
}

void ConferenceWindow::onEncodedPacketReady(uint32_t streamId, int frameNumber, const QByteArray& packet)
{
    // Forward encoded packets to StreamManager for network transmission
    if (m_streamManager) {
        // TODO: StreamManager needs interface for sending video packets in conference context
        qDebug() << "Encoded packet ready in conference for stream:" << streamId 
                 << "frame:" << frameNumber << "size:" << packet.size();
    }
}

// ===== VideoSelectionDialog Slot =====

void ConferenceWindow::onDeviceSelected(int deviceIndex)
{
    qDebug() << "Device selected for conference:" << deviceIndex;
    startStreamInConference(deviceIndex);
}

// ===== StreamManager Slots =====

void ConferenceWindow::onStreamCreated(uint32_t streamId)
{
    qDebug() << "Stream created in conference:" << streamId;
    
    // Convert streamId to display ID and add to available streams
    char displayId[7] = {0};
    id_to_string(streamId, displayId);
    QString streamDisplayId = QString::fromLatin1(displayId, 6);
    
    addAvailableStream(streamId, streamDisplayId);
}

void ConferenceWindow::onStreamDeleted(uint32_t streamId)
{
    qDebug() << "Stream deleted in conference:" << streamId;
    removeAvailableStream(streamId);
}

void ConferenceWindow::onStreamJoined(uint32_t streamId)
{
    qDebug() << "Stream joined in conference:" << streamId;
    
    // The stream is now ready for viewing
    if (m_availableStreams.contains(streamId)) {
        // If we requested to watch this stream, add viewer widget
        if (!m_watchedStreams.contains(streamId)) {
            joinStreamInConference(streamId);
        }
    }
}

void ConferenceWindow::onErrorOccurred(const QString& message)
{
    qCritical() << "Error occurred in conference:" << message;
    showError(message);
}

// ===== Helper Methods =====

void ConferenceWindow::startStreamInConference(int deviceIndex)
{
    qDebug() << "Starting stream in conference for device:" << deviceIndex;

    // Add streamer widget to video grid
    m_videoGrid->addStreamerWidget(deviceIndex);

    // Create stream through StreamManager
    if (m_streamManager) {
        m_streamManager->createStream(deviceIndex);
    } else {
        showError("Stream manager not available");
    }
}

void ConferenceWindow::joinStreamInConference(uint32_t streamId)
{
    qDebug() << "Joining stream in conference:" << streamId;

    if (!m_availableStreams.contains(streamId)) {
        qWarning() << "Stream not available in conference:" << streamId;
        return;
    }

    if (m_watchedStreams.contains(streamId)) {
        qWarning() << "Already watching stream in conference:" << streamId;
        return;
    }

    QString displayId = m_availableStreams[streamId];
    
    // Add viewer widget to video grid
    m_videoGrid->addViewerWidget(streamId, displayId);
    m_watchedStreams[streamId] = displayId;

    // Join stream through StreamManager
    if (m_streamManager) {
        m_streamManager->joinStream(displayId);
    } else {
        showError("Stream manager not available");
    }
    
    qDebug() << "Joined stream in conference - ID:" << streamId << "Display:" << displayId;
}

void ConferenceWindow::updateConferenceInfo()
{
    // Update participants and streams count in control panel
    m_controlPanel->setParticipantsCount(m_participants.size());
    m_controlPanel->setStreamsCount(m_availableStreams.size());
    
    // Update window title
    setWindowTitle(QString("Conference: %1 (%2 participants, %3 streams)")
                  .arg(m_displayId)
                  .arg(m_participants.size())
                  .arg(m_availableStreams.size()));
}

void ConferenceWindow::closeEvent(QCloseEvent* event)
{
    qDebug() << "ConferenceWindow closing - ID:" << m_callId;
    
    // Уведомляем о закрытии конференции
    emit conferenceClosed(m_callId);
    
    // Выполняем cleanup
    cleanup();
    
    QMainWindow::closeEvent(event);
}
