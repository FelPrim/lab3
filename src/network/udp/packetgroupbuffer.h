#pragma once

#include <QObject>
#include <QHash>
#include <QVector>
#include <QByteArray>
#include <QDateTime>
#include <QMutex>
#include "network_packet.h"

class PacketGroupBuffer : public QObject
{
    Q_OBJECT

public:
    struct FramePackets {
        int frameNumber;
        QHash<int, QByteArray> packets;  // packetIndex -> packetData
        QVector<bool> received;
        qint64 timestamp;
        bool hasStartFrame;
        int totalPackets;
        
        FramePackets() : frameNumber(-1), timestamp(0), 
                        hasStartFrame(false), totalPackets(0) {}
        
        bool isComplete() const {
            if (!hasStartFrame) return false;
            
            for (bool recv : received) {
                if (!recv) return false;
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
    int m_completedCount = 0;
    
    // Обработка разных типов пакетов
    void processStartFrame(const NetworkPacket &packet, const QByteArray &payload);
    void processContinueFrame(const NetworkPacket &packet, const QByteArray &payload, int packetIndex);
    void processEndFrame(const NetworkPacket &packet, const QByteArray &payload, int packetIndex);
    
    // Сборка фрейма
    QByteArray assembleFrame(const FramePackets &frameData);
    
    // Утилиты
    int extractFrameNumber(const QByteArray &payload, int offset = 0) const;
    void checkFrameCompletion(int frameNumber);
};