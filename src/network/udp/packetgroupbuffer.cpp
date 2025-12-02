#include "packetgroupbuffer.h"
#include "network_packet.h"
#include <QDataStream>
#include <QDebug>

PacketGroupBuffer::PacketGroupBuffer(int streamId, QObject *parent)
    : QObject(parent), m_streamId(streamId), m_completedCount(0)
{
}

void PacketGroupBuffer::addPacket(const NetworkPacket &packet)
{
    if (packet.isXorPacket()) return; // XOR пакеты обрабатываются в FecBuffer
    
    const DataPacket* dataPacket = packet.asDataPacket();
    if (!dataPacket) return;
    
    PacketHeader header;
    memcpy(&header, &packet.route, sizeof(PacketHeader));
    cast_from_nbe(header);
    
    uint8_t packetType = PacketProcessor::getPacketType(packet);
    QByteArray payload = PacketProcessor::getDataPacketPayload(packet);
    uint32_t packetSequence = header.header.packetSequence;
    
    switch (packetType) {
    case START_FRAME:
        processStartFrame(packet, payload, packetSequence);
        processOrphanedPackets(extractFrameNumber(payload, 0));
        break;
        
    case CONTINUE_FRAME: {
        int frameNumber = extractFrameNumber(payload, 0);
        if (frameNumber == -1) return;
        
        if (!m_frames.contains(frameNumber)) {
            qDebug() << "PacketGroupBuffer: Orphaned CONTINUE_FRAME for frame" << frameNumber;
            m_orphanedPackets.insert(frameNumber, qMakePair(CONTINUE_FRAME, payload));
            return;
        }
        processContinueFrame(packet, payload, packetSequence, frameNumber);
        break;
    }
        
    case END_FRAME: {
        int frameNumber = extractFrameNumber(payload, 0);
        if (frameNumber == -1) return;
        
        if (!m_frames.contains(frameNumber)) {
            qDebug() << "PacketGroupBuffer: Orphaned END_FRAME for frame" << frameNumber;
            m_orphanedPackets.insert(frameNumber, qMakePair(END_FRAME, payload));
            return;
        }
        processEndFrame(packet, payload, packetSequence, frameNumber);
        break;
    }
    }
}

void PacketGroupBuffer::processStartFrame(const NetworkPacket &packet, 
                                          const QByteArray &payload, 
                                          uint32_t packetSequence)
{
    if (payload.size() < 8) return; // frameNumber(4) + frameSize(4)
    
    int frameNumber = extractFrameNumber(payload, 0);
    int frameSize = extractFrameNumber(payload, 4);
    
    if (frameSize <= 0 || frameSize > 10 * 1024 * 1024) { // 10 MB макс
        qWarning() << "PacketGroupBuffer: Invalid frame size" << frameSize;
        return;
    }
    
    // Получаем данные из первого пакета (после 8 байт заголовка)
    int firstPacketDataSize = qMax(0, payload.size() - 8);
    
    // Вычисляем общее количество пакетов
    int totalPackets = calculateTotalPackets(frameSize, firstPacketDataSize);
    
    qDebug() << "PacketGroupBuffer: START_FRAME - frame:" << frameNumber 
             << "size:" << frameSize << "total packets:" << totalPackets
             << "first packet data:" << firstPacketDataSize << "bytes";
    
    // Создаем новый фрейм
    FramePackets frame;
    frame.frameNumber = frameNumber;
    frame.startSequence = packetSequence;
    frame.frameSize = frameSize;
    frame.totalPackets = totalPackets;
    frame.timestamp = QDateTime::currentMSecsSinceEpoch();
    frame.received.resize(totalPackets, false);
    
    // Сохраняем данные из START_FRAME (индекс 0)
    if (firstPacketDataSize > 0) {
        frame.packets[0] = payload.mid(8, firstPacketDataSize);
        frame.received[0] = true;
    }
    
    m_frames[frameNumber] = frame;
}

