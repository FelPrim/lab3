#include "networkfacade.h"
#include "udp/networkmanager.h"
#include <QDebug>
#include <QtEndian>
#include <QNetworkInterface>
#include <QThread>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

NetworkFacade::NetworkFacade(QObject *parent)
    : QObject(parent)
    , m_tcp(new TCPManager(this))
    , m_udpManager(new UDPManager(this))
    , m_handshakeService(new HandshakeService(m_udpManager, this))
{
    // TCP connections
    connect(m_tcp, &TCPManager::connected, this, &NetworkFacade::onTcpConnected);
    connect(m_tcp, &TCPManager::disconnected, this, &NetworkFacade::onTcpDisconnected);
    connect(m_tcp, &TCPManager::errorOccurred, this, &NetworkFacade::onTcpError);

    // Handshake messages from server
    connect(m_tcp, &TCPManager::serverHandshakeStart, this, &NetworkFacade::onServerHandshakeStart);
    connect(m_tcp, &TCPManager::serverHandshakeEnd, this, &NetworkFacade::onServerHandshakeEnd);

    // Handshake service signals
    connect(m_handshakeService, &HandshakeService::handshakeStarted, this, &NetworkFacade::onHandshakeStarted);
    connect(m_handshakeService, &HandshakeService::handshakeCompleted, this, &NetworkFacade::onHandshakeCompleted);
    connect(m_handshakeService, &HandshakeService::handshakeFailed, this, &NetworkFacade::onHandshakeFailed);

    // Server message handlers
    connect(m_tcp, &TCPManager::serverStreamCreated, this, &NetworkFacade::onServerStreamCreated);
    connect(m_tcp, &TCPManager::serverStreamDeleted, this, &NetworkFacade::onServerStreamDeleted);
    connect(m_tcp, &TCPManager::serverStreamJoined, this, &NetworkFacade::onServerStreamJoined);
    connect(m_tcp, &TCPManager::serverStreamStart, this, &NetworkFacade::onServerStreamStart);
    connect(m_tcp, &TCPManager::serverStreamEnd, this, &NetworkFacade::onServerStreamEnd);

    connect(m_tcp, &TCPManager::serverCallCreated, this, &NetworkFacade::onServerCallCreated);
    connect(m_tcp, &TCPManager::serverCallConnJoined, this, &NetworkFacade::onServerCallConnJoined);
    connect(m_tcp, &TCPManager::serverCallConnNew, this, &NetworkFacade::onServerCallConnNew);
    connect(m_tcp, &TCPManager::serverCallConnLeft, this, &NetworkFacade::onServerCallConnLeft);
    connect(m_tcp, &TCPManager::serverCallStreamNew, this, &NetworkFacade::onServerCallStreamNew);
    connect(m_tcp, &TCPManager::serverCallStreamDeleted, this, &NetworkFacade::onServerCallStreamDeleted);
    connect(m_tcp, &TCPManager::serverErrorReceived, this, &NetworkFacade::serverErrorReceived);
    connect(m_tcp, &TCPManager::serverSuccessReceived, this, &NetworkFacade::serverSuccessReceived);

    // Initialize UDP manager
    m_udpManager->initialize();
    m_localUdpPort = m_udpManager->getLocalPort();

    // ✅ ДОБАВЛЕНО: Инициализация call 0 (публичные стримы)
    CallState publicCall;
    publicCall.callId = 0;
    m_callStates[0] = publicCall;
}

NetworkFacade::~NetworkFacade()
{
    for (auto it = m_networkManagers.begin(); it != m_networkManagers.end(); ++it) {
        it.value()->cleanup();
        delete it.value();  
    }
    m_networkManagers.clear();
}

void NetworkFacade::setServer(const QString &host, quint16 tcpPort, quint16 udpPort)
{
    m_serverHost = host;
    m_serverTcpPort = tcpPort;
    m_serverUdpPort = udpPort;
    if (m_tcp) {
        m_tcp->setServer(host, tcpPort);
    }
}

