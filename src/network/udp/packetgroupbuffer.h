#pragma once

#include <QObject>
#include <QHash>
#include <QVector>
#include <QByteArray>
#include <QDateTime>
#include <QMutex>
#include "network_packet.h"
#include "myfec.h"
#include "../../video_defaults.h"

class PacketGroupBuffer : public QObject
{
    Q_OBJECT

public:
    struct FramePackets {
        int frameNumber;
        int startSequence; // packetSequence из START_FRAME
        int frameSize;     // Общий размер фрейма в байтах
        int totalPackets;  // Общее количество пакетов во фрейме
        QHash<int, QByteArray> packets;  // packetIndex -> packetData
        QVector<bool> received;
        qint64 timestamp;
        
        FramePackets() : frameNumber(-1), startSequence(-1), frameSize(0), 
                         totalPackets(0), timestamp(0) {}
        
        bool isComplete() const {
            for (int i = 0; i < totalPackets; ++i) {
                if (!received[i]) return false;
            }
            return true;
        }
    };

    explicit PacketGroupBuffer(int streamId, QObject *parent = nullptr);
    
    // Добавить пакет (после FEC)
    void addPacket(const NetworkPacket &packet);
    
    // Получить готовые фреймы
    QList<QPair<int, QByteArray>> getCompleteFrames();
    
    // Очистить старые данные
    void cleanup(qint64 maxAgeMs = 1000);
    
    // Статистика
    int getFrameCount() const { return m_frames.size(); }
    int getCompletedCount() const { return m_completedCount; }

signals:
    void frameComplete(int streamId, int frameNumber, const QByteArray &frameData);

private:
    int m_streamId;
    QHash<int, FramePackets> m_frames;  // frameNumber -> FramePackets
    QList<QPair<int, QByteArray>> m_completeFrames;
    QMultiHash<int, QPair<PacketType, QByteArray>> m_orphanedPackets; // frameNumber -> (type, payload)
    int m_completedCount = 0;
    
    // Константы размеров (согласованы с FrameSender)
    static const int START_PAYLOAD = DATA_PAYLOAD_SIZE - 8;     // 1179 байт
    static const int CONTINUE_PAYLOAD = DATA_PAYLOAD_SIZE - 4;  // 1183 байт
    static const int END_PAYLOAD = DATA_PAYLOAD_SIZE - 4;       // 1183 байт
    
    // Обработка разных типов пакетов
    void processStartFrame(const NetworkPacket &packet, const QByteArray &payload, uint32_t packetSequence);
    void processContinueFrame(const NetworkPacket &packet, const QByteArray &payload, 
                             uint32_t packetSequence, int frameNumber);
    void processEndFrame(const NetworkPacket &packet, const QByteArray &payload,
                        uint32_t packetSequence, int frameNumber);
    
    // Сборка фрейма
    QByteArray assembleFrame(const FramePackets &frameData);
    
    // Утилиты
    int extractFrameNumber(const QByteArray &payload, int offset = 0) const;
    void checkFrameCompletion(int frameNumber);
    void processOrphanedPackets(int frameNumber);
    
    // Вычисление количества пакетов
    int calculateTotalPackets(int frameSize, int firstPacketDataSize) const;
};
