// networkmanager.cpp
#include "networkmanager.h"
#include "udpmanager.h"
#include <QDebug>

// Временные заглушки для отсутствующих компонентов
class FrameAssembler : public QObject {
    Q_OBJECT
public:
    FrameAssembler(QObject* parent) : QObject(parent) {}
    void processPacket(int streamId, int packetType, const QByteArray& payload) {
        Q_UNUSED(streamId);
        Q_UNUSED(packetType); 
        Q_UNUSED(payload);
        // TODO: Реализовать сборку фреймов
    }
    void cleanupOldAssemblies(int timeout) { Q_UNUSED(timeout); }
signals:
    void frameAssembled(int streamId, int frameNumber, const QByteArray& frameData);
};

class FrameSender : public QObject {
    Q_OBJECT
public:
    FrameSender(QObject* parent) : QObject(parent) {}
    void addFrame(int streamId, int frameNumber, const QByteArray& frameData) {
        Q_UNUSED(streamId);
        Q_UNUSED(frameNumber);
        Q_UNUSED(frameData);
        // TODO: Реализовать отправку фреймов
    }
    bool hasPacketsToSend() { return false; }
    QVector<QPair<int, QByteArray>> takePacketsToSend() { return {}; }
};

NetworkManager::NetworkManager(int streamId, QObject *parent)
    : QObject(parent)
    , m_streamId(streamId)
    , m_cleanupTimer(new QTimer(this))
    , m_statsTimer(new QTimer(this))
    , m_handshakeTimer(new QTimer(this))
    , m_frameAssembler(new FrameAssembler(this))
    , m_frameSender(new FrameSender(this))
{
    connect(m_cleanupTimer, &QTimer::timeout, this, &NetworkManager::cleanupOldAssemblies);
    connect(m_statsTimer, &QTimer::timeout, this, &NetworkManager::printStatistics);
    connect(m_handshakeTimer, &QTimer::timeout, this, &NetworkManager::sendHandshakePacket);
    connect(m_frameAssembler, &FrameAssembler::frameAssembled, this, &NetworkManager::onFrameAssembled);
    
    m_cleanupTimer->setInterval(5000);
    m_statsTimer->setInterval(10000);
    m_handshakeTimer->setInterval(100);
}

NetworkManager::~NetworkManager()
{
    cleanup();
}

bool NetworkManager::initialize(UDPManager *udpManager)
{
    if (m_initialized) {
        return true;
    }
    
    if (!udpManager) {
        qWarning() << "NetworkManager: Null UDP manager provided";
        return false;
    }
    
    m_udpManager = udpManager;
    m_udpManager->registerNetworkManager(m_streamId, this);
    
    m_operationTimer.start();
    m_initialized = true;
    
    qDebug() << "NetworkManager: Initialized for stream" << m_streamId;
    return true;
}

void NetworkManager::cleanup()
{
    m_cleanupTimer->stop();
    m_statsTimer->stop();
    m_handshakeTimer->stop();
    
    if (m_udpManager) {
        m_udpManager->unregisterNetworkManager(m_streamId);
        m_udpManager = nullptr;
    }
    
    m_initialized = false;
}

void NetworkManager::setServerAddress(const QString &address, quint16 port)
{
    m_serverAddress = QHostAddress(address);
    m_serverPort = port;
}

void NetworkManager::sendVideoFrame(int frameNumber, const QByteArray &frameData)
{
    if (!m_initialized || !m_sendingEnabled || !m_udpManager) {
        return;
    }
    
    if (frameData.isEmpty()) {
        qWarning() << "NetworkManager: Attempt to send empty frame";
        return;
    }
    
    // Используем FrameSender из старой реализации
    m_frameSender->addFrame(m_streamId, frameNumber, frameData);
    
    // Отправляем все готовые пакеты
    while (m_frameSender->hasPacketsToSend()) {
        auto packets = m_frameSender->takePacketsToSend();
        for (const auto &packet : packets) {
            sendPacketNewProtocol(packet.second, packet.first);
        }
    }
    
    m_stats.framesSent++;
}

void NetworkManager::processPacket(const QByteArray &data, const QHostAddress &sender, quint16 port)
{
    Q_UNUSED(sender);
    Q_UNUSED(port);
    
    // Используем логику из старой реализации
    processPacketNewProtocol(data);
}

void NetworkManager::start()
{
    m_sendingEnabled = true;
    m_cleanupTimer->start();
    m_statsTimer->start();
    qDebug() << "NetworkManager: Started for stream" << m_streamId;
}

void NetworkManager::stop()
{
    m_sendingEnabled = false;
    m_cleanupTimer->stop();
    m_statsTimer->stop();
    qDebug() << "NetworkManager: Stopped for stream" << m_streamId;
}

