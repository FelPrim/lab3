#include "myfec.h"
#include <QTimer>
#include <QDataStream>
#include <QDateTime>
#include <QDebug>

// 1187 - 8 = 1179
// 1183
// 1183
// Используем константы из video_defaults
const int START_PAYLOAD = DATA_PAYLOAD_SIZE - 8;  // 1179 байта (1191 - frameNumber(4) - frameSize(4))
const int CONTINUE_PAYLOAD = DATA_PAYLOAD_SIZE - 4; // 1183 байт (1191 - frameNumber(4))
const int END_PAYLOAD = DATA_PAYLOAD_SIZE - 4;      // 1183 байт (1191 - frameNumber(4))

// FrameAssembler implementation
FrameAssembler::FrameAssembler(QObject *parent) : QObject(parent) 
{
    qDebug() << "FrameAssembler: Created";
}

void FrameAssembler::processOrphanedPackets(int streamId)
{
    if (m_orphanedPackets.contains(streamId)) {
        auto orphaned = m_orphanedPackets.values(streamId);
        qDebug() << "FrameAssembler: Processing" << orphaned.size() 
                 << "orphaned packets for stream:" << streamId;
        
        for (const auto& packet : orphaned) {
            qDebug() << "FrameAssembler: Reprocessing orphaned packet - Stream:" << streamId
                     << "Type:" << packet.first << "Size:" << packet.second.size();
            processPacket(streamId, packet.first, packet.second);
        }
        m_orphanedPackets.remove(streamId);
    }
}

void FrameAssembler::processPacket(int streamId, PacketType type, const QByteArray &payload)
{
    qDebug() << "FrameAssembler: Processing packet - Stream:" << streamId 
             << "Type:" << type << "Payload size:" << payload.size();
    
    if (type != START_FRAME && !m_streamAssemblies.contains(streamId)) {
        qDebug() << "FrameAssembler: Orphaned packet - Stream:" << streamId 
                 << "Type:" << type << "Saving for later processing";
        m_orphanedPackets.insert(streamId, qMakePair(type, payload));
        return;
    }
    
    switch (type) {
    case START_FRAME:
        processStartFrame(streamId, payload);
        processOrphanedPackets(streamId);
        break;
    case CONTINUE_FRAME:
        processContinueFrame(streamId, payload);
        break;
    case END_FRAME:
        processEndFrame(streamId, payload);
        break;
    default:
        qWarning() << "FrameAssembler: Unknown packet type:" << type;
        break;
    }
}

void FrameAssembler::processStartFrame(int streamId, const QByteArray &data)
{
    const int START_FRAME_PAYLOAD_HEADER = 8; // frameNumber(4) + frameSize(4)
    if (data.size() < START_FRAME_PAYLOAD_HEADER) { 
        qWarning() << "FrameAssembler: START_FRAME too small - Stream:" << streamId
                   << "Expected at least" << START_FRAME_PAYLOAD_HEADER << "bytes, got:" << data.size();
        return;
    }

    QDataStream stream(data);
    stream.setByteOrder(QDataStream::BigEndian);

    int frameNumber, frameSize;
    stream >> frameNumber >> frameSize;

    qDebug() << "FrameAssembler: START_FRAME header - Stream:" << streamId
             << "Frame:" << frameNumber << "Total frame size:" << frameSize;

    if (frameSize <= 0 || frameSize > 10 * 1024 * 1024) {
        qWarning() << "FrameAssembler: Invalid frame size - Stream:" << streamId
                   << "Frame:" << frameNumber << "Size:" << frameSize;
        return;
    }

    QByteArray frameData = data.mid(START_FRAME_PAYLOAD_HEADER); 
    int bytesToTake = qMin(frameData.size(), frameSize);

    qDebug() << "FrameAssembler: START_FRAME data - Stream:" << streamId 
             << "Frame:" << frameNumber << "Header size:" << START_FRAME_PAYLOAD_HEADER
             << "Available data:" << frameData.size() << "Taking:" << bytesToTake;

    m_streamAssemblies[streamId] = StreamAssembly(streamId, frameNumber);

    StreamAssembly& assembly = m_streamAssemblies[streamId];
    assembly.totalSize = frameSize;
    assembly.data = frameData.left(bytesToTake);
    assembly.receivedSize = bytesToTake;
    assembly.hasStartFrame = true;

    qDebug() << "FrameAssembler: START_FRAME assembly created - Stream:" << streamId
             << "Frame:" << frameNumber << "Progress:" << assembly.receivedSize << "/" << assembly.totalSize;

    if (assembly.isComplete()) {
        qDebug() << "FrameAssembler: Frame completed immediately after START_FRAME";
        completeFrame(streamId);
    }
}

