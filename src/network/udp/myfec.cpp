#include "myfec.h"
#include <QTimer>
#include <QDataStream>
#include <QDateTime>
#include <QDebug>

// Используем константы из video_defaults
const int START_PAYLOAD = DATA_PAYLOAD_SIZE - 8;  // 1179 байта (1191 - frameNumber(4) - frameSize(4))
const int CONTINUE_PAYLOAD = DATA_PAYLOAD_SIZE - 4; // 1183 байт (1191 - frameNumber(4))
const int END_PAYLOAD = DATA_PAYLOAD_SIZE - 4;      // 1183 байт (1191 - frameNumber(4))

// FrameAssembler implementation
FrameAssembler::FrameAssembler(QObject *parent) : QObject(parent) {}

void FrameAssembler::processOrphanedPackets(int streamId)
{
    if (m_orphanedPackets.contains(streamId)) {
        auto orphaned = m_orphanedPackets.values(streamId);
        qDebug() << "Processing" << orphaned.size() << "orphaned packets for stream:" << streamId;
        
        for (const auto& packet : orphaned) {
            processPacket(streamId, packet.first, packet.second);
        }
        m_orphanedPackets.remove(streamId);
    }
}

void FrameAssembler::processPacket(int streamId, PacketType type, const QByteArray &payload)
{
    qDebug() << "FrameAssembler: Processing packet - Stream:" << streamId 
             << "Type:" << type;
    
    if (type != START_FRAME && !m_streamAssemblies.contains(streamId)) {
        m_orphanedPackets.insert(streamId, qMakePair(type, payload));
        qDebug() << "Orphaned packet saved - Stream:" << streamId << "Type:" << type;
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
        qDebug() << "Unknown packet type in FrameAssembler:" << type;
        break;
    }
}

void FrameAssembler::processStartFrame(int streamId, const QByteArray &data)
{

    const int START_FRAME_PAYLOAD_HEADER = 8; // frameNumber(4) + frameSize(4)
    if (data.size() < START_FRAME_PAYLOAD_HEADER) { 
        qDebug() << "START_FRAME too small:" << data.size();
        return;
    }

    QDataStream stream(data);
    stream.setByteOrder(QDataStream::BigEndian);

    int frameNumber, frameSize;
    stream >> frameNumber >> frameSize;

    if (frameSize <= 0 || frameSize > 10 * 1024 * 1024) {
        qDebug() << "Invalid frame size in START_FRAME:" << frameSize;
        return;
    }

    QByteArray frameData = data.mid(START_FRAME_PAYLOAD_HEADER); 

    // В START_FRAME тоже может быть дополнение, берем только нужное количество
    int bytesToTake = qMin(frameData.size(), frameSize);

    qDebug() << "START_FRAME - Stream:" << streamId << "Frame:" << frameNumber 
             << "Size:" << frameSize << "Data:" << bytesToTake;

    m_streamAssemblies[streamId] = StreamAssembly(streamId, frameNumber);

    StreamAssembly& assembly = m_streamAssemblies[streamId];
    assembly.totalSize = frameSize;
    assembly.data = frameData.left(bytesToTake);
    assembly.receivedSize = bytesToTake;
    assembly.hasStartFrame = true;

    if (assembly.isComplete()) {
        completeFrame(streamId);
    }
}

void FrameAssembler::processContinueFrame(int streamId, const QByteArray &data)
{
    if (!m_streamAssemblies.contains(streamId)) {
        qDebug() << "CONTINUE_FRAME for unknown stream:" << streamId;
        return;
    }
    
    StreamAssembly& assembly = m_streamAssemblies[streamId];
    
    if (data.size() < 4) {
        qDebug() << "CONTINUE_FRAME too small:" << data.size();
        return;
    }
    
    QDataStream stream(data);
    stream.setByteOrder(QDataStream::BigEndian);
    
    int frameNumber;
    stream >> frameNumber;
    
    if (frameNumber != assembly.frameNumber) {
        qDebug() << "CONTINUE_FRAME frame number mismatch. Expected:" 
                 << assembly.frameNumber << "Got:" << frameNumber;
        return;
    }
    
    // В CONTINUE_FRAME нет дополнения нулями - берем все данные как есть
    QByteArray frameData = data.mid(4);
    
    qDebug() << "CONTINUE_FRAME - Stream:" << streamId << "Frame:" << frameNumber 
             << "Data:" << frameData.size() << "Total received:" << assembly.receivedSize + frameData.size();
    
    assembly.data.append(frameData);
    assembly.receivedSize += frameData.size();
    
    if (assembly.isComplete()) {
        completeFrame(streamId);
    }
}

