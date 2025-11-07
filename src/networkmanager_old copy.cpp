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
    
    // Инициализация FEC
    m_fec = new SimpleFEC(this);
    connect(m_fec, &SimpleFEC::groupDecoded, this, &NetworkManager::onFECGroupDecoded);
}
void NetworkManager::processPacketNewProtocol(const QNetworkDatagram &datagram)
{
    QByteArray data = datagram.data();
    if (data.size() < PACKET_HEADER_SIZE) return;
    
    try {
        QDataStream stream(data);
        int streamId, packetNumber;
        quint8 packetType, frameCount;
        
        stream >> streamId >> packetNumber >> packetType >> frameCount;
        
        // Пропускаем резервные байты
        for (int i = 0; i < 2; ++i) {
            quint8 dummy;
            stream >> dummy;
        }
        
        QByteArray payload = data.mid(PACKET_HEADER_SIZE);
        
        // Обработка FEC пакетов
        if (packetType >= XOR12_PACKET && packetType <= XOR1234_PACKET) {
            processFECPacket(streamId, packetNumber, packetType, payload);
            return;
        }
        
        // Существующая обработка обычных пакетов
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
        
        updateReceiveStats(1, payload.size());
        
    } catch (const std::exception &e) {
        qDebug() << "Exception processing packet:" << e.what();
    }
}

void NetworkManager::processFECPacket(int streamId, int packetNumber, quint8 packetType, const QByteArray &data)
{
    // Вычисляем groupId из packetNumber (каждые 4 пакета данных = 1 группа)
    int groupId = packetNumber / 9; // 4 данных + 5 FEC = 9 пакетов на группу
    int frameNumber = 0; // Для FEC frameNumber не важен
    
    // Преобразуем тип пакета FEC в внутреннее представление SimpleFEC
    int fecPacketType = packetType - XOR12_PACKET + 4; // 4-8 для FEC пакетов
    
    m_fec->addPacket(streamId, frameNumber, groupId, fecPacketType, data);
}

NetworkManager::~NetworkManager()
{
    // Очистка FEC декодеров
    for (auto decoder : m_fecDecoders) {
        delete decoder;
    }
    m_fecDecoders.clear();
    
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
    
    m_streamAssemblies.clear();
    m_sendBuffer.clear();
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
    
    if (data.size() < 4) return; // Минимум frameNumber
    
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
    // Сначала завершаем предыдущий фрейм
    if (m_streamAssemblies.contains(streamId)) {
        StreamAssembly& prevAssembly = m_streamAssemblies[streamId];
        if (prevAssembly.isComplete()) {
            processCompleteFrame(streamId);
        }
    }
    
    // Затем обрабатываем как START_FRAME для следующего фрейма
    handleStartFrame(streamId, data);
}

void NetworkManager::handleFailedBoundaryFrame(int streamId, const QByteArray& data)
{
    // Обрабатываем как CONTINUE_FRAME, но игнорируем нулевое дополнение
    handleContinueFrame(streamId, data);
}

void NetworkManager::sendVideoFrame(int streamId, int frameNumber, const QByteArray &frameData)
{
    if (!m_initialized || !m_udpSocket) {
        return;
    }
    
    try {
        // Используем FEC для отправки
        sendFECFrame(streamId, frameNumber, frameData);
        
        m_stats.framesSent++;
        m_stats.expectedFrames.insert(qMakePair(streamId, frameNumber));
        
    } catch (const std::exception &e) {
        qDebug() << "Send video frame failed:" << e.what();
        emit errorOccurred(QString("Send video frame failed: %1").arg(e.what()));
    }
}

void NetworkManager::sendFECFrame(int streamId, int frameNumber, const QByteArray &frameData)
{
    // Кодируем данные с FEC
    auto symbols = m_fecEncoder.encode(frameData);
    
    if (symbols.size() != FEC_N) {
        qWarning() << "FEC encoding failed: expected" << FEC_N << "symbols, got" << symbols.size();
        return;
    }
    
    qDebug() << "FEC encoding: frame" << frameNumber << "split into" << symbols.size() << "symbols";
    
    // Отправляем исходные символы (первые K)
    for (int i = 0; i < FEC_K; ++i) {
        QByteArray packetData;
        QDataStream stream(&packetData, QIODevice::WriteOnly);
        stream << frameNumber << (quint16)i << (quint16)FEC_K << (quint16)FEC_N;
        stream.writeRawData(symbols[i].constData(), symbols[i].size());
        
        sendPacketNewProtocol(packetData, streamId, FEC_DATA_PACKET);
    }
    
    // Отправляем восстанавливающие символы (оставшиеся N-K)
    for (int i = FEC_K; i < FEC_N; ++i) {
        QByteArray packetData;
        QDataStream stream(&packetData, QIODevice::WriteOnly);
        stream << frameNumber << (quint16)i << (quint16)FEC_K << (quint16)FEC_N;
        stream.writeRawData(symbols[i].constData(), symbols[i].size());
        
        sendPacketNewProtocol(packetData, streamId, FEC_REPAIR_PACKET);
    }
    
    updateSendStats(FEC_N, frameData.size());
}