void FrameAssembler::processContinueFrame(int streamId, const QByteArray &data)
{
    if (!m_streamAssemblies.contains(streamId)) {
        qWarning() << "FrameAssembler: CONTINUE_FRAME for unknown stream:" << streamId;
        return;
    }
    
    StreamAssembly& assembly = m_streamAssemblies[streamId];
    
    if (data.size() < 4) {
        qWarning() << "FrameAssembler: CONTINUE_FRAME too small - Stream:" << streamId
                   << "Expected at least 4 bytes, got:" << data.size();
        return;
    }
    
    QDataStream stream(data);
    stream.setByteOrder(QDataStream::BigEndian);
    
    int frameNumber;
    stream >> frameNumber;
    
    if (frameNumber != assembly.frameNumber) {
        qWarning() << "FrameAssembler: CONTINUE_FRAME frame number mismatch - Stream:" << streamId
                   << "Expected:" << assembly.frameNumber << "Got:" << frameNumber;
        return;
    }
    
    QByteArray frameData = data.mid(4);
    int previousReceived = assembly.receivedSize;
    
    qDebug() << "FrameAssembler: CONTINUE_FRAME - Stream:" << streamId 
             << "Frame:" << frameNumber << "New data:" << frameData.size()
             << "Previous total:" << previousReceived << "New total:" << previousReceived + frameData.size();
    
    assembly.data.append(frameData);
    assembly.receivedSize += frameData.size();
    
    if (assembly.isComplete()) {
        qDebug() << "FrameAssembler: Frame completed by CONTINUE_FRAME";
        completeFrame(streamId);
    } else {
        qDebug() << "FrameAssembler: CONTINUE_FRAME progress - Stream:" << streamId
                 << "Frame:" << frameNumber << "Progress:" << assembly.receivedSize << "/" << assembly.totalSize;
    }
}

void FrameAssembler::processEndFrame(int streamId, const QByteArray &data)
{
    if (!m_streamAssemblies.contains(streamId)) {
        qWarning() << "FrameAssembler: END_FRAME for unknown stream:" << streamId;
        return;
    }
    
    StreamAssembly& assembly = m_streamAssemblies[streamId];
    
    if (data.size() < 4) {
        qWarning() << "FrameAssembler: END_FRAME too small - Stream:" << streamId
                   << "Expected at least 4 bytes, got:" << data.size();
        return;
    }
    
    QDataStream stream(data);
    stream.setByteOrder(QDataStream::BigEndian);
    
    int frameNumber;
    stream >> frameNumber;
    
    if (frameNumber != assembly.frameNumber) {
        qWarning() << "FrameAssembler: END_FRAME frame number mismatch - Stream:" << streamId
                   << "Expected:" << assembly.frameNumber << "Got:" << frameNumber;
        return;
    }
    
    QByteArray frameData = data.mid(4);
    int neededBytes = assembly.totalSize - assembly.receivedSize;
    
    qDebug() << "FrameAssembler: END_FRAME analysis - Stream:" << streamId
             << "Frame:" << frameNumber << "Needed bytes:" << neededBytes
             << "Available in packet:" << frameData.size();
    
    if (neededBytes <= 0) {
        qWarning() << "FrameAssembler: END_FRAME unnecessary - Stream:" << streamId
                   << "Frame:" << frameNumber << "Already received:" << assembly.receivedSize;
        return;
    }
    
    int bytesToTake = qMin(neededBytes, frameData.size());
    QByteArray realData = frameData.left(bytesToTake);
    int previousReceived = assembly.receivedSize;
    
    qDebug() << "FrameAssembler: END_FRAME taking data - Stream:" << streamId 
             << "Frame:" << frameNumber << "Taking:" << bytesToTake << "bytes"
             << "Progress:" << previousReceived << "->" << previousReceived + bytesToTake;
    
    assembly.data.append(realData);
    assembly.receivedSize += bytesToTake;
    
    if (assembly.isComplete()) {
        qDebug() << "FrameAssembler: Frame completed by END_FRAME";
        completeFrame(streamId);
    } else {
        qWarning() << "FrameAssembler: END_FRAME did not complete frame - Stream:" << streamId
                   << "Frame:" << frameNumber << "Remaining:" << assembly.totalSize - assembly.receivedSize;
    }
}

