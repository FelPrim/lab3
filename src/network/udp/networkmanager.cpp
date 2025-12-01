#include "networkmanager.h"
#include "udpmanager.h"
#include <QDataStream>
#include <QDebug>
#include <QNetworkInterface>
#include <QVariant>
#include "network_packet.h"
#include "../../video_defaults.h"
#include <cstdint>

NetworkManager::NetworkManager(int streamId, QObject *parent)
    : QObject(parent)
    , m_streamId(streamId)
    , m_serverAddress(DEFAULT_ECHO_SERVER_ADDRESS)
    , m_serverPort(DEFAULT_ECHO_SERVER_PORT)
    , m_frameAssembler(new FrameAssembler(this))
    , m_frameSender(new FrameSender(this))
    , m_fecBufferCount(0)
{
    memset(m_fecBuffer, 0, sizeof(m_fecBuffer));
    qDebug() << "NetworkManager created for stream:" << streamId;
    
    connect(m_frameAssembler, &FrameAssembler::frameAssembled,
            this, &NetworkManager::onFrameAssembled); 
}

NetworkManager::~NetworkManager()
{
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

    // Создаем таймеры
    m_cleanupTimer = new QTimer(this);
    m_cleanupTimer->setInterval(5000); // Очистка каждые 5 секунд
    connect(m_cleanupTimer, &QTimer::timeout, this, &NetworkManager::cleanupOldAssemblies);
    
    m_statsTimer = new QTimer(this);
    m_statsTimer->setInterval(5000);
    connect(m_statsTimer, &QTimer::timeout, this, &NetworkManager::printStatistics);
    
    m_operationTimer.start();
    m_initialized = true;
    
    qDebug() << "NetworkManager: Initialized for stream" << m_streamId;
    qDebug() << "Server:" << m_serverAddress.toString() << ":" << m_serverPort;
    
    return true;
}

