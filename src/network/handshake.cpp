#include "handshake.h"
#include "udp/udpmanager.h"
#include <QDebug>
#include <QtEndian>

HandshakeService::HandshakeService(UDPManager *udpManager, QObject *parent)
    : QObject(parent)
    , m_udpManager(udpManager)
    , m_handshakeTimer(new QTimer(this))
{
    connect(m_handshakeTimer, &QTimer::timeout, this, &HandshakeService::sendHandshakePacket);
    m_handshakeTimer->setInterval(HANDSHAKE_INTERVAL_MS);
}

HandshakeService::~HandshakeService()
{
    stopHandshake();
}

void HandshakeService::startHandshake(uint32_t connectionId, const QHostAddress &serverAddress, quint16 serverPort)
{
    if (!m_udpManager) {
        emit handshakeFailed("UDP manager is not available");
        return;
    }

    m_connectionId = connectionId;
    m_serverAddress = serverAddress;
    m_serverPort = serverPort;
    m_handshakeCompleted = false;
    m_attempts = 0;

    qDebug() << "HandshakeService: Starting handshake for connection" << connectionId;
    
    m_handshakeTimer->start();
    emit handshakeStarted(connectionId);
    
    // Send first packet immediately
    sendHandshakePacket();
}

void HandshakeService::stopHandshake()
{
    if (m_handshakeTimer->isActive()) {
        m_handshakeTimer->stop();
        qDebug() << "HandshakeService: Handshake stopped";
    }
}

void HandshakeService::onHandshakeConfirmed()
{
    if (m_handshakeCompleted) {
        return; // Already completed
    }

    m_handshakeCompleted = true;
    stopHandshake();
    
    qDebug() << "HandshakeService: Handshake completed for connection" << m_connectionId;
    emit handshakeCompleted(m_connectionId);
}

void HandshakeService::sendHandshakePacket()
{
    if (!m_udpManager || m_handshakeCompleted) {
        return;
    }

    m_attempts++;
    if (m_attempts > MAX_ATTEMPTS) {
        QString error = QString("Handshake timeout after %1 attempts").arg(MAX_ATTEMPTS);
        qWarning() << "HandshakeService:" << error;
        stopHandshake();
        emit handshakeFailed(error);
        return;
    }

    // Create handshake packet: 8 zero bytes + 4 bytes connectionId
    QByteArray handshakePacket;
    handshakePacket.resize(12);
    
    // Fill first 8 bytes with zeros (without resizing)
    memset(handshakePacket.data(), 0, 8);
    
    // Add connectionId in network byte order
    quint32 connectionIdBe = qToBigEndian<quint32>(m_connectionId);
    memcpy(handshakePacket.data() + 8, &connectionIdBe, 4);
    
    
    // Send to server UDP port
    m_udpManager->sendPacket(handshakePacket, m_serverAddress, m_serverPort);
    
    if (m_attempts % 10 == 0) { // Log every 10 attempts
        qDebug() << "HandshakeService: Sent handshake packet, attempt:" << m_attempts;
    }
}