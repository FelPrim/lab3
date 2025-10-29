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
    qDebug() << "NetworkManager: Constructor with new FEC protocol (K=(N+1)/2)";
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
    
    m_assemblies.clear();
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

void NetworkManager::sendVideoFrame(int streamId, int frameNumber, const QByteArray &frameData)
{
    if (!m_initialized || !m_udpSocket) {
        return;
    }
    
    try {
        qDebug() << "Sending video frame with N+2N FEC, size:" << frameData.size();
        
        // Вычисляем оптимальное N
        int N = calculateOptimalN(frameData.size());
        
        // K = 2 * N (высокая избыточность для плохих сетей)
        int K = 2 * N;
        
        qDebug() << "Selected FEC configuration: N=" << N << "K=" << K << "totalParts=" << (N + K);
        
        // Создаем временную конфигурацию FEC
        ReedSolomonFEC::Config dynamicConfig(N, K);
        
        // Проверяем совместимость размера фрейма
        if (!ReedSolomonFEC::isFrameSizeCompatible(frameData.size(), dynamicConfig)) {
            qWarning() << "Frame too large for FEC, using simple packetization";
            sendVideoFrameSimple(streamId, frameNumber, frameData);
            return;
        }
        
        // Безопасное FEC кодирование
        auto shards = ReedSolomonFEC::encode(frameData, dynamicConfig);
        
        // Отправляем каждый шард
        for (size_t i = 0; i < shards.size(); ++i) {
            sendPacketWithSize(shards[i], streamId, frameNumber, 
                             static_cast<int>(i), dynamicConfig.total_shards, 
                             frameData.size());
        }
        
        updateSendStats(static_cast<int>(shards.size()), frameData.size());
        m_stats.framesSent++;
        m_stats.expectedFrames.insert(qMakePair(streamId, frameNumber));
        
        qDebug() << "Frame" << frameNumber << "encoded into" << shards.size() 
                 << "FEC shards using config N=" << N << "K=" << K;
        
    } catch (const std::exception &e) {
        qCritical() << "Failed to send video frame with N+2N FEC:" << e.what();
        // Fallback на простую отправку
        sendVideoFrameSimple(streamId, frameNumber, frameData);
    }
}

QByteArray NetworkManager::createPacketWithHeader(const QByteArray &data, int streamId, int frameNumber, 
                                                int partIndex, int totalParts, int originalSize)
{
    QByteArray packet;
    QDataStream stream(&packet, QIODevice::WriteOnly);
    
    // Первый байт: N + 1 (где N = количество data shards)
    // totalParts = N + K = N + 2*N = 3*N
    // => N = totalParts / 3
    int N = totalParts / 3;
    quint8 firstByte = static_cast<quint8>(N + 1);
    
    qDebug() << "Creating packet: N=" << N << "firstByte=" << firstByte << "totalParts=" << totalParts;
    
    stream << firstByte;
    stream << streamId;
    stream << frameNumber;
    stream << partIndex;
    stream << totalParts;
    stream << originalSize;
    stream.writeRawData(data.constData(), data.size());
    
    return packet;
}

bool NetworkManager::parsePacketWithHeader(const QByteArray &packetData, int &streamId, int &frameNumber,
                                         int &partIndex, int &totalParts, int &originalSize, QByteArray &payload)
{
    if (packetData.size() < HEADER_SIZE) {
        return false;
    }
    
    try {
        QDataStream stream(packetData);
        quint8 firstByte;
        
        stream >> firstByte;
        stream >> streamId;
        stream >> frameNumber;
        stream >> partIndex;
        stream >> totalParts;
        stream >> originalSize;
        
        // Проверяем корректность первого байта
        int N = firstByte - 1;
        
        // Вычисляем ожидаемое totalParts для данного N
        // K = 2 * N
        int expectedTotalParts = N + (2 * N); // N + 2N = 3N
        
        qDebug() << "Parsed packet: firstByte=" << firstByte << "N=" << N 
                 << "totalParts=" << totalParts << "expectedTotalParts=" << expectedTotalParts;
        
        if (totalParts != expectedTotalParts) {
            qWarning() << "Packet header mismatch: firstByte indicates N =" << N 
                       << "K =" << (2 * N) << "but totalParts =" << totalParts << "expected:" << expectedTotalParts;
            return false;
        }
        
        payload = packetData.mid(HEADER_SIZE);
        return true;
        
    } catch (const std::exception &e) {
        qCritical() << "Error parsing packet header:" << e.what();
        return false;
    }
}

