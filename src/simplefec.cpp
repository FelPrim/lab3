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

QByteArray SimpleFEC::encodeXORGroup(const QVector<QByteArray> &packets)
{
    if (packets.size() != XOR_FEC_K) {
        qWarning() << "XOR FEC: Need exactly" << XOR_FEC_K << "packets for encoding, got" << packets.size();
        return QByteArray();
    }
    
    return xorPackets(packets);
}

void SimpleFEC::addPacket(int streamId, int groupId, int packetIndex, const QByteArray &data)
{
    int uniqueGroupId = generateGroupId(streamId, groupId);
    
    if (!m_groups.contains(uniqueGroupId)) {
        m_groups[uniqueGroupId] = XORFECGroup();
    }
    
    XORFECGroup &group = m_groups[uniqueGroupId];
    group.lastUpdateTime = QDateTime::currentMSecsSinceEpoch();
    
    // packetIndex: 0-3 = данные, 4 = XOR пакет
    if (packetIndex >= 0 && packetIndex < XOR_FEC_K) {
        if (!group.hasData[packetIndex]) {
            group.dataPackets[packetIndex] = data;
            group.hasData[packetIndex] = true;
        }
    } 
    else if (packetIndex == XOR_FEC_K) { // XOR packet
        if (!group.hasXor) {
            group.xorPacket = data;
            group.hasXor = true;
        }
    }
    
    // Пытаемся декодировать группу
    if (tryDecodeGroup(streamId, groupId)) {
        emit groupDecoded(streamId, groupId, group.dataPackets);
    }
}

bool SimpleFEC::tryDecodeGroup(int streamId, int groupId)
{
    int uniqueGroupId = generateGroupId(streamId, groupId);
    
    if (!m_groups.contains(uniqueGroupId)) {
        return false;
    }
    
    XORFECGroup &group = m_groups[uniqueGroupId];
    
    // Проверяем, есть ли все пакеты
    bool allComplete = true;
    for (bool has : group.hasData) {
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
        return m_groups[uniqueGroupId].dataPackets;
    }
    return QVector<QByteArray>();
}

bool SimpleFEC::isGroupComplete(int streamId, int groupId) const
{
    int uniqueGroupId = generateGroupId(streamId, groupId);
    
    if (!m_groups.contains(uniqueGroupId)) {
        return false;
    }
    
    const XORFECGroup &group = m_groups[uniqueGroupId];
    for (bool has : group.hasData) {
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
    XORFECGroup &group = m_groups[uniqueGroupId];
    
    if (!group.hasXor) {
        return false; // Нет XOR пакета для восстановления
    }
    
    // Подсчитываем потерянные пакеты
    int lostCount = 0;
    int lostIndex = -1;
    
    for (int i = 0; i < XOR_FEC_K; ++i) {
        if (!group.hasData[i]) {
            lostCount++;
            lostIndex = i;
        }
    }
    
    // XOR FEC может восстановить только один потерянный пакет
    if (lostCount != 1) {
        return false;
    }
    
    // Восстанавливаем потерянный пакет
    QVector<QByteArray> availablePackets;
    for (int i = 0; i < XOR_FEC_K; ++i) {
        if (i != lostIndex && group.hasData[i]) {
            availablePackets.append(group.dataPackets[i]);
        }
    }
    availablePackets.append(group.xorPacket);
    
    group.dataPackets[lostIndex] = xorPackets(availablePackets);
    group.hasData[lostIndex] = true;
    
    qDebug() << "XOR FEC: Recovered packet" << lostIndex << "in group" << groupId;
    return true;
}