void NetworkFacade::setLocalUdpInfo(const QHostAddress &localIp, quint16 localUdpPort)
{
    m_localUdpIp = localIp;
    m_localUdpPort = localUdpPort;
}

bool NetworkFacade::initialize()
{
    return true;
}

void NetworkFacade::connectToServer()
{
    if (!m_tcp) return;
    m_tcp->connectToServer();
}

void NetworkFacade::disconnect()
{
    m_handshakeService->stopHandshake();
    
    if (m_tcp) {
        m_tcp->disconnectFromServer();
    }
    
    // Clean up all NetworkManagers
    for (auto it = m_networkManagers.begin(); it != m_networkManagers.end(); ++it) {
        it.value()->cleanup();
        it.value()->deleteLater();
    }
    m_networkManagers.clear();

    // ✅ ДОБАВЛЕНО: Очистка состояния
    m_streamStates.clear();
    m_callStates.clear();
    m_ownedStreams.clear();
    m_joinedStreams.clear();
    m_activeStreams.clear();
    m_joinedCalls.clear();

    // Восстанавливаем только call 0
    CallState publicCall;
    publicCall.callId = 0;
    m_callStates[0] = publicCall;
}

NetworkManager* NetworkFacade::createNetworkManager(int streamId)
{
    if (streamId <= 0) {
        qWarning() << "NetworkFacade: Invalid streamId" << streamId;
        return nullptr;
    }
    
    if (m_networkManagers.contains(streamId)) {
        qDebug() << "NetworkFacade: NetworkManager for stream" << streamId << "already exists";
        return m_networkManagers[streamId];
    }
    
    NetworkManager *manager = new NetworkManager(streamId, this);
    if (manager->initialize(m_udpManager)) {
        manager->setServerAddress(m_serverHost, m_serverUdpPort);
        
        // Set callId if it was previously set for this stream
        if (m_streamCallIds.contains(streamId)) {
            manager->setCallId(m_streamCallIds[streamId]);
        }
        
        connect(manager, &NetworkManager::frameAssembled, 
                this, &NetworkFacade::frameAssembled);
        connect(manager, &NetworkManager::errorOccurred,
                this, &NetworkFacade::networkErrorOccurred);
        
        m_networkManagers[streamId] = manager;
        qDebug() << "NetworkFacade: Created NetworkManager for stream" << streamId;
        return manager;
    } else {
        qWarning() << "NetworkFacade: Failed to initialize NetworkManager for stream" << streamId;
        delete manager;
        return nullptr;
    }
}

void NetworkFacade::removeNetworkManager(int streamId)
{
    if (m_networkManagers.contains(streamId)) {
        NetworkManager *manager = m_networkManagers.take(streamId);
        manager->cleanup();
        manager->deleteLater();
        qDebug() << "NetworkFacade: Removed NetworkManager for stream" << streamId;
    }
}

NetworkManager* NetworkFacade::getNetworkManager(int streamId)
{
    return m_networkManagers.value(streamId, nullptr);
}

void NetworkFacade::sendStreamCreate(uint32_t callId)
{
    if (m_tcp) m_tcp->sendClientStreamCreate(callId);
}

void NetworkFacade::sendStreamDelete(uint32_t streamId)
{
    if (m_tcp) m_tcp->sendClientStreamDelete(streamId);
}

void NetworkFacade::sendStreamJoin(uint32_t streamId)
{
    if (m_tcp) m_tcp->sendClientStreamJoin(streamId);
    
    // ✅ ДОБАВЛЕНО: Отправляем UDP-пакеты для проброса NAT
    if (isHandshakeCompleted()) {
        uint32_t connectionId = getConnectionId();
        sendNatTraversalPackets(connectionId);
    } else {
        qWarning() << "NetworkFacade: Cannot send NAT traversal packets - handshake not completed";
    }
}

void NetworkFacade::sendStreamLeave(uint32_t streamId)
{
    if (m_tcp) m_tcp->sendClientStreamLeave(streamId);
}

