#include "networkfacade.h"
#include "tcpmanager.h"
#include <QDebug>
#include <QtEndian>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#endif

NetworkFacade::NetworkFacade(QObject *parent)
    : QObject(parent)
    , m_tcp(new TCPManager(this))
{
    connect(m_tcp, &TCPManager::connected, this, &NetworkFacade::onTcpConnected);
    connect(m_tcp, &TCPManager::disconnected, this, &NetworkFacade::onTcpDisconnected);
    connect(m_tcp, &TCPManager::errorOccurred, this, &NetworkFacade::onTcpError);

    connect(m_tcp, &TCPManager::serverStreamCreated, this, &NetworkFacade::onServerStreamCreated);
    connect(m_tcp, &TCPManager::serverStreamDeleted, this, &NetworkFacade::onServerStreamDeleted);
    connect(m_tcp, &TCPManager::serverStreamJoined, this, &NetworkFacade::onServerStreamJoined);
    connect(m_tcp, &TCPManager::serverStreamStart, this, &NetworkFacade::onServerStreamStart);
    connect(m_tcp, &TCPManager::serverStreamEnd, this, &NetworkFacade::onServerStreamEnd);
}

NetworkFacade::~NetworkFacade()
{
}

void NetworkFacade::setServer(const QString &host, quint16 tcpPort, quint16 udpPort)
{
    m_serverHost = host;
    m_serverTcpPort = tcpPort;
    m_serverUdpPort = udpPort;
    if (m_tcp) m_tcp->setServer(host, tcpPort);
}

void NetworkFacade::setLocalUdpInfo(const QHostAddress &localIp, quint16 localUdpPort)
{
    m_localUdpIp = localIp;
    m_localUdpPort = localUdpPort;
}

bool NetworkFacade::initialize()
{
    // Nothing UDP-specific here — facade is agnostic; initialization of UDP should be done by your UDP manager.
    return true;
}

void NetworkFacade::connectToServer()
{
    if (!m_tcp) return;
    m_tcp->connectToServer();
}

void NetworkFacade::disconnect()
{
    if (m_tcp) m_tcp->disconnectFromServer();
}

void NetworkFacade::sendStreamCreate() { if (m_tcp) m_tcp->sendClientStreamCreate(); }
void NetworkFacade::sendStreamDelete(uint32_t id) { if (m_tcp) m_tcp->sendClientStreamDelete(id); }
void NetworkFacade::sendStreamJoin(uint32_t id) { if (m_tcp) m_tcp->sendClientStreamJoin(id); }
void NetworkFacade::sendStreamLeave(uint32_t id) { if (m_tcp) m_tcp->sendClientStreamLeave(id); }
void NetworkFacade::sendDisconnect() { if (m_tcp) m_tcp->sendClientDisconnect(); }

void NetworkFacade::onTcpConnected()
{
    qDebug() << "NetworkFacade::onTcpConnected — serverHost:" << m_serverHost
         << "tcpPort:" << m_serverTcpPort
         << "localUdpIp:" << m_localUdpIp << "localUdpPort:" << m_localUdpPort;

    // Build sockaddr_in-like 16 bytes in network byte order:
    QByteArray sa; sa.resize(16); sa.fill(0);

    // sin_family uint16_t (AF_INET)
    quint16 family_be = qToBigEndian<quint16>(AF_INET);
    memcpy(sa.data() + 0, &family_be, 2);

    // sin_port uint16_t (local UDP port)
    quint16 port_be = qToBigEndian<quint16>(static_cast<quint16>(m_localUdpPort));
    memcpy(sa.data() + 2, &port_be, 2);

    // sin_addr uint32_t
    quint32 ip_be = qToBigEndian<quint32>(static_cast<quint32>(m_localUdpIp.toIPv4Address()));
    memcpy(sa.data() + 4, &ip_be, 4);
    // remaining 8 bytes are zero.

    if (m_tcp) m_tcp->sendClientUdpAddr(sa);
    
    // NAT punch: emit a signal so existing UDP manager may actually send datagrams
    QByteArray ping(1, 0);
    emit sendUdpDatagram(ping, QHostAddress(m_serverHost), m_serverUdpPort);
    // Slight delay between two quick pings can be useful, but we just emit twice quickly:
    emit sendUdpDatagram(ping, QHostAddress(m_serverHost), m_serverUdpPort);

    emit connected();
}

void NetworkFacade::onTcpDisconnected()
{
    qDebug() << "NetworkFacade: TCP disconnected";
    emit disconnected();
}

void NetworkFacade::onTcpError(const QString &err)
{
    qWarning() << "NetworkFacade: TCP error:" << err;
    emit errorOccurred(err);
}

void NetworkFacade::onServerStreamCreated(uint32_t id) { emit serverStreamCreated(id); }
void NetworkFacade::onServerStreamDeleted(uint32_t id) { emit serverStreamDeleted(id); }
void NetworkFacade::onServerStreamJoined(uint32_t id)  { emit serverStreamJoined(id); }
void NetworkFacade::onServerStreamStart(uint32_t id)   { emit serverStreamStart(id); }
void NetworkFacade::onServerStreamEnd(uint32_t id)     { emit serverStreamEnd(id); }
