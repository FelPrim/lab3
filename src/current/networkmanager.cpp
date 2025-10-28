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
    qDebug() << "NetworkManager: Constructor";
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
        m_statsTimer->setInterval(10000); // Статистика каждые 10 секунд
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

void NetworkManager::sendPacket(const QByteArray &data, int streamId, int frameNumber, int partIndex, int totalParts)
{
    if (!m_udpSocket || m_udpSocket->state() != QAbstractSocket::BoundState) {
        qCritical() << "Socket not ready for sending";
        return;
    }
    
    try {
        // Создаем датаграмму с заголовком
        QByteArray datagram;
        QDataStream stream(&datagram, QIODevice::WriteOnly);
        
        // Заголовок: streamId, frameNumber, totalParts, partIndex
        stream << streamId;
        stream << frameNumber;
        stream << totalParts;
        stream << partIndex;
        
        // Полезная нагрузка
        stream.writeRawData(data.constData(), data.size());
        
        // Проверяем размер
        if (datagram.size() > MAX_UDP_PACKET_SIZE) {
            qCritical() << "Datagram too large:" << datagram.size() << "bytes";
            return;
        }
        
        // Отправляем
        qint64 sent = m_udpSocket->writeDatagram(datagram, m_serverAddress, m_serverPort);
        
        if (sent == -1) {
            qCritical() << "Failed to send UDP packet:" << m_udpSocket->errorString();
        } else {
            qDebug() << "Sent packet - Stream:" << streamId << "Frame:" << frameNumber 
                     << "Part:" << partIndex << "/" << totalParts << "Size:" << sent << "bytes";
        }
        
    } catch (const std::exception &e) {
        qCritical() << "Exception in sendPacket:" << e.what();
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
        qWarning() << "Received datagram too small:" << data.size() << "bytes";
        return;
    }
    
    try {
        QDataStream stream(data);
        int streamId, frameNumber, totalParts, partIndex, originalSize;
        
        // Читаем расширенный заголовок
        stream >> streamId;
        stream >> frameNumber;
        stream >> totalParts;
        stream >> partIndex;
        stream >> originalSize;
        
        // Читаем данные
        QByteArray payload = data.mid(HEADER_SIZE);
        
        qDebug() << "📥 Received part" << partIndex << "/" << totalParts 
                 << "for frame" << frameNumber
                 << "size:" << payload.size() << "original:" << originalSize << "bytes";
        
        // Сохраняем данные с информацией о размере
        QPair<QByteArray, int> packetWithSize = qMakePair(payload, originalSize);
        
        // Обрабатываем пакет
        addPacketToAssembly(streamId, frameNumber, partIndex, totalParts, packetWithSize);
        
        updateReceiveStats(1, payload.size());
        
    } catch (const std::exception &e) {
        qCritical() << "Exception processing packet:" << e.what();
    }
}

void NetworkManager::processIncomingPacket(const QNetworkDatagram &datagram)
{
    QByteArray data = datagram.data();
    
    if (data.size() < HEADER_SIZE) {
        qWarning() << "Received datagram too small:" << data.size() << "bytes";
        return;
    }
    
    try {
        QDataStream stream(data);
        int streamId, frameNumber, totalParts, partIndex;
        
        // Читаем заголовок
        stream >> streamId;
        stream >> frameNumber;
        stream >> totalParts;
        stream >> partIndex;
        
        // Читаем полезную нагрузку
        QByteArray payload = data.mid(HEADER_SIZE);
        
        qDebug() << "Received packet - Stream:" << streamId << "Frame:" << frameNumber 
                 << "Part:" << partIndex << "/" << totalParts << "Size:" << payload.size() << "bytes";
        
        // Обрабатываем пакет
        addPacketToAssembly(streamId, frameNumber, partIndex, totalParts, payload);
        
        // Обновляем статистику
        updateReceiveStats(1, payload.size());
        
    } catch (const std::exception &e) {
        qCritical() << "Exception processing incoming packet:" << e.what();
    }
}

