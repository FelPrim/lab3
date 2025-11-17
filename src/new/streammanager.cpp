#include "streammanager.h"
#include "streampublisherwindow.h"
#include "streamviewerwindow.h"
#include <QRandomGenerator>
#include <QDebug>
#include <QMessageBox>
#include "networkfacade.h"
#include "video_defaults.h"

StreamManager::StreamManager(QObject *parent)
    : QObject(parent)
    , m_connectedToServer(false)
{
    qDebug() << "StreamManager created";
}

StreamManager::~StreamManager()
{
    cleanup();
}

void StreamManager::createStream(int deviceIndex)
{
    qDebug() << "Creating stream with device:" << deviceIndex;

    // Проверяем, нет ли уже ожидающего окна
    if (m_pendingWindow) {
        emit errorOccurred("Another stream is being created, please wait");
        return;
    }

    // Отправляем запрос на сервер
    if (m_networkFacade) {
        qDebug() << "StreamManager: sending CLIENT_STREAM_CREATE to server";
        m_networkFacade->sendStreamCreate();
    } else {
        qWarning() << "StreamManager: no network facade - cannot send CREATE";
        emit errorOccurred("Not connected to server");
        return;
    }

    // Создаем окно с временным ID (0)
    m_pendingWindow = new StreamPublisherWindow(0, deviceIndex);
    connect(m_pendingWindow, &StreamPublisherWindow::streamStopped, this, &StreamManager::onStreamStopped);
    connect(m_pendingWindow, &StreamWindow::windowClosed, this, &StreamManager::onWindowClosed);

    m_pendingDeviceIndex = deviceIndex;

    // Показываем окно сразу с временным заголовком
    m_pendingWindow->setStreamId(0, "Creating...");
    m_pendingWindow->initialize();
    emit streamWindowCreated(m_pendingWindow);
    
    qDebug() << "Stream publisher window created (pending server ID)";
}

void StreamManager::onServerStreamCreated(uint32_t streamId)
{
    qDebug() << "StreamManager: SERVER_STREAM_CREATED id=" << streamId;
    
    if (!m_pendingWindow) {
        qWarning() << "StreamManager: no pending window for server ID" << streamId;
        return;
    }

    StreamPublisherWindow *window = m_pendingWindow;
    m_pendingWindow = nullptr;
    
    QString displayId = StreamIdConverter::streamIdToDisplayString(streamId);
    
    // Обновляем окно с реальным ID
    window->setStreamId(streamId, displayId);
    m_openWindows[streamId] = window;
    
    // Настраиваем соединения для управления потоком
    setupStreamConnections(window, streamId);
    
    qDebug() << "Stream publisher window updated with server ID:" << streamId << "display:" << displayId;
    
    // Создаем NetworkManager для этого стрима
    if (m_networkFacade) {
        NetworkManager* manager = m_networkFacade->createNetworkManager(streamId);
        if (manager) {
            qDebug() << "NetworkManager created for stream:" << streamId;
        }
    }
}

void StreamManager::joinStream(const QString &displayId)
{
    if (!StreamIdConverter::isValidDisplayString(displayId)) {
        emit errorOccurred("Stream ID must be exactly 6 capital letters (A-Z)");
        return;
    }

    // Преобразуем displayId в serverId
    uint32_t streamId = StreamIdConverter::displayStringToStreamId(displayId);
    if (streamId == 0) {
        emit errorOccurred("Invalid stream ID");
        return;
    }

    qDebug() << "Joining stream:" << displayId << "-> server ID:" << streamId;

    // Проверяем, не открыто ли уже окно для этого streamId
    if (m_openWindows.contains(streamId)) {
        emit errorOccurred("Stream is already open");
        return;
    }

    // Отправляем запрос на сервер
    if (m_networkFacade) {
        qDebug() << "StreamManager: sending CLIENT_STREAM_JOIN id=" << streamId;
        m_networkFacade->sendStreamJoin(streamId);
    } else {
        qWarning() << "StreamManager: no network facade - cannot send JOIN";
        emit errorOccurred("Not connected to server");
        return;
    }

    // Создаем окно зрителя
    StreamViewerWindow *window = new StreamViewerWindow();
    connect(window, &StreamViewerWindow::streamLeft, this, &StreamManager::onStreamLeft);
    connect(window, &StreamWindow::windowClosed, this, &StreamManager::onWindowClosed);

    m_openWindows[streamId] = window;

    QTimer::singleShot(500, this, [this, window, streamId, displayId]() {
        window->setStreamId(streamId, displayId);
        window->initialize();
        emit streamWindowCreated(window);
        
        qDebug() << "Stream viewer window created for server ID:" << streamId << "display:" << displayId;
    });
}

