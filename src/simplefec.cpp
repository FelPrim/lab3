#include "simplefec.h"
#include <QDebug>
#include <QDateTime>

SimpleFEC::SimpleFEC(QObject *parent) : QObject(parent)
{
    m_cleanupTimer = new QTimer(this);
    m_cleanupTimer->setInterval(30000);
    connect(m_cleanupTimer, &QTimer::timeout, this, &SimpleFEC::cleanupOldGroups);
    m_cleanupTimer->start();
}

int SimpleFEC::generateGroupId(int streamId, int groupId) const
{
    return (streamId << 16) | (groupId & 0xFFFF);
}

QByteArray SimpleFEC::encodeGroup(const QVector<QByteArray> &packets)
{
    if (packets.size() != FEC_GROUP_SIZE) {
        qWarning() << "FEC: Need exactly" << FEC_GROUP_SIZE << "packets for encoding, got" << packets.size();
        return QByteArray();
    }
    
    // Простой XOR двух пакетов - УПРОЩЕНО
    return xorPackets(packets);
}

void SimpleFEC::addPacket(int streamId, int groupId, int packetType, const QByteArray &data)
{
    int uniqueGroupId = generateGroupId(streamId, groupId);
    
    if (!m_groups.contains(uniqueGroupId)) {
        m_groups[uniqueGroupId] = FECGroup();
    }
    
    FECGroup &group = m_groups[uniqueGroupId];
    group.lastUpdateTime = QDateTime::currentMSecsSinceEpoch();
    
    // packetType: 0 = P1, 1 = P2, 2 = XOR
    if (packetType >= 0 && packetType < FEC_GROUP_SIZE) {
        if (!group.hasOriginal[packetType]) {
            group.originalPackets[packetType] = data;
            group.hasOriginal[packetType] = true;
        }
    } 
    else if (packetType == FEC_GROUP_SIZE) { // XOR packet
        if (!group.hasXor) {
            group.xorPacket = data;
            group.hasXor = true;
        }
    }
    
    // Пытаемся декодировать группу
    if (tryDecodeGroup(streamId, groupId)) {
        emit groupDecoded(streamId, groupId, group.originalPackets);
    }
}

bool SimpleFEC::tryDecodeGroup(int streamId, int groupId)
{
    int uniqueGroupId = generateGroupId(streamId, groupId);
    
    if (!m_groups.contains(uniqueGroupId)) {
        return false;
    }
    
    FECGroup &group = m_groups[uniqueGroupId];
    
    // Проверяем, есть ли все пакеты
    bool allComplete = true;
    for (bool has : group.hasOriginal) {
        if (!has) {
            allComplete = false;
            break;
        }
    }
    
    if (allComplete) {
        return true;
    }
    
    // Пытаемся восстановить потерянные пакеты
    return recoverLostPackets(streamId, groupId);
}

QVector<QByteArray> SimpleFEC::getDecodedPackets(int streamId, int groupId) const
{
    int uniqueGroupId = generateGroupId(streamId, groupId);
    
    if (m_groups.contains(uniqueGroupId)) {
        return m_groups[uniqueGroupId].originalPackets;
    }
    return QVector<QByteArray>();
}

bool SimpleFEC::isGroupComplete(int streamId, int groupId) const
{
    int uniqueGroupId = generateGroupId(streamId, groupId);
    
    if (!m_groups.contains(uniqueGroupId)) {
        return false;
    }
    
    const FECGroup &group = m_groups[uniqueGroupId];
    for (bool has : group.hasOriginal) {
        if (!has) return false;
    }
    return true;
}

void SimpleFEC::clearGroup(int streamId, int groupId)
{
    int uniqueGroupId = generateGroupId(streamId, groupId);
    m_groups.remove(uniqueGroupId);
}

void SimpleFEC::cleanupOldGroups()
{
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    qint64 timeout = 60000;
    
    auto it = m_groups.begin();
    while (it != m_groups.end()) {
        if (currentTime - it->lastUpdateTime > timeout) {
            it = m_groups.erase(it);
        } else {
            ++it;
        }
    }
}

QByteArray SimpleFEC::xorPackets(const QVector<QByteArray> &packets) const
{
    if (packets.isEmpty()) return QByteArray();
    
    int maxSize = 0;
    for (const QByteArray &packet : packets) {
        if (packet.size() > maxSize) {
            maxSize = packet.size();
        }
    }
    
    QByteArray result(maxSize, 0);
    
    for (int i = 0; i < maxSize; ++i) {
        quint8 xorByte = 0;
        for (const QByteArray &packet : packets) {
            if (i < packet.size()) {
                xorByte ^= packet[i];
            }
        }
        result[i] = xorByte;
    }
    
    return result;
}

bool SimpleFEC::recoverLostPackets(int streamId, int groupId)
{
    int uniqueGroupId = generateGroupId(streamId, groupId);
    FECGroup &group = m_groups[uniqueGroupId];
    
    // УПРОЩЕНО: только один потерянный пакет можно восстановить
    if (!group.hasXor) {
        return false; // Нет XOR пакета для восстановления
    }
    
    int lostIndex = -1;
    int foundCount = 0;
    
    // Находим индекс потерянного пакета
    for (int i = 0; i < FEC_GROUP_SIZE; ++i) {
        if (!group.hasOriginal[i]) {
            if (lostIndex != -1) {
                // Больше одного потерянного пакета - не можем восстановить
                return false;
            }
            lostIndex = i;
        } else {
            foundCount++;
        }
    }
    
    if (lostIndex == -1 || foundCount != 1) {
        return false; // Нет потерь или неправильное количество
    }
    
    // Восстанавливаем потерянный пакет
    int knownIndex = (lostIndex == 0) ? 1 : 0;
    group.originalPackets[lostIndex] = xorPackets({group.originalPackets[knownIndex], group.xorPacket});
    group.hasOriginal[lostIndex] = true;
    
    qDebug() << "FEC: Recovered packet" << lostIndex << "in group" << groupId;
    return true;
}