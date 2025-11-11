#include "networkmanager.h"
#include <QDataStream>
#include <QDebug>
#include <QNetworkInterface>
#include <QVariant>
#include "network_packet.h"
#include "video_defaults.h"

#define FEC_FLAG 0x80
#define PACKET_TYPE_MASK 0x7F

NetworkManager::NetworkManager(QObject *parent)
    : QObject(parent)
    , m_serverAddress(DEFAULT_ECHO_SERVER_ADDRESS)
    , m_serverPort(DEFAULT_ECHO_SERVER_PORT)
    , m_localPort(0)
    , m_frameAssembler(new FrameAssembler(this))
    , m_frameSender(new FrameSender(this))
{
    qDebug() << "NetworkManager: Constructor";
    
    connect(m_frameAssembler, &FrameAssembler::frameAssembled,
            this, &NetworkManager::onFrameAssembled); 
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
        m_cleanupTimer->setInterval(5000); // Очистка каждые 5 секунд
        connect(m_cleanupTimer, &QTimer::timeout, this, &NetworkManager::cleanupOldAssemblies);
        
        m_statsTimer = new QTimer(this);
        m_statsTimer->setInterval(5000);
        connect(m_statsTimer, &QTimer::timeout, this, &NetworkManager::printStatistics);
        
        m_operationTimer.start();
        m_initialized = true;
        
        qDebug() << "NetworkManager: Initialized successfully";
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
    // Увеличиваем размеры буферов
    m_udpSocket->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, QVariant(1024 * 1024));
    m_udpSocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, QVariant(1024 * 1024));
    
    // Используем фиксированный порт из video_defaults
    m_localPort = DEFAULT_UDP_CLIENT_PORT;
    
    // Пробуем привязаться к фиксированному порту
    if (!m_udpSocket->bind(QHostAddress::Any, m_localPort)) {
        QString error = QString("Failed to bind UDP socket to port %1: %2")
                          .arg(m_localPort).arg(m_udpSocket->errorString());
        qCritical() << error;
        
        // Fallback: пробуем любой доступный порт
        if (!m_udpSocket->bind(QHostAddress::Any, 0)) {
            emit errorOccurred(QString("Failed to bind UDP socket to any port: %1")
                              .arg(m_udpSocket->errorString()));
            return;
        }
        m_localPort = m_udpSocket->localPort();
        qWarning() << "Falling back to port:" << m_localPort;
    }
    
    connect(m_udpSocket, &QUdpSocket::readyRead, this, &NetworkManager::onPacketReceived);
    
    // Обработчики ошибок
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

void NetworkManager::onFrameAssembled(int streamId, int frameNumber, const QByteArray &frameData)
{
    m_stats.framesReceived++;
    m_stats.receivedFrames.insert(qMakePair(streamId, frameNumber));
    
    // Передаем сигнал дальше
    emit frameAssembled(streamId, frameNumber, frameData);
}



void NetworkManager::sendVideoFrame(int streamId, int frameNumber, const QByteArray &frameData)
{
    if (!m_initialized || !m_udpSocket) {
        return;
    }
    
    try {
        m_streamId = streamId;
        m_frameSender->addFrame(streamId, frameNumber, frameData);
        
        // Отправляем все готовые пакеты сразу
        while (m_frameSender->hasPacketsToSend()) {
            auto packets = m_frameSender->takePacketsToSend();
            for (const auto &packet : packets) {
                sendPacketNewProtocol(packet.second, streamId, packet.first);
            }
        }
        
        m_stats.framesSent++;
        m_stats.expectedFrames.insert(qMakePair(streamId, frameNumber));
        
    } catch (const std::exception &e) {
        qDebug() << "Send video frame failed:" << e.what();
        emit errorOccurred(QString("Send video frame failed: %1").arg(e.what()));
    }
}













// Обновим processPacketNewProtocol
void NetworkManager::processPacketNewProtocol(const QNetworkDatagram& datagram) {
    QByteArray data = datagram.data();
    if (data.size() < sizeof(NetworkPacket)) {
        qDebug() << "Packet too small:" << data.size();
        return;
    }
    
    // Конвертируем в NetworkPacket
    NetworkPacket packet = PacketProcessor::fromByteArray(data);
    
    if (packet.isXorPacket()) {
        // Обрабатываем XOR пакет
        processXorPacket(packet);
    } else {
        // Обрабатываем обычный пакет
        const DataPacket* dataPacket = packet.asDataPacket();
        if (dataPacket && PacketProcessor::isValidPacketType(packet)) {    
	    QByteArray payload = PacketProcessor::getDataPacketPayload(packet);
            
            qDebug() << "Received data packet - Stream:" << packet.route.streamId 
                     << "Type:" << dataPacket->type 
                     << "Seq:" << packet.route.packetSequence
                     << "Size:" << payload.size();
            
            // Передаем в FrameAssembler
            m_frameAssembler->processPacket(packet.route.streamId, 
                                          static_cast<PacketType>(dataPacket->type), 
                                          payload);
            updateReceiveStats(1, payload.size());
            
            // Добавляем в FEC буфер приема
            int groupId = packet.route.packetSequence / FEC_GROUP_SIZE;
            int positionInGroup = packet.route.packetSequence % FEC_GROUP_SIZE;
            
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
        }
    }
}

