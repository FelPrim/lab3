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
    
    // Если мы еще не очищали пакеты, принимаем все
    if (m_lastCleanedSequence == 0) {
        return true;
    }
    
    // Если packetSequence меньше последнего очищенного (не <=!), пакет слишком старый
    if (packetSequence < m_lastCleanedSequence) {
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

    if (!PacketProcessor::isValidPacketType(packet)) {
        qDebug() << "FecBuffer: Invalid packet type";
        return false;
    }

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
    
   // qDebug() << "FecBuffer: Adding packet - group:" << groupId 
   //          << "position:" << position << "isXor:" << packet.isXorPacket();
    
    // Шаг 2: Создаем группу если не существует
    if (!m_groups.contains(groupId)) {
        FecGroup newGroup;
        newGroup.groupId = groupId;
        newGroup.callId = header.header.callId;
        newGroup.streamId = header.header.streamId;
        newGroup.creationTime = currentTime;
        newGroup.lastUpdateTime = currentTime;
        
        // ВАЖНО: Явно инициализируем vectors
        newGroup.packets.resize(FEC_TOTAL_PACKETS);
        newGroup.received.resize(FEC_TOTAL_PACKETS, false);
        
        m_groups[groupId] = newGroup;
   //     qDebug() << "FecBuffer: Created new group" << groupId 
   //              << "packets size:" << newGroup.packets.size()
   //              << "received size:" << newGroup.received.size();
    }
    
    FecGroup &group = m_groups[groupId];
    group.lastUpdateTime = currentTime;
    
    // ВАЖНО: Проверяем границы массива
    if (position < 0 || position >= FEC_TOTAL_PACKETS) {
        qWarning() << "FecBuffer: Invalid position" << position 
                   << "for FEC_TOTAL_PACKETS =" << FEC_TOTAL_PACKETS;
        return false;
    }
    
    // Шаг 3: Проверка на дубликат
    if (position < group.received.size() && group.received[position]) {
   //     qDebug() << "FecBuffer: Duplicate packet, ignoring. Sequence:" << packetSequence;
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
        
       
        // Создаем packetData правильного размера
        packetData.resize(XOR_PACKET_DATA_SIZE);
        char* buffer = packetData.data();
        
        // Копируем тип пакета (без FEC бита)
        buffer[0] = dataPacket->type & 0x7F;
        
        // Копируем payload
     //   qDebug() << "FecBuffer: dataPacket payload pointer:" << (void*)dataPacket->payload;
        memcpy(buffer + 1, dataPacket->payload, DATA_PAYLOAD_SIZE);
     //   qDebug() << "FecBuffer: The end of memcpy, packetData size:" << packetData.size();
    }

    // Шаг 5: Сохраняем данные в группу
    if (!packetData.isEmpty()) {
        // Проверяем, что вектор packets имеет достаточный размер
        if (position >= group.packets.size()) {
            qWarning() << "FecBuffer: packets vector too small! position:" << position
                       << "size:" << group.packets.size();
            // Исправляем ситуацию
            group.packets.resize(position + 1);
            group.received.resize(position + 1, false);
        }
        
        // Сохраняем данные пакета
        if (position < group.packets.size()) {
            group.packets[position] = packetData;
       //     qDebug() << "FecBuffer: Assignment successful";
        } else {
            qCritical() << "FecBuffer: CRITICAL - position out of bounds!";
            qCritical() << "  position:" << position;
            qCritical() << "  packets.size:" << group.packets.size();
            qCritical() << "  groupId:" << groupId;
            // Попробуем исправить
            group.packets.resize(position + 1);
            group.received.resize(position + 1, false);
            group.packets[position] = packetData;
        }
        group.received[position] = true;
        
   //     qDebug() << "FecBuffer: Stored packet in group, position:" << position
   //              << "packetData size:" << packetData.size()
   //              << "group.packets[" << position << "] size:" << group.packets[position].size();
   //     
        // Шаг 6: Отправляем только пакеты данных (не XOR) в PacketGroupBuffer
        if (!packet.isXorPacket()) {
            forwardPacketToGroupBuffer(packet);
        //    qDebug() << "FecBuffer: Forwarding data packet to PacketGroupBuffer, sequence:" 
       //              << packetSequence;
        } else {
           // qDebug() << "FecBuffer: XOR packet received, NOT forwarding, sequence:" 
           //          << packetSequence;
        }
        
        // Шаг 7: Проверяем возможность восстановления
        if (group.canRecover()) {
            qDebug() << "FecBuffer: Group" << groupId << "can be recovered, attempting recovery";
            int missingIndex = group.getMissingIndex();
            if (missingIndex != -1) {
                recoverPacketInGroup(groupId, missingIndex, currentTime);
            }
        }
    } else {
        qWarning() << "FecBuffer: packetData is empty!";
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
        qCritical() << "FecBuffer: Invalid recovered packet type:" << packetType;
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
  //  qDebug() << "FecBuffer::forwardPacketToGroupBuffer - START";
 //   qDebug() << "  this:" << (void*)this;
  //  qDebug() << "  m_streamId:" << m_streamId;
  //  qDebug() << "  m_packetGroupBuffer:" << (void*)m_packetGroupBuffer;
    
    // Проверяем валидность packet
   // qDebug() << "  packet.isXorPacket():" << packet.isXorPacket();
    
    if (!m_packetGroupBuffer) {
        qCritical() << "FecBuffer: CRITICAL - m_packetGroupBuffer is NULL!";
        qCritical() << "  Cannot forward packet, streamId:" << m_streamId;
        return;
    }
    
    // Проверяем, что m_packetGroupBuffer указывает на валидный объект
  //  qDebug() << "  m_packetGroupBuffer streamId:" << m_packetGroupBuffer->getStreamId();
    
    try {
    //    qDebug() << "  Calling m_packetGroupBuffer->addPacket()";
        m_packetGroupBuffer->addPacket(packet);
   //     qDebug() << "  m_packetGroupBuffer->addPacket() completed";
    } catch (const std::exception& e) {
        qCritical() << "FecBuffer: Exception in addPacket:" << e.what();
    } catch (...) {
        qCritical() << "FecBuffer: Unknown exception in addPacket";
    }
    
 //   qDebug() << "FecBuffer::forwardPacketToGroupBuffer - END";
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