void StreamManager::deleteStream(uint32_t streamId)
{
    qDebug() << "Delete stream requested:" << streamId;

    // Отправляем команду на сервер
    if (m_networkFacade) {
        qDebug() << "StreamManager: sending CLIENT_STREAM_DELETE id=" << streamId;
        m_networkFacade->sendStreamDelete(streamId);
    } else {
        qWarning() << "StreamManager: no network facade - cannot send DELETE";
    }

    // Локальные действия: закрываем окно
    if (m_openWindows.contains(streamId)) {
        m_openWindows[streamId]->close();
        m_openWindows.remove(streamId);
        emit streamWindowClosed(streamId);
    }
}

void StreamManager::leaveStream(uint32_t streamId)
{
    qDebug() << "Leave stream requested:" << streamId;

    // Отправляем команду на сервер
    if (m_networkFacade) {
        qDebug() << "StreamManager: sending CLIENT_STREAM_LEAVE id=" << streamId;
        m_networkFacade->sendStreamLeave(streamId);
    } else {
        qWarning() << "StreamManager: no network facade - cannot send LEAVE";
    }

    // Локальные действия: закрываем окно
    if (m_openWindows.contains(streamId)) {
        m_openWindows[streamId]->close();
        m_openWindows.remove(streamId);
        emit streamWindowClosed(streamId);
    }
}

// Остальные методы StreamManager остаются аналогичными, но с uint32_t
void StreamManager::onStreamStopped(uint32_t streamId)
{
    qDebug() << "Stream stopped by user, ID:" << streamId;
    deleteStream(streamId);
}

void StreamManager::onStreamLeft(uint32_t streamId)
{
    qDebug() << "Stream left by user, ID:" << streamId;
    leaveStream(streamId);
}


void StreamManager::onWindowClosed(uint32_t streamId)
{
    qDebug() << "Stream window closed, ID:" << streamId;
    
    // Если закрылось ожидающее окно
    if (m_pendingWindow && m_pendingWindow->getStreamId() == streamId) {
        m_pendingWindow = nullptr;
        m_pendingDeviceIndex = -1;
    }
    
    // Удаляем из открытых окон
    if (m_openWindows.contains(streamId)) {
        m_openWindows.remove(streamId);
    }
    
    emit streamWindowClosed(streamId);
}

bool StreamManager::isStreamActive(uint32_t streamId) const
{
    return m_openWindows.contains(streamId);
}

QVector<uint32_t> StreamManager::getActiveStreams() const
{
    QVector<uint32_t> keys;
    for (auto it = m_openWindows.begin(); it != m_openWindows.end(); ++it) {
        keys.append(it.key());
    }
    return keys;
}

QString StreamManager::getStreamStatus(uint32_t streamId) const
{
    return m_openWindows.contains(streamId) ? "Active" : "Inactive";
}

// Добавляем метод для настройки соединений
void StreamManager::setupStreamConnections(StreamPublisherWindow* window, uint32_t streamId)
{
    connect(window, &StreamPublisherWindow::streamingStateChanged,
            this, [this, streamId](bool enabled) {
                if (m_networkFacade) {
                    NetworkManager* manager = m_networkFacade->getNetworkManager(streamId);
                    if (manager) {
                        manager->setSendingEnabled(enabled);
                    }
                }
            });
}



void StreamManager::initialize()
{
    qDebug() << "StreamManager initialized";
    
    // Заглушка: имитируем успешное подключение через 1 секунду
    QTimer::singleShot(1000, this, [this]() {
        m_connectedToServer = true;
        emit connectionStatusChanged(true);
        qDebug() << "Connected to server (simulated)";
    });
    if (!m_networkFacade) {
        m_networkFacade = new NetworkFacade(this);
        // Используем константы из video_defaults.h (если их изменил на 127.0.0.1) либо явно localhost:
        m_networkFacade->setServer(QString::fromUtf8(DEFAULT_ECHO_SERVER_ADDRESS), DEFAULT_ECHO_SERVER_PORT, 23230);

        // Сообщаем фасаду, что у нас пока нет конкретной локальной UDP информации:
        m_networkFacade->setLocalUdpInfo(QHostAddress::AnyIPv4, 0);

        // Пересылаем сигналы фасада в методы StreamManager
        connect(m_networkFacade, &NetworkFacade::connected, this, [](){ qDebug() << "NetworkFacade connected"; });
        connect(m_networkFacade, &NetworkFacade::errorOccurred, this, [](const QString &err){ qWarning() << "NetworkFacade error:" << err; });
        connect(m_networkFacade, &NetworkFacade::connected, this, [this]() {
            m_connectedToServer = true;
            emit connectionStatusChanged(true);
            qDebug() << "StreamManager: NetworkFacade reports connected";
        });
        connect(m_networkFacade, &NetworkFacade::disconnected, this, [this]() {
            m_connectedToServer = false;
            emit connectionStatusChanged(false);
            qDebug() << "StreamManager: NetworkFacade reports disconnected";
        });
        // Перенаправляем события сервера в StreamManager (реализуй методы-обработчики, если их ещё нет)
        connect(m_networkFacade, &NetworkFacade::serverStreamCreated, this, &StreamManager::onServerStreamCreated);
        connect(m_networkFacade, &NetworkFacade::serverStreamDeleted, this, &StreamManager::onServerStreamDeleted);
        connect(m_networkFacade, &NetworkFacade::serverStreamJoined, this, &StreamManager::onServerStreamJoined);
        connect(m_networkFacade, &NetworkFacade::serverStreamStart, this, &StreamManager::onServerStreamStart);
        connect(m_networkFacade, &NetworkFacade::serverStreamEnd, this, &StreamManager::onServerStreamEnd);
    }
}

