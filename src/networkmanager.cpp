#include "networkmanager.h"
#include <QDataStream>
#include <QDebug>
#include <QNetworkInterface>
#include <QVariant>

NetworkManager::NetworkManager(QObject *parent)
    : QObject(parent)
    , m_serverAddress(DEFAULT_ECHO_SERVER_ADDRESS)
    , m_serverPort(DEFAULT_ECHO_SERVER_PORT)
    , m_localPort(0)
{
    m_xorFEC = new SimpleFEC(this);
    connect(m_xorFEC, &SimpleFEC::groupDecoded, this, &NetworkManager::onXORFECGroupDecoded);
    qDebug() << "NetworkManager: Constructor with XOR FEC";
}

NetworkManager::~NetworkManager()
{
    cleanup();
    qDebug() << "NetworkManager: Destructor";
}

bool NetworkManager::initialize()
{
    if (m_initialized) {
        return true;
    }

    try {
        setupSocket();
        
        // Создаем таймеры
        m_cleanupTimer = new QTimer(this);
        m_cleanupTimer->setInterval(5000);
        connect(m_cleanupTimer, &QTimer::timeout, this, &NetworkManager::cleanupOldAssemblies);
        
        m_statsTimer = new QTimer(this);
        m_statsTimer->setInterval(5000);
        connect(m_statsTimer, &QTimer::timeout, this, &NetworkManager::printStatistics);
        
        m_operationTimer.start();
        m_initialized = true;
        
        qDebug() << "NetworkManager: Initialized successfully with XOR FEC";
        qDebug() << "Server:" << m_serverAddress.toString() << ":" << m_serverPort;
        qDebug() << "Local port:" << m_localPort;
        
        return true;
        
    } catch (const std::exception &e) {
        emit errorOccurred(QString("NetworkManager initialization failed: %1").arg(e.what()));
        return false;
    }
}

void NetworkManager::cleanup()
{
    if (m_cleanupTimer) {
        m_cleanupTimer->stop();
        delete m_cleanupTimer;
        m_cleanupTimer = nullptr;
    }
    
    if (m_statsTimer) {
        m_statsTimer->stop();
        delete m_statsTimer;
        m_statsTimer = nullptr;
    }
    
    if (m_udpSocket) {
        m_udpSocket->close();
        delete m_udpSocket;
        m_udpSocket = nullptr;
    }
    
    m_streamAssemblies.clear();
    m_sendBuffer.clear();
    m_currentDataPackets.clear();
    m_initialized = false;
    
    qDebug() << "NetworkManager: Cleaned up";
}

void NetworkManager::setupSocket()
{
    if (m_udpSocket) {
        m_udpSocket->close();
        delete m_udpSocket;
    }
    
    m_udpSocket = new QUdpSocket(this);
    m_udpSocket->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, QVariant(1024 * 1024));
    m_udpSocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, QVariant(1024 * 1024));
    
    m_localPort = DEFAULT_UDP_CLIENT_PORT;
    
    if (!m_udpSocket->bind(QHostAddress::Any, m_localPort)) {
        QString error = QString("Failed to bind UDP socket to port %1: %2")
                          .arg(m_localPort).arg(m_udpSocket->errorString());
        qCritical() << error;
        
        if (!m_udpSocket->bind(QHostAddress::Any, 0)) {
            emit errorOccurred(QString("Failed to bind UDP socket to any port: %1")
                              .arg(m_udpSocket->errorString()));
            return;
        }
        m_localPort = m_udpSocket->localPort();
        qWarning() << "Falling back to port:" << m_localPort;
    }
    
    connect(m_udpSocket, &QUdpSocket::readyRead, this, &NetworkManager::onPacketReceived);
    
    connect(m_udpSocket, &QUdpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError error) {
        qWarning() << "UDP Socket error:" << error << m_udpSocket->errorString();
    });
    
    qDebug() << "NetworkManager: Socket bound to port" << m_localPort;
}

void NetworkManager::start()
{
    if (!m_initialized && !initialize()) {
        return;
    }
    
    if (m_cleanupTimer) m_cleanupTimer->start();
    if (m_statsTimer) m_statsTimer->start();
    
    qDebug() << "NetworkManager: Started";
}

void NetworkManager::stop()
{
    if (m_cleanupTimer) m_cleanupTimer->stop();
    if (m_statsTimer) m_statsTimer->stop();
    
    qDebug() << "NetworkManager: Stopped";
}

void NetworkManager::setServerAddress(const QString &address, quint16 port)
{
    m_serverAddress = QHostAddress(address);
    m_serverPort = port;
    qDebug() << "NetworkManager: Server address set to" << address << ":" << port;
}