void NetworkManager::sendBufferedData()
{
    if (m_sendBuffer.isEmpty() || !m_udpSocket) return;
    
    int bytesSent = 0;
    bool isFirstPacket = true;
    
    while (bytesSent < m_sendBuffer.size()) {
        int remaining = m_sendBuffer.size() - bytesSent;
        int chunkSize;
        
        if (isFirstPacket) {
            // Для START_FRAME: заголовок фрейма (8 байт) уже в данных
            chunkSize = qMin(MAX_PAYLOAD_SIZE, remaining);
            QByteArray packetData = m_sendBuffer.mid(bytesSent, chunkSize);
            sendPacketNewProtocol(packetData, m_streamId, START_FRAME);
            isFirstPacket = false;
        } else {
            // Для CONTINUE_FRAME: добавляем 4 байта номера фрейма
            chunkSize = qMin(MAX_PAYLOAD_SIZE - 4, remaining);
            QByteArray packetData = m_sendBuffer.mid(bytesSent, chunkSize);
            // Добавляем номер фрейма в CONTINUE_FRAME
            QByteArray continueData;
            QDataStream stream(&continueData, QIODevice::WriteOnly);
            stream << m_currentFrameNumber;
            stream.writeRawData(packetData.constData(), packetData.size());
            sendPacketNewProtocol(continueData, m_streamId, CONTINUE_FRAME);
        }
        
        bytesSent += chunkSize;
    }
    
    m_sendBuffer.clear();
}


void NetworkManager::sendPacketNewProtocol(const QByteArray &data, int streamId, PacketType type)
{
    QByteArray datagram;
    QDataStream stream(&datagram, QIODevice::WriteOnly);
    
    // Заголовок пакета
    stream << streamId << m_packetSequence++ << (quint8)type << (quint8)0;
    
    // Резервные байты
    for (int i = 0; i < 2; ++i) {
        stream << (quint8)0;
    }
    
    // Полезная нагрузка
    stream.writeRawData(data.constData(), data.size());
    
    // Дополняем нулями до фиксированного размера
    if (datagram.size() < MAX_UDP_PACKET_SIZE) {
        datagram.append(QByteArray(MAX_UDP_PACKET_SIZE - datagram.size(), 0));
    }
    
    // Отправка пакета
    qint64 sent = m_udpSocket->writeDatagram(datagram, m_serverAddress, m_serverPort);
    
    if (sent == -1) {
        qDebug() << "Failed to send packet:" << m_udpSocket->errorString();
    } else {
        m_stats.totalPacketsSent++;
        m_stats.totalBytesSent += data.size();
        
        // Сохраняем данные для FEC (только обычные пакеты)
        if (type >= START_FRAME && type <= FAILED_BOUNDARY_FRAME) {
            m_currentDataPackets.append(data);
            m_packetsSinceLastFEC++;
            
            // Когда накопилось 4 пакета, отправляем FEC
            if (m_packetsSinceLastFEC >= 4) {
                sendFECPackets(m_currentDataPackets);
                m_currentDataPackets.clear();
                m_packetsSinceLastFEC = 0;
            }
        }
    }
}

void NetworkManager::sendFECPackets(const QVector<QByteArray> &dataPackets)
{
    if (dataPackets.size() != 4) {
        qWarning() << "FEC: Need exactly 4 packets for FEC encoding";
        return;
    }
    
    // Генерируем FEC пакеты
    QVector<QByteArray> fecPackets = m_fec->encodeGroup(dataPackets);
    
    // Отправляем FEC пакеты
    for (int i = 0; i < fecPackets.size(); ++i) {
        quint8 fecType = XOR12_PACKET + i;
        sendPacketNewProtocol(fecPackets[i], m_streamId, static_cast<PacketType>(fecType));
    }
}