int PacketGroupBuffer::calculateTotalPackets(int frameSize, int firstPacketDataSize) const
{
    // Случай 1: Фрейм полностью помещается в START_FRAME
    if (frameSize <= firstPacketDataSize) {
        return 1;
    }
    
    // Случай 2: Фрейм требует дополнительных пакетов
    int remainingBytes = frameSize - firstPacketDataSize;
    
    // Вычисляем, сколько CONTINUE_FRAME пакетов нужно
    // Каждый CONTINUE_FRAME может нести до 1183 байт
    int continuePackets = (remainingBytes + CONTINUE_PAYLOAD - 1) / CONTINUE_PAYLOAD;
    
    // Общее количество пакетов: START_FRAME + CONTINUE_FRAME (последний будет END_FRAME)
    return 1 + continuePackets;
}

void PacketGroupBuffer::processContinueFrame(const NetworkPacket &packet,
                                             const QByteArray &payload,
                                             uint32_t packetSequence,
                                             int frameNumber)
{
    if (payload.size() < 4) return;
    
    FramePackets &frame = m_frames[frameNumber];
    
    // Вычисляем индекс пакета (0 = START_FRAME, 1... = CONTINUE/END_FRAME)
    int packetIndex = packetSequence - frame.startSequence;
    
    if (packetIndex < 1 || packetIndex >= frame.totalPackets) {
        qWarning() << "PacketGroupBuffer: Invalid packet index" << packetIndex 
                   << "for frame" << frameNumber << "total:" << frame.totalPackets;
        return;
    }
    
    // Сохраняем данные (без frameNumber)
    int dataSize = qMax(0, payload.size() - 4);
    if (dataSize > 0) {
        frame.packets[packetIndex] = payload.mid(4, dataSize);
        frame.received[packetIndex] = true;
    }
    
    qDebug() << "PacketGroupBuffer: CONTINUE_FRAME - frame:" << frameNumber 
             << "index:" << packetIndex << "data:" << dataSize << "bytes";
    
    checkFrameCompletion(frameNumber);
}

void PacketGroupBuffer::processEndFrame(const NetworkPacket &packet,
                                        const QByteArray &payload,
                                        uint32_t packetSequence,
                                        int frameNumber)
{
    if (payload.size() < 4) return;
    
    FramePackets &frame = m_frames[frameNumber];
    
    // Вычисляем индекс пакета
    int packetIndex = packetSequence - frame.startSequence;
    
    if (packetIndex < 1 || packetIndex >= frame.totalPackets) {
        qWarning() << "PacketGroupBuffer: Invalid packet index" << packetIndex 
                   << "for frame" << frameNumber << "total:" << frame.totalPackets;
        return;
    }
    
    // Для END_FRAME вычисляем, сколько реальных данных (может быть меньше END_PAYLOAD)
    int remainingBytes = frame.frameSize;
    for (int i = 0; i < packetIndex; ++i) {
        if (frame.packets.contains(i)) {
            remainingBytes -= frame.packets[i].size();
        }
    }
    
    int dataSize = qMin(qMax(0, payload.size() - 4), remainingBytes);
    if (dataSize > 0) {
        frame.packets[packetIndex] = payload.mid(4, dataSize);
        frame.received[packetIndex] = true;
    }
    
    qDebug() << "PacketGroupBuffer: END_FRAME - frame:" << frameNumber 
             << "index:" << packetIndex << "real data:" << dataSize 
             << "remaining:" << remainingBytes << "bytes";
    
    checkFrameCompletion(frameNumber);
}