void NetworkFacade::setCallIdForStream(int streamId, uint32_t callId)
{
    m_streamCallIds[streamId] = callId;
    
    // Update existing NetworkManager if it exists
    if (m_networkManagers.contains(streamId)) {
        m_networkManagers[streamId]->setCallId(callId);
    }

    // ✅ ДОБАВЛЕНО: Обновление состояния
    if (m_streamStates.contains(streamId)) {
        m_streamStates[streamId].callId = callId;
    }

    // Обновляем call state
    if (m_callStates.contains(callId)) {
        m_callStates[callId].streams.insert(streamId);
    }
}

void NetworkFacade::onTcpConnected()
{
    qDebug() << "NetworkFacade: TCP connected to" << m_serverHost << ":" << m_serverTcpPort;
    
    // Wait for SERVER_HANDSHAKE_START from server
    qDebug() << "Waiting for handshake start from server...";
    
    emit connected();
}

void NetworkFacade::onTcpDisconnected()
{
    qDebug() << "NetworkFacade: TCP disconnected";
    
    m_handshakeService->stopHandshake();
    
    // Clean up all NetworkManagers
    for (auto it = m_networkManagers.begin(); it != m_networkManagers.end(); ++it) {
        it.value()->cleanup();
        it.value()->deleteLater();
    }
    m_networkManagers.clear();
    
    // ✅ ДОБАВЛЕНО: Очистка состояния
    m_streamStates.clear();
    m_callStates.clear();
    m_ownedStreams.clear();
    m_joinedStreams.clear();
    m_activeStreams.clear();
    m_joinedCalls.clear();

    // Восстанавливаем только call 0
    CallState publicCall;
    publicCall.callId = 0;
    m_callStates[0] = publicCall;
    
    emit disconnected();
}

void NetworkFacade::onTcpError(const QString &err)
{
    qWarning() << "NetworkFacade: TCP error:" << err;
    emit errorOccurred(err);
}

void NetworkFacade::onServerHandshakeStart(uint32_t connectionId)
{
    qDebug() << "NetworkFacade: Server handshake start, connectionId:" << connectionId;
    
    // Start UDP handshake process
    m_handshakeService->startHandshake(connectionId, QHostAddress(m_serverHost), m_serverUdpPort);
}

void NetworkFacade::onServerHandshakeEnd(uint32_t connectionId)
{
    qDebug() << "NetworkFacade: Server handshake end, connectionId:" << connectionId;
    
    // Notify handshake service that handshake is confirmed
    m_handshakeService->onHandshakeConfirmed();
}

void NetworkFacade::onHandshakeStarted(uint32_t connectionId)
{
    qDebug() << "NetworkFacade: Handshake started for connection" << connectionId;
    emit handshakeStarted(connectionId);
}

void NetworkFacade::onHandshakeCompleted(uint32_t connectionId)
{
    qDebug() << "NetworkFacade: Handshake completed for connection" << connectionId;
    emit handshakeCompleted(connectionId);
}

void NetworkFacade::onHandshakeFailed(const QString &error)
{
    qWarning() << "NetworkFacade: Handshake failed:" << error;
    emit errorOccurred(error);
}

void NetworkFacade::onServerStreamCreated(uint32_t id)
{
    qDebug() << "NetworkFacade: Server stream created, id:" << id;
    
    // Create NetworkManager for this stream
    NetworkManager *manager = createNetworkManager(static_cast<int>(id));
    if (manager) {
        qDebug() << "NetworkFacade: NetworkManager created for stream" << id;
    }
    
    // ✅ ДОБАВЛЕНО: Обновление состояния
    StreamState state;
    state.streamId = id;
    state.callId = 0; // по умолчанию публичный
    state.isOwner = true;
    state.isActive = false;
    state.status = "Created, waiting for start";
    m_streamStates[id] = state;
    m_ownedStreams.insert(id);

    // Печатаем обновленное состояние
    printClientState();
    
    emit serverStreamCreated(id);
}