void NetworkManager::onFECGroupDecoded(int streamId, int frameNumber, int groupId, const QVector<QByteArray> &packets)
{
    for (int i = 0; i < packets.size(); ++i) {
        const QByteArray &recoveredPacket = packets[i];
        
        if (recoveredPacket.isEmpty()) {
            qDebug() << "FEC: Empty packet in recovered group, skipping";
            continue;
        }
        
        // Восстановленный пакет содержит полные данные (заголовок + полезная нагрузка)
        // Нужно разобрать его и обработать как обычный пакет
        processRecoveredPacket(streamId, groupId, i, recoveredPacket);
    }
    
    m_stats.assembliesCompleted++;
    emit statisticsUpdated("FEC: Recovered group " + QString::number(groupId));
}

void NetworkManager::processRecoveredPacket(int streamId, int groupId, int packetIndex, const QByteArray &fullPacketData)
{
    try {
        // Восстановленный пакет имеет тот же формат, что и обычный пакет
        if (fullPacketData.size() < PACKET_HEADER_SIZE) {
            qWarning() << "FEC: Recovered packet too small:" << fullPacketData.size();
            return;
        }
        
        QDataStream stream(fullPacketData);
        int recoveredStreamId, packetNumber;
        quint8 packetType, frameCount;
        
        // Читаем заголовок восстановленного пакета
        stream >> recoveredStreamId >> packetNumber >> packetType >> frameCount;
        
        // Пропускаем резервные байты
        for (int i = 0; i < 2; ++i) {
            quint8 dummy;
            stream >> dummy;
        }
        
        // Извлекаем полезную нагрузку
        QByteArray payload = fullPacketData.mid(PACKET_HEADER_SIZE);
        
        // Вычисляем оригинальный frameNumber на основе groupId и packetIndex
        // В нашей схеме groupId соответствует начальному frameNumber для группы
        int originalFrameNumber = groupId * 4 + packetIndex;
        
        qDebug() << "FEC: Processing recovered packet - Type:" << packetType 
                 << "Frame:" << originalFrameNumber << "Size:" << payload.size();
        
        // Обрабатываем восстановленный пакет в зависимости от его типа
        // Используем оригинальный frameNumber вместо восстановленного
        switch (packetType) {
        case START_FRAME:
            handleRecoveredStartFrame(streamId, originalFrameNumber, payload);
            break;
        case CONTINUE_FRAME:
            handleRecoveredContinueFrame(streamId, originalFrameNumber, payload);
            break;
        case BOUNDARY_FRAME:
            handleRecoveredBoundaryFrame(streamId, originalFrameNumber, payload);
            break;
        case FAILED_BOUNDARY_FRAME:
            handleRecoveredFailedBoundaryFrame(streamId, originalFrameNumber, payload);
            break;
        default:
            qDebug() << "FEC: Unknown packet type in recovered packet:" << packetType;
            break;
        }
        
        // Обновляем статистику
        updateReceiveStats(1, payload.size());
        m_stats.totalPacketsReceived++; // Учитываем восстановленные пакеты
        
    } catch (const std::exception &e) {
        qCritical() << "FEC: Exception processing recovered packet:" << e.what();
    }
}

void NetworkManager::handleRecoveredStartFrame(int streamId, int frameNumber, const QByteArray& data)
{
    if (data.size() < FRAME_HEADER_SIZE) {
        qWarning() << "FEC: Recovered START_FRAME too small";
        return;
    }
    
    QDataStream stream(data);
    int storedFrameNumber, frameSize;
    stream >> storedFrameNumber >> frameSize;
    
    QByteArray frameData = data.mid(FRAME_HEADER_SIZE);
    
    // Создаем или обновляем сборку для восстановленного фрейма
    if (!m_streamAssemblies.contains(streamId)) {
        m_streamAssemblies[streamId] = StreamAssembly(streamId, frameNumber);
    } else if (m_streamAssemblies[streamId].frameNumber != frameNumber) {
        // Если это новый фрейм, заменяем старую сборку
        m_streamAssemblies[streamId] = StreamAssembly(streamId, frameNumber);
    }
    
    StreamAssembly& assembly = m_streamAssemblies[streamId];
    assembly.totalSize = frameSize;
    assembly.data = frameData;
    assembly.receivedSize = frameData.size();
    assembly.hasStartFrame = true;
    
    qDebug() << "FEC: Recovered START_FRAME for frame" << frameNumber 
             << "total:" << frameSize << "received:" << assembly.receivedSize;
    
    // Проверяем завершенность
    if (assembly.isComplete()) {
        processCompleteFrame(streamId);
    }
}

