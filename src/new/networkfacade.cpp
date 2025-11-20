#include "networkfacade.h"
#include "networkmanager.h"
#include "streamidconverter.h"
#include <QDebug>
#include <QtEndian>
#include <QNetworkInterface> 

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

NetworkFacade::NetworkFacade(QObject *parent)
    : QObject(parent)
    , m_tcp(new TCPManager(this))
    , m_udpManager(new UDPManager(this))
{
    // TCP connections
    connect(m_tcp, &TCPManager::connected, this, &NetworkFacade::onTcpConnected);
    connect(m_tcp, &TCPManager::disconnected, this, &NetworkFacade::onTcpDisconnected);
    connect(m_tcp, &TCPManager::errorOccurred, this, &NetworkFacade::onTcpError);

    // Server message handlers
    connect(m_tcp, &TCPManager::serverStreamCreated, this, &NetworkFacade::onServerStreamCreated);
    connect(m_tcp, &TCPManager::serverStreamDeleted, this, &NetworkFacade::onServerStreamDeleted);
    connect(m_tcp, &TCPManager::serverStreamJoined, this, &NetworkFacade::onServerStreamJoined);
    connect(m_tcp, &TCPManager::serverStreamStart, this, &NetworkFacade::onServerStreamStart);
    connect(m_tcp, &TCPManager::serverStreamEnd, this, &NetworkFacade::onServerStreamEnd);

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
    m_tcp->connectToServer();
}

void NetworkFacade::disconnect()
{
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

void NetworkFacade::sendStreamCreate() { 
    if (m_tcp) m_tcp->sendClientStreamCreate(); 
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

void NetworkFacade::sendDisconnect() { 
    if (m_tcp) m_tcp->sendClientDisconnect(); 
}

void NetworkFacade::onTcpConnected()
{
    qDebug() << "NetworkFacade::onTcpConnected — serverHost:" << m_serverHost
         << "tcpPort:" << m_serverTcpPort
         << "localUdpIp:" << m_localUdpIp << "localUdpPort:" << m_localUdpPort;

    // ОПРЕДЕЛЯЕМ РЕАЛЬНЫЙ ЛОКАЛЬНЫЙ IP ДЛЯ UDP
    if (m_localUdpIp.isNull() || m_localUdpIp == QHostAddress::Any) {
        // Способ 1: Используем IP из TCP соединения (чаще всего правильный)
        QHostAddress tcpLocalAddress = m_tcp->m_socket->localAddress();
        if (!tcpLocalAddress.isNull() && tcpLocalAddress.protocol() == QAbstractSocket::IPv4Protocol) {
            m_localUdpIp = tcpLocalAddress;
            qDebug() << "Using TCP local address for UDP:" << m_localUdpIp.toString();
        } else {
            // Способ 2: Ищем первый не-loopback IPv4 адрес
            QList<QHostAddress> addresses = QNetworkInterface::allAddresses();
            for (const QHostAddress &address : addresses) {
                if (address.protocol() == QAbstractSocket::IPv4Protocol && 
                    address != QHostAddress::LocalHost) {
                    m_localUdpIp = address;
                    qDebug() << "Found alternative local address:" << m_localUdpIp.toString();
                    break;
                }
            }
            // Если ничего не нашли, используем LocalHost
            if (m_localUdpIp.isNull()) {
                m_localUdpIp = QHostAddress::LocalHost;
                qDebug() << "Using LocalHost as fallback";
            }
        }
    }

    // Убеждаемся, что порт установлен
    if (m_localUdpPort == 0) {
        m_localUdpPort = m_udpManager->getLocalPort();
        qDebug() << "Using UDP manager port:" << m_localUdpPort;
    }

    // Build sockaddr_in-like 16 bytes in network byte order
    QByteArray sa; 
    sa.resize(16); 
    sa.fill(0);

    // sin_family uint16_t (AF_INET)
    quint16 family_be = qToBigEndian<quint16>(AF_INET);
    memcpy(sa.data() + 0, &family_be, 2);

    // sin_port uint16_t (local UDP port)
    quint16 port_be = qToBigEndian<quint16>(static_cast<quint16>(m_localUdpPort));
    memcpy(sa.data() + 2, &port_be, 2);

    // sin_addr uint32_t
    quint32 ip_be = qToBigEndian<quint32>(static_cast<quint32>(m_localUdpIp.toIPv4Address()));
    memcpy(sa.data() + 4, &ip_be, 4);
    // remaining 8 bytes are zero

    // ОТЛАДОЧНЫЙ ВЫВОД
    qDebug() << "Sending UDP address to server: IP:" << m_localUdpIp.toString() 
             << "Port:" << m_localUdpPort
             << "Raw IP bytes:" << QString::number(qFromBigEndian(ip_be), 16); // ИСПРАВЛЕНО

    // Send UDP address to server
    if (m_tcp) {
        m_tcp->sendClientUdpAddr(sa);
    }
    
    // NAT punch: send ping packets to open NAT
    QByteArray ping(1, 0);
    for (int i = 0; i < 2; ++i) {
        m_udpManager->sendPacket(ping, QHostAddress(m_serverHost), m_serverUdpPort);
    }

    emit connected();
}

void NetworkFacade::onTcpDisconnected()
{
    qDebug() << "NetworkFacade: TCP disconnected";
    
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