void NetworkManager::sendVideoFrame(int streamId, int frameNumber, const QByteArray &frameData)
{
    if (!m_initialized || !m_udpSocket) {
        qWarning() << "NetworkManager not initialized, cannot send frame";
        return;
    }
    
    try {
        // Разбиваем фрейм на части
        int dataSize = frameData.size();
        int dataParts = (dataSize + MAX_PAYLOAD_SIZE - 1) / MAX_PAYLOAD_SIZE;
        
        qDebug() << "📦 Splitting frame" << frameNumber << "of size" << dataSize 
                 << "into" << dataParts << "data parts";
        
        // Создаем сегменты данных и запоминаем их оригинальные размеры
        std::vector<QByteArray> dataShards;
        std::vector<int> originalSizes;
        
        for (int i = 0; i < dataParts; i++) {
            int start = i * MAX_PAYLOAD_SIZE;
            int length = qMin(MAX_PAYLOAD_SIZE, dataSize - start);
            QByteArray shard = frameData.mid(start, length);
            
            dataShards.push_back(shard);
            originalSizes.push_back(length);
            
            qDebug() << "  Part" << i << "size:" << length << "bytes";
        }
        
        // Вычисляем XOR-пакет для всех сегментов данных (с дополнением)
        QByteArray xorPacket = XORFEC::computeXOR(dataShards);
        
        // Общее количество пакетов: данные + XOR
        int totalParts = dataParts + 1;
        
        qDebug() << "🔒 XOR protection: data parts =" << dataParts 
                 << "xor size =" << xorPacket.size() << "total parts =" << totalParts;
        
        // Отправляем все сегменты данных с информацией об оригинальном размере
        for (int partIndex = 0; partIndex < dataParts; partIndex++) {
            // В заголовок добавляем информацию об оригинальном размере
            sendPacketWithSize(dataShards[partIndex], streamId, frameNumber, 
                             partIndex, totalParts, originalSizes[partIndex]);
        }
        
        // Отправляем XOR-пакет (последний пакет)
        sendPacketWithSize(xorPacket, streamId, frameNumber, dataParts, totalParts, xorPacket.size());
        
        updateSendStats(totalParts, frameData.size());
        m_stats.framesSent++;
        m_stats.expectedFrames.insert(qMakePair(streamId, frameNumber));
        
        qDebug() << "✅ XOR protected frame" << frameNumber << "sent successfully";
        
    } catch (const std::exception &e) {
        qCritical() << "XOR encoding failed:" << e.what();
        emit errorOccurred(QString("XOR encoding failed: %1").arg(e.what()));
    }
}

void NetworkManager::sendPacketWithSize(const QByteArray &data, int streamId, int frameNumber, 
                                      int partIndex, int totalParts, int originalSize)
{
    if (!m_udpSocket || m_udpSocket->state() != QAbstractSocket::BoundState) {
        return;
    }
    
    try {
        QByteArray datagram;
        QDataStream stream(&datagram, QIODevice::WriteOnly);
        
        // Расширенный заголовок с информацией о размере
        stream << streamId;
        stream << frameNumber;
        stream << totalParts;
        stream << partIndex;
        stream << originalSize;  // Добавляем оригинальный размер
        
        stream.writeRawData(data.constData(), data.size());
        
        if (datagram.size() > MAX_UDP_PACKET_SIZE) {
            qCritical() << "Datagram too large:" << datagram.size() << "bytes";
            return;
        }
        
        qint64 sent = m_udpSocket->writeDatagram(datagram, m_serverAddress, m_serverPort);
        
        if (sent == -1) {
            qCritical() << "Failed to send packet:" << m_udpSocket->errorString();
        } else {
            qDebug() << "📤 Sent part" << partIndex << "/" << totalParts 
                     << "size:" << data.size() << "original:" << originalSize << "bytes";
        }
        
    } catch (const std::exception &e) {
        qCritical() << "Exception in sendPacketWithSize:" << e.what();
    }
}

void NetworkManager::addPacketToAssembly(int streamId, int frameNumber, int partIndex, 
                                       int totalParts, const QPair<QByteArray, int>& packetWithSize)
{
    QPair<int, int> key = qMakePair(streamId, frameNumber);
    
    // Находим или создаем сборку
    if (!m_assemblies.contains(key)) {
        m_assemblies[key] = FrameAssembly(streamId, frameNumber, totalParts);
        qDebug() << "🔄 Starting assembly for frame" << frameNumber << "with" << totalParts << "parts";
    }
    
    FrameAssembly &assembly = m_assemblies[key];
    
    // Проверяем согласованность
    if (assembly.totalParts != totalParts) {
        qWarning() << "Part count mismatch for frame" << frameNumber 
                   << "expected:" << assembly.totalParts << "got:" << totalParts;
        m_stats.assembliesDropped++;
        m_assemblies.remove(key);
        return;
    }
    
    // Добавляем сегмент (только если он еще не был добавлен)
    if (assembly.parts[partIndex].first.isEmpty()) {
        assembly.parts[partIndex] = packetWithSize;
        assembly.receivedParts++;
        
        qDebug() << "📥 Part" << partIndex + 1 << "/" << totalParts 
                 << "received for frame" << frameNumber 
                 << "size:" << packetWithSize.first.size() 
                 << "original:" << packetWithSize.second << "bytes";
        
        // Статистика прогресса
        int received = 0;
        for (const auto& part : assembly.parts) {
            if (!part.first.isEmpty()) received++;
        }
        qDebug() << "📊 Frame" << frameNumber << "progress:" << received << "/" << totalParts;
        
        // Проверяем возможность восстановления с помощью XOR
        checkAndRecoverFrame(assembly);
    }
}