void NetworkFacade::onServerStreamDeleted(uint32_t id)
{
    qDebug() << "NetworkFacade: Server stream deleted, id:" << id;
    removeNetworkManager(static_cast<int>(id));
    
    // ✅ ДОБАВЛЕНО: Обновление состояния
    if (m_streamStates.contains(id)) {
        uint32_t callId = m_streamStates[id].callId;
        m_streamStates.remove(id);
        m_ownedStreams.remove(id);
        m_joinedStreams.remove(id);
        m_activeStreams.remove(id);

        // Удаляем из call state
        if (m_callStates.contains(callId)) {
            m_callStates[callId].streams.remove(id);
        }
    }

    // Печатаем обновленное состояние
    printClientState();
    
    emit serverStreamDeleted(id);
}

void NetworkFacade::onServerStreamJoined(uint32_t id)
{
    qDebug() << "NetworkFacade: Server stream joined, id:" << id;
    
    NetworkManager *manager = createNetworkManager(static_cast<int>(id));
    if (manager) {
        qDebug() << "NetworkFacade: NetworkManager ready to receive stream" << id;
    }
    
    // ✅ ДОБАВЛЕНО: Обновление состояния
    if (!m_streamStates.contains(id)) {
        StreamState state;
        state.streamId = id;
        state.callId = 0; // по умолчанию публичный
        state.isOwner = false;
        state.isActive = false;
        state.status = "Joined as viewer";
        m_streamStates[id] = state;
        m_joinedStreams.insert(id);
    }

    // Печатаем обновленное состояние
    printClientState();
    
    emit serverStreamJoined(id);
}

void NetworkFacade::onServerStreamStart(uint32_t id)
{
    qDebug() << "NetworkFacade: Server stream start, id:" << id;
    
    NetworkManager *manager = getNetworkManager(static_cast<int>(id));
    if (manager) {
        manager->setSendingEnabled(true);
        manager->start();
        qDebug() << "NetworkFacade: Stream" << id << "started sending";
    }
    
    // ✅ ДОБАВЛЕНО: Обновление состояния
    if (m_streamStates.contains(id)) {
        m_streamStates[id].isActive = true;
        m_streamStates[id].status = "Active streaming";
        m_activeStreams.insert(id);
    }

    // Печатаем обновленное состояние
    printClientState();
    
    emit serverStreamStart(id);
}

void NetworkFacade::onServerStreamEnd(uint32_t id)
{
    qDebug() << "NetworkFacade: Server stream end, id:" << id;
    
    NetworkManager *manager = getNetworkManager(static_cast<int>(id));
    if (manager) {
        manager->setSendingEnabled(false);
        manager->stop();
    }
    
    // ✅ ДОБАВЛЕНО: Обновление состояния
    if (m_streamStates.contains(id)) {
        m_streamStates[id].isActive = false;
        m_streamStates[id].status = "Streaming stopped";
        m_activeStreams.remove(id);
    }

    // Печатаем обновленное состояние
    printClientState();
    
    emit serverStreamEnd(id);
}

void NetworkFacade::sendCallCreate()
{
    if (m_tcp) m_tcp->sendClientCallCreate();
}

void NetworkFacade::sendCallJoin(uint32_t callId)
{
    if (m_tcp) m_tcp->sendClientCallJoin(callId);
}

void NetworkFacade::sendCallLeave(uint32_t callId)
{
    if (m_tcp) m_tcp->sendClientCallLeave(callId);
}

// ✅ ДОБАВЛЕНО: Обработчики сообщений о звонках
void NetworkFacade::onServerCallCreated(uint32_t callId)
{
    qDebug() << "NetworkFacade: Server call created, id:" << callId;
    
    // ✅ ДОБАВЛЕНО: Обновление состояния
    if (!m_callStates.contains(callId)) {
        CallState state;
        state.callId = callId;
        m_callStates[callId] = state;
    }
    m_joinedCalls.insert(callId);

    // Печатаем обновленное состояние
    printClientState();
    
    emit serverCallCreated(callId);
}