void NetworkManager::cleanupOldAssemblies()
{
    // Используем логику из старой реализации
    m_frameAssembler->cleanupOldAssemblies(10000);
    
    // Очищаем старые FEC группы
    QList<int> groupsToRemove;
    for (auto it = m_fecReceiveBuffers.begin(); it != m_fecReceiveBuffers.end(); ++it) {
        int groupId = it.key();
        QVector<bool>& received = m_fecReceived[groupId];
        bool allDataReceived = true;
        for (int i = 0; i < FEC_DATA_PACKETS; ++i) {
            if (!received[i]) {
                allDataReceived = false;
                break;
            }
        }
        
        if (allDataReceived) {
            groupsToRemove.append(groupId);
        }
    }
    
    for (int groupId : groupsToRemove) {
        m_fecReceiveBuffers.remove(groupId);
        m_fecReceived.remove(groupId);
    }
}

void NetworkManager::printStatistics()
{
    double elapsedSeconds = m_operationTimer.elapsed() / 1000.0;
    
    if (elapsedSeconds == 0) return;
    
    double sendRate = (m_stats.totalBytesSent / 1024.0) / elapsedSeconds;
    double receiveRate = (m_stats.totalBytesReceived / 1024.0) / elapsedSeconds;
    
    double lossRate = 0.0;
    if (m_stats.expectedFrames.size() > 0) {
        int lostFrames = m_stats.expectedFrames.size() - m_stats.receivedFrames.size();
        lossRate = (double)lostFrames / m_stats.expectedFrames.size() * 100.0;
    }
    
    QString stats = QString(
        "Stream %1: %2s | Frames: %3 sent, %4 received (%5% loss) | "
        "Packets: %6 sent, %7 received | Rate: %8/%9 KB/s | "
        "FEC: %10 groups sent, %11 recovered"
    ).arg(m_streamId)
     .arg(elapsedSeconds, 0, 'f', 1)
     .arg(m_stats.framesSent)
     .arg(m_stats.framesReceived)
     .arg(lossRate, 0, 'f', 2)
     .arg(m_stats.totalPacketsSent)
     .arg(m_stats.totalPacketsReceived)
     .arg(sendRate, 0, 'f', 2)
     .arg(receiveRate, 0, 'f', 2)
     .arg(m_stats.fecGroupsSent)
     .arg(m_stats.fecGroupsRecovered);

    emit statisticsUpdated(stats);
}

void NetworkManager::onFrameAssembled(int streamId, int frameNumber, const QByteArray &frameData)
{
    m_stats.framesReceived++;
    m_stats.receivedFrames.insert(qMakePair(streamId, frameNumber));
    emit frameAssembled(streamId, frameNumber, frameData);
}

// Остальные методы из старой реализации должны быть перенесены сюда:
// processPacketNewProtocol, sendPacketNewProtocol, processXorPacket, 
// sendXorPackets, calculateXorForGroup, tryRecoverLostPackets и т.д.

void NetworkManager::startHandshake(uint32_t connectionId)
{
    m_connectionId = connectionId;
    m_handshakeCompleted = false;
    m_handshakeTimer->start();
    qDebug() << "NetworkManager: Starting handshake for connection" << connectionId;
}

void NetworkManager::completeHandshake()
{
    m_handshakeCompleted = true;
    m_handshakeTimer->stop();
    qDebug() << "NetworkManager: Handshake completed for stream" << m_streamId;
    emit handshakeCompleted();
}

void NetworkManager::sendHandshakePacket()
{
    if (!m_initialized || !m_udpManager || m_handshakeCompleted || m_connectionId == 0) {
        return;
    }
    
    QByteArray handshakePacket;
    handshakePacket.resize(12);
    handshakePacket.fill(0, 8);
    
    quint32 connectionIdBe = qToBigEndian<quint32>(m_connectionId);
    memcpy(handshakePacket.data() + 8, &connectionIdBe, 4);
    
    m_udpManager->sendPacket(handshakePacket, m_serverAddress, m_serverPort);
}

void NetworkManager::updateSendStats(int packets, int bytes)
{
    m_stats.totalPacketsSent += packets;
    m_stats.totalBytesSent += bytes;
}

void NetworkManager::updateReceiveStats(int packets, int bytes)
{
    m_stats.totalPacketsReceived += packets;
    m_stats.totalBytesReceived += bytes;
}

uint32_t NetworkManager::calculateCRC32(const QByteArray &data)
{
    uLong crc = crc32(0L, Z_NULL, 0);
    if (!data.isEmpty()) {
        crc = crc32(crc, (const Bytef*)data.constData(), data.size());
    }
    return (uint32_t)crc;
}