void NetworkManager::onPacketReceived()
{
    if (!m_udpSocket) return;
    
    while (m_udpSocket->hasPendingDatagrams()) {
        try {
            QNetworkDatagram datagram = m_udpSocket->receiveDatagram();
            
            if (!datagram.isValid()) {
                qWarning() << "Received invalid datagram";
                continue;
            }
            
            processPacketNewProtocol(datagram);
            
        } catch (const std::exception &e) {
            qCritical() << "Exception in onPacketReceived:" << e.what();
        }
    }
}

void NetworkManager::processPacketNewProtocol(const QNetworkDatagram &datagram)
{
    QByteArray data = datagram.data();
    if (data.size() < PACKET_HEADER_SIZE) return;
    
    try {
        QDataStream stream(data);
        int streamId, packetSequence;
        quint8 packetType;
        
        stream >> streamId >> packetSequence >> packetType;
        
        // Пропускаем резервные байты
        for (int i = 0; i < 3; ++i) {
            quint8 dummy;
            stream >> dummy;
        }
        
        QByteArray payload = data.mid(PACKET_HEADER_SIZE);
        
        // Вычисляем groupId и packetIndex из packetSequence
        int groupId = packetSequence / XOR_FEC_N;
        int packetIndex = packetSequence % XOR_FEC_N;
        
        if (packetType == XOR_FEC_PACKET) {
            processXORFECPacket(streamId, groupId, payload);
        } else {
            // Обработка обычных пакетов
            switch (packetType) {
            case START_FRAME:
                handleStartFrame(streamId, payload);
                break;
            case CONTINUE_FRAME:
                handleContinueFrame(streamId, payload);
                break;
            case BOUNDARY_FRAME:
                handleBoundaryFrame(streamId, payload);
                break;
            case FAILED_BOUNDARY_FRAME:
                handleFailedBoundaryFrame(streamId, payload);
                break;
            default:
                qDebug() << "Unknown packet type:" << packetType;
                break;
            }
            
            // Добавляем data пакет в XOR FEC
            if (packetIndex < XOR_FEC_K) {
                m_xorFEC->addPacket(streamId, groupId, packetIndex, data);
            }
        }
        
        updateReceiveStats(1, payload.size());
        
    } catch (const std::exception &e) {
        qDebug() << "Exception processing packet:" << e.what();
    }
}

void NetworkManager::processXORFECPacket(int streamId, int groupId, const QByteArray &data)
{
    // Добавляем XOR пакет в декодер
    m_xorFEC->addPacket(streamId, groupId, XOR_FEC_K, data);
}

void NetworkManager::onXORFECGroupDecoded(int streamId, int groupId, const QVector<QByteArray> &packets)
{
    qDebug() << "XOR FEC: Group decoded - Stream:" << streamId << "Group:" << groupId;
    m_stats.xorGroupsRecovered++;
    
    // Обрабатываем каждый восстановленный пакет
    for (const QByteArray &packet : packets) {
        processRecoveredPacket(packet);
        m_stats.packetsRecoveredByXOR++;
    }
}

void NetworkManager::processRecoveredPacket(const QByteArray &packetData)
{
    // Восстановленный пакет имеет тот же формат, что и обычный пакет
    if (packetData.size() < PACKET_HEADER_SIZE) return;
    
    QDataStream stream(packetData);
    int streamId, packetSequence;
    quint8 packetType;
    
    stream >> streamId >> packetSequence >> packetType;
    
    // Пропускаем резервные байты
    for (int i = 0; i < 3; ++i) {
        quint8 dummy;
        stream >> dummy;
    }
    
    QByteArray payload = packetData.mid(PACKET_HEADER_SIZE);
    
    // Обрабатываем восстановленный пакет по его типу
    switch (packetType) {
    case START_FRAME:
        handleStartFrame(streamId, payload);
        break;
    case CONTINUE_FRAME:
        handleContinueFrame(streamId, payload);
        break;
    case BOUNDARY_FRAME:
        handleBoundaryFrame(streamId, payload);
        break;
    case FAILED_BOUNDARY_FRAME:
        handleFailedBoundaryFrame(streamId, payload);
        break;
    default:
        break;
    }
    
    qDebug() << "Processed XOR recovered packet - Type:" << packetType << "Size:" << payload.size();
}

void NetworkManager::sendPacketNewProtocol(const QByteArray &data, int streamId, PacketType type)
{
    sendPacketNewProtocol(data, streamId, type, m_packetSequence++);
}