void FrameAssembler::processEndFrame(int streamId, const QByteArray &data)
{
    if (!m_streamAssemblies.contains(streamId)) {
        qDebug() << "END_FRAME for unknown stream:" << streamId;
        return;
    }
    
    StreamAssembly& assembly = m_streamAssemblies[streamId];
    
    if (data.size() < 4) {
        qDebug() << "END_FRAME too small:" << data.size();
        return;
    }
    
    QDataStream stream(data);
    stream.setByteOrder(QDataStream::BigEndian);
    
    int frameNumber;
    stream >> frameNumber;
    
    if (frameNumber != assembly.frameNumber) {
        qDebug() << "END_FRAME frame number mismatch. Expected:" 
                 << assembly.frameNumber << "Got:" << frameNumber;
        return;
    }
    
    // В END_FRAME есть дополнение нулями - берем только нужное количество данных
    QByteArray frameData = data.mid(4);
    
    // Вычисляем сколько данных нам еще нужно
    int neededBytes = assembly.totalSize - assembly.receivedSize;
    if (neededBytes <= 0) {
        qDebug() << "END_FRAME: already have all data, ignoring";
        return;
    }
    
    // Берем только нужное количество байт (игнорируем дополнение нулями)
    int bytesToTake = qMin(neededBytes, frameData.size());
    QByteArray realData = frameData.left(bytesToTake);
    
    qDebug() << "END_FRAME - Stream:" << streamId << "Frame:" << frameNumber 
             << "Needed:" << neededBytes << "Taking:" << bytesToTake
             << "Total received:" << assembly.receivedSize + bytesToTake;
    
    assembly.data.append(realData);
    assembly.receivedSize += bytesToTake;
    
    if (assembly.isComplete()) {
        completeFrame(streamId);
    }
}

void FrameAssembler::completeFrame(int streamId)
{
    if (!m_streamAssemblies.contains(streamId)) {
        return;
    }
    
    StreamAssembly& assembly = m_streamAssemblies[streamId];
    
    if (assembly.data.size() >= assembly.totalSize) {
        QByteArray completeData = assembly.data.left(assembly.totalSize);
        
        qDebug() << "FrameAssembler: COMPLETE - Stream:" << streamId 
                 << "Frame:" << assembly.frameNumber 
                 << "Total size:" << assembly.totalSize 
                 << "Received:" << assembly.receivedSize;

        // Добавляем в очередь завершенных фреймов
        m_completeFrames.append(qMakePair(streamId, completeData));
        
        // Испускаем сигнал
        emit frameAssembled(streamId, assembly.frameNumber, completeData);
        
        // Удаляем из текущих сборок
        m_streamAssemblies.remove(streamId);
    }
}

bool FrameAssembler::hasCompleteFrame() const
{
    return !m_completeFrames.isEmpty();
}

QPair<int, QByteArray> FrameAssembler::takeCompleteFrame()
{
    if (m_completeFrames.isEmpty()) {
        return qMakePair(0, QByteArray());
    }
    return m_completeFrames.takeFirst();
}

void FrameAssembler::cleanupOldAssemblies(qint64 maxAgeMs)
{
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    
    auto it = m_streamAssemblies.begin();
    while (it != m_streamAssemblies.end()) {
        if (currentTime - it->creationTime > maxAgeMs) {
            qDebug() << "Cleaning up old assembly - Stream:" << it->streamId 
                     << "Frame:" << it->frameNumber;
            it = m_streamAssemblies.erase(it);
        } else {
            ++it;
        }
    }
}

// StreamAssembly implementation
FrameAssembler::StreamAssembly::StreamAssembly(int stream, int frame)
    : streamId(stream), frameNumber(frame), totalSize(0), receivedSize(0),
      creationTime(QDateTime::currentMSecsSinceEpoch()), hasStartFrame(false)
{}

bool FrameAssembler::StreamAssembly::isComplete() const
{
    return hasStartFrame && receivedSize >= totalSize;
}

// FrameSender implementation
FrameSender::FrameSender(QObject *parent) : QObject(parent) {}

bool FrameSender::hasPacketsToSend() const
{
    return !m_packetsToSend.isEmpty();
}

QVector<QPair<PacketType, QByteArray>> FrameSender::takePacketsToSend()
{
    QVector<QPair<PacketType, QByteArray>> packets = m_packetsToSend;
    m_packetsToSend.clear();
    return packets;
}

void FrameSender::clear()
{
    m_frameQueue.clear();
    m_packetsToSend.clear();
}