void FrameAssembler::completeFrame(int streamId)
{
    if (!m_streamAssemblies.contains(streamId)) {
        qCritical() << "FrameAssembler: completeFrame called for unknown stream:" << streamId;
        return;
    }
    
    StreamAssembly& assembly = m_streamAssemblies[streamId];
    
    if (assembly.data.size() >= assembly.totalSize) {
        QByteArray completeData = assembly.data.left(assembly.totalSize);
        
        qDebug() << "FrameAssembler: FRAME COMPLETE - Stream:" << streamId 
                 << "Frame:" << assembly.frameNumber 
                 << "Expected:" << assembly.totalSize 
                 << "Actual:" << completeData.size()
                 << "Assembly time:" << (QDateTime::currentMSecsSinceEpoch() - assembly.creationTime) << "ms";

        m_completeFrames.append(qMakePair(streamId, completeData));
        emit frameAssembled(streamId, assembly.frameNumber, completeData);
        m_streamAssemblies.remove(streamId);
        
        qDebug() << "FrameAssembler: Frame emitted and assembly cleaned up";
    } else {
        qWarning() << "FrameAssembler: completeFrame called but data incomplete - Stream:" << streamId
                   << "Frame:" << assembly.frameNumber << "Have:" << assembly.data.size() << "Need:" << assembly.totalSize;
    }
}

bool FrameAssembler::hasCompleteFrame() const
{
    bool hasFrames = !m_completeFrames.isEmpty();
    qDebug() << "FrameAssembler: hasCompleteFrame check - Result:" << hasFrames << "Count:" << m_completeFrames.size();
    return hasFrames;
}

QPair<int, QByteArray> FrameAssembler::takeCompleteFrame()
{
    if (m_completeFrames.isEmpty()) {
        qDebug() << "FrameAssembler: takeCompleteFrame - No frames available";
        return qMakePair(0, QByteArray());
    }
    
    auto frame = m_completeFrames.takeFirst();
    qDebug() << "FrameAssembler: takeCompleteFrame - Stream:" << frame.first 
             << "Size:" << frame.second.size();
    return frame;
}

void FrameAssembler::cleanupOldAssemblies(qint64 maxAgeMs)
{
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    int removedCount = 0;
    
    auto it = m_streamAssemblies.begin();
    while (it != m_streamAssemblies.end()) {
        qint64 age = currentTime - it->creationTime;
        if (age > maxAgeMs) {
            qWarning() << "FrameAssembler: Cleaning up old assembly - Stream:" << it->streamId 
                     << "Frame:" << it->frameNumber << "Age:" << age << "ms"
                     << "Progress:" << it->receivedSize << "/" << it->totalSize;
            it = m_streamAssemblies.erase(it);
            removedCount++;
        } else {
            ++it;
        }
    }
    
    if (removedCount > 0) {
        qDebug() << "FrameAssembler: Cleanup completed - Removed" << removedCount << "assemblies";
    }
}

// StreamAssembly implementation
FrameAssembler::StreamAssembly::StreamAssembly(int stream, int frame)
    : streamId(stream), frameNumber(frame), totalSize(0), receivedSize(0),
      creationTime(QDateTime::currentMSecsSinceEpoch()), hasStartFrame(false)
{
    qDebug() << "StreamAssembly: Created - Stream:" << streamId << "Frame:" << frameNumber;
}