void NetworkFacade::onServerCallConnJoined(uint32_t callId, const QVector<uint32_t>& participants, const QVector<uint32_t>& streams)
{
    qDebug() << "NetworkFacade: Server call conn joined, callId:" << callId 
             << "participants:" << participants.size() << "streams:" << streams.size();
    
    // Установить callId для всех стримов в этом звонке
    for (uint32_t streamId : streams) {
        m_streamCallIds[static_cast<int>(streamId)] = callId;
        
        // Обновить существующий NetworkManager
        NetworkManager* manager = getNetworkManager(static_cast<int>(streamId));
        if (manager) {
            manager->setCallId(callId);
        }

        // ✅ ДОБАВЛЕНО: Обновление состояния
        if (m_streamStates.contains(streamId)) {
            m_streamStates[streamId].callId = callId;
        }
    }
    
    // ✅ ДОБАВЛЕНО: Обновление состояния call
    if (!m_callStates.contains(callId)) {
        CallState state;
        state.callId = callId;
        m_callStates[callId] = state;
    }
    
    m_callStates[callId].participants = QSet<uint32_t>(participants.begin(), participants.end());
    m_callStates[callId].streams = QSet<uint32_t>(streams.begin(), streams.end());
    m_joinedCalls.insert(callId);

    // Печатаем обновленное состояние
    printClientState();
    
    emit serverCallConnJoined(callId, participants, streams);
}

void NetworkFacade::onServerCallConnNew(uint32_t callId, uint32_t participantId)
{
    qDebug() << "NetworkFacade: Server call conn new, callId:" << callId << "participantId:" << participantId;
    
    // ✅ ДОБАВЛЕНО: Обновление состояния
    if (m_callStates.contains(callId)) {
        m_callStates[callId].participants.insert(participantId);
    }

    // Печатаем обновленное состояние
    printClientState();
    
    emit serverCallConnNew(callId, participantId);
}

void NetworkFacade::onServerCallConnLeft(uint32_t callId, uint32_t participantId)
{
    qDebug() << "NetworkFacade: Server call conn left, callId:" << callId << "participantId:" << participantId;
    
    // ✅ ДОБАВЛЕНО: Обновление состояния
    if (m_callStates.contains(callId)) {
        m_callStates[callId].participants.remove(participantId);
    }

    // Печатаем обновленное состояние
    printClientState();
    
    emit serverCallConnLeft(callId, participantId);
}

void NetworkFacade::onServerCallStreamNew(uint32_t callId, uint32_t streamId)
{
    qDebug() << "NetworkFacade: Server call stream new, callId:" << callId << "streamId:" << streamId;
    
    // Установить callId для этого стрима
    m_streamCallIds[static_cast<int>(streamId)] = callId;
    
    // Обновить существующий NetworkManager
    NetworkManager* manager = getNetworkManager(static_cast<int>(streamId));
    if (manager) {
        manager->setCallId(callId);
    }
    
    // ✅ ДОБАВЛЕНО: Обновление состояния
    if (m_streamStates.contains(streamId)) {
        m_streamStates[streamId].callId = callId;
    }

    if (m_callStates.contains(callId)) {
        m_callStates[callId].streams.insert(streamId);
    }

    // Печатаем обновленное состояние
    printClientState();
    
    emit serverCallStreamNew(callId, streamId);
}

void NetworkFacade::onServerCallStreamDeleted(uint32_t callId, uint32_t streamId)
{
    qDebug() << "NetworkFacade: Server call stream deleted, callId:" << callId << "streamId:" << streamId;
    
    // Удалить callId для этого стрима
    m_streamCallIds.remove(static_cast<int>(streamId));
    
    // ✅ ДОБАВЛЕНО: Обновление состояния
    if (m_streamStates.contains(streamId)) {
        m_streamStates[streamId].callId = 0; // возвращаем в публичные
    }

    if (m_callStates.contains(callId)) {
        m_callStates[callId].streams.remove(streamId);
    }

    // Печатаем обновленное состояние
    printClientState();
    
    emit serverCallStreamDeleted(callId, streamId);
}

