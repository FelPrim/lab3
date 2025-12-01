#include "fecbuffer.h"
#include "network_packet.h"
#include <QDebug>

FecBuffer::FecBuffer(int streamId, QObject *parent)
    : QObject(parent), m_streamId(streamId), m_recoveredCount(0)
{
}

bool FecBuffer::addPacket(const NetworkPacket &packet)
{
    // Получаем заголовок в host byte order
    PacketHeader header;
    memcpy(&header, &packet.route, sizeof(PacketHeader));
    cast_from_nbe(header);
    
    uint32_t packetSequence = header.header.packetSequence;
    int groupId = packetSequence / FEC_GROUP_SIZE;
    int position = packetSequence % FEC_GROUP_SIZE;
    
    qDebug() << "FecBuffer: Adding packet - group:" << groupId << "position:" << position;
    
    // Создаем группу если не существует
    if (!m_groups.contains(groupId)) {
        FecGroup newGroup;
        newGroup.groupId = groupId;
        newGroup.timestamp = QDateTime::currentMSecsSinceEpoch();
        newGroup.packets.resize(FEC_TOTAL_PACKETS);
        newGroup.received.resize(FEC_TOTAL_PACKETS, false);
        m_groups[groupId] = newGroup;
    }
    
    FecGroup &group = m_groups[groupId];
    
    // Сохраняем данные пакета
    QByteArray packetData;
    if (packet.isXorPacket()) {
        // Для XOR пакетов используем getXorPacketData
        packetData = PacketProcessor::getXorPacketData(packet);
        // Добавляем обратно FEC флаг для хранения
        if (!packetData.isEmpty()) {
            packetData[0] |= 0x80;
        }
    } else {
        // Для обычных пакетов создаем данные вручную
        packetData.resize(1188);
        const DataPacket* dataPacket = packet.asDataPacket();
        if (dataPacket) {
            packetData[0] = dataPacket->type;
            memcpy(packetData.data() + 1, dataPacket->payload, 1187);
        }
    }
    
    if (!packetData.isEmpty()) {
        group.packets[position] = packetData;
        group.received[position] = true;
        
        // Если это XOR пакет (position = 4), пытаемся восстановить
        if (position == FEC_DATA_PACKETS) {
            tryRecoverLostPackets();
        }
        
        // Добавляем оригинальный пакет в список готовых
        m_readyPackets.append(packet);
        emit packetReady(packet);
        
        // Если пакет данных и группа уже может быть восстановлена
        if (position < FEC_DATA_PACKETS && group.canRecover()) {
            tryRecoverLostPackets();
        }
    }
    
    return true;
}

int FecBuffer::calculateGroupId(uint32_t packetSequence) const
{
    return packetSequence / FEC_GROUP_SIZE;
}

int FecBuffer::calculatePositionInGroup(uint32_t packetSequence) const
{
    return packetSequence % FEC_GROUP_SIZE;
}

void FecBuffer::tryRecoverLostPackets()
{
    auto it = m_groups.begin();
    while (it != m_groups.end()) {
        FecGroup &group = it.value();
        
        if (group.canRecover()) {
            int missingIndex = group.getMissingIndex();
            if (missingIndex != -1) {
                // Восстанавливаем пакет
                uint8_t recoveredData[1188] = {0};
                
                // Начинаем с XOR пакета (индекс 4)
                if (group.received[FEC_DATA_PACKETS] && !group.packets[FEC_DATA_PACKETS].isEmpty()) {
                    QByteArray xorData = group.packets[FEC_DATA_PACKETS];
                    if (!xorData.isEmpty()) {
                        xorData[0] &= 0x7F; // Сбрасываем FEC флаг
                        memcpy(recoveredData, xorData.constData(), xorData.size());
                    }
                }
                
                // XOR со всеми полученными пакетами данных
                for (int i = 0; i < FEC_DATA_PACKETS; ++i) {
                    if (i != missingIndex && group.received[i] && !group.packets[i].isEmpty()) {
                        const QByteArray &packetData = group.packets[i];
                        const uint8_t *data = reinterpret_cast<const uint8_t*>(packetData.constData());
                        
                        for (int j = 0; j < 1188; ++j) {
                            recoveredData[j] ^= data[j];
                        }
                    }
                }
                
                // Сбрасываем FEC флаг в первом байте
                recoveredData[0] &= 0x7F;
                
                // Создаем восстановленный пакет
                uint32_t packetSequence = group.groupId * FEC_GROUP_SIZE + missingIndex;
                uint8_t packetType = recoveredData[0];
                QByteArray payload(reinterpret_cast<const char*>(recoveredData + 1), 1187);
                
                // Нужно получить callId из группы (берем из первого полученного пакета)
                uint32_t callId = 0;
                for (int i = 0; i < FEC_TOTAL_PACKETS; ++i) {
                    if (group.received[i]) {
                        // TODO: Извлечь callId из заголовка пакета
                        break;
                    }
                }
                
                NetworkPacket recoveredPacket = PacketProcessor::createDataPacket(
                    callId, m_streamId, packetSequence, packetType, payload);
                
                // Сохраняем восстановленный пакет в группе
                group.packets[missingIndex] = QByteArray(reinterpret_cast<const char*>(recoveredData), 1188);
                group.received[missingIndex] = true;
                
                // Добавляем в список готовых пакетов
                m_readyPackets.append(recoveredPacket);
                m_recoveredCount++;
                
                emit packetRecovered(packetSequence);
                emit packetReady(recoveredPacket);
                
                qDebug() << "FecBuffer: Recovered packet - sequence:" << packetSequence;
            }
        }
        
        ++it;
    }
}

QList<NetworkPacket> FecBuffer::getReadyPackets()
{
    QList<NetworkPacket> ready = m_readyPackets;
    m_readyPackets.clear();
    return ready;
}

void FecBuffer::cleanup(qint64 maxAgeMs)
{
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    auto it = m_groups.begin();
    
    while (it != m_groups.end()) {
        if (currentTime - it.value().timestamp > maxAgeMs) {
            qDebug() << "FecBuffer: Cleaning up old group" << it.key();
            it = m_groups.erase(it);
        } else {
            ++it;
        }
    }
}