void FrameSender::addFrame(int streamId, int frameNumber, const QByteArray &frameData)
{
    m_frameQueue.append(FrameQueueItem(streamId, frameNumber, frameData));
    processAllFrames();
}

int FrameSender::calculateOptimalChunkSize(const FrameQueueItem &frame, PacketType type) const
{
    int remaining = frame.frameData.size() - frame.currentPosition;
    
    switch (type) {
    case START_FRAME:
        return qMin(START_PAYLOAD, remaining);
    case CONTINUE_FRAME:
        return qMin(CONTINUE_PAYLOAD, remaining);
    case END_FRAME:
        return qMin(END_PAYLOAD, remaining);
    default:
        return qMin(DATA_PAYLOAD_SIZE, remaining);
    }
}

// FrameQueueItem implementation
FrameSender::FrameQueueItem::FrameQueueItem(int stream, int frame, const QByteArray &data)
    : streamId(stream), frameNumber(frame), frameData(data), currentPosition(0)
{}

void FrameSender::sendStartFrame(const FrameQueueItem &frame, int dataSize)
{
    QByteArray payload;
    QDataStream stream(&payload, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    
    stream << frame.frameNumber << (int)frame.frameData.size();
    stream.writeRawData(frame.frameData.constData(), dataSize);
    
    // Дополняем нулями если нужно
    if (payload.size() < DATA_PAYLOAD_SIZE) {
        payload.append(QByteArray(DATA_PAYLOAD_SIZE - payload.size(), 0));
    }
    
    m_packetsToSend.append(qMakePair(START_FRAME, payload));
    
    qDebug() << "Sent START_FRAME - Stream:" << frame.streamId 
             << "Frame:" << frame.frameNumber 
             << "Size:" << frame.frameData.size()
             << "Chunk:" << dataSize;
}

void FrameSender::processAllFrames()
{
    while (!m_frameQueue.isEmpty()) {
        FrameQueueItem &currentFrame = m_frameQueue.first();
        int remaining = currentFrame.frameData.size() - currentFrame.currentPosition;
        
        if (remaining <= 0) {
            m_frameQueue.removeFirst();
            continue;
        }

        if (currentFrame.currentPosition == 0) {
            // START_FRAME - никогда не дополняется нулями
            int chunkSize = qMin(START_PAYLOAD, remaining);
            sendStartFrame(currentFrame, chunkSize);
            currentFrame.currentPosition += chunkSize;
        } else {
            if (remaining > END_PAYLOAD) {
                // CONTINUE_FRAME - никогда не дополняется нулями
                int chunkSize = qMin(CONTINUE_PAYLOAD, remaining);
                sendContinueFrame(currentFrame, chunkSize);
                currentFrame.currentPosition += chunkSize;
            } else {
                // END_FRAME - единственный пакет, который дополняется нулями
                sendEndFrame(currentFrame, remaining);
                currentFrame.currentPosition += remaining;
            }
        }
        
        // Удаляем завершенный фрейм
        if (currentFrame.currentPosition >= currentFrame.frameData.size()) {
            m_frameQueue.removeFirst();
        }
    }
}

void FrameSender::sendContinueFrame(const FrameQueueItem &frame, int dataSize)
{
    QByteArray payload;
    QDataStream stream(&payload, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    
    stream << frame.frameNumber;
    stream.writeRawData(frame.frameData.constData() + frame.currentPosition, dataSize);
    
    // CONTINUE_FRAME НИКОГДА не дополняется нулями!
    m_packetsToSend.append(qMakePair(CONTINUE_FRAME, payload));
    
    qDebug() << "Sent CONTINUE_FRAME - Stream:" << frame.streamId 
             << "Frame:" << frame.frameNumber 
             << "Chunk:" << dataSize
             << "Position:" << frame.currentPosition;
}

void FrameSender::sendEndFrame(const FrameQueueItem &frame, int dataSize)
{
    QByteArray payload;
    QDataStream stream(&payload, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    
    stream << frame.frameNumber;
    stream.writeRawData(frame.frameData.constData() + frame.currentPosition, dataSize);
    
    // ТОЛЬКО END_FRAME дополняется нулями до DATA_PAYLOAD_SIZE
    if (payload.size() < DATA_PAYLOAD_SIZE) {
        payload.append(QByteArray(DATA_PAYLOAD_SIZE - payload.size(), 0));
    }
    
    m_packetsToSend.append(qMakePair(END_FRAME, payload));
    
    qDebug() << "Sent END_FRAME - Stream:" << frame.streamId 
             << "Frame:" << frame.frameNumber 
             << "Real data:" << dataSize
             << "Padded to:" << payload.size();
}
