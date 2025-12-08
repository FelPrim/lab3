#pragma once

#include <QObject>
#include <QByteArray>
#include <QVector>
#include <QHash>
#include <QPair>
#include <QSet>
#include <QDataStream>
#include <QIODevice>
#include "../../video_defaults.h"

enum PacketType {
    START_FRAME = 0x01,
    CONTINUE_FRAME = 0x02,
    END_FRAME = 0x03,
    FEC_PACKET = 0x04
};

// Добавьте макросы для работы с FEC-флагом
#define FEC_FLAG 0x80
#define PACKET_TYPE_MASK 0x7F

class FrameAssembler : public QObject
{
    Q_OBJECT

public:
    explicit FrameAssembler(QObject *parent = nullptr);
    
    // Обработка входящих пакетов
    void processPacket(int streamId, PacketType type, const QByteArray &payload);
    
    // Получение собранных фреймов
    bool hasCompleteFrame() const;
    QPair<int, QByteArray> takeCompleteFrame();
    // Очистка устаревших данных
    void cleanupOldAssemblies(qint64 maxAgeMs = 10000);
    
    // Методы для тестирования
    bool hasAssemblyForStream(int streamId) const { return m_streamAssemblies.contains(streamId); }
    int getAssemblyFrameNumber(int streamId) const { 
        return m_streamAssemblies.contains(streamId) ? m_streamAssemblies[streamId].frameNumber : -1; 
    }
    int getAssemblyReceivedSize(int streamId) const { 
        return m_streamAssemblies.contains(streamId) ? m_streamAssemblies[streamId].receivedSize : -1; 
    }
    void processOrphanedPackets(int streamId);
    virtual ~FrameAssembler(); 

signals:
    void frameAssembled(int streamId, int frameNumber, const QByteArray &frameData);

private:
    struct StreamAssembly {
        int streamId;
        int frameNumber;
        int totalSize;
        int receivedSize;
        QByteArray data;
        qint64 creationTime;
        bool hasStartFrame;
        
        StreamAssembly() 
            : streamId(0), frameNumber(0), totalSize(0), receivedSize(0),
              creationTime(0), hasStartFrame(false) 
        {}
        
        StreamAssembly(int stream, int frame);
        bool isComplete() const;
    };
    
    void processStartFrame(int streamId, const QByteArray &data);
    void processContinueFrame(int streamId, const QByteArray &data);
    void processEndFrame(int streamId, const QByteArray &data);
    void completeFrame(int streamId);
    
    QHash<int, StreamAssembly> m_streamAssemblies;
    QMultiHash<int, QPair<PacketType, QByteArray>> m_orphanedPackets; 
    QList<QPair<int, QByteArray>> m_completeFrames; 
};

class FrameSender : public QObject
{
    Q_OBJECT

public:
    explicit FrameSender(QObject *parent = nullptr);
    virtual ~FrameSender(); 
    // Добавление фрейма для отправки
    void addFrame(int streamId, int frameNumber, const QByteArray &frameData);
    
    // Получение пакетов для отправки
    bool hasPacketsToSend() const;
    QVector<QPair<PacketType, QByteArray>> takePacketsToSend();
    
    // Управление буфером
    void clear();

private:
    void processFecPacket(int streamId, int packetSequence, const QByteArray &payload);
    void sendFecPackets();
    QByteArray calculateXorForGroup(const QVector<QByteArray> &packets);
    struct FrameQueueItem {
        int streamId;
        int frameNumber;
        QByteArray frameData;
        int currentPosition;
        
        FrameQueueItem(int stream, int frame, const QByteArray &data);
    };

    QVector<FrameQueueItem> m_frameQueue;
    QVector<QPair<PacketType, QByteArray>> m_packetsToSend;
    
    void processAllFrames();
    void sendStartFrame(const FrameQueueItem &frame, int dataSize);
    void sendContinueFrame(const FrameQueueItem &frame, int dataSize);
    void sendEndFrame(const FrameQueueItem &frame, int dataSize);
    
    int calculateOptimalChunkSize(const FrameQueueItem &frame, PacketType type) const;
    QHash<int, QVector<QByteArray>> m_fecSendBuffers; // groupId -> packets
    QHash<int, QVector<QByteArray>> m_fecReceiveBuffers; // groupId -> packets
    QHash<int, QVector<bool>> m_fecReceived; // groupId -> received flags
private:
    static const int MAX_FRAME_QUEUE_SIZE = DEFAULT_FPS; 
};
