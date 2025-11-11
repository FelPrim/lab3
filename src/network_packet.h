#pragma once

#include <cstdint>
#include <QByteArray>

#pragma pack(push, 1)

// ROUTE_HEADER - 8 байт
struct RouteHeader {
    uint32_t streamId;
    uint32_t packetSequence;
};

// Обычный пакет (FEC_FLAG = 0)
struct DataPacket {
    uint8_t type;  // 7 бит - тип пакета, старший бит = 0
    uint8_t payload[1191]; // данные пакета
    
    bool isValidType() const {
        uint8_t packetType = type & 0x7F; // Игнорируем FEC_FLAG
        return (packetType >= 0x01 && packetType <= 0x03); // START_FRAME, CONTINUE_FRAME, END_FRAME
    }
    
    uint8_t getType() const {
        return type & 0x7F; // Возвращает тип без FEC_FLAG
    }
};

// XOR пакет (FEC_FLAG = 1)  
struct XorPacket {
    uint8_t xorData[1192]; // XOR данных 4 пакетов (включая их type байты)
};

union PacketContent {
    DataPacket dataPacket;
    XorPacket xorPacket;
};

struct NetworkPacket {
    RouteHeader route;      // 8 байт
    PacketContent content;  // 1192 байта
    
    bool isXorPacket() const {
        return (content.dataPacket.type & 0x80) != 0;
    }
    
    const DataPacket* asDataPacket() const {
        if (isXorPacket()) return nullptr;
        return &content.dataPacket;
    }
    
    DataPacket* asDataPacket() {
        if (isXorPacket()) return nullptr;
        return &content.dataPacket;
    }
    
    const XorPacket* asXorPacket() const {
        if (!isXorPacket()) return nullptr;
        return &content.xorPacket;
    }
    
    XorPacket* asXorPacket() {
        if (!isXorPacket()) return nullptr;
        return &content.xorPacket;
    }
};

#pragma pack(pop)

class PacketProcessor
{
public:
    static const int ROUTE_HEADER_SIZE = 8;
    static const int XOR_PACKET_DATA_SIZE = 1192;
    
    static NetworkPacket fromByteArray(const QByteArray& data) {
        NetworkPacket packet;
        if (data.size() >= sizeof(NetworkPacket)) {
            memcpy(&packet, data.constData(), sizeof(NetworkPacket));
        }
        return packet;
    }
    
    static QByteArray toByteArray(const NetworkPacket& packet) {
        return QByteArray(reinterpret_cast<const char*>(&packet), sizeof(NetworkPacket));
    }
    
    static NetworkPacket createDataPacket(int streamId, int sequence, uint8_t type, const QByteArray& payload) {
        NetworkPacket packet;
        packet.route.streamId = streamId;
        packet.route.packetSequence = sequence;
        packet.content.dataPacket.type = type & 0x7F; // FEC_FLAG = 0
        int copySize = qMin(payload.size(), 1191);
        if (copySize > 0) {
            memcpy(packet.content.dataPacket.payload, payload.constData(), copySize);
        }
        return packet;
    }
    
    static NetworkPacket createXorPacket(int streamId, int sequence, const QByteArray& xorData) {
        NetworkPacket packet;
        packet.route.streamId = streamId;
        packet.route.packetSequence = sequence;
        
        // Устанавливаем FEC_FLAG = 1 в первом байте
        packet.content.dataPacket.type = 0x80;
        
        int copySize = qMin(xorData.size(), 1192);
        if (copySize > 0) {
            memcpy(packet.content.xorPacket.xorData, xorData.constData(), copySize);
        }
        return packet;
    }
    
    static QByteArray getDataPacketPayload(const NetworkPacket& packet) {
        if (packet.isXorPacket()) return QByteArray();
        return QByteArray(reinterpret_cast<const char*>(packet.content.dataPacket.payload), 1191);
    }
    
    static QByteArray getXorPacketData(const NetworkPacket& packet) {
        if (!packet.isXorPacket()) return QByteArray();
        return QByteArray(reinterpret_cast<const char*>(packet.content.xorPacket.xorData), 1192);
    }
    
    // Получить тип пакета (для обычных пакетов)
    static uint8_t getPacketType(const NetworkPacket& packet) {
        if (packet.isXorPacket()) return 0;
        return packet.content.dataPacket.type & 0x7F;
    }
    
    // Проверить валидность типа пакета
    static bool isValidPacketType(const NetworkPacket& packet) {
        if (packet.isXorPacket()) return true; // XOR пакеты всегда валидны
        return packet.content.dataPacket.isValidType();
    }
};