void NetworkManager::sendPacketNewProtocol(const QByteArray &data, int streamId, PacketType type, int customSequence)
{
    QByteArray datagram;
    QDataStream stream(&datagram, QIODevice::WriteOnly);
    
    stream << streamId << customSequence << (quint8)type;
    
    // Резервные байты (3 байта)
    for (int i = 0; i < 3; ++i) {
        stream << (quint8)0;
    }
    
    stream.writeRawData(data.constData(), data.size());
    
    qint64 sent = m_udpSocket->writeDatagram(datagram, m_serverAddress, m_serverPort);
    
    if (sent == -1) {
        qDebug() << "Failed to send packet:" << m_udpSocket->errorString();
    } else {
        m_stats.totalPacketsSent++;
        m_stats.totalBytesSent += data.size();
        
        // Сохраняем data пакеты для XOR FEC
        if (type != XOR_FEC_PACKET) {
            m_currentDataPackets.append(datagram);
        }
    }
}

void NetworkManager::sendBufferedData()
{
    if (m_sendBuffer.isEmpty() || !m_udpSocket) return;
    
    int bytesSent = 0;
    bool isFirstPacket = true;
    m_currentGroupStartSequence = m_packetSequence;
    
    while (bytesSent < m_sendBuffer.size()) {
        int remaining = m_sendBuffer.size() - bytesSent;
        int chunkSize;
        QByteArray packetData;
        
        if (isFirstPacket) {
            chunkSize = qMin(MAX_PAYLOAD_SIZE, remaining);
            packetData = m_sendBuffer.mid(bytesSent, chunkSize);
            sendPacketNewProtocol(packetData, m_streamId, START_FRAME);
            isFirstPacket = false;
        } else {
            chunkSize = qMin(MAX_PAYLOAD_SIZE - 4, remaining);
            packetData = m_sendBuffer.mid(bytesSent, chunkSize);
            QByteArray continueData;
            QDataStream stream(&continueData, QIODevice::WriteOnly);
            stream << m_currentFrameNumber;
            stream.writeRawData(packetData.constData(), packetData.size());
            sendPacketNewProtocol(continueData, m_streamId, CONTINUE_FRAME);
        }
        
        bytesSent += chunkSize;
        
        // Когда накопили XOR_FEC_K пакетов, отправляем XOR FEC
        if (m_currentDataPackets.size() >= XOR_FEC_K) {
            sendXORFECGroup(m_streamId, m_currentGroupStartSequence);
            m_currentDataPackets.clear();
            m_currentGroupStartSequence = m_packetSequence;
        }
    }
    
    m_sendBuffer.clear();
}

void NetworkManager::sendXORFECGroup(int streamId, int groupStartSequence)
{
    if (m_currentDataPackets.size() != XOR_FEC_K) return;
    
    // Генерируем XOR пакет
    QByteArray xorPacket = m_xorFEC->encodeXORGroup(m_currentDataPackets);
    if (xorPacket.isEmpty()) {
        qWarning() << "Failed to generate XOR FEC packet";
        return;
    }
    
    // Отправляем XOR пакет
    int xorSequence = groupStartSequence + XOR_FEC_K;
    sendPacketNewProtocol(xorPacket, streamId, XOR_FEC_PACKET, xorSequence);
    
    m_stats.xorGroupsSent++;
    qDebug() << "Sent XOR FEC group - Stream:" << streamId << "StartSeq:" << groupStartSequence;
}

// Остальные методы остаются без изменений...
void NetworkManager::handleStartFrame(int streamId, const QByteArray& data)
{
    if (data.size() < FRAME_HEADER_SIZE) return;
    
    QDataStream stream(data);
    int frameNumber, frameSize;
    stream >> frameNumber >> frameSize;
    
    QByteArray frameData = data.mid(FRAME_HEADER_SIZE);
    
    // Создаем или обновляем сборку
    if (!m_streamAssemblies.contains(streamId)) {
        m_streamAssemblies[streamId] = StreamAssembly(streamId, frameNumber);
    } else {
        m_streamAssemblies[streamId] = StreamAssembly(streamId, frameNumber);
    }
    
    StreamAssembly& assembly = m_streamAssemblies[streamId];
    assembly.totalSize = frameSize;
    assembly.data = frameData;
    assembly.receivedSize = frameData.size();
    assembly.hasStartFrame = true;
    
    // Проверяем завершенность
    if (assembly.isComplete()) {
        processCompleteFrame(streamId);
    }
}

void NetworkManager::handleContinueFrame(int streamId, const QByteArray& data)
{
    if (!m_streamAssemblies.contains(streamId)) return;
    
    StreamAssembly& assembly = m_streamAssemblies[streamId];
    
    if (data.size() < 4) return;
    
    QDataStream stream(data);
    int frameNumber;
    stream >> frameNumber;
    
    if (frameNumber != assembly.frameNumber) return;
    
    QByteArray frameData = data.mid(4);
    assembly.data.append(frameData);
    assembly.receivedSize += frameData.size();
    
    if (assembly.isComplete()) {
        processCompleteFrame(streamId);
    }
}

