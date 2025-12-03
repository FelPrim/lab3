#include "networkmanager.h"
#include "udpmanager.h"
#include <QDataStream>
#include <QDebug>
#include <QNetworkInterface>
#include <QVariant>
#include "network_packet.h"
#include "../../video_defaults.h"
#include <cstdint>

void NetworkManager::checkMemory()
{
    static qint64 lastCheck = 0;
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    
    if (now - lastCheck > 1000) {
        lastCheck = now;
        
        // Проверить все NetworkManager
        qDebug()    << "FEC groups:" << m_fecBuffer->getGroupCount()
                    << "Frame groups:" << m_packetBuffer->getFrameCount();
        
    }
}

NetworkManager::NetworkManager(int streamId, QObject *parent)
    : QObject(parent)
    , m_streamId(streamId)
    , m_serverAddress(DEFAULT_ECHO_SERVER_ADDRESS)
    , m_serverPort(DEFAULT_ECHO_SERVER_PORT)
    , m_fecBuffer(new FecBuffer(streamId, this))
    , m_packetBuffer(new PacketGroupBuffer(streamId, this))
    , m_frameSender(new FrameSender(this))
    , m_fecSendBufferCount(0)
{
    memset(m_fecSendBuffer, 0, sizeof(m_fecSendBuffer));
    qDebug() << "NetworkManager created for stream:" << streamId;
    
    // Устанавливаем связь между FecBuffer и PacketGroupBuffer
    m_fecBuffer->setPacketGroupBuffer(m_packetBuffer);
    
    // Подключаем сигналы новых компонентов
    connect(m_fecBuffer, &FecBuffer::packetRecovered,
            this, &NetworkManager::onPacketRecovered);
    
    connect(m_packetBuffer, &PacketGroupBuffer::frameComplete,
            this, &NetworkManager::onFrameComplete);

}

NetworkManager::~NetworkManager()
{
    // Таймеры удалятся автоматически благодаря parent/child механизму Qt
    // Вызываем cleanup() для остановки таймеров и отписки
    cleanup();
    qDebug() << "NetworkManager: Destructor for stream" << m_streamId;
}

bool NetworkManager::initialize(UDPManager *udpManager)
{
    if (m_initialized) {
        return true;
    }

    if (!udpManager) {
        qCritical() << "NetworkManager: UDPManager is null for stream" << m_streamId;
        return false;
    }

    m_udpManager = udpManager;
    m_udpManager->registerNetworkManager(m_streamId, this);

    // Создаем таймеры только если они еще не созданы
    if (!m_cleanupTimer) {
        m_cleanupTimer = new QTimer(this);
        m_cleanupTimer->setInterval(5000); // Очистка каждые 5 секунд
        connect(m_cleanupTimer, &QTimer::timeout, 
                this, &NetworkManager::cleanupOldAssemblies);
    }
    
    if (!m_statsTimer) {
        m_statsTimer = new QTimer(this);
        m_statsTimer->setInterval(5000);
        connect(m_statsTimer, &QTimer::timeout, 
                this, &NetworkManager::printStatistics);
    }
    
    m_operationTimer.start();
    m_initialized = true;
    
    qDebug() << "NetworkManager: Initialized for stream" << m_streamId;
    return true;
}

void NetworkManager::cleanup()
{
    // Останавливаем таймеры
    if (m_cleanupTimer) {
        m_cleanupTimer->stop();
    }
    
    if (m_statsTimer) {
        m_statsTimer->stop();
    }
    
    // Отписываемся от UDPManager
    if (m_udpManager) {
        m_udpManager->unregisterNetworkManager(m_streamId);
        m_udpManager = nullptr; // Не удаляем, только обнуляем
    }
    
    m_initialized = false;
    
    qDebug() << "NetworkManager: Cleaned up for stream" << m_streamId;
}

void NetworkManager::start()
{
    if (!m_initialized && !initialize(m_udpManager)) {
        return;
    }
    
    if (m_cleanupTimer) m_cleanupTimer->start();
    if (m_statsTimer) m_statsTimer->start();
    
    qDebug() << "NetworkManager: Started for stream" << m_streamId;
}

