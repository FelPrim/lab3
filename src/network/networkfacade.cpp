#include "networkfacade.h"
#include "udp/networkmanager.h"
#include <QDebug>
#include <QtEndian>
#include <QNetworkInterface>
#include "../ui/id_utils.h"

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

NetworkFacade::NetworkFacade(QObject *parent)
    : QObject(parent)
    , m_tcp(new TCPManager(this))
    , m_udpManager(new UDPManager(this))
    , m_handshakeTimer(new QTimer(this))
{
    // TCP connections
    connect(m_tcp, &TCPManager::connected, this, &NetworkFacade::onTcpConnected);
    connect(m_tcp, &TCPManager::disconnected, this, &NetworkFacade::onTcpDisconnected);
    connect(m_tcp, &TCPManager::errorOccurred, this, &NetworkFacade::onTcpError);

    // Handshake messages
    connect(m_tcp, &TCPManager::serverHandshakeStart, this, &NetworkFacade::onServerHandshakeStart);
    connect(m_tcp, &TCPManager::serverHandshakeEnd, this, &NetworkFacade::onServerHandshakeEnd);

    // Error/Success messages
    connect(m_tcp, &TCPManager::serverErrorReceived, this, &NetworkFacade::onServerErrorReceived);
    connect(m_tcp, &TCPManager::serverSuccessReceived, this, &NetworkFacade::onServerSuccessReceived);

    // Server message handlers - Streams
    connect(m_tcp, &TCPManager::serverStreamCreated, this, &NetworkFacade::onServerStreamCreated);
    connect(m_tcp, &TCPManager::serverStreamDeleted, this, &NetworkFacade::onServerStreamDeleted);
    connect(m_tcp, &TCPManager::serverStreamJoined, this, &NetworkFacade::onServerStreamJoined);
    connect(m_tcp, &TCPManager::serverStreamStart, this, &NetworkFacade::onServerStreamStart);
    connect(m_tcp, &TCPManager::serverStreamEnd, this, &NetworkFacade::onServerStreamEnd);

    // Server message handlers - Calls
    connect(m_tcp, &TCPManager::serverCallCreated, this, &NetworkFacade::onServerCallCreated);
    connect(m_tcp, &TCPManager::serverCallConnJoined, this, &NetworkFacade::onServerCallConnJoined);
    connect(m_tcp, &TCPManager::serverCallConnNew, this, &NetworkFacade::onServerCallConnNew);
    connect(m_tcp, &TCPManager::serverCallConnLeft, this, &NetworkFacade::onServerCallConnLeft);
    connect(m_tcp, &TCPManager::serverCallStreamNew, this, &NetworkFacade::onServerCallStreamNew);
    connect(m_tcp, &TCPManager::serverCallStreamDeleted, this, &NetworkFacade::onServerCallStreamDeleted);

    // UDP handshake timer
    connect(m_handshakeTimer, &QTimer::timeout, this, &NetworkFacade::onUdpHandshakeTimeout);
    m_handshakeTimer->setInterval(100); // 10 packets per second

    // Initialize UDP manager
    m_udpManager->initialize();
    m_localUdpPort = m_udpManager->getLocalPort();
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
    // UDP manager is already initialized in constructor
    return true;
}

void NetworkFacade::connectToServer()
{
    if (!m_tcp) return;
    
    // Reset handshake state
    m_connectionId = 0;
    m_handshakeCompleted = false;
    m_handshakeAttempts = 0;
    
    m_tcp->connectToServer();
}

