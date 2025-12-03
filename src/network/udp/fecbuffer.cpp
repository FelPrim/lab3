#include "fecbuffer.h"
#include "packetgroupbuffer.h"
#include "network_packet.h"
#include <QDebug>
#include <cassert>

FecBuffer::FecBuffer(int streamId, QObject *parent)
    : QObject(parent), m_streamId(streamId), m_packetGroupBuffer(nullptr), 
      m_recoveredCount(0), m_lastCleanedSequence(0)
{
}

void FecBuffer::setPacketGroupBuffer(PacketGroupBuffer* packetGroupBuffer)
{
    m_packetGroupBuffer = packetGroupBuffer;
    qDebug() << "FecBuffer: PacketGroupBuffer pointer set for stream" << m_streamId;
}

bool FecBuffer::shouldProcessPacket(uint32_t packetSequence, qint64 currentTime)
{
    Q_UNUSED(currentTime);
    QMutexLocker locker(&m_sequenceMutex);
    
    // Если packetSequence меньше или равен последнему очищенному, пакет слишком старый
    if (packetSequence <= m_lastCleanedSequence) {
        qDebug() << "FecBuffer: Packet too old, sequence:" << packetSequence 
                 << "last cleaned:" << m_lastCleanedSequence;
        return false;
    }
    
    return true;
}

void FecBuffer::updateLastCleanedSequence(uint32_t packetSequence)
{
    QMutexLocker locker(&m_sequenceMutex);
    if (packetSequence > m_lastCleanedSequence) {
        m_lastCleanedSequence = packetSequence;
    }
}

bool FecBuffer::addPacket(const NetworkPacket &packet)
{
    PacketHeader header;
    memcpy(&header, &packet.route, sizeof(PacketHeader));
    cast_from_nbe(header);
    
    uint32_t packetSequence = header.header.packetSequence;
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    
    // Шаг 1: Проверка на слишком старый пакет
    if (!shouldProcessPacket(packetSequence, currentTime)) {
        qDebug() << "FecBuffer: Ignoring too old packet, sequence:" << packetSequence;
        return false;
    }
    
    int groupId = packetSequence / FEC_GROUP_SIZE;
    int position = packetSequence % FEC_GROUP_SIZE;
    
    qDebug() << "FecBuffer: Adding packet - group:" << groupId 
             << "position:" << position << "isXor:" << packet.isXorPacket();
    
    // Шаг 2: Создаем группу если не существует
    if (!m_groups.contains(groupId)) {
        FecGroup newGroup;
        newGroup.groupId = groupId;
        newGroup.callId = header.header.callId;
        newGroup.streamId = header.header.streamId;
        newGroup.creationTime = currentTime;
        newGroup.lastUpdateTime = currentTime;
        m_groups[groupId] = newGroup;
    }
    
    FecGroup &group = m_groups[groupId];
    group.lastUpdateTime = currentTime;
    
    // Шаг 3: Проверка на дубликат
    if (group.received[position]) {
        qDebug() << "FecBuffer: Duplicate packet, ignoring. Sequence:" << packetSequence;
        return false;
    }
    
    // Шаг 4: Сохраняем данные пакета
    QByteArray packetData;
    if (packet.isXorPacket()) {
        packetData = PacketProcessor::getXorPacketData(packet);
        if (packetData.size() != XOR_PACKET_DATA_SIZE) {
            qDebug() << "FecBuffer: Invalid XOR packet size:" << packetData.size()
                     << "expected:" << XOR_PACKET_DATA_SIZE;
            return false;
        }
    } else {
        const DataPacket* dataPacket = packet.asDataPacket();
        if (!dataPacket) {
            qDebug() << "FecBuffer: Invalid data packet";
            return false;
        }
        
        packetData.resize(XOR_PACKET_DATA_SIZE);
        char* buffer = packetData.data(); // Сохраняем указатель
        buffer[0] = dataPacket->type & 0x7F;
        memcpy(buffer + 1, dataPacket->payload, DATA_PAYLOAD_SIZE);
    }
    
    if (!packetData.isEmpty()) {
        // Сохраняем время получения пакета
        group.packets[position] = packetData;
        group.received[position] = true;
        
        // Шаг 5: Отправляем только пакеты данных (не XOR) в PacketGroupBuffer
        if (!packet.isXorPacket()) {
            forwardPacketToGroupBuffer(packet);
            qDebug() << "FecBuffer: Forwarding data packet to PacketGroupBuffer, sequence:" 
                     << packetSequence;
        } else {
            qDebug() << "FecBuffer: XOR packet received, NOT forwarding, sequence:" 
                     << packetSequence;
        }
        
        // Шаг 6: Проверяем возможность восстановления и восстанавливаем при необходимости
        if (group.canRecover()) {
            qDebug() << "FecBuffer: Group" << groupId << "can be recovered, attempting recovery";
            int missingIndex = group.getMissingIndex();
            if (missingIndex != -1) {
                recoverPacketInGroup(groupId, missingIndex, currentTime);
            }
        }
    }
    
    return true;
}