bool NetworkManager::canRecoverWithFEC(const FrameAssembly &assembly) const
{
    // Для FEC восстановления нужно как минимум N пакетов
    // totalParts = N + K = N + 2*N = 3*N
    // => N = totalParts / 3
    
    if (assembly.totalParts % 3 != 0) {
        qDebug() << "Invalid totalParts for 2N FEC:" << assembly.totalParts;
        return false;
    }
    
    int N = assembly.totalParts / 3;
    int K = 2 * N;
    
    int missing = assembly.totalParts - assembly.receivedParts;
    bool canRecover = (assembly.receivedParts >= N) && (missing <= K);
    
    if (canRecover) {
        qDebug() << "2N FEC recovery possible for frame" << assembly.frameNumber 
                 << "(" << assembly.receivedParts << "/" << assembly.totalParts << "packets)"
                 << "N=" << N << "K=" << K << "can recover up to" << K << "lost packets";
    } else {
        qDebug() << "2N FEC recovery NOT possible for frame" << assembly.frameNumber 
                 << "(" << assembly.receivedParts << "/" << assembly.totalParts << "packets)"
                 << "N=" << N << "K=" << K << "missing:" << missing;
    }
    
    return canRecover;
}

int NetworkManager::calculateOptimalN(int frame_size)
{
    // Используем консервативную конфигурацию для плохих сетей
    const int MIN_N = 3;
    const int MAX_N = 10; // Ограничиваем чтобы не слишком много пакетов
    const int TARGET_SHARD_SIZE = 800; // Уменьшаем для лучшей надежности
    const int MAX_SHARD_SIZE = 1179; 
    // Вычисляем N на основе размера фрейма
    int calculated_n = qMax(MIN_N, frame_size / TARGET_SHARD_SIZE);
    
    // Ограничиваем максимальным значением
    calculated_n = qMin(calculated_n, MAX_N);
    
    // Проверяем размер шарда
    int shard_size = (frame_size + calculated_n - 1) / calculated_n;
    
    if (shard_size > MAX_SHARD_SIZE) {
        // Шарды слишком большие - увеличиваем N
        calculated_n = qMin(MAX_N, frame_size / MAX_SHARD_SIZE + 1);
        qDebug() << "Shards too large (" << shard_size << " bytes), adjusted N to:" << calculated_n;
    }
    
    // Гарантируем, что N ≥ 3
    calculated_n = qMax(MIN_N, calculated_n);
    
    // Пересчитываем окончательный размер шарда
    int final_shard_size = (frame_size + calculated_n - 1) / calculated_n;
    int total_packets = calculated_n * 3; // N + 2N = 3N
    
    qDebug() << "Calculated optimal N:" << calculated_n 
             << "for frame size:" << frame_size 
             << "shard size:" << final_shard_size << "bytes"
             << "total packets:" << total_packets;
    
    return calculated_n;
}





void NetworkManager::sendPacketWithSize(const QByteArray &data, int streamId, int frameNumber, 
                                      int partIndex, int totalParts, int originalSize)
{
    if (!m_udpSocket || m_udpSocket->state() != QAbstractSocket::BoundState) {
        return;
    }
    
    try {
        // Создаем пакет с новым заголовком
        QByteArray datagram = createPacketWithHeader(data, streamId, frameNumber, 
                                                   partIndex, totalParts, originalSize);
        
        if (datagram.size() > MAX_UDP_PACKET_SIZE) {
            qDebug() << "Datagram too large:" << datagram.size() << "bytes";
            return;
        }
        
        qint64 sent = m_udpSocket->writeDatagram(datagram, m_serverAddress, m_serverPort);
        
        if (sent == -1) {
            qDebug() << "Failed to send packet:" << m_udpSocket->errorString();
        }
        
    } catch (const std::exception &e) {
        qDebug() << "Exception in sendPacketWithSize:" << e.what();
    }
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
            
            processPacketWithSize(datagram);
            
        } catch (const std::exception &e) {
            qCritical() << "Exception in onPacketReceived:" << e.what();
        }
    }
}

