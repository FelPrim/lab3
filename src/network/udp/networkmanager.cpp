#include "networkmanager.h"
#include "udpmanager.h"
#include <QDataStream>
#include <QDebug>
#include <QNetworkInterface>
#include <QVariant>
#include "network_packet.h"
#include "../../video_defaults.h"
#include <cstdint>    

#pragma pack(push, 1)
struct StartData{
    nuint32_t frameNumber;
    nuint32_t frameSize;
    uint8_t payload[START_PAYLOAD];
};

#pragma pack(push, 1)
struct ContinueData{
    nuint32_t frameNumber;
    uint8_t payload[CONTINUE_PAYLOAD];
};

#pragma pack(push, 1)
struct EndData{
    nuint32_t frameNumber;
    uint8_t payload[END_PAYLOAD];
};

constexpr int StartDataSize = sizeof(StartData);
constexpr int ContinueDataSize = sizeof(ContinueData);
constexpr int EndDataSize = sizeof(EndData);
constexpr int XorPacketSize = sizeof(XorPacket)-1;

static_assert(StartDataSize == ContinueDataSize);
static_assert(EndDataSize == XorPacketSize);
static_assert(StartDataSize == EndDataSize);

#pragma pack(push, 1)
struct TypedPacket{
    PacketHeader route;
    union {
        struct XorPacket xorpacket;
        struct NotXorPack{
            uint8_t type;
            union {
                struct StartData start_;
                struct ContinueData continue_;
                struct EndData end_;
            } packet;
        } datapacket;
    } content;
};

constexpr int TypedPacketSize = sizeof(TypedPacket);
static_assert(TypedPacketSize == 1200);

NetworkManager::NetworkManager(int streamId, QObject *parent)
    : QObject(parent)
    , m_streamId(streamId)
    , m_serverAddress(DEFAULT_ECHO_SERVER_ADDRESS)
    , m_serverPort(DEFAULT_ECHO_SERVER_PORT)
    , m_fecBuffer(new FecBuffer(streamId, this))
    , m_packetBuffer(new PacketGroupBuffer(streamId, this))
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
    // Передаем сигнал дальше

    emit frameAssembled(streamId, frameNumber, frameData);
}

void NetworkManager::onPacketRecovered(uint32_t packetSequence)
{
    m_stats.packetsRecoveredByFEC++;
    m_stats.fecGroupsRecovered++;
    
    qDebug() << "NetworkManager: Packet recovered by FEC, sequence:" << packetSequence;
}

void NetworkManager::sendVideoFrame(int frameNumber, const QByteArray& frameData)
{
    if (!m_initialized || !m_udpManager || !m_sendingEnabled) return;
    QByteArray firstBytes = frameData.left(qMin(20, frameData.size()));
    qDebug() << "Sending frame" << frameNumber 
         << "size:" << frameData.size() 
         << "first packet:" << m_packetSequence;
      //   << "firstBytes(hex):" << firstBytes.toHex();
        
    // Логируем начало фрейма для отладки
    if (frameData.size() >= 20) {
        QByteArray header = frameData.left(20);
       // qDebug() << "Frame header (hex):" << header.toHex();
        
        // Проверяем наличие SPS/PPS
        if (frameData.size() > 4) {
            for (int i = 0; i < qMin(frameData.size() - 4, 100); i++) {
                if (frameData[i] == 0x00 && frameData[i+1] == 0x00 && 
                    frameData[i+2] == 0x00 && frameData[i+3] == 0x01) {
                    uint8_t nal_type = static_cast<uint8_t>(frameData[i+4]) & 0x1F;
                    if (nal_type == 7 || nal_type == 8) {
                        qDebug() << "Found NAL unit at offset" << i 
                                << "type:" << nal_type 
                                << (nal_type == 7 ? "(SPS)" : "(PPS)");
                    }
                }
            }
        }
    }
        
    try {
        // m_callId, m_streamId, frameNumber, frameData
        frameSize = frameData.size();
        this->frameNumber = frameNumber;
        const char* arg = frameData.data();
        if (frameSize <= START_PAYLOAD){
            // фрейм помещается в один пакет
            sendPacketNewProtocol(arg, frameSize, START_FRAME);
        }
        else{
            // frameSize = START_PAYLOAD + N*CONTINUE_PAYLOAD + END_PAYLOAD*k, k in (0, 1]
            const uint32_t size_without_first = frameSize - START_PAYLOAD;
            uint32_t ContinueQuan = size_without_first / CONTINUE_PAYLOAD;
            uint32_t EndPayloadOrZero = size_without_first % CONTINUE_PAYLOAD;

            if (EndPayloadOrZero == 0){
                // frameSize > START_PAYLOAD -> size_without_first > 0 -> EndPayloadOrZero и ContinueQuan не могут быть одновременно = 0
                assert(ContinueQuan != 0);
                ContinueQuan -= 1;
                EndPayloadOrZero = END_PAYLOAD;
            }

            sendPacketNewProtocol(arg, START_PAYLOAD, START_FRAME);
            arg += START_PAYLOAD; 
            for (uint32_t i = 0; i < ContinueQuan; ++i){
                sendPacketNewProtocol(arg, CONTINUE_PAYLOAD, CONTINUE_FRAME);
                arg += CONTINUE_PAYLOAD; 
            }
            sendPacketNewProtocol(arg, EndPayloadOrZero, END_FRAME);
        }
        
        m_stats.framesSent++;
        
    } catch (const std::exception &e) {
        qDebug() << "Send video frame failed:" << e.what();
        emit errorOccurred(QString("Send video frame failed: %1").arg(e.what()));
    }
}