void NetworkManager::handleRecoveredContinueFrame(int streamId, int frameNumber, const QByteArray& data)
{
    if (!m_streamAssemblies.contains(streamId)) {
        qDebug() << "FEC: No assembly for recovered CONTINUE_FRAME, frame:" << frameNumber;
        return;
    }
    
    StreamAssembly& assembly = m_streamAssemblies[streamId];
    
    if (data.size() < 4) {
        qWarning() << "FEC: Recovered CONTINUE_FRAME too small";
        return;
    }
    
    QDataStream stream(data);
    int storedFrameNumber;
    stream >> storedFrameNumber;
    
    if (storedFrameNumber != assembly.frameNumber) {
        qDebug() << "FEC: Frame number mismatch in recovered CONTINUE_FRAME. Expected:" 
                 << assembly.frameNumber << "Got:" << storedFrameNumber;
        return;
    }
    
    QByteArray frameData = data.mid(4);
    assembly.data.append(frameData);
    assembly.receivedSize += frameData.size();
    
    qDebug() << "FEC: Recovered CONTINUE_FRAME for frame" << frameNumber 
             << "added:" << frameData.size() << "total received:" << assembly.receivedSize;
    
    if (assembly.isComplete()) {
        processCompleteFrame(streamId);
    }
}

void NetworkManager::handleRecoveredBoundaryFrame(int streamId, int frameNumber, const QByteArray& data)
{
    // Сначала завершаем предыдущий фрейм, если он есть
    if (m_streamAssemblies.contains(streamId)) {
        StreamAssembly& prevAssembly = m_streamAssemblies[streamId];
        if (prevAssembly.isComplete()) {
            processCompleteFrame(streamId);
        }
    }
    
    // Обрабатываем как START_FRAME для следующего фрейма
    handleRecoveredStartFrame(streamId, frameNumber, data);
}

void NetworkManager::handleRecoveredFailedBoundaryFrame(int streamId, int frameNumber, const QByteArray& data)
{
    // Обрабатываем как CONTINUE_FRAME, но игнорируем нулевое дополнение
    handleRecoveredContinueFrame(streamId, frameNumber, data);
}

void NetworkManager::processPacketNewProtocol(const QNetworkDatagram &datagram)
{
    QByteArray data = datagram.data();
    if (data.size() < PACKET_HEADER_SIZE) return;
    
    try {
        QDataStream stream(data);
        int streamId, packetNumber;
        quint8 packetType, frameCount;
        
        stream >> streamId >> packetNumber >> packetType >> frameCount;
        
        // Пропускаем резервные байты
        for (int i = 0; i < 6; ++i) {
            quint8 dummy;
            stream >> dummy;
        }
        
        QByteArray payload = data.mid(PACKET_HEADER_SIZE);
        
        // Обработка FEC пакетов
        if (packetType == FEC_DATA_PACKET || packetType == FEC_REPAIR_PACKET) {
            processFECPacket(streamId, payload, packetType);
        } else {
            // Старая обработка не-FEC пакетов
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
        }
        
        updateReceiveStats(1, payload.size());
        
    } catch (const std::exception &e) {
        qDebug() << "Exception processing packet:" << e.what();
    }
}

void NetworkManager::processFECPacket(int streamId, const QByteArray& data, quint8 packetType)
{
    if (data.size() < 10) return; // Минимум для FEC заголовка
    
    QDataStream stream(data);
    int frameNumber;
    quint16 symbolId, K, N;
    stream >> frameNumber >> symbolId >> K >> N;
    
    QByteArray symbolData = data.mid(10); // Пропускаем FEC заголовок
    
    if (packetType == FEC_DATA_PACKET) {
        handleFECDataPacket(streamId, data);
    } else if (packetType == FEC_REPAIR_PACKET) {
        handleFECRepairPacket(streamId, data);
    }
}

void NetworkManager::handleFECDataPacket(int streamId, const QByteArray& data)
{
    QDataStream stream(data);
    int frameNumber;
    quint16 symbolId, K, N;
    stream >> frameNumber >> symbolId >> K >> N;
    
    QByteArray symbolData = data.mid(10);
    
    // Создаем или получаем FEC декодер для этого streamId
    if (!m_fecDecoders.contains(streamId)) {
        m_fecDecoders[streamId] = new FECDecoder(FEC_K, FEC_N, this);
    }
    
    FECDecoder* decoder = m_fecDecoders[streamId];
    
    // Добавляем символ в декодер
    if (decoder->addSymbol(symbolId, symbolData)) {
        qDebug() << "Added FEC data symbol" << symbolId << "for frame" << frameNumber 
                 << "(" << decoder->receivedSymbols() << "/" << decoder->totalSymbols() << ")";
        
        // Пытаемся декодировать если получили достаточно символов
        if (decoder->receivedSymbols() >= FEC_K) {
            tryFECDecode(streamId, frameNumber);
        }
    }
}