void NetworkManager::handleBoundaryFrame(int streamId, const QByteArray& data)
{
    if (m_streamAssemblies.contains(streamId)) {
        StreamAssembly& prevAssembly = m_streamAssemblies[streamId];
        if (prevAssembly.isComplete()) {
            processCompleteFrame(streamId);
        }
    }
    
    handleStartFrame(streamId, data);
}

void NetworkManager::handleFailedBoundaryFrame(int streamId, const QByteArray& data)
{
    handleContinueFrame(streamId, data);
}

void NetworkManager::sendVideoFrame(int streamId, int frameNumber, const QByteArray &frameData)
{
    if (!m_initialized || !m_udpSocket) {
        return;
    }
    
    try {
        m_streamId = streamId;
        m_currentFrameNumber = frameNumber;
        
        QByteArray framedData;
        QDataStream stream(&framedData, QIODevice::WriteOnly);
        stream << frameNumber << (int)frameData.size();
        stream.writeRawData(frameData.constData(), frameData.size());
        
        m_sendBuffer.append(framedData);
        
        sendBufferedData();
        
        updateSendStats(1, frameData.size());
        m_stats.framesSent++;
        m_stats.expectedFrames.insert(qMakePair(streamId, frameNumber));
        
    } catch (const std::exception &e) {
        qDebug() << "Send video frame failed:" << e.what();
        emit errorOccurred(QString("Send video frame failed: %1").arg(e.what()));
    }
}

void NetworkManager::cleanupOldAssemblies()
{
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    int cleanupThreshold = 10000;
    
    auto it = m_streamAssemblies.begin();
    while (it != m_streamAssemblies.end()) {
        if (currentTime - it->creationTime > cleanupThreshold) {
            qDebug() << "Cleaning up old assembly - Stream:" << it->streamId 
                     << "Frame:" << it->frameNumber;
            m_stats.assembliesDropped++;
            it = m_streamAssemblies.erase(it);
        } else {
            ++it;
        }
    }
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
    
    double lossRate = 0.0;
    if (m_stats.expectedFrames.size() > 0) {
        int lostFrames = m_stats.expectedFrames.size() - m_stats.receivedFrames.size();
        lossRate = (double)lostFrames / m_stats.expectedFrames.size() * 100.0;
    }
    
    QString stats = QString(
        "=== XOR FEC Protocol Statistics ===\n"
        "Time: %1s | Frames: %2 sent, %3 received (%4% loss)\n"
        "Packets: %5 sent, %6 received | Data Rate: %7/%8 KB/s\n"
        "XOR FEC: %9 groups sent, %10 recovered, %11 packets recovered\n"
        "Send Buffer: %12 bytes | Active Assemblies: %13\n"
        "================================="
    ).arg(elapsedSeconds, 0, 'f', 1)
     .arg(m_stats.framesSent)
     .arg(m_stats.framesReceived)
     .arg(lossRate, 0, 'f', 2)
     .arg(m_stats.totalPacketsSent)
     .arg(m_stats.totalPacketsReceived)
     .arg(sendRate, 0, 'f', 2)
     .arg(receiveRate, 0, 'f', 2)
     .arg(m_stats.xorGroupsSent)
     .arg(m_stats.xorGroupsRecovered)
     .arg(m_stats.packetsRecoveredByXOR)
     .arg(m_sendBuffer.size())
     .arg(m_streamAssemblies.size());
    
    qDebug().noquote() << stats;
    emit statisticsUpdated(stats);
}

uint32_t NetworkManager::calculateCRC32(const QByteArray &data)
{
    return crc32(0, (const Bytef*)data.constData(), data.size());
}

void NetworkManager::processCompleteFrame(int streamId)
{
    StreamAssembly& assembly = m_streamAssemblies[streamId];
    
    if (assembly.data.size() >= assembly.totalSize) {
        QByteArray completeData = assembly.data.left(assembly.totalSize);
        
        emit frameAssembled(streamId, assembly.frameNumber, completeData);
        
        m_stats.framesReceived++;
        m_stats.assembliesCompleted++;
        m_stats.receivedFrames.insert(qMakePair(streamId, assembly.frameNumber));
        
        qDebug() << "Frame" << assembly.frameNumber << "successfully assembled, size:" << completeData.size() << "bytes";
    }
    
    m_streamAssemblies.remove(streamId);
}
