#include "packetgroupbuffer.h"
#include "network_packet.h"
#include <QDataStream>
#include <QDebug>

enum PacketType {
    START_FRAME = 0x01,
    CONTINUE_FRAME = 0x02,
    END_FRAME = 0x03
};


PacketGroupBuffer::PacketGroupBuffer(int streamId, QObject *parent)
    : QObject(parent), m_streamId(streamId)
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
    
    // Получаем номер пакета для определения индекса во фрейме
    uint32_t packetSequence = header.header.packetSequence;
    
    switch (packetType) {
    case START_FRAME:
        processStartFrame(packet, payload);
        break;
        
    case CONTINUE_FRAME:
        processContinueFrame(packet, payload, packetSequence);
        break;
        
    case END_FRAME:
        processEndFrame(packet, payload, packetSequence);
        break;
    }
}

void PacketGroupBuffer::processStartFrame(const NetworkPacket &packet, const QByteArray &payload)
{
    if (payload.size() < 8) return; // frameNumber(4) + frameSize(4)
    
    int frameNumber = extractFrameNumber(payload, 0);
    int frameSize = extractFrameNumber(payload, 4);
    
    // Создаем новый фрейм
    FramePackets frame;
    frame.frameNumber = frameNumber;
    frame.timestamp = QDateTime::currentMSecsSinceEpoch();
    frame.hasStartFrame = true;
    
    // Предполагаем общее количество пакетов (можно уточнить из frameSize)
    // Пока устанавливаем размер по умолчанию
    frame.totalPackets = 10; // TODO: Вычислить на основе frameSize
    
    // Сохраняем START_FRAME данные (без заголовка frameNumber+frameSize)
    frame.packets[0] = payload.mid(8);
    frame.received.resize(frame.totalPackets, false);
    frame.received[0] = true;
    
    m_frames[frameNumber] = frame;
    qDebug() << "PacketGroupBuffer: START_FRAME for frame" << frameNumber;
}

void PacketGroupBuffer::processContinueFrame(const NetworkPacket &packet, 
                                             const QByteArray &payload, int packetIndex)
{
    if (payload.size() < 4) return;
    
    int frameNumber = extractFrameNumber(payload, 0);
    
    if (!m_frames.contains(frameNumber)) {
        qWarning() << "PacketGroupBuffer: CONTINUE_FRAME for unknown frame" << frameNumber;
        return;
    }
    
    FramePackets &frame = m_frames[frameNumber];
    
    // Определяем индекс пакета во фрейме
    int framePacketIndex = packetIndex % frame.totalPackets;
    
    // Сохраняем данные (без frameNumber)
    frame.packets[framePacketIndex] = payload.mid(4);
    frame.received[framePacketIndex] = true;
    
    qDebug() << "PacketGroupBuffer: CONTINUE_FRAME for frame" << frameNumber 
             << "at index" << framePacketIndex;
    
    checkFrameCompletion(frameNumber);
}

void PacketGroupBuffer::processEndFrame(const NetworkPacket &packet, 
                                        const QByteArray &payload, int packetIndex)
{
    if (payload.size() < 4) return;
    
    int frameNumber = extractFrameNumber(payload, 0);
    
    if (!m_frames.contains(frameNumber)) {
        qWarning() << "PacketGroupBuffer: END_FRAME for unknown frame" << frameNumber;
        return;
    }
    
    FramePackets &frame = m_frames[frameNumber];
    
    // Обновляем totalPackets если нужно
    int framePacketIndex = packetIndex % frame.totalPackets;
    frame.totalPackets = framePacketIndex + 1;
    frame.received.resize(frame.totalPackets, false);
    
    // Сохраняем данные (без frameNumber)
    frame.packets[framePacketIndex] = payload.mid(4);
    frame.received[framePacketIndex] = true;
    
    qDebug() << "PacketGroupBuffer: END_FRAME for frame" << frameNumber 
             << "total packets:" << frame.totalPackets;
    
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
        m_completeFrames.append(qMakePair(frameNumber, frameData));
        m_completedCount++;
        
        // Сигнализируем о готовности
        emit frameComplete(m_streamId, frameNumber, frameData);
        
        // Удаляем из буфера
        m_frames.remove(frameNumber);
    }
}

QByteArray PacketGroupBuffer::assembleFrame(const FramePackets &frame)
{
    QByteArray result;
    
    // Собираем все пакеты в правильном порядке
    for (int i = 0; i < frame.totalPackets; ++i) {
        if (frame.packets.contains(i)) {
            result.append(frame.packets[i]);
        }
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

QList<QPair<int, QByteArray>> PacketGroupBuffer::getCompleteFrames()
{
    QList<QPair<int, QByteArray>> frames = m_completeFrames;
    m_completeFrames.clear();
    return frames;
}

void PacketGroupBuffer::cleanup(qint64 maxAgeMs)
{
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    auto it = m_frames.begin();
    
    while (it != m_frames.end()) {
        if (currentTime - it.value().timestamp > maxAgeMs) {
            qDebug() << "PacketGroupBuffer: Cleaning up old frame" << it.key();
            it = m_frames.erase(it);
        } else {
            ++it;
        }
    }
}