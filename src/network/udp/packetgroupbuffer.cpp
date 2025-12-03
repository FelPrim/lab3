#include "packetgroupbuffer.h"
#include <QDataStream>
#include <QDebug>
#include <algorithm>
#include <cassert>
#include "network_packet.h"


enum PacketType {
    START_FRAME = 0x01,
    CONTINUE_FRAME = 0x02,
    END_FRAME = 0x03,
    FEC_PACKET = 0x04
};


PacketGroupBuffer::PacketGroupBuffer(int streamId, QObject *parent)
    : QObject(parent), m_streamId(streamId)
{
    m_lastActivityTime = QDateTime::currentMSecsSinceEpoch();
}

int PacketGroupBuffer::getRelativePacketIndex(uint32_t currentSequence, uint32_t startSequence) const
{
    int currentIdx = packetSequenceToDataIndex(currentSequence);
    int startIdx   = packetSequenceToDataIndex(startSequence);
    if (currentIdx < 0 || startIdx < 0) return -1;
    return currentIdx - startIdx;
}

int PacketGroupBuffer::calculateTotalPackets(int frameSize, int firstPacketDataSize) const
{
    if (frameSize <= firstPacketDataSize) return 1;
    int remaining = frameSize - firstPacketDataSize;
    int contCount = (remaining + CONTINUE_DATA - 1) / CONTINUE_DATA;
    return 1 + contCount;
}

int PacketGroupBuffer::extractFrameNumber(const QByteArray &payload, int offset) const
{
    if (payload.size() < offset + 4) return -1;
    QDataStream ds(payload.mid(offset,4));
    ds.setByteOrder(QDataStream::BigEndian);
    qint32 fn;
    ds >> fn;
    return static_cast<int>(fn);
}

int PacketGroupBuffer::extractFrameSize(const QByteArray &payload, int offset) const
{
    if (payload.size() < offset + 4) return -1;
    QDataStream ds(payload.mid(offset,4));
    ds.setByteOrder(QDataStream::BigEndian);
    qint32 sz;
    ds >> sz;
    return static_cast<int>(sz);
}


void PacketGroupBuffer::cleanupOldestIncompleteFrame()
{
    if (m_groups.isEmpty()) return;
    
    int oldestFrame = std::numeric_limits<int>::max();
    qint64 oldestTime = std::numeric_limits<qint64>::max();
    
    // Находим самый старый НЕПОЛНЫЙ кадр
    for (auto it = m_groups.begin(); it != m_groups.end(); ++it) {
        if (!it->isComplete() && it->creationTimeMs < oldestTime) {
            oldestTime = it->creationTimeMs;
            oldestFrame = it.key();
        }
    }
    
    // Если нашли неполный кадр - удаляем его
    if (oldestFrame != std::numeric_limits<int>::max()) {
        QString reason = QString("forced_cleanup_incomplete_%1/%2")
                            .arg(m_groups[oldestFrame].packetsReceived)
                            .arg(m_groups[oldestFrame].totalPackets);
        
        qDebug() << "PacketGroupBuffer: forced cleanup of incomplete frame" 
                 << oldestFrame << "(" << m_groups[oldestFrame].packetsReceived 
                 << "/" << m_groups[oldestFrame].totalPackets << "packets)";
        
        m_groups.remove(oldestFrame);
        emit frameDropped(m_streamId, oldestFrame, reason);
    }
}

void PacketGroupBuffer::addPacket(const NetworkPacket &packet)
{
    if (m_groups.size() >= DEFAULT_FPS*DEFAULT_BUFFERSECONDS) {
        cleanupOldestIncompleteFrame();
    }
    // Обновляем время последней активности
    m_lastActivityTime = QDateTime::currentMSecsSinceEpoch();
    
    // packet.route is nRouteHeader; convert to host order
    PacketHeader hdr;
    memcpy(&hdr, &packet.route, sizeof(PacketHeader));
    cast_from_nbe(hdr);
    uint32_t packetSequence = hdr.header.packetSequence;

    // We assume fecbuffer already filters XOR-packets out; but double-check just in case:
    if (packet.isXorPacket()) {
        return;
    }

    uint8_t pktType = PacketProcessor::getPacketType(packet); // returns 1/2/3
    QByteArray payload = PacketProcessor::getDataPacketPayload(packet); // 1187 bytes

    // payload always has frameNumber at offset 0
    int frameNumber = extractFrameNumber(payload, 0);
    if (frameNumber < 0) {
        qWarning() << "PacketGroupBuffer: can't parse frameNumber";
        return;
    }

    // Prepare temp packet
    TempPacket tp;
    tp.packetSequence = packetSequence;
    tp.rawType = static_cast<uint8_t>(pktType);
    tp.payload = payload;

    // find or create group
    bool existed = m_groups.contains(frameNumber);
    FrameGroup &group = m_groups[frameNumber];
    if (!existed) {
        group = FrameGroup();
        group.frameNumber = frameNumber;
    }
    
    // Обновляем время последней активности группы
    group.updateLastActivity();

    if (frameNumber > m_latestFrameNumber) m_latestFrameNumber = frameNumber;

    switch (pktType) {
    case START_FRAME:
        handleStart(group, tp, packetSequence);
        break;
    case CONTINUE_FRAME:
        handleContinue(group, tp, packetSequence);
        break;
    case END_FRAME:
        handleEnd(group, tp, packetSequence);
        break;
    default:
        qWarning() << "PacketGroupBuffer: unknown packet type" << pktType;
        return;
    }

    if (group.isComplete()) {
        tryAssemble(group);
    }

    cleanupOldFrames();
}