void NetworkManager::stop()
{
    if (m_cleanupTimer) m_cleanupTimer->stop();
    if (m_statsTimer) m_statsTimer->stop();

    // Очищаем буфер отправки
    m_fecSendBufferCount = 0;
    memset(m_fecSendBuffer, 0, sizeof(m_fecSendBuffer));
    
    // Очищаем отправитель
    if (m_frameSender) {
        m_frameSender->clear();
    }
    
    // Очищаем приемные буферы
    m_fecBuffer->cleanup(0);
    m_packetBuffer->cleanupOldFramesByTimeout(0); // Немедленная очистка
    
    qDebug() << "NetworkManager: Stopped for stream" << m_streamId;
}

void NetworkManager::setServerAddress(const QString &address, quint16 port)
{
    m_serverAddress = QHostAddress(address);
    m_serverPort = port;
    qDebug() << "NetworkManager: Server address set to" << address << ":" << port;
}

void NetworkManager::processPacket(const QByteArray &data, const QHostAddress &sender, quint16 port)
{
    Q_UNUSED(sender);
    Q_UNUSED(port);
    processPacketNewProtocol(data);
}

void NetworkManager::onFrameComplete(int streamId, int frameNumber, const QByteArray &frameData)
{
    m_stats.framesReceived++;
    m_stats.receivedFrames.insert(qMakePair(streamId, frameNumber));
    
    // Передаем сигнал дальше
    emit frameAssembled(streamId, frameNumber, frameData);
}

void NetworkManager::onPacketRecovered(uint32_t packetSequence)
{
    m_stats.packetsRecoveredByFEC++;
    m_stats.fecGroupsRecovered++;
    
    qDebug() << "NetworkManager: Packet recovered by FEC, sequence:" << packetSequence;
}

void NetworkManager::sendVideoFrame(int frameNumber, const QByteArray &frameData)
{
    if (!m_initialized || !m_udpManager || !m_sendingEnabled) return;
    
    try {
        qDebug() << "NetworkManager: Sending video frame" << frameNumber << "size:" << frameData.size();
        
        m_frameSender->addFrame(m_streamId, frameNumber, frameData);
        
        while (m_frameSender->hasPacketsToSend()) {
            auto packets = m_frameSender->takePacketsToSend();
            for (const auto &packet : packets) {
                sendPacketNewProtocol(packet.second, packet.first, m_packetSequence);
            }
        }
        
        m_stats.framesSent++;
        
    } catch (const std::exception &e) {
        qDebug() << "Send video frame failed:" << e.what();
        emit errorOccurred(QString("Send video frame failed: %1").arg(e.what()));
    }
}

void NetworkManager::sendPacketNewProtocol(const QByteArray &data, PacketType type, int customSequence)
{
    if (!m_udpManager) return;

    NetworkPacket packet = PacketProcessor::createDataPacket(m_callId, m_streamId, 
                                                           customSequence, static_cast<uint8_t>(type), data);
    QByteArray datagram = PacketProcessor::toByteArray(packet);
    m_udpManager->sendPacket(datagram, m_serverAddress, m_serverPort);
    m_packetSequence++;

    m_stats.totalPacketsSent++;
    m_stats.totalBytesSent += data.size();

    // FEC логика отправки: сохраняем пакет в буфер для отправки XOR
    QByteArray dataPart = datagram.mid(PacketProcessor::ROUTE_HEADER_SIZE);
    
    if (m_fecSendBufferCount < 4) {
        memcpy(m_fecSendBuffer[m_fecSendBufferCount], dataPart.constData(), 1188);
        m_fecSendBufferCount++;
    }

    // Отправляем XOR когда набралось 4 пакета
    if (m_fecSendBufferCount == 4) {
        uint8_t xorData[1188] = {0};
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 1188; j++) {
                xorData[j] ^= m_fecSendBuffer[i][j];
            }
        }
        xorData[0] |= 0x80;

        int xorSequence = m_packetSequence++; // Используем текущий sequence
        
        QByteArray xorDataArray(reinterpret_cast<const char*>(xorData), 1188);
        NetworkPacket xorPacket = PacketProcessor::createXorPacket(m_callId, m_streamId, xorSequence, xorDataArray);
        QByteArray xorDatagram = PacketProcessor::toByteArray(xorPacket);
        m_udpManager->sendPacket(xorDatagram, m_serverAddress, m_serverPort);

        m_stats.totalPacketsSent++;
        m_stats.totalBytesSent += 1188;
        m_stats.fecGroupsSent++;

        qDebug() << "NetworkManager: Sent XOR packet, sequence:" << xorSequence;

        // Сбрасываем буфер ПОСЛЕ отправки XOR
        m_fecSendBufferCount = 0;
        memset(m_fecSendBuffer, 0, sizeof(m_fecSendBuffer));
    }
}