void StreamManager::cleanup()
{
    qDebug() << "StreamManager cleanup";
    for (auto window : m_openWindows) {
        if (window) {
            window->close();
            window->deleteLater();
        }
    }
    m_openWindows.clear();
}


void StreamManager::setServerAddress(const QString &address, quint16 port)
{
    qDebug() << "Set server address:" << address << ":" << port;
    if (!m_networkFacade) {
        m_networkFacade = new NetworkFacade(this);
        // подключим обработчики как в initialize(), если нужно
    }
    m_networkFacade->setServer(address, port, /*udpPort*/ static_cast<quint16>(port - 1)); // если UDP порт = TCP-1, или укажи явный
}


void StreamManager::connectToServer()
{
    qDebug() << "Connect to server";
    
    if (!m_networkFacade) {
        qWarning() << "StreamManager: network facade is null; calling initialize()";
        initialize();
    }
    if (m_networkFacade) {
        m_networkFacade->connectToServer();
    } else {
        qWarning() << "StreamManager: cannot connect - no NetworkFacade";
    }
    
    m_connectedToServer = true;
    emit connectionStatusChanged(true);
}

void StreamManager::disconnectFromServer()
{
    qDebug() << "Disconnect from server";
    m_connectedToServer = false;
    emit connectionStatusChanged(false);
}



void StreamManager::onServerStreamDeleted(uint32_t streamId)
{
    qDebug() << "StreamManager: SERVER_STREAM_DELETED id=" << streamId;
    // Если у нас есть окно с таким streamId — закроем его.
    if (m_openWindows.contains((int)streamId)) {
        m_openWindows[(int)streamId]->close();
        m_openWindows.remove((int)streamId);
        emit streamWindowClosed((int)streamId);
    }
}

void StreamManager::onServerStreamJoined(uint32_t streamId)
{
    qDebug() << "StreamManager: SERVER_STREAM_JOINED id=" << streamId;
    // Заглушка: можно создать окно viewer или обновить существующее — сейчас просто логируем.
}

void StreamManager::onServerStreamStart(uint32_t streamId)
{
    qDebug() << "StreamManager: SERVER_STREAM_START id=" << streamId;
    
    if (m_openWindows.contains(streamId)) {
        auto w = m_openWindows[streamId];
        StreamPublisherWindow *pub = qobject_cast<StreamPublisherWindow*>(w);
        if (pub) {
            pub->setViewersStatus(true);
            pub->setStreamingEnabled(true); 
            qDebug() << "Stream publishing enabled for:" << streamId;
        } else {
            qDebug() << "StreamManager: window is not a publisher for id" << streamId;
        }
    } else {
        qDebug() << "StreamManager: no open window for stream" << streamId;
    }
}

void StreamManager::onServerStreamEnd(uint32_t streamId)
{
    qDebug() << "StreamManager: SERVER_STREAM_END id=" << streamId;
    
    if (m_openWindows.contains(streamId)) {
        auto w = m_openWindows[streamId];
        StreamPublisherWindow *pub = qobject_cast<StreamPublisherWindow*>(w);
        if (pub) {
            pub->setViewersStatus(false);
            pub->setStreamingEnabled(false); 
            qDebug() << "Stream publishing disabled for:" << streamId;
        } else {
            qDebug() << "StreamManager: window is not a publisher for id" << streamId;
        }
    } else {
        qDebug() << "StreamManager: no open window for stream" << streamId;
    }
}