void NetworkFacade::disconnect()
{
    // Stop handshake timer
    stopUdpHandshake();
    
    if (m_tcp) {
        m_tcp->disconnectFromServer();
    }
    
    // Clean up all NetworkManagers on disconnect
    for (auto it = m_networkManagers.begin(); it != m_networkManagers.end(); ++it) {
        it.value()->cleanup();
        it.value()->deleteLater();
    }
    m_networkManagers.clear();
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

// Stream methods
void NetworkFacade::sendStreamCreate(uint32_t callId) { 
    if (m_tcp) m_tcp->sendClientStreamCreate(callId); 
}

void NetworkFacade::sendStreamDelete(uint32_t id) { 
    if (m_tcp) m_tcp->sendClientStreamDelete(id); 
}

void NetworkFacade::sendStreamJoin(uint32_t id) { 
    if (m_tcp) m_tcp->sendClientStreamJoin(id); 
}

void NetworkFacade::sendStreamLeave(uint32_t id) { 
    if (m_tcp) m_tcp->sendClientStreamLeave(id); 
}

// Call methods
void NetworkFacade::sendCallCreate() {
    if (m_tcp) m_tcp->sendClientCallCreate();
}

void NetworkFacade::sendCallJoin(uint32_t callId) {
    if (m_tcp) m_tcp->sendClientCallJoin(callId);
}

void NetworkFacade::sendCallLeave(uint32_t callId) {
    if (m_tcp) m_tcp->sendClientCallLeave(callId);
}

// Error/Success methods
void NetworkFacade::sendClientError(uint8_t originalMessageType, const QString &errorMessage) {
    if (m_tcp) m_tcp->sendClientError(originalMessageType, errorMessage);
}

void NetworkFacade::sendClientSuccess(uint8_t originalMessageType, const QString &successMessage) {
    if (m_tcp) m_tcp->sendClientSuccess(originalMessageType, successMessage);
}

void NetworkFacade::onTcpConnected()
{
    qDebug() << "NetworkFacade::onTcpConnected — serverHost:" << m_serverHost
         << "tcpPort:" << m_serverTcpPort
         << "localUdpIp:" << m_localUdpIp << "localUdpPort:" << m_localUdpPort;

    // Wait for SERVER_HANDSHAKE_START from server
    // Server will send us our connection ID and then we start UDP handshake
    qDebug() << "TCP connected, waiting for SERVER_HANDSHAKE_START...";
    
    emit connected();
}

void NetworkFacade::onTcpDisconnected()
{
    qDebug() << "NetworkFacade: TCP disconnected";
    
    // Stop handshake process
    stopUdpHandshake();
    
    // Clean up all NetworkManagers on TCP disconnect
    for (auto it = m_networkManagers.begin(); it != m_networkManagers.end(); ++it) {
        it.value()->cleanup();
        it.value()->deleteLater();
    }
    m_networkManagers.clear();
    
    emit disconnected();
}

void NetworkFacade::onTcpError(const QString &err)
{
    qWarning() << "NetworkFacade: TCP error:" << err;
    emit errorOccurred(err);
}

// Handshake handling
void NetworkFacade::onServerHandshakeStart(uint32_t connectionId)
{
    qDebug() << "NetworkFacade: SERVER_HANDSHAKE_START received, connectionId:" << connectionId;
    
    m_connectionId = connectionId;
    m_handshakeAttempts = 0;
    
    emit handshakeStarted(connectionId);
    
    // Start sending UDP handshake packets
    m_handshakeTimer->start();
    qDebug() << "Started UDP handshake process, sending packets to server...";
}

void NetworkFacade::onServerHandshakeEnd(uint32_t connectionId)
{
    if (connectionId != m_connectionId) {
        qWarning() << "NetworkFacade: Handshake end for wrong connectionId, expected:" 
                   << m_connectionId << "got:" << connectionId;
        return;
    }
    
    qDebug() << "NetworkFacade: SERVER_HANDSHAKE_END received, handshake completed!";
    
    stopUdpHandshake();
    m_handshakeCompleted = true;
    
    emit handshakeCompleted(connectionId);
}

void NetworkFacade::onUdpHandshakeTimeout()
{
    if (m_handshakeCompleted) {
        m_handshakeTimer->stop();
        return;
    }
    
    m_handshakeAttempts++;
    if (m_handshakeAttempts > MAX_HANDSHAKE_ATTEMPTS) {
        qWarning() << "NetworkFacade: UDP handshake timeout after" << MAX_HANDSHAKE_ATTEMPTS << "attempts";
        stopUdpHandshake();
        emit errorOccurred("UDP handshake timeout - cannot establish connection with server");
        return;
    }
    
    sendUdpHandshakePacket();
}

void NetworkFacade::sendUdpHandshakePacket()
{
    if (!m_udpManager || m_connectionId == 0) {
        return;
    }
    
    // Create UDP handshake packet: 8 zero bytes + 4 bytes connectionId
    QByteArray handshakePacket;
    handshakePacket.resize(12); // 8 zeros + 4 bytes connectionId
    
    // Fill with zeros for first 8 bytes
    handshakePacket.fill(0, 8);
    
    // Add connectionId in network byte order
    quint32 connectionIdBe = qToBigEndian<quint32>(m_connectionId);
    memcpy(handshakePacket.data() + 8, &connectionIdBe, 4);
    
    // Send to server UDP port
    m_udpManager->sendPacket(handshakePacket, QHostAddress(m_serverHost), m_serverUdpPort);
    
    if (m_handshakeAttempts % 10 == 0) { // Log every 10 attempts
        qDebug() << "NetworkFacade: Sent UDP handshake packet, attempt:" << m_handshakeAttempts;
    }
}

void NetworkFacade::stopUdpHandshake()
{
    if (m_handshakeTimer->isActive()) {
        m_handshakeTimer->stop();
        qDebug() << "Stopped UDP handshake process";
    }
}

// Error/Success handling
void NetworkFacade::onServerErrorReceived(uint8_t originalMessageType, const QString &errorMessage)
{
    qWarning() << "NetworkFacade: SERVER_ERROR for message type" << originalMessageType 
               << "message:" << errorMessage;
    emit serverErrorReceived(originalMessageType, errorMessage);
}

void NetworkFacade::onServerSuccessReceived(uint8_t originalMessageType, const QString &successMessage)
{
    qDebug() << "NetworkFacade: SERVER_SUCCESS for message type" << originalMessageType 
             << "message:" << successMessage;
    emit serverSuccessReceived(originalMessageType, successMessage);
}

// Existing stream message handlers
void NetworkFacade::onServerStreamCreated(uint32_t id)
{
    qDebug() << "NetworkFacade: >>> SERVER_STREAM_CREATED received id=" << id;
    
    // Create NetworkManager for this stream
    NetworkManager *manager = createNetworkManager(static_cast<int>(id));
    if (manager) {
        qDebug() << "NetworkFacade: NetworkManager created for stream" << id;
    } else {
        qWarning() << "NetworkFacade: Failed to create NetworkManager for stream" << id;
    }
    
    qDebug() << "NetworkFacade: Emitting serverStreamCreated signal for id=" << id;
    emit serverStreamCreated(id);
}

void NetworkFacade::onServerStreamDeleted(uint32_t id)
{
    qDebug() << "NetworkFacade: SERVER_STREAM_DELETED id=" << id;
    
    // Remove NetworkManager for this stream
    removeNetworkManager(static_cast<int>(id));
    
    emit serverStreamDeleted(id);
}

void NetworkFacade::onServerStreamJoined(uint32_t id)
{
    qDebug() << "NetworkFacade: SERVER_STREAM_JOINED id=" << id;
    
    // Create NetworkManager for receiving this stream
    NetworkManager *manager = createNetworkManager(static_cast<int>(id));
    if (manager) {
        // NetworkManager is ready to receive video for this stream
        qDebug() << "NetworkFacade: NetworkManager ready to receive stream" << id;
    }
    
    emit serverStreamJoined(id);
}

void NetworkFacade::onServerStreamStart(uint32_t id)
{
    qDebug() << "NetworkFacade: SERVER_STREAM_START id=" << id;
    
    // Find the NetworkManager and ensure it's ready to send
    NetworkManager *manager = getNetworkManager(static_cast<int>(id));
    if (manager) {
        qDebug() << "NetworkFacade: Stream" << id << "can start sending video";
    }
    
    emit serverStreamStart(id);
}

void NetworkFacade::onServerStreamEnd(uint32_t id)
{
    qDebug() << "NetworkFacade: SERVER_STREAM_END id=" << id;
    
    // Note: We don't remove the NetworkManager here, just stop sending
    // The manager will be removed on SERVER_STREAM_DELETED or CLIENT_STREAM_LEAVE
    
    emit serverStreamEnd(id);
}

// Call message handlers
void NetworkFacade::onServerCallCreated(uint32_t callId)
{
    qDebug() << "NetworkFacade: >>> SERVER_CALL_CREATED received callId=" << callId;
    emit serverCallCreated(callId);
}

void NetworkFacade::onServerCallConnJoined(uint32_t callId, const QVector<uint32_t>& participants, const QVector<uint32_t>& streams)
{
    qDebug() << "NetworkFacade: >>> SERVER_CALL_CONN_JOINED callId=" << callId
             << "participants:" << participants.size() << "streams:" << streams.size();
    emit serverCallConnJoined(callId, participants, streams);
}

void NetworkFacade::onServerCallConnNew(uint32_t callId, uint32_t participantId)
{
    qDebug() << "NetworkFacade: >>> SERVER_CALL_CONN_NEW callId=" << callId << "participantId=" << participantId;
    emit serverCallConnNew(callId, participantId);
}

void NetworkFacade::onServerCallConnLeft(uint32_t callId, uint32_t participantId)
{
    qDebug() << "NetworkFacade: >>> SERVER_CALL_CONN_LEFT callId=" << callId << "participantId=" << participantId;
    emit serverCallConnLeft(callId, participantId);
}

void NetworkFacade::onServerCallStreamNew(uint32_t callId, uint32_t streamId)
{
    qDebug() << "NetworkFacade: >>> SERVER_CALL_STREAM_NEW callId=" << callId << "streamId=" << streamId;
    emit serverCallStreamNew(callId, streamId);
}

void NetworkFacade::onServerCallStreamDeleted(uint32_t callId, uint32_t streamId)
{
    qDebug() << "NetworkFacade: >>> SERVER_CALL_STREAM_DELETED callId=" << callId << "streamId=" << streamId;
    emit serverCallStreamDeleted(callId, streamId);
}

void NetworkFacade::onHandshakeStart(uint32_t connectionId)
{
    m_connectionId = connectionId;
    for (auto &manager : m_networkManagers) {
        manager->startHandshake(connectionId);
    }
}

// При получении SERVER_HANDSHAKE_END:
void NetworkFacade::onHandshakeEnd(uint32_t connectionId)
{
    if (connectionId == m_connectionId) {
        for (auto &manager : m_networkManagers) {
            manager->completeHandshake();
        }
    }
}