void NetworkManager::processPacketNewProtocol(const QByteArray& data) {
    if (data.size() < sizeof(NetworkPacket)) {
        qDebug() << "NetworkManager: Packet too small:" << data.size();
        return;
    }
    
    // Конвертируем в NetworkPacket
    NetworkPacket packet = PacketProcessor::fromByteArray(data);
    
    // Извлекаем callId и проверяем его
    PacketHeader header;
    memcpy(&header, &packet.route, sizeof(PacketHeader));
    cast_from_nbe(header);  // Используем правильное имя
    
    uint32_t packetCallId = header.header.callId;
    uint32_t packetStreamId = header.header.streamId;
    
    qDebug() << "=== PACKET RECEIVED ===";
    qDebug() << "Expected callId:" << m_callId << "streamId:" << m_streamId;
    qDebug() << "Received callId:" << packetCallId << "streamId:" << packetStreamId;
    qDebug() << "Packet size:" << data.size();

    // Проверяем соответствие callId и streamId
    if (packetCallId != m_callId || packetStreamId != static_cast<uint32_t>(m_streamId)) {
        qDebug() << "NetworkManager: Packet callId/streamId mismatch. Expected:"
                 << m_callId << "/" << m_streamId << "Got:" << packetCallId << "/" << packetStreamId;
        return;
    }
    
    // Передаем пакет в FEC-буфер (слой 3)
    m_fecBuffer->addPacket(packet);
    
    // Обновляем статистику
    updateReceiveStats(1, data.size());
}

void NetworkManager::cleanupOldAssemblies()
{
    // Очищаем все три слоя с таймаутом 1 секунда
    m_fecBuffer->cleanup(1000);
        m_packetBuffer->cleanupOldFramesByTimeout(1000); // НОВАЯ СТРОКА
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

void NetworkManager::printStatistics()
{
    double elapsedSeconds = m_operationTimer.elapsed() / 1000.0;
    
    if (elapsedSeconds == 0) return;
    
    double sendRate = (m_stats.totalBytesSent / 1024.0) / elapsedSeconds;
    double receiveRate = (m_stats.totalBytesReceived / 1024.0) / elapsedSeconds;
    
    // Расчет потерь
    double lossRate = 0.0;
    if (m_stats.expectedFrames.size() > 0) {
        int lostFrames = m_stats.expectedFrames.size() - m_stats.receivedFrames.size();
        lossRate = (double)lostFrames / m_stats.expectedFrames.size() * 100.0;
    }
    
    checkMemory();
    QString stats = QString(
        "=== New Protocol Statistics (Stream: %1) ===\n"
        "Time: %2s | Frames: %3 sent, %4 received (%5% loss)\n"
        "Packets: %6 sent, %7 received | Data Rate: %8/%9 KB/s\n"
        "FEC: %10 groups sent, %11 recovered, %12 packets recovered\n"
        "================================="
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
     .arg(m_stats.fecGroupsRecovered)
     .arg(m_stats.packetsRecoveredByFEC);

    qDebug().noquote() << stats;
    emit statisticsUpdated(stats);
}

uint32_t NetworkManager::calculateCRC32(const QByteArray &data)
{
    return crc32(0, (const Bytef*)data.constData(), data.size());
}