bool FrameAssembler::StreamAssembly::isComplete() const
{
    bool complete = hasStartFrame && receivedSize >= totalSize;
    if (complete) {
        qDebug() << "StreamAssembly: Completion check - Stream:" << streamId 
                 << "Frame:" << frameNumber << "COMPLETE";
    }
    return complete;
}

// FrameSender implementation
FrameSender::FrameSender(QObject *parent) : QObject(parent) 
{
    qDebug() << "FrameSender: Created";
}

bool FrameSender::hasPacketsToSend() const
{
    bool hasPackets = !m_packetsToSend.isEmpty();
    qDebug() << "FrameSender: hasPacketsToSend check - Result:" << hasPackets << "Count:" << m_packetsToSend.size();
    return hasPackets;
}

QVector<QPair<PacketType, QByteArray>> FrameSender::takePacketsToSend()
{
    QVector<QPair<PacketType, QByteArray>> packets = m_packetsToSend;
    m_packetsToSend.clear();
    qDebug() << "FrameSender: takePacketsToSend - Returning" << packets.size() << "packets";
    return packets;
}

void FrameSender::clear()
{
    int frameCount = m_frameQueue.size();
    int packetCount = m_packetsToSend.size();
    
    m_frameQueue.clear();
    m_packetsToSend.clear();
    
    qDebug() << "FrameSender: clear - Removed" << frameCount << "frames and" << packetCount << "packets";
}

void FrameSender::addFrame(int streamId, int frameNumber, const QByteArray &frameData)
{
    qDebug() << "FrameSender: addFrame - Stream:" << streamId 
             << "Frame:" << frameNumber << "Size:" << frameData.size()
             << "Queue size before:" << m_frameQueue.size();
    
    if (m_frameQueue.size() >= MAX_FRAME_QUEUE_SIZE) {
        FrameQueueItem dropped = m_frameQueue.first();
        qWarning() << "FrameSender: Queue full - Dropping oldest frame - Stream:" << dropped.streamId
                   << "Frame:" << dropped.frameNumber << "Position:" << dropped.currentPosition;
        m_frameQueue.removeFirst();
    }
    
    m_frameQueue.append(FrameQueueItem(streamId, frameNumber, frameData));
    qDebug() << "FrameSender: Frame added to queue - New queue size:" << m_frameQueue.size();
    
    processAllFrames();
}

int FrameSender::calculateOptimalChunkSize(const FrameQueueItem &frame, PacketType type) const
{
    int remaining = frame.frameData.size() - frame.currentPosition;
    int chunkSize = 0;
    
    switch (type) {
    case START_FRAME:
        chunkSize = qMin(START_PAYLOAD, remaining);
        break;
    case CONTINUE_FRAME:
        chunkSize = qMin(CONTINUE_PAYLOAD, remaining);
        break;
    case END_FRAME:
        chunkSize = qMin(END_PAYLOAD, remaining);
        break;
    default:
        chunkSize = qMin(DATA_PAYLOAD_SIZE, remaining);
        break;
    }
    
    qDebug() << "FrameSender: calculateOptimalChunkSize - Type:" << type
             << "Remaining:" << remaining << "Chunk:" << chunkSize;
    
    return chunkSize;
}

// FrameQueueItem implementation
FrameSender::FrameQueueItem::FrameQueueItem(int stream, int frame, const QByteArray &data)
    : streamId(stream), frameNumber(frame), frameData(data), currentPosition(0)
{
    qDebug() << "FrameQueueItem: Created - Stream:" << streamId 
             << "Frame:" << frameNumber << "Size:" << frameData.size();
}