void NetworkManager::cleanup()
{
    // Отписываемся от UDPManager
    if (m_udpManager) {
        m_udpManager->unregisterNetworkManager(m_streamId);
    }
    
    if (m_cleanupTimer) {
        m_cleanupTimer->stop();
    }
    
    if (m_statsTimer) {
        m_statsTimer->stop();
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

    // Очищаем ВСЕ FEC буферы
    m_fecReceiveBuffers.clear();
    m_fecReceived.clear();
    m_fecGroupTimestamps.clear();
    
    // Очищаем буфер отправки
    m_fecBufferCount = 0;
    memset(m_fecBuffer, 0, sizeof(m_fecBuffer));
    
    // Очищаем отправитель и сборщик
    if (m_frameSender) {
        m_frameSender->clear();
    }
    
    if (m_frameAssembler) {
        // Удаляем ВСЕ сборки (0 - удалить все, независимо от возраста)
        m_frameAssembler->cleanupOldAssemblies(0);
    }
    
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

void NetworkManager::onFrameAssembled(int streamId, int frameNumber, const QByteArray &frameData)
{
    m_stats.framesReceived++;
    m_stats.receivedFrames.insert(qMakePair(streamId, frameNumber));
    
    // Передаем сигнал дальше
    emit frameAssembled(streamId, frameNumber, frameData);
}

void NetworkManager::sendVideoFrame(int frameNumber, const QByteArray &frameData)
{
    if (!m_initialized || !m_udpManager || !m_sendingEnabled) return;
    
    try {
        qDebug() << "jhbfvgsjlkhbgfd";
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

    // ИСПРАВЛЕНО: используем правильный вызов
    NetworkPacket packet = PacketProcessor::createDataPacket(m_callId, m_streamId, 
                                                           customSequence, static_cast<uint8_t>(type), data);
    QByteArray datagram = PacketProcessor::toByteArray(packet);
    m_udpManager->sendPacket(datagram, m_serverAddress, m_serverPort);
    m_packetSequence++;

    m_stats.totalPacketsSent++;
    m_stats.totalBytesSent += data.size();

    // FEC логика: ВСЕГДА сохраняем пакет в буфер
    // ИСПРАВЛЕНО: используем правильный размер заголовка
    QByteArray dataPart = datagram.mid(PacketProcessor::ROUTE_HEADER_SIZE);
    
    if (m_fecBufferCount < 4) {
        memcpy(m_fecBuffer[m_fecBufferCount], dataPart.constData(), 1188);
        m_fecBufferCount++;
    }

    // ВСЕГДА отправляем XOR когда набралось 4 пакета
    if (m_fecBufferCount == 4) {
        uint8_t xorData[1188] = {0};
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 1188; j++) {
                xorData[j] ^= m_fecBuffer[i][j];
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

        qDebug() << "Sent XOR packet for group, sequence:" << xorSequence;

        // Сбрасываем буфер ПОСЛЕ отправки XOR
        m_fecBufferCount = 0;
        memset(m_fecBuffer, 0, sizeof(m_fecBuffer));
    }
}

void NetworkManager::processPacketNewProtocol(const QByteArray& data) {
    if (data.size() < sizeof(NetworkPacket)) {
        qDebug() << "Packet too small:" << data.size();
        return;
    }
    
    // Конвертируем в NetworkPacket
    NetworkPacket packet = PacketProcessor::fromByteArray(data);
    
    // ИСПРАВЛЕНИЕ: Извлекаем callId и проверяем его
    PacketHeader header;
    memcpy(&header, &packet.route, sizeof(PacketHeader));
    cast_from_nbe(header);
    
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
    
    if (packet.isXorPacket()) {
        // Обрабатываем XOR пакет
        processXorPacket(packet);
    } else {
        // Обрабатываем обычный пакет
        const DataPacket* dataPacket = packet.asDataPacket();
        if (dataPacket && PacketProcessor::isValidPacketType(packet)) {    
            QByteArray payload = PacketProcessor::getDataPacketPayload(packet);
            
            qDebug() << "Received data packet - Call:" << packetCallId << "Stream:" << packetStreamId 
                     << "Type:" << dataPacket->type << "Seq:" << header.header.packetSequence
                     << "Size:" << payload.size();
            
            // Передаем в FrameAssembler
            m_frameAssembler->processPacket(m_streamId, 
                                          static_cast<PacketType>(dataPacket->type), 
                                          payload);
            updateReceiveStats(1, payload.size());
            
            // Добавляем в FEC буфер приема
            int groupId = header.header.packetSequence / FEC_GROUP_SIZE;
            int positionInGroup = header.header.packetSequence % FEC_GROUP_SIZE;
            
            if (!m_fecReceiveBuffers.contains(groupId)) {
                m_fecReceiveBuffers[groupId] = QVector<QByteArray>(FEC_TOTAL_PACKETS);
                m_fecReceived[groupId] = QVector<bool>(FEC_TOTAL_PACKETS, false);
            }
            
            // Сохраняем часть после RouteHeader
            QByteArray dataPart = data.mid(PACKET_HEADER_SIZE);
            m_fecReceiveBuffers[groupId][positionInGroup] = dataPart;
            m_fecReceived[groupId][positionInGroup] = true;
            
            // Пытаемся восстановить потерянные пакеты
            tryRecoverLostPackets(groupId);
            if (m_fecReceiveBuffers.size() > MAX_FEC_BUFFER_SIZE) {
                // Удаляем самую старую группу
                int oldestGroup = m_fecGroupTimestamps.key(*std::min_element(
                    m_fecGroupTimestamps.begin(), m_fecGroupTimestamps.end()));
                m_fecReceiveBuffers.remove(oldestGroup);
                m_fecReceived.remove(oldestGroup);
                m_fecGroupTimestamps.remove(oldestGroup);
            }
        }
    }
}

// Остальные методы FEC и статистики копируем без изменений из старого кода

void NetworkManager::processXorPacket(const NetworkPacket& packet) {
    qDebug() << "Received XOR packet - Stream:" << packet.route.streamId 
             << "Seq:" << packet.route.packetSequence;
    
    int groupId = packet.route.packetSequence / FEC_GROUP_SIZE;
    
    if (!m_fecReceiveBuffers.contains(groupId)) {
        m_fecReceiveBuffers[groupId] = QVector<QByteArray>(FEC_TOTAL_PACKETS);
        m_fecReceived[groupId] = QVector<bool>(FEC_TOTAL_PACKETS, false);
    }
    
    // Сохраняем XOR данные
    QByteArray xorData = PacketProcessor::getXorPacketData(packet);
    m_fecReceiveBuffers[groupId][FEC_DATA_PACKETS] = xorData;
    m_fecReceived[groupId][FEC_DATA_PACKETS] = true;
    
    // Пытаемся восстановить потерянные пакеты
    tryRecoverLostPackets(groupId);
}

void NetworkManager::tryRecoverLostPackets(int groupId)
{
    if (!m_fecReceiveBuffers.contains(groupId)) return;
    
    QVector<QByteArray>& packets = m_fecReceiveBuffers[groupId];
    QVector<bool>& received = m_fecReceived[groupId];
    
    // Используем pre-allocated буферы для избежания лишних аллокаций
    static thread_local QByteArray recoveredBuffer;
    recoveredBuffer.resize(1188); // Предварительное выделение
    
    // Проверяем потери
    int lostCount = 0;
    int lostIndex = -1;
    
    for (int i = 0; i < FEC_DATA_PACKETS; ++i) {
        if (!received[i]) {
            lostCount++;
            lostIndex = i;
        }
    }

    // Если потерян ровно один пакет данных и у нас есть XOR пакет, восстанавливаем
    if (lostCount == 1 && received[FEC_DATA_PACKETS] && !packets[FEC_DATA_PACKETS].isEmpty()) {
        qDebug() << "🔧 Recovering lost packet in group:" << groupId << "at position:" << lostIndex;

        // Получаем XOR данные и зануляем FEC_FLAG ПЕРЕД восстановлением
        QByteArray xorData = packets[FEC_DATA_PACKETS];
        if (!xorData.isEmpty()) {
            xorData[0] = xorData[0] & 0x7F; // Зануляем старший бит
        }

        // Восстанавливаем потерянные данные через XOR
        QByteArray recoveredData = xorData;
        
        for (int i = 0; i < FEC_DATA_PACKETS; ++i) {
            if (i != lostIndex && received[i] && !packets[i].isEmpty()) {
                const QByteArray& packetData = packets[i];
                int minSize = qMin(recoveredData.size(), packetData.size());
                for (int j = 0; j < minSize; ++j) {
                    recoveredData[j] = recoveredData[j] ^ packetData[j];
                }
            }
        }

        // Создаем полный восстановленный пакет
        if (!recoveredData.isEmpty()) {
            // Определяем streamId из любого полученного пакета в группе
            int streamId = 0;
            for (int i = 0; i < FEC_TOTAL_PACKETS; ++i) {
                if (received[i] && !packets[i].isEmpty()) {
                    // Создаем временный пакет для извлечения streamId
                    QByteArray fullPacket = packets[i];
                    NetworkPacket tempPacket = PacketProcessor::fromByteArray(fullPacket);
                    //streamId = tempPacket.route.streamId; // TODO: проверить, что это действительно network byte order

                    PacketHeader tempHeader;
                    memcpy(&tempHeader, &tempPacket.route, sizeof(PacketHeader));
                    cast_from_nbe(tempHeader);
                    streamId = tempHeader.header.streamId;

                    break;
                }
            }

            // Вычисляем sequence number восстановленного пакета
            int recoveredSequence = groupId * FEC_GROUP_SIZE + lostIndex;

            // Определяем тип восстановленного пакета из восстановленных данных
            uint8_t recoveredType = recoveredData[0] & 0x7F;

            // Проверяем валидность типа
            if (recoveredType >= START_FRAME && recoveredType <= END_FRAME) {
                // Создаем полный пакет
                QByteArray payload = recoveredData.mid(1); // Пропускаем байт типа
                NetworkPacket recoveredPacket = PacketProcessor::createDataPacket(
                     m_callId, m_streamId, recoveredSequence, recoveredType, payload);

                // Конвертируем в QByteArray для обработки
                QByteArray recoveredDatagram = PacketProcessor::toByteArray(recoveredPacket);
                
                // Вызываем processPacketNewProtocol напрямую с QByteArray
                processPacketNewProtocol(recoveredDatagram);

                // Обновляем статистику
                m_stats.packetsRecoveredByFEC++;
                m_stats.fecGroupsRecovered++;

                // Помечаем пакет как полученный в FEC буфере
                packets[lostIndex] = recoveredDatagram.mid(PACKET_HEADER_SIZE);
                received[lostIndex] = true;

                qDebug() << "✅ Successfully recovered packet - Stream:" << streamId 
                         << "Type:" << recoveredType << "Seq:" << recoveredSequence;
            } else {
                qWarning() << "❌ Invalid packet type in recovered data:" << recoveredType;
            }
        }
    }
}

void NetworkManager::cleanupOldAssemblies()
{
    // 1. Очищаем старые сборки фреймов
    m_frameAssembler->cleanupOldAssemblies(1000); // 1 секунда
    
    // 2. Очищаем старые FEC группы
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    QList<int> groupsToRemove;
    
    for (auto it = m_fecReceiveBuffers.begin(); it != m_fecReceiveBuffers.end(); ++it) {
        int groupId = it.key();
        
        // Находим время создания группы (можно добавить поле creationTime в FEC буфер)
        // Пока удаляем только завершенные группы, как в cleanupCompletedGroups
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
    
    // Удаляем завершенные группы
    for (int groupId : groupsToRemove) {
        m_fecReceiveBuffers.remove(groupId);
        m_fecReceived.remove(groupId);
    }
    
    qDebug() << "🧹 Cleanup: FEC groups" << groupsToRemove.size();
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

     QString memoryInfo = QString(
        "Memory: FEC send=%1, recv=%2 groups | FrameQueue: %3 frames"
    )
     .arg(m_fecReceiveBuffers.size())
     .arg(0); // Можно добавить размер очереди FrameSender если доступно
    
    qDebug().noquote() << stats << "\n" << memoryInfo;
    emit statisticsUpdated(stats);
}

uint32_t NetworkManager::calculateCRC32(const QByteArray &data)
{
    return crc32(0, (const Bytef*)data.constData(), data.size());
}