void NetworkManager::processPacketWithSize(const QNetworkDatagram &datagram)
{
    QByteArray data = datagram.data();
    
    if (data.size() < HEADER_SIZE) {
        return;
    }
    
    try {
        int streamId, frameNumber, partIndex, totalParts, originalSize;
        QByteArray payload;
        
        if (parsePacketWithHeader(data, streamId, frameNumber, partIndex, totalParts, originalSize, payload)) {
            QPair<QByteArray, int> packetWithSize = qMakePair(payload, originalSize);
            addPacketToAssembly(streamId, frameNumber, partIndex, totalParts, packetWithSize);
            updateReceiveStats(1, payload.size());
        }
        
    } catch (const std::exception &e) {
        qDebug() << "Exception processing packet:" << e.what();
    }
}

void NetworkManager::addPacketToAssembly(int streamId, int frameNumber, int partIndex, 
                                       int totalParts, const QPair<QByteArray, int>& packetWithSize)
{
    QPair<int, int> key = qMakePair(streamId, frameNumber);
    
    if (!m_assemblies.contains(key)) {
        m_assemblies[key] = FrameAssembly(streamId, frameNumber, totalParts);
    }
    
    FrameAssembly &assembly = m_assemblies[key];
    
    if (assembly.totalParts != totalParts) {
        qWarning() << "Assembly total parts mismatch for frame" << frameNumber 
                   << "expected:" << assembly.totalParts << "got:" << totalParts;
        m_stats.assembliesDropped++;
        m_assemblies.remove(key);
        return;
    }
    
    // Проверка границ
    if (partIndex < 0 || partIndex >= assembly.parts.size()) {
        qWarning() << "Invalid part index:" << partIndex << "for total parts:" << assembly.totalParts;
        return;
    }
    
    if (assembly.parts[partIndex].first.isEmpty()) {
        assembly.parts[partIndex] = packetWithSize;
        assembly.receivedParts++;
        
        qDebug() << "Frame" << frameNumber << "progress:" << assembly.receivedParts << "/" << assembly.totalParts;
        
        // Пытаемся собрать фрейм если получили достаточно пакетов
        if (assembly.isComplete() || canRecoverWithFEC(assembly)) {
            completeFrameAssembly(streamId, frameNumber);
        }
    }
}



void NetworkManager::cleanupOldAssemblies()
{
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    int cleanupThreshold = 10000; // 10 секунд
    
    auto it = m_assemblies.begin();
    while (it != m_assemblies.end()) {
        if (currentTime - it->creationTime > cleanupThreshold) {
            qDebug() << "Cleaning up old assembly - Stream:" << it->streamId 
                     << "Frame:" << it->frameNumber << "(" << it->receivedParts << "/" << it->totalParts << "parts)";
            m_stats.assembliesDropped++;
            it = m_assemblies.erase(it);
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



void NetworkManager::sendVideoFrameSimple(int streamId, int frameNumber, const QByteArray &frameData)
{
    try {
        qDebug() << "Using simple packetization (FEC fallback) for frame:" << frameData.size() << "bytes";
        
        int dataSize = frameData.size();
        int totalParts = 15; // Фиксированное количество пакетов для fallback
        
        // Вычисляем размер каждого пакета
        int payloadPerPart = (dataSize + totalParts - 1) / totalParts;
        
        qDebug() << "Simple packetization: parts=" << totalParts 
                 << "frameSize=" << dataSize << "bytes, payloadPerPart=" << payloadPerPart;
        
        // Отправляем все пакеты
        for (int partIndex = 0; partIndex < totalParts; partIndex++) {
            int start = partIndex * payloadPerPart;
            int length = qMin(payloadPerPart, dataSize - start);
            QByteArray packet = frameData.mid(start, length);
            
            sendPacketWithSize(packet, streamId, frameNumber, 
                             partIndex, totalParts, length);
        }
        
        updateSendStats(totalParts, frameData.size());
        m_stats.framesSent++;
        m_stats.expectedFrames.insert(qMakePair(streamId, frameNumber));
        
    } catch (const std::exception &e) {
        qCritical() << "Simple packetization also failed:" << e.what();
        emit errorOccurred(QString("All sending methods failed: %1").arg(e.what()));
    }
}

uint32_t NetworkManager::calculateCRC32(const QByteArray &data)
{
    return crc32(0, (const Bytef*)data.constData(), data.size());
}

void NetworkManager::completeFrameAssembly(int streamId, int frameNumber)
{
    QPair<int, int> key = qMakePair(streamId, frameNumber);
    
    if (!m_assemblies.contains(key)) {
        return;
    }
    
    FrameAssembly &assembly = m_assemblies[key];
    
    try {
        QByteArray frameData;
        bool recoveredWithFEC = false;
        
        if (assembly.isComplete()) {
            // Все пакеты получены - просто собираем
            frameData = assembly.assembleFrame();
            qDebug() << "Frame" << frameNumber << "assembled without FEC, size:" << frameData.size();
        } else if (canRecoverWithFEC(assembly)) {
            // Пытаемся восстановить с помощью FEC
            recoveredWithFEC = safeDecodeFrame(assembly, frameData);
            if (recoveredWithFEC) {
                qDebug() << "Frame" << frameNumber << "recovered with FEC, size:" << frameData.size();
            } else {
                qWarning() << "FEC recovery failed for frame" << frameNumber;
                m_stats.assembliesDropped++;
                m_assemblies.remove(key);
                return;
            }
        } else {
            qDebug() << "Not enough packets for frame" << frameNumber 
                     << "(" << assembly.receivedParts << "/" << assembly.totalParts << ")";
            return;
        }
        
        // Проверяем целостность данных
        if (frameData.isEmpty()) {
            qWarning() << "Empty frame after assembly for frame" << frameNumber;
            m_stats.assembliesDropped++;
            m_assemblies.remove(key);
            return;
        }
        
        if (frameData.size() < 100) {
            qWarning() << "Frame too small after assembly:" << frameData.size() << "bytes";
            m_stats.assembliesDropped++;
            m_assemblies.remove(key);
            return;
        }
        
        // Отправляем собранный фрейм
        emit frameAssembled(streamId, frameNumber, frameData);
        
        m_stats.framesReceived++;
        m_stats.assembliesCompleted++;
        m_stats.receivedFrames.insert(key);
        
        qDebug() << "Frame" << frameNumber << "successfully processed, size:" << frameData.size() << "bytes";
        
    } catch (const std::exception &e) {
        qCritical() << "Frame assembly failed for frame" << frameNumber << ":" << e.what();
        m_stats.assembliesDropped++;
    }
    
    m_assemblies.remove(key);
}

std::vector<QByteArray> NetworkManager::safeEncodeFrame(const QByteArray &frameData)
{
    try {
        qDebug() << "Safe FEC encoding started, frame size:" << frameData.size();
        
        // Вычисляем оптимальную конфигурацию для этого фрейма
        int N = calculateOptimalN(frameData.size());
        int K = (N + 1) / 2;
        ReedSolomonFEC::Config dynamicConfig(N, K);
        
        auto encoded_shards = ReedSolomonFEC::encode(frameData, dynamicConfig);
        
        qDebug() << "Safe FEC encoding successful, produced" 
                 << encoded_shards.size() << "shards (N=" << N << "K=" << K << ")";
        return encoded_shards;
        
    } catch (const ReedSolomonException& e) {
        qCritical() << "FEC encoding error:" << e.what();
        throw;
    } catch (const std::exception& e) {
        qCritical() << "Unexpected error during FEC encoding:" << e.what();
        throw;
    }
}

bool NetworkManager::safeDecodeFrame(FrameAssembly &assembly, QByteArray &result)
{
    try {
        qDebug() << "Safe FEC decoding started for frame" << assembly.frameNumber;
        
        // Вычисляем N и K из totalParts для схемы K=2N
        // totalParts = N + K = N + 2N = 3N
        if (assembly.totalParts % 3 != 0) {
            qCritical() << "Invalid totalParts for 2N FEC:" << assembly.totalParts;
            return false;
        }
        
        int N = assembly.totalParts / 3;
        int K = 2 * N;  // СХЕМА K = 2*N
        
        // Создаем конфигурацию с K = 2*N
        ReedSolomonFEC::Config config(N, K);
        
        qDebug() << "2N FEC decoding: N=" << N << "K=" << K << "totalParts=" << assembly.totalParts;
        
        // Остальной код остается без изменений...
        std::vector<QByteArray> shards(config.total_shards);
        int received_count = 0;
        int original_size = 0;
        
        for (int i = 0; i < config.total_shards; ++i) {
            if (!assembly.parts[i].first.isEmpty()) {
                shards[i] = assembly.parts[i].first;
                received_count++;
                if (original_size == 0 && assembly.parts[i].second > 0) {
                    original_size = assembly.parts[i].second;
                }
            }
        }
        
        if (original_size == 0) {
            qCritical() << "Cannot determine original size for frame" << assembly.frameNumber;
            return false;
        }
        
        // Пытаемся декодировать
        if (ReedSolomonFEC::decode(shards, config)) {
            QByteArray recovered_data;
            for (int i = 0; i < config.data_shards; ++i) {
                recovered_data.append(shards[i]);
            }
            
            if (recovered_data.size() > original_size) {
                recovered_data.resize(original_size);
            }
            
            result = std::move(recovered_data);
            return true;
        } else {
            return false;
        }
        
    } catch (const std::exception& e) {
        qCritical() << "Error during 2N FEC decoding:" << e.what();
        return false;
    }
}

void NetworkManager::printStatistics()
{
    double elapsedSeconds = m_operationTimer.elapsed() / 1000.0;
    
    if (elapsedSeconds == 0) return;
    
    double sendRate = (m_stats.totalBytesSent / 1024.0) / elapsedSeconds;
    double receiveRate = (m_stats.totalBytesReceived / 1024.0) / elapsedSeconds;
    
    // Расчет потерь
    double packetLossRate = 0.0;
    if (m_stats.totalPacketsSent > 0) {
        packetLossRate = (1.0 - (double)m_stats.totalPacketsReceived / m_stats.totalPacketsSent) * 100.0;
    }
    
    double frameLossRate = 0.0;
    if (m_stats.expectedFrames.size() > 0) {
        int lostFrames = m_stats.expectedFrames.size() - m_stats.receivedFrames.size();
        frameLossRate = (double)lostFrames / m_stats.expectedFrames.size() * 100.0;
    }
    
    // Вычисляем текущие N и K из статистики сборок
    int currentN = 0;
    int currentK = 0;
    if (!m_assemblies.isEmpty()) {
        auto it = m_assemblies.constBegin();
        if (it != m_assemblies.constEnd()) {
            FrameAssembly firstAssembly = it.value();
            int totalParts = firstAssembly.totalParts;
            // totalParts = N + K = N + 2N = 3N
            if (totalParts % 3 == 0) {
                currentN = totalParts / 3;
                currentK = 2 * currentN;
            }
        }
    }
    
    QString stats = QString(
        "=== Network Statistics (5s) ===\n"
        "Time: %1s | Frames: %2 sent, %3 received (%4% loss)\n"
        "Packets: %5 sent, %6 received (%7% loss)\n"
        "Data Rate: %8/%9 KB/s | Active Assemblies: %10\n"
        "FEC Scheme: N=%11, K=2N=%12 (Total: %13 packets)\n"
        "First Byte Protocol: N+1=%14 | Recovery: up to %15 lost packets\n"
        "================================="
    ).arg(elapsedSeconds, 0, 'f', 1)
     .arg(m_stats.framesSent)
     .arg(m_stats.framesReceived)
     .arg(frameLossRate, 0, 'f', 1)
     .arg(m_stats.totalPacketsSent)
     .arg(m_stats.totalPacketsReceived)
     .arg(packetLossRate, 0, 'f', 1)
     .arg(sendRate, 0, 'f', 2)
     .arg(receiveRate, 0, 'f', 2)
     .arg(m_assemblies.size())
     .arg(currentN)
     .arg(currentK)
     .arg(currentN + currentK)
     .arg(currentN + 1)
     .arg(currentK); // Максимум потерянных пакетов которые можно восстановить
    
    qDebug().noquote() << stats;
    emit statisticsUpdated(stats);
}