void NetworkManager::handleFECRepairPacket(int streamId, const QByteArray& data)
{
    QDataStream stream(data);
    int frameNumber;
    quint16 symbolId, K, N;
    stream >> frameNumber >> symbolId >> K >> N;
    
    QByteArray symbolData = data.mid(10);
    
    if (!m_fecDecoders.contains(streamId)) {
        m_fecDecoders[streamId] = new FECDecoder(FEC_K, FEC_N, this);
    }
    
    FECDecoder* decoder = m_fecDecoders[streamId];
    
    // Добавляем восстанавливающий символ
    if (decoder->addSymbol(symbolId, symbolData)) {
        qDebug() << "Added FEC repair symbol" << symbolId << "for frame" << frameNumber
                 << "(" << decoder->receivedSymbols() << "/" << decoder->totalSymbols() << ")";
        
        // Пытаемся декодировать
        if (decoder->canDecode()) {
            tryFECDecode(streamId, frameNumber);
        }
    }
}

void NetworkManager::tryFECDecode(int streamId, int frameNumber)
{
    if (!m_fecDecoders.contains(streamId)) {
        return;
    }
    
    FECDecoder* decoder = m_fecDecoders[streamId];
    
    if (decoder->canDecode()) {
        QByteArray decodedData = decoder->decode();
        if (!decodedData.isEmpty()) {
            qDebug() << "FEC decoding successful for frame" << frameNumber 
                     << "size:" << decodedData.size() << "bytes";
            
            emit frameAssembled(streamId, frameNumber, decodedData);
            
            m_stats.framesReceived++;
            m_stats.assembliesCompleted++;
            m_stats.receivedFrames.insert(qMakePair(streamId, frameNumber));
            
            // Сбрасываем декодер для следующего фрейма
            decoder->reset();
        } else {
            qWarning() << "FEC decoding failed for frame" << frameNumber;
        }
    }
}


void NetworkManager::cleanupOldAssemblies()
{
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    int cleanupThreshold = 10000; // 10 секунд
    
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
    
    // Расчет потерь
    double lossRate = 0.0;
    if (m_stats.expectedFrames.size() > 0) {
        int lostFrames = m_stats.expectedFrames.size() - m_stats.receivedFrames.size();
        lossRate = (double)lostFrames / m_stats.expectedFrames.size() * 100.0;
    }
    
    // FEC статистика
    int recoveredCount = m_recoveredPackets.size();
    QString fecStats = recoveredCount > 0 ? 
        QString(" | FEC recovered: %1 packets").arg(recoveredCount) : "";
    
    QString stats = QString(
        "=== New Protocol Statistics ===\n"
        "Time: %1s | Frames: %2 sent, %3 received (%4% loss)%5\n"
        "Packets: %6 sent, %7 received | Data Rate: %8/%9 KB/s\n"
        "Send Buffer: %10 bytes | Active Assemblies: %11\n"
        "================================="
    ).arg(elapsedSeconds, 0, 'f', 1)
     .arg(m_stats.framesSent)
     .arg(m_stats.framesReceived)
     .arg(lossRate, 0, 'f', 2)
     .arg(fecStats)
     .arg(m_stats.totalPacketsSent)
     .arg(m_stats.totalPacketsReceived)
     .arg(sendRate, 0, 'f', 2)
     .arg(receiveRate, 0, 'f', 2)
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
        // Обрезаем данные до заявленного размера (на случай дополнения нулями)
        QByteArray completeData = assembly.data.left(assembly.totalSize);
        
        emit frameAssembled(streamId, assembly.frameNumber, completeData);
        
        m_stats.framesReceived++;
        m_stats.assembliesCompleted++;
        m_stats.receivedFrames.insert(qMakePair(streamId, assembly.frameNumber));
        
        qDebug() << "Frame" << assembly.frameNumber << "successfully assembled, size:" << completeData.size() << "bytes";
    }
    
    m_streamAssemblies.remove(streamId);
}