void PacketGroupBuffer::checkFrameCompletion(int frameNumber)
{
    if (!m_frames.contains(frameNumber)) return;
    
    FramePackets &frame = m_frames[frameNumber];
    
    if (frame.isComplete()) {
        qDebug() << "PacketGroupBuffer: Frame" << frameNumber << "completed";
        
        // Собираем фрейм
        QByteArray frameData = assembleFrame(frame);
        
        // Проверяем размер
        if (frameData.size() != frame.frameSize) {
            qWarning() << "PacketGroupBuffer: Frame size mismatch. Expected:" 
                       << frame.frameSize << "Got:" << frameData.size();
            // Обрезаем или дополняем
            if (frameData.size() > frame.frameSize) {
                frameData.resize(frame.frameSize);
            }
        }
        
        m_completeFrames.append(qMakePair(frameNumber, frameData));
        m_completedCount++;
        
        // Сигнализируем о готовности
        emit frameComplete(m_streamId, frameNumber, frameData);
        
        // Удаляем из буфера
        m_frames.remove(frameNumber);
        m_orphanedPackets.remove(frameNumber);
    }
}

QByteArray PacketGroupBuffer::assembleFrame(const FramePackets &frame)
{
    QByteArray result;
    result.reserve(frame.frameSize);
    
    // Собираем все пакеты в правильном порядке (по индексу)
    for (int i = 0; i < frame.totalPackets; ++i) {
        if (frame.packets.contains(i)) {
            result.append(frame.packets[i]);
        } else {
            qWarning() << "PacketGroupBuffer: Missing packet" << i 
                       << "in frame" << frame.frameNumber;
            // Добавляем нули для отсутствующих данных
            result.append(QByteArray(CONTINUE_PAYLOAD, 0));
        }
    }
    
    // Обрезаем до нужного размера
    if (result.size() > frame.frameSize) {
        result.resize(frame.frameSize);
    }
    
    return result;
}

int PacketGroupBuffer::extractFrameNumber(const QByteArray &payload, int offset) const
{
    if (payload.size() < offset + 4) return -1;
    
    QDataStream stream(payload.mid(offset));
    stream.setByteOrder(QDataStream::BigEndian);
    
    int frameNumber;
    stream >> frameNumber;
    return frameNumber;
}

void PacketGroupBuffer::processOrphanedPackets(int frameNumber)
{
    if (!m_orphanedPackets.contains(frameNumber)) return;
    
    auto orphaned = m_orphanedPackets.values(frameNumber);
    qDebug() << "PacketGroupBuffer: Processing" << orphaned.size() 
             << "orphaned packets for frame" << frameNumber;
    
    for (const auto& packet : orphaned) {
        // Нужно создать временный NetworkPacket для обработки
        // Так как у нас нет полного пакета, только payload и тип
        // В реальной реализации нужно хранить полные пакеты
        qDebug() << "Reprocessing orphaned packet type:" << packet.first;
        
        // Здесь нужно было бы вызвать соответствующий метод обработки
        // Но для упрощения просто добавляем в буфер и надеемся, что
        // пакет придет снова через нормальный путь
    }
    
    m_orphanedPackets.remove(frameNumber);
}

QList<QPair<int, QByteArray>> PacketGroupBuffer::getCompleteFrames()
{
    QList<QPair<int, QByteArray>> frames = m_completeFrames;
    m_completeFrames.clear();
    return frames;
}

void PacketGroupBuffer::cleanup(qint64 maxAgeMs)
{
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    
    // Очищаем старые фреймы
    auto it = m_frames.begin();
    while (it != m_frames.end()) {
        if (currentTime - it.value().timestamp > maxAgeMs) {
            qDebug() << "PacketGroupBuffer: Cleaning up old frame" << it.key();
            it = m_frames.erase(it);
        } else {
            ++it;
        }
    }
    
    // Очищаем старые orphaned пакеты
    auto orphanIt = m_orphanedPackets.begin();
    while (orphanIt != m_orphanedPackets.end()) {
        // Для orphaned пакетов пока нет timestamp, удаляем все старые
        // В реальной реализации нужно хранить timestamp для каждого пакета
        orphanIt = m_orphanedPackets.erase(orphanIt);
    }
}