bool FecBuffer::recoverPacketInGroup(int groupId, int missingIndex, qint64 currentTime)
{
    if (!m_groups.contains(groupId)) {
        qWarning() << "FecBuffer: Group" << groupId << "not found for recovery";
        return false;
    }
    
    FecGroup &group = m_groups[groupId];
    
    qDebug() << "FecBuffer: Recovering packet in group" << groupId 
             << "missing index:" << missingIndex;
    
    // Проверяем, что XOR-пакет есть и не пуст
    if (!group.received[FEC_DATA_PACKETS] || group.packets[FEC_DATA_PACKETS].isEmpty()) {
        qWarning() << "FecBuffer: No XOR packet available for recovery";
        return false;
    }
    
    QByteArray xorData = group.packets[FEC_DATA_PACKETS];
    if (xorData.size() != XOR_PACKET_DATA_SIZE) {
        qWarning() << "FecBuffer: XOR packet has invalid size:" << xorData.size();
        return false;
    }
    
    // Восстанавливаем пакет
    uint8_t recoveredData[XOR_PACKET_DATA_SIZE] = {0};
    
    // Начинаем с XOR пакета
    memcpy(recoveredData, xorData.constData(), XOR_PACKET_DATA_SIZE);
    
    // XOR со всеми полученными пакетами данных
    for (int i = 0; i < FEC_DATA_PACKETS; ++i) {
        if (i != missingIndex && group.received[i] && !group.packets[i].isEmpty()) {
            const QByteArray &packetData = group.packets[i];
            if (packetData.size() != XOR_PACKET_DATA_SIZE) {
                qWarning() << "FecBuffer: Packet" << i << "has invalid size:" << packetData.size();
                continue;
            }
            
            const uint8_t *data = reinterpret_cast<const uint8_t*>(packetData.constData());
            for (int j = 0; j < XOR_PACKET_DATA_SIZE; ++j) {
                recoveredData[j] ^= data[j];
            }
        }
    }
    
    // Проверяем, что восстановленный тип пакета валиден
    uint8_t packetType = recoveredData[0] & 0x7F;
    if (packetType < 0x01 || packetType > 0x03) {
        qWarning() << "FecBuffer: Invalid recovered packet type:" << packetType;
        return false;
    }
    
    // Создаем восстановленный пакет
    uint32_t packetSequence = group.groupId * FEC_GROUP_SIZE + missingIndex;
    QByteArray payload(reinterpret_cast<const char*>(recoveredData + 1), DATA_PAYLOAD_SIZE);
    
    NetworkPacket recoveredPacket = PacketProcessor::createDataPacket(
        group.callId, group.streamId, packetSequence, packetType, payload);
    
    // Сохраняем восстановленный пакет в группе
    group.packets[missingIndex] = QByteArray(reinterpret_cast<const char*>(recoveredData), XOR_PACKET_DATA_SIZE);
    group.received[missingIndex] = true;
    group.lastUpdateTime = currentTime;
    
    // Отправляем восстановленный пакет в PacketGroupBuffer
    forwardPacketToGroupBuffer(recoveredPacket);
    m_recoveredCount++;
    
    emit packetRecovered(packetSequence);
    
    qDebug() << "FecBuffer: Successfully recovered packet - sequence:" << packetSequence
             << "type:" << packetType;
    
    return true;
}

void FecBuffer::forwardPacketToGroupBuffer(const NetworkPacket &packet)
{
    if (m_packetGroupBuffer) {
        m_packetGroupBuffer->addPacket(packet);
    } else {
        qWarning() << "FecBuffer: PacketGroupBuffer pointer is not set, cannot forward packet";
    }
}
void FecBuffer::cleanup(qint64 maxAgeMs)
{
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    auto it = m_groups.begin();
    int removedCount = 0;
    uint32_t maxCleanedSequence = 0;
    
    while (it != m_groups.end()) {
        FecGroup &group = it.value();
        
        // Удаляем группу, если она не обновлялась дольше maxAgeMs
        if ((currentTime - group.lastUpdateTime) > maxAgeMs) {
            // Максимальный возможный sequence в этой группе
            uint32_t maxPossibleSeqInGroup = (group.groupId + 1) * FEC_GROUP_SIZE - 1;
            
            if (maxPossibleSeqInGroup > maxCleanedSequence) {
                maxCleanedSequence = maxPossibleSeqInGroup;
            }
            
            qDebug() << "FecBuffer: Cleaning up old group" << it.key() 
                     << "lastUpdate:" << group.lastUpdateTime
                     << "max possible sequence:" << maxPossibleSeqInGroup;
            it = m_groups.erase(it);
            removedCount++;
        } else {
            ++it;
        }
    }
    
    // Обновляем последний очищенный sequence
    if (maxCleanedSequence > 0) {
        updateLastCleanedSequence(maxCleanedSequence);
    }
    
    if (removedCount > 0) {
        qDebug() << "FecBuffer: Removed" << removedCount << "old groups, last cleaned sequence:" 
                 << m_lastCleanedSequence;
    }
}