void FrameSender::sendStartFrame(const FrameQueueItem &frame, int dataSize)
{
    QByteArray payload;
    QDataStream stream(&payload, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    
    stream << frame.frameNumber << (int)frame.frameData.size();
    stream.writeRawData(frame.frameData.constData(), dataSize);
    
    int padding = 0;
    if (payload.size() < DATA_PAYLOAD_SIZE) {
        padding = DATA_PAYLOAD_SIZE - payload.size();
        payload.append(QByteArray(padding, 0));
    }
    
    m_packetsToSend.append(qMakePair(START_FRAME, payload));
    
    qDebug() << "FrameSender: START_FRAME - Stream:" << frame.streamId 
             << "Frame:" << frame.frameNumber 
             << "Total size:" << frame.frameData.size()
             << "Chunk:" << dataSize
             << "Payload:" << payload.size()
             << "Padding:" << padding
             << "Position:" << frame.currentPosition << "->" << (frame.currentPosition + dataSize);
}

void FrameSender::processAllFrames()
{
    qDebug() << "FrameSender: processAllFrames - Queue size:" << m_frameQueue.size();
    
    while (!m_frameQueue.isEmpty()) {
        FrameQueueItem &currentFrame = m_frameQueue.first();
        int remaining = currentFrame.frameData.size() - currentFrame.currentPosition;
        
        qDebug() << "FrameSender: Processing frame - Stream:" << currentFrame.streamId
                 << "Frame:" << currentFrame.frameNumber
                 << "Remaining:" << remaining;
        
        if (remaining <= 0) {
            qDebug() << "FrameSender: Frame completed - Stream:" << currentFrame.streamId
                     << "Frame:" << currentFrame.frameNumber;
            m_frameQueue.removeFirst();
            continue;
        }

        if (currentFrame.currentPosition == 0) {
            int chunkSize = qMin(START_PAYLOAD, remaining);
            sendStartFrame(currentFrame, chunkSize);
            currentFrame.currentPosition += chunkSize;
        } else {
            if (remaining > END_PAYLOAD) {
                int chunkSize = qMin(CONTINUE_PAYLOAD, remaining);
                sendContinueFrame(currentFrame, chunkSize);
                currentFrame.currentPosition += chunkSize;
            } else {
                sendEndFrame(currentFrame, remaining);
                currentFrame.currentPosition += remaining;
            }
        }
        
        if (currentFrame.currentPosition >= currentFrame.frameData.size()) {
            qDebug() << "FrameSender: Frame fully processed - Stream:" << currentFrame.streamId
                     << "Frame:" << currentFrame.frameNumber;
            m_frameQueue.removeFirst();
        }
    }
    
    qDebug() << "FrameSender: processAllFrames completed - Packets to send:" << m_packetsToSend.size();
}

void FrameSender::sendContinueFrame(const FrameQueueItem &frame, int dataSize)
{
    QByteArray payload;
    QDataStream stream(&payload, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    
    stream << frame.frameNumber;
    stream.writeRawData(frame.frameData.constData() + frame.currentPosition, dataSize);
    
    m_packetsToSend.append(qMakePair(CONTINUE_FRAME, payload));
    
    qDebug() << "FrameSender: CONTINUE_FRAME - Stream:" << frame.streamId 
             << "Frame:" << frame.frameNumber 
             << "Chunk:" << dataSize
             << "Payload:" << payload.size()
             << "Position:" << frame.currentPosition << "->" << (frame.currentPosition + dataSize)
             << "Remaining:" << (frame.frameData.size() - frame.currentPosition - dataSize);
}

void FrameSender::sendEndFrame(const FrameQueueItem &frame, int dataSize)
{
    QByteArray payload;
    QDataStream stream(&payload, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    
    stream << frame.frameNumber;
    stream.writeRawData(frame.frameData.constData() + frame.currentPosition, dataSize);
    
    int padding = 0;
    if (payload.size() < DATA_PAYLOAD_SIZE) {
        padding = DATA_PAYLOAD_SIZE - payload.size();
        payload.append(QByteArray(padding, 0));
    }
    
    m_packetsToSend.append(qMakePair(END_FRAME, payload));
    
    qDebug() << "FrameSender: END_FRAME - Stream:" << frame.streamId 
             << "Frame:" << frame.frameNumber 
             << "Real data:" << dataSize
             << "Payload:" << payload.size()
             << "Padding:" << padding
             << "Position:" << frame.currentPosition << "->" << (frame.currentPosition + dataSize);
}

FrameAssembler::~FrameAssembler()
{
    // Очищаем все контейнеры
    m_streamAssemblies.clear();
    m_orphanedPackets.clear();
    m_completeFrames.clear();
    qDebug() << "FrameAssembler: Destroyed";
}

FrameSender::~FrameSender()
{
    clear();
    qDebug() << "FrameSender: Destroyed";
}