void NetworkManager::sendPacketNewProtocol(const char *data, const uint32_t size, PacketType type)
{
    if (!m_udpManager) return;
    
    TypedPacket packet = {0};    
    memset(&packet, 0, sizeof(packet));
    PacketHeader& header = packet.route;
    header.header.callId = m_callId;
    header.header.streamId = m_streamId;
    header.header.packetSequence = m_packetSequence;
    cast_to_nbe(header);
    switch (type){
        case START_FRAME:
        {
            packet.content.datapacket.type = START_FRAME;
            struct StartData& start = packet.content.datapacket.packet.start_;
            start.frameNumber= qToBigEndian(frameNumber);
            start.frameSize = qToBigEndian(frameSize);
            memcpy(&start.payload, data, size);
            qDebug() << "DBG send Start packet:" << m_packetSequence 
            <<"frameNumber:" << frameNumber
         << "frameSize:" << frameSize;
   //      << "start(13):" << QByteArray(data, 13).toHex();
            break;
        }
        case CONTINUE_FRAME:
        {
            packet.content.datapacket.type = CONTINUE_FRAME;
            struct ContinueData& continue_ = packet.content.datapacket.packet.continue_;
            continue_.frameNumber = qToBigEndian(frameNumber);
            memcpy(&continue_.payload, data, size);
        qDebug() << "DBG send Continue packet:" << m_packetSequence 
            <<"frameNumber:" << frameNumber;
       //  << "continue(12):" << QByteArray(data, 12).toHex();
            break;
        }
        case END_FRAME:
        {
            packet.content.datapacket.type = END_FRAME;
            struct EndData& end = packet.content.datapacket.packet.end_;
            end.frameNumber = qToBigEndian(frameNumber);
            memcpy(&end.payload, data, size);
            qDebug() << "DBG send End packet:" << m_packetSequence 
            <<"frameNumber:" << frameNumber;
        // << "end(12):" << QByteArray(data, 12).toHex();
            break;
        }
    }
    
    m_udpManager->sendPacket(QByteArray::fromRawData((const char*)&packet, sizeof(packet)), m_serverAddress, m_serverPort);
    m_packetSequence++;

    m_stats.totalPacketsSent++;

    QByteArray dataPart = QByteArray::fromRawData((const char*)&packet.content, sizeof(packet.content));
    
    if (m_fecSendBufferCount < 4) {
        memcpy(m_fecSendBuffer[m_fecSendBufferCount], dataPart.constData(), sizeof(packet.content));
        m_fecSendBufferCount++;
    }

    if (m_fecSendBufferCount >= 4) {
        uint8_t xorData[XOR_PAYLOAD] = {0};
        for (int i = 0; i < 4; i++) {
            #pragma omp simd
            for (int j = 0; j < XOR_PAYLOAD; j++) {
                xorData[j] ^= m_fecSendBuffer[i][j];
            }
        }
        xorData[0] |= 0x80;

        int xorSequence = m_packetSequence; 
        
        NetworkPacket xorPacket = {0}; // = PacketProcessor::createXorPacket(m_callId, m_streamId, xorSequence, xorDataArray);
        static_assert(sizeof(PacketHeader) == sizeof(nRouteHeader));
        PacketHeader& route = (PacketHeader&) xorPacket.route;
        route.header.callId = m_callId;
        route.header.streamId = m_streamId;
        route.header.packetSequence = m_packetSequence;
        cast_to_nbe(route);
        memcpy(&xorPacket.content.xorPacket, xorData, XOR_PAYLOAD);

        QByteArray xorDatagram = QByteArray::fromRawData((const char*)&xorPacket, MAX_PACKET_SIZE);//PacketProcessor::toByteArray(xorPacket);
        m_udpManager->sendPacket(xorDatagram, m_serverAddress, m_serverPort);

        m_stats.totalPacketsSent++;
        m_packetSequence++;
     //   qDebug() << "NetworkManager: Sent XOR packet, sequence:" << xorSequence;

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
    
   // qDebug() << "=== PACKET RECEIVED ===";
   // qDebug() << "Expected callId:" << m_callId << "streamId:" << m_streamId;
   // qDebug() << "Received callId:" << packetCallId << "streamId:" << packetStreamId;
   // qDebug() << "Packet size:" << data.size();

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
}

void NetworkManager::updateReceiveStats(int packets, int bytes)
{
    m_stats.totalPacketsReceived += packets;
}

void NetworkManager::printStatistics()
{
    double elapsedSeconds = m_operationTimer.elapsed() / 1000.0;
    
    if (elapsedSeconds == 0) return;
    
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
     .arg(m_stats.totalPacketsSent)
     .arg(m_stats.totalPacketsReceived)
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