void PacketGroupBuffer::handleStart(FrameGroup &group, const TempPacket &tp, uint32_t packetSequence)
{
    // START: [frameNumber(4)][frameSize(4)][data up to 1179 padded with zeros]
    if (tp.payload.size() < 8) {
        qWarning() << "PacketGroupBuffer: START payload too small";
        return;
    }

    int frameSize = extractFrameSize(tp.payload, 4);
    if (frameSize <= 0) {
        qWarning() << "PacketGroupBuffer: invalid frameSize" << frameSize;
        return;
    }

    int firstDataSize = qMax(0, tp.payload.size() - 8); // likely 1179
    int totalPackets = calculateTotalPackets(frameSize, firstDataSize);

    // init group
    group.frameSize = frameSize;
    group.totalPackets = totalPackets;
    group.startSequence = packetSequence;
    group.packets.resize(totalPackets);
    group.received.fill(totalPackets, 0);
    group.packetsReceived = 0;

    // store START payload into index 0 (trim padding if frame smaller)
    if (firstDataSize > 0) {
        int toCopy = qMin(firstDataSize, frameSize); // if whole frame fits into START
        group.packets[0] = tp.payload.mid(8, toCopy);
        group.received[0] = 1;
        group.packetsReceived = 1;
    }

    // place any previously stored temp packets
    for (auto it = group.tempPackets.begin(); it != group.tempPackets.end(); ) {
        int relIdx = getRelativePacketIndex(it->packetSequence, group.startSequence);
        if (relIdx >= 1 && relIdx < group.totalPackets) {
            uint8_t t = it->rawType;
            if (t == CONTINUE_FRAME) {
                if (it->payload.size() >= 4) {
                    QByteArray d = it->payload.mid(4); // full continue data
                    if (!group.received[relIdx]) {
                        group.packets[relIdx] = d;
                        group.received[relIdx] = 1;
                        group.packetsReceived++;
                    }
                }
            } else if (t == END_FRAME) {
                if (it->payload.size() >= 4) {
                    QByteArray d = it->payload.mid(4);
                    if (!group.received[relIdx]) {
                        group.packets[relIdx] = d;
                        group.received[relIdx] = 1;
                        group.packetsReceived++;
                    }
                }
            }
            it = group.tempPackets.erase(it);
        } else {
            ++it;
        }
    }
}

void PacketGroupBuffer::handleContinue(FrameGroup &group, const TempPacket &tp, uint32_t packetSequence)
{
    // CONTINUE: [frameNumber(4)][1183 bytes data]
    if (tp.payload.size() < 4) {
        qWarning() << "PacketGroupBuffer: CONTINUE payload too small";
        return;
    }

    if (!group.hasStart()) {
        group.tempPackets.append(tp);
        return;
    }

    int relIdx = getRelativePacketIndex(packetSequence, group.startSequence);
    if (relIdx <= 0 || relIdx >= group.totalPackets) {
        // either duplicate of start or out of range
        return;
    }

    if (group.received[relIdx]) return; // duplicate

    QByteArray data = tp.payload.mid(4);
    group.packets[relIdx] = data;
    group.received[relIdx] = 1;
    group.packetsReceived++;
}

void PacketGroupBuffer::handleEnd(FrameGroup &group, const TempPacket &tp, uint32_t packetSequence)
{
    // END: [frameNumber(4)][1183 bytes data possibly padded]
    if (tp.payload.size() < 4) {
        qWarning() << "PacketGroupBuffer: END payload too small";
        return;
    }

    if (!group.hasStart()) {
        group.tempPackets.append(tp);
        return;
    }

    int relIdx = getRelativePacketIndex(packetSequence, group.startSequence);
    if (relIdx != (group.totalPackets - 1)) {
        // not the expected last packet for this frame -> ignore
        return;
    }

    if (group.received[relIdx]) return; // duplicate

    QByteArray data = tp.payload.mid(4);
    group.packets[relIdx] = data;
    group.received[relIdx] = 1;
    group.packetsReceived++;
}