void NetworkManager::sendXorPackets()
{
    auto it = m_fecSendBuffers.begin();
    while (it != m_fecSendBuffers.end()) {
        int groupId = it.key();
        QVector<QByteArray>& packets = it.value();
        
        // Проверяем, что у нас есть все 4 пакета данных
        bool hasAllDataPackets = true;
        for (int i = 0; i < FEC_DATA_PACKETS; ++i) {
            if (packets[i].isEmpty()) {
                hasAllDataPackets = false;
                break;
            }
        }
        
        if (hasAllDataPackets) {
            // Вычисляем XOR
            QByteArray xorData = calculateXorForGroup(groupId);
            
            // Создаем и отправляем XOR пакет
            int xorSequence = groupId * FEC_GROUP_SIZE + FEC_DATA_PACKETS;
            NetworkPacket xorPacket = PacketProcessor::createXorPacket(m_streamId, xorSequence, xorData);
            
            QByteArray datagram = PacketProcessor::toByteArray(xorPacket);
            qint64 sent = m_udpSocket->writeDatagram(datagram, m_serverAddress, m_serverPort);
            
            if (sent != -1) {
                m_stats.totalPacketsSent++;
                m_stats.totalBytesSent += xorData.size();
                m_stats.fecGroupsSent++;
                qDebug() << "Sent XOR packet for group:" << groupId;
            }
            
            it = m_fecSendBuffers.erase(it);
        } else {
            ++it;
        }
    }
}



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

// Удаляем старую версию sendPacketNewProtocol и заменяем на:
void NetworkManager::sendPacketNewProtocol(const QByteArray &data, int streamId, PacketType type, int customSequence)
{
    // Создаем DataPacket
    NetworkPacket packet = PacketProcessor::createDataPacket(streamId, customSequence, 
                                                           static_cast<uint8_t>(type), data);
    
    // Отправляем пакет
    QByteArray datagram = PacketProcessor::toByteArray(packet);
    qint64 sent = m_udpSocket->writeDatagram(datagram, m_serverAddress, m_serverPort);
    
    if (sent == -1) {
        qDebug() << "Failed to send packet:" << m_udpSocket->errorString();
    } else {
        m_stats.totalPacketsSent++;
        m_stats.totalBytesSent += data.size();
        
        // Добавляем в FEC буфер (только данные после RouteHeader)
        int groupId = customSequence / FEC_GROUP_SIZE;
        int positionInGroup = customSequence % FEC_GROUP_SIZE;
        
        if (positionInGroup < FEC_DATA_PACKETS) {
            if (!m_fecSendBuffers.contains(groupId)) {
                m_fecSendBuffers[groupId] = QVector<QByteArray>(FEC_TOTAL_PACKETS);
            }
            
            // Сохраняем часть пакета после RouteHeader для XOR
            QByteArray dataPart = datagram.mid(PACKET_HEADER_SIZE);
            m_fecSendBuffers[groupId][positionInGroup] = dataPart;
            
            // Если накопилось 4 пакета, отправляем XOR пакет
            if (positionInGroup == FEC_DATA_PACKETS - 1) {
                sendXorPackets();
            }
        }
    }
}


QByteArray NetworkManager::calculateXorForGroup(int groupId)
{
    if (!m_fecSendBuffers.contains(groupId)) {
        return QByteArray();
    }
    
    QVector<QByteArray>& packets = m_fecSendBuffers[groupId];
    
    // Используем фиксированный размер 1192 байта
    const int PACKET_SIZE = 1192;
    QByteArray result(PACKET_SIZE, 0);
    
    for (int i = 0; i < FEC_DATA_PACKETS; ++i) {
        if (packets[i].isEmpty() || packets[i].size() != PACKET_SIZE) {
            qWarning() << "Invalid packet size in FEC group" << groupId << "position" << i;
            continue;
        }
        
        const QByteArray& packetData = packets[i];
        for (int j = 0; j < PACKET_SIZE; ++j) {
            result[j] = result[j] ^ packetData[j];
        }
    }
    
    // Устанавливаем FEC_FLAG = 1 в первом байте
    if (!result.isEmpty()) {
        result[0] = (result[0] & 0x7F) | 0x80;
    }
    
    return result;
}

// В tryRecoverLostPackets исправляем логику восстановления
void NetworkManager::tryRecoverLostPackets(int groupId)
{
    if (!m_fecReceiveBuffers.contains(groupId) || !m_fecReceived.contains(groupId)) {
        return;
    }

    QVector<QByteArray>& packets = m_fecReceiveBuffers[groupId];
    QVector<bool>& received = m_fecReceived[groupId];

    // Проверяем, сколько пакетов данных потеряно
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
                for (int j = 0; j < recoveredData.size() && j < packetData.size(); ++j) {
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
                    streamId = tempPacket.route.streamId;
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
                    streamId, recoveredSequence, recoveredType, payload);

                // Конвертируем в QByteArray для обработки
                QByteArray recoveredDatagram = PacketProcessor::toByteArray(recoveredPacket);
                
                // Создаем QNetworkDatagram и обрабатываем как обычный пакет
                QNetworkDatagram datagram(recoveredDatagram, m_serverAddress, m_serverPort);
                processPacketNewProtocol(datagram);

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
    // 1. Очищаем старые сборки фреймов (существующая логика)
    m_frameAssembler->cleanupOldAssemblies(10000); // 10 секунд
    
    // 2. Очищаем старые FEC группы (новая логика)
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
        "=== New Protocol Statistics ===\n"
        "Time: %1s | Frames: %2 sent, %3 received (%4% loss)\n"
        "Packets: %5 sent, %6 received | Data Rate: %7/%8 KB/s\n"
        "FEC: %9 groups sent, %10 recovered, %11 packets recovered\n"
        "================================="
    ).arg(elapsedSeconds, 0, 'f', 1)
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

void NetworkManager::sendPacketNewProtocol(const QByteArray &data, int streamId, PacketType type)
{
    sendPacketNewProtocol(data, streamId, type, m_packetSequence++);
}
