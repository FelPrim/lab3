#pragma once

#include <QtEndian>
#include <cstdint>
#include <QByteArray>
#include "../endianness.h"
#include "../../video_defaults.h"

#pragma pack(push, 1)

constexpr int START_PAYLOAD = DATA_PAYLOAD_SIZE - 8;
constexpr int CONTINUE_PAYLOAD = DATA_PAYLOAD_SIZE - 4; 
constexpr int END_PAYLOAD = DATA_PAYLOAD_SIZE - 4;
constexpr int XOR_PAYLOAD = XOR_PACKET_DATA_SIZE;

enum PacketType {
    START_FRAME = 0x01,
    CONTINUE_FRAME = 0x02,
    END_FRAME = 0x03
};



// ROUTE_HEADER - 12 байт
struct RouteHeader {
    uint32_t callId;
    uint32_t streamId;
    uint32_t packetSequence;
};

// Для обозначения того, что значения хранятся в network byte ordering
struct nRouteHeader{
    nuint32_t callId;
    nuint32_t streamId;
    nuint32_t packetSequence;
};

// Обычный пакет (FEC_FLAG = 0)
struct DataPacket {
    uint8_t type;  // 7 бит - тип пакета, старший бит = 0
    uint8_t payload[1187]; // данные пакета
    
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
    uint8_t xorData[1188]; // XOR данных 4 пакетов (включая их type байты)
};

union PacketContent {
    DataPacket dataPacket;
    XorPacket xorPacket;
};

union PacketHeader{
    RouteHeader   header;
    nRouteHeader nheader;
};

inline static void cast_to_nbe(PacketHeader& source){
    const uint32_t streamId = source.header.streamId;
    const uint32_t packetSequence = source.header.packetSequence;
    const uint32_t callId = source.header.callId;
    const nuint32_t nstreamId = qToBigEndian(streamId);
    const nuint32_t npacketSequence = qToBigEndian(packetSequence);
    const nuint32_t ncallId = qToBigEndian(callId);
    source.nheader.streamId = nstreamId;
    source.nheader.packetSequence = npacketSequence;
    source.nheader.callId = ncallId;
}

inline static void cast_from_nbe(PacketHeader& source){
    const nuint32_t nstreamId = source.nheader.streamId;
    const nuint32_t npacketSequence = source.nheader.packetSequence;
    const nuint32_t ncallId = source.nheader.callId;
    const uint32_t streamId = qFromBigEndian(nstreamId);
    const uint32_t packetSequence = qFromBigEndian(npacketSequence);
    const uint32_t callId = qFromBigEndian(ncallId);
    source.header.streamId = streamId;
    source.header.packetSequence = packetSequence;
    source.header.callId = callId;
}

struct NetworkPacket {
    nRouteHeader route;      // 8 байт
    PacketContent content;  // 1188 байта
    
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
    static const int ROUTE_HEADER_SIZE = 12;
    static const int XOR_PACKET_DATA_SIZE = 1188;
    
    static NetworkPacket fromByteArray(const QByteArray& data) {
        NetworkPacket packet;
        if (data.size() >= sizeof(NetworkPacket)) {
            memcpy(&packet, data.constData(), sizeof(NetworkPacket));
        }
        return packet;
    }
    
    
    static QByteArray toByteArray(const NetworkPacket& packet) {
        NetworkPacket networkOrderPacket = packet;
        return QByteArray(reinterpret_cast<const char*>(&networkOrderPacket), sizeof(NetworkPacket));
    }
    
    static NetworkPacket createDataPacket(uint32_t callId, uint32_t streamId, uint32_t sequence, uint8_t type, const QByteArray& payload) {
        NetworkPacket packet;
        
        PacketHeader route;
        route.header.callId = callId;
        route.header.streamId = streamId;
        route.header.packetSequence = sequence;
        cast_to_nbe(route);
        packet.route.callId = route.nheader.callId;
        packet.route.streamId = route.nheader.streamId;
        packet.route.packetSequence = route.nheader.packetSequence;

        packet.content.dataPacket.type = type & 0x7F;
        int copySize = qMin(payload.size(), 1187);
        if (copySize > 0) {
            memcpy(packet.content.dataPacket.payload, payload.constData(), copySize);
        }
        return packet;
    }

    static NetworkPacket createXorPacket(uint32_t callId, uint32_t streamId, uint32_t sequence, const QByteArray& xorData) {
        NetworkPacket packet;

        PacketHeader route;
        route.header.callId = callId;
        route.header.streamId = streamId;
        route.header.packetSequence = sequence;
        cast_to_nbe(route);

        packet.route.callId = route.nheader.callId;
        packet.route.streamId = route.nheader.streamId;
        packet.route.packetSequence = route.nheader.packetSequence;
        
        int copySize = qMin(xorData.size(), 1188);
        if (copySize > 0) {
            memcpy(packet.content.xorPacket.xorData, xorData.constData(), copySize);
        }
        
        packet.content.xorPacket.xorData[0] |= 0x80;
        return packet;
    }

    static QByteArray getDataPacketPayload(const NetworkPacket& packet) {
        if (packet.isXorPacket()) return QByteArray();
        return QByteArray(reinterpret_cast<const char*>(packet.content.dataPacket.payload), 1187);
    }
    
    static QByteArray getXorPacketData(const NetworkPacket& packet) {
        if (!packet.isXorPacket()) return QByteArray();
        QByteArray result = QByteArray(reinterpret_cast<const char*>(packet.content.xorPacket.xorData), 1188);
        result[0] &= 0x7f;
        return result;
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

// Добавить в network_packet.h после класса PacketProcessor
inline static int packetSequenceToDataIndex(uint32_t packetSequence) {
    int group = packetSequence / FEC_GROUP_SIZE;
    int position = packetSequence % FEC_GROUP_SIZE;
    
    // XOR-пакет не имеет индекса данных
    assert(position != FEC_GROUP_SIZE - 1);
    
    return group * FEC_DATA_PACKETS + position;
}

inline static bool isXorPacketSequence(uint32_t packetSequence) {
    return (packetSequence % FEC_GROUP_SIZE) == FEC_GROUP_SIZE - 1;
}