void PacketGroupBuffer::tryAssemble(FrameGroup &group)
{
    if (!group.isComplete()) return;

    QByteArray frame;
    frame.reserve(group.frameSize);

    for (int i = 0; i < group.totalPackets; ++i) {
        if (!group.received[i]) {
            qWarning() << "PacketGroupBuffer: missing packet" << i << "for frame" << group.frameNumber;
            return;
        }
        const QByteArray &p = group.packets[i];
        if (i == group.totalPackets - 1) {
            int remaining = group.frameSize - frame.size();
            int toAppend = qMin(remaining, p.size());
            if (toAppend > 0) frame.append(p.constData(), toAppend);
        } else {
            frame.append(p);
        }
    }

    if (frame.size() != group.frameSize) {
        if (frame.size() < group.frameSize) {
            frame.append(QByteArray(group.frameSize - frame.size(), '\0'));
        } else {
            frame.resize(group.frameSize);
        }
    }

    emit frameComplete(m_streamId, group.frameNumber, frame);
    m_completedCount++;
    m_groups.remove(group.frameNumber);
}

void PacketGroupBuffer::cleanupOldFrames()
{
    int maxFrames = static_cast<int>(DEFAULT_FPS * DEFAULT_BUFFERSECONDS) + MARGIN;
    if (m_groups.size() <= maxFrames) return;

    QList<int> keys = m_groups.keys();
    std::sort(keys.begin(), keys.end());
    while (m_groups.size() > maxFrames && !keys.isEmpty()) {
        int k = keys.takeFirst();
        m_groups.remove(k);
        qDebug() << "PacketGroupBuffer: dropped old frame" << k << "(by count)";
        emit frameDropped(m_streamId, k, "buffer_limit_exceeded");
    }
}



void PacketGroupBuffer::cleanupExpiredFrames(qint64 maxAgeMs)
{
    if (maxAgeMs <= 0) return;
    
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    QList<int> expiredFrames;
    
    for (auto it = m_groups.begin(); it != m_groups.end(); ++it) {
        // Удаляем если кадр устарел (по creationTime) И не собран
        if (it->isStale(currentTime, maxAgeMs)) {
            expiredFrames.append(it.key());
        }
    }
    
    // Удаляем истекшие кадры
    for (int frameNumber : expiredFrames) {
        const FrameGroup &group = m_groups[frameNumber];
        QString reason = QString("timeout_expired_%1ms_incomplete_%2/%3")
                            .arg(maxAgeMs)
                            .arg(group.packetsReceived)
                            .arg(group.totalPackets);
        
        qDebug() << "PacketGroupBuffer: dropping expired frame" 
                 << frameNumber << "(" << group.packetsReceived << "/" 
                 << group.totalPackets << "packets, age:" 
                 << (currentTime - group.lastUpdateTimeMs) << "ms)";
        
        m_groups.remove(frameNumber);
        emit frameDropped(m_streamId, frameNumber, reason);
    }
    
    if (!expiredFrames.isEmpty()) {
        qDebug() << "PacketGroupBuffer: cleaned" << expiredFrames.size() 
                 << "expired frames (maxAge:" << maxAgeMs << "ms)";
    }
}

void PacketGroupBuffer::cleanupOldFramesByTimeout(qint64 maxAgeMs)
{
    // Сначала очищаем по таймауту
    cleanupExpiredFrames(maxAgeMs);
    
    // Затем очищаем по количеству (если все еще много)
    cleanupOldFrames();
    
    // Если нет активности долгое время, очищаем всё
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    qint64 inactivityTime = currentTime - m_lastActivityTime;
    
    if (inactivityTime > maxAgeMs * 3 && !m_groups.isEmpty()) {
        qDebug() << "PacketGroupBuffer: long inactivity (" << inactivityTime 
                 << "ms), cleaning all frames";
        
        QList<int> allFrames = m_groups.keys();
        for (int frameNumber : allFrames) {
            emit frameDropped(m_streamId, frameNumber, "inactivity_timeout");
        }
        
        m_groups.clear();
    }
}

void PacketGroupBuffer::cleanup()
{
    int droppedCount = m_groups.size();
    QList<int> allFrames = m_groups.keys();
    
    for (int frameNumber : allFrames) {
        emit frameDropped(m_streamId, frameNumber, "manual_cleanup");
    }
    
    m_groups.clear();
    m_completedCount = 0;
    m_latestFrameNumber = -1;
    m_lastActivityTime = QDateTime::currentMSecsSinceEpoch();
    
    qDebug() << "PacketGroupBuffer: cleaned all" << droppedCount << "frames";
}