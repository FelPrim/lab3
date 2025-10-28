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

void NetworkManager::sendVideoFrame(int streamId, int frameNumber, const QByteArray &frameData)
{
    if (!m_initialized || !m_udpSocket) {
        qWarning() << "NetworkManager not initialized, cannot send frame";
        return;
    }
    
    if (frameData.isEmpty()) {
        qWarning() << "Attempt to send empty frame data";
        return;
    }
    
    try {
        // Разбиваем фрейм на пакеты
        int dataSize = frameData.size();
        int totalParts = (dataSize + MAX_PAYLOAD_SIZE - 1) / MAX_PAYLOAD_SIZE;
        
        qDebug() << "Sending frame" << frameNumber << "from stream" << streamId 
                 << "size:" << dataSize << "bytes, parts:" << totalParts;
        
        // Отправляем каждый пакет
        for (int partIndex = 0; partIndex < totalParts; partIndex++) {
            int start = partIndex * MAX_PAYLOAD_SIZE;
            int length = qMin(MAX_PAYLOAD_SIZE, dataSize - start);
            
            QByteArray payload = frameData.mid(start, length);
            sendPacket(payload, streamId, frameNumber, partIndex, totalParts);
        }
        
        // Обновляем статистику
        updateSendStats(totalParts, dataSize);
        m_stats.framesSent++;
        m_stats.expectedFrames.insert(qMakePair(streamId, frameNumber));
        
        qDebug() << "Successfully sent frame" << frameNumber << "from stream" << streamId;
        
    } catch (const std::exception &e) {
        qCritical() << "Exception in sendVideoFrame:" << e.what();
        emit errorOccurred(QString("Send video frame failed: %1").arg(e.what()));
    }
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
            
            processIncomingPacket(datagram);
            
        } catch (const std::exception &e) {
            qCritical() << "Exception in onPacketReceived:" << e.what();
        }
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

void NetworkManager::addPacketToAssembly(int streamId, int frameNumber, int partIndex, int totalParts, const QByteArray &packetData)
{
	 if (totalParts <= 0 || partIndex < 0 || partIndex >= totalParts || packetData.isEmpty()) {
        qWarning() << "Invalid packet parameters";
        return;
    }
    
    QPair<int, int> key = qMakePair(streamId, frameNumber);
    
    // Находим или создаем сборку - теперь работает с конструктором по умолчанию
    if (!m_assemblies.contains(key)) {
        m_assemblies[key] = FrameAssembly(streamId, frameNumber, totalParts);
    }
    
    FrameAssembly &assembly = m_assemblies[key];
    
    // Если это новая сборка
    if (assembly.totalParts == 0) {
        assembly = FrameAssembly(streamId, frameNumber, totalParts);
        qDebug() << "Starting assembly for stream" << streamId << "frame" << frameNumber 
                 << "with" << totalParts << "parts";
    }
    
    // Проверяем согласованность
    if (assembly.totalParts != totalParts) {
        qWarning() << "Total parts mismatch for stream" << streamId << "frame" << frameNumber 
                   << ":" << assembly.totalParts << "vs" << totalParts;
        m_stats.assembliesDropped++;
        m_assemblies.remove(key);
        return;
    }
    
    // Добавляем пакет (если он еще не был добавлен)
    if (assembly.parts[partIndex].isEmpty()) {
        assembly.parts[partIndex] = packetData;
        assembly.receivedParts++;
        qDebug() << "Stream" << streamId << "frame" << frameNumber 
                 << "part" << partIndex + 1 << "/" << totalParts << "added";
    }
    
    // Проверяем, собран ли фрейм
    if (assembly.isComplete()) {
        qDebug() << "Stream" << streamId << "frame" << frameNumber << "completed";
        completeFrameAssembly(streamId, frameNumber);
    }
}

void NetworkManager::completeFrameAssembly(int streamId, int frameNumber)
{
    QPair<int, int> key = qMakePair(streamId, frameNumber);
    
    if (!m_assemblies.contains(key)) {
        qWarning() << "Attempt to complete non-existent assembly";
        return;
    }
    
    FrameAssembly &assembly = m_assemblies[key];
    QByteArray completedFrame = assembly.assembleFrame();
    
    // Эмитируем сигнал о собранном фрейме
    emit frameAssembled(streamId, frameNumber, completedFrame);
    
    // Обновляем статистику
    m_stats.framesReceived++;
    m_stats.assembliesCompleted++;
    m_stats.receivedFrames.insert(key);
    
    // Удаляем сборку из хештаблицы
    m_assemblies.remove(key);
    
    qDebug() << "Frame assembly completed - Stream:" << streamId << "Frame:" << frameNumber 
             << "Size:" << completedFrame.size() << "bytes";
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