// ✅ ДОБАВЛЕНО: Функции для отслеживания состояния клиента
void NetworkFacade::printClientState() const
{
    qDebug() << "=== Client Network State ===";
    qDebug() << "Connection ID:" << getConnectionId();
    
    // Calls
    QList<uint32_t> callList = m_callStates.keys();
    std::sort(callList.begin(), callList.end());
    qDebug() << "Calls:" << callList;
    
    // Streams
    QList<uint32_t> streamList = m_streamStates.keys();
    std::sort(streamList.begin(), streamList.end());
    qDebug() << "Streams:" << streamList;
    
    // Detailed call information
    qDebug() << "=== Call Details ===";
    for (uint32_t callId : callList) {
        const CallState& call = m_callStates[callId];
        QList<uint32_t> streamsList = call.streams.values();
        std::sort(streamsList.begin(), streamsList.end());
        qDebug() << "Call" << callId << "-> Streams:" << streamsList;
    }
    
    // Detailed stream information
    qDebug() << "=== Stream Details ===";
    for (uint32_t streamId : streamList) {
        const StreamState& stream = m_streamStates[streamId];
        QString role = stream.isOwner ? "Streamer" : "Viewer";
        QString active = stream.isActive ? "Active" : "Inactive";
        qDebug() << "Stream" << streamId << "-> Call:" << stream.callId 
                 << ", Role:" << role << ", Status:" << active << "-" << stream.status;
    }
    
    qDebug() << "=== Summary ===";
    qDebug() << "Owned Streams:" << m_ownedStreams.size() << "-" << m_ownedStreams.values();
    qDebug() << "Joined Streams:" << m_joinedStreams.size() << "-" << m_joinedStreams.values();
    qDebug() << "Active Streams:" << m_activeStreams.size() << "-" << m_activeStreams.values();
    qDebug() << "Joined Calls:" << m_joinedCalls.size() << "-" << m_joinedCalls.values();
    qDebug() << "=====================";
}

QVector<uint32_t> NetworkFacade::getCallIds() const
{
    return QVector<uint32_t>(m_callStates.keys().begin(), m_callStates.keys().end());
}

QVector<uint32_t> NetworkFacade::getStreamIds() const
{
    return QVector<uint32_t>(m_streamStates.keys().begin(), m_streamStates.keys().end());
}

StreamState NetworkFacade::getStreamState(uint32_t streamId) const
{
    return m_streamStates.value(streamId, StreamState{0, 0, false, false, "Not found"});
}

CallState NetworkFacade::getCallState(uint32_t callId) const
{
    return m_callStates.value(callId, CallState{0, QSet<uint32_t>(), QSet<uint32_t>()});
}

void NetworkFacade::sendNatTraversalPackets(uint32_t connectionId)
{
    if (!m_udpManager) {
        qWarning() << "NetworkFacade: UDP manager not available for NAT traversal";
        return;
    }
    
    // Создаем пакет такой же, как в HandshakeService
    QByteArray packet;
    packet.resize(12);
    memset(packet.data(), 0, 8);
    quint32 connectionIdBe = qToBigEndian<quint32>(connectionId);
    memcpy(packet.data() + 8, &connectionIdBe, 4);
    
    // Отправляем несколько пакетов (например, 5-10) для надежности
    const int NUM_PACKETS = 10;
    qDebug() << "NetworkFacade: Sending" << NUM_PACKETS << "NAT traversal packets for connection" << connectionId;
    
    for (int i = 0; i < NUM_PACKETS; ++i) {
        m_udpManager->sendPacket(packet, QHostAddress(m_serverHost), m_serverUdpPort);
        
        // Небольшая задержка между пакетами (например, 20 мс)
        if (i < NUM_PACKETS - 1) {
            QThread::msleep(20);
        }
    }
}