void NetworkManager::checkAndRecoverFrame(FrameAssembly &assembly)
{
    // Преобразуем в вектор QByteArray для XORFEC (игнорируем размеры)
    std::vector<QByteArray> shards;
    int missingIndex = -1;
    
    for (int i = 0; i < assembly.parts.size(); i++) {
        if (assembly.parts[i].first.isEmpty()) {
            shards.push_back(QByteArray());
            if (missingIndex == -1) {
                missingIndex = i;
            }
        } else {
            shards.push_back(assembly.parts[i].first);
        }
    }
    
    // Проверяем возможность восстановления
    if (XORFEC::canRecover(shards)) {
        qDebug() << "🎯 Can recover missing part" << missingIndex << "using XOR for frame" << assembly.frameNumber;
        completeFrameAssembly(assembly.streamId, assembly.frameNumber);
    } else if (assembly.isComplete()) {
        qDebug() << "✅ All parts received for frame" << assembly.frameNumber;
        completeFrameAssembly(assembly.streamId, assembly.frameNumber);
    }
}

void NetworkManager::completeFrameAssembly(int streamId, int frameNumber)
{
    QPair<int, int> key = qMakePair(streamId, frameNumber);
    
    if (!m_assemblies.contains(key)) {
        return;
    }
    
    FrameAssembly &assembly = m_assemblies[key];
    
    try {
        QByteArray recoveredFrame;
        
        if (assembly.isComplete()) {
            // Все сегменты получены - просто собираем
            recoveredFrame = assembly.assembleFrame();
            qDebug() << "✅ Frame" << frameNumber << "assembled without recovery, size:" << recoveredFrame.size();
        } else {
            // Используем XOR для восстановления одного отсутствующего сегмента
            qDebug() << "🔧 Recovering frame" << frameNumber << "using XOR";
            
            // Подготавливаем данные для XOR восстановления
            std::vector<QByteArray> shards;
            int missingIndex = -1;
            
            for (int i = 0; i < assembly.parts.size(); i++) {
                if (assembly.parts[i].first.isEmpty()) {
                    shards.push_back(QByteArray());
                    missingIndex = i;
                } else {
                    shards.push_back(assembly.parts[i].first);
                }
            }
            
            if (missingIndex == -1) {
                throw std::runtime_error("No missing part found but assembly is incomplete");
            }
            
            // Восстанавливаем недостающий сегмент
            QByteArray recoveredData = XORFEC::recover(shards, missingIndex);
            
            // Определяем оригинальный размер восстановленного сегмента
            int originalSize = recoveredData.size();
            if (missingIndex < assembly.parts.size() - 1) {
                // Для данных сегментов используем типичный размер
                originalSize = MAX_PAYLOAD_SIZE;
            } else {
                // Для XOR сегмента используем фактический размер
                originalSize = recoveredData.size();
            }
            
            // Заменяем недостающий сегмент
            assembly.parts[missingIndex] = qMakePair(recoveredData, originalSize);
            assembly.receivedParts++;
            
            // Теперь собираем фрейм
            recoveredFrame = assembly.assembleFrame();
            qDebug() << "✅ Frame" << frameNumber << "recovered using XOR, size:" << recoveredFrame.size();
        }
        
        // Проверяем целостность восстановленного фрейма
        if (recoveredFrame.isEmpty()) {
            throw std::runtime_error("Recovered frame is empty");
        }
        
        emit frameAssembled(streamId, frameNumber, recoveredFrame);
        
        // Обновляем статистику
        m_stats.framesReceived++;
        m_stats.assembliesCompleted++;
        m_stats.receivedFrames.insert(key);
        
        qDebug() << "🎉 Frame" << frameNumber << "successfully processed";
        
    } catch (const std::exception &e) {
        qCritical() << "Frame recovery failed for frame" << frameNumber << ":" << e.what();
        m_stats.assembliesDropped++;
        emit errorOccurred(QString("Frame %1 recovery failed: %2").arg(frameNumber).arg(e.what()));
    }
    
    m_assemblies.remove(key);
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
        "=== Network Manager Statistics ===\n"
        "Time: %1 seconds\n"
        "Frames - Sent: %2, Received: %3\n"
        "Packets - Sent: %4, Received: %5\n"
        "Data Rate - Send: %6 KB/s, Receive: %7 KB/s\n"
        "Assemblies - Completed: %8, Dropped: %9\n"
        "Frame Loss Rate: %10%\n"
        "Active Assemblies: %11\n"
        "=================================="
    ).arg(elapsedSeconds, 0, 'f', 1)
     .arg(m_stats.framesSent)
     .arg(m_stats.framesReceived)
     .arg(m_stats.totalPacketsSent)
     .arg(m_stats.totalPacketsReceived)
     .arg(sendRate, 0, 'f', 2)
     .arg(receiveRate, 0, 'f', 2)
     .arg(m_stats.assembliesCompleted)
     .arg(m_stats.assembliesDropped)
     .arg(lossRate, 0, 'f', 2)
     .arg(m_assemblies.size());
    
    qDebug().noquote() << stats;
    emit statisticsUpdated(stats);
}
