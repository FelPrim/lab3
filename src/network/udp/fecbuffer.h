#pragma once

#include <QObject>
#include <QHash>
#include <QVector>
#include <QByteArray>
#include <QDateTime>
#include <QMutex>
#include "network_packet.h"
#include "../../video_defaults.h"

class PacketGroupBuffer;

class FecBuffer : public QObject
{
    Q_OBJECT

public:
    explicit FecBuffer(int streamId, QObject *parent = nullptr);
    
    void setPacketGroupBuffer(PacketGroupBuffer* packetGroupBuffer);
    bool addPacket(const NetworkPacket &packet);
    void cleanup(qint64 maxAgeMs = 10000);
    
    // Статистика
    int getRecoveredCount() const { return m_recoveredCount; }
    int getGroupCount() const { return m_groups.size(); }
    uint32_t getLastCleanedSequence() const { return m_lastCleanedSequence; }

signals:
    void packetRecovered(uint32_t packetSequence);

private:
    struct FecGroup {
        int groupId;
        uint32_t callId;
        uint32_t streamId;
        QVector<QByteArray> packets;      // 5 элементов: 0-3 - данные, 4 - XOR
        QVector<bool> received;           // Флаги получения
        qint64 creationTime;              // Время создания группы
        qint64 lastUpdateTime;            // Время последнего обновления
        
        FecGroup() : groupId(-1), callId(0), streamId(0), 
                    creationTime(0), lastUpdateTime(0) {
            packets.resize(FEC_TOTAL_PACKETS);
            received.resize(FEC_TOTAL_PACKETS, false);
        }
        
        bool canRecover() const {
            int missingCount = 0;
            for (int i = 0; i < FEC_DATA_PACKETS; ++i) {
                if (!received[i]) missingCount++;
            }
            return missingCount == 1 && received[FEC_DATA_PACKETS];
        }
        
        int getMissingIndex() const {
            for (int i = 0; i < FEC_DATA_PACKETS; ++i) {
                if (!received[i]) return i;
            }
            return -1;
        }
    };
    
    int m_streamId;
    QHash<int, FecGroup> m_groups;
    PacketGroupBuffer* m_packetGroupBuffer;
    int m_recoveredCount = 0;
    
    // Трекер последнего очищенного packetSequence
    uint32_t m_lastCleanedSequence = 0;
    QMutex m_sequenceMutex;
    
    // Вспомогательные методы
    void forwardPacketToGroupBuffer(const NetworkPacket &packet);
    bool shouldProcessPacket(uint32_t packetSequence, qint64 currentTime);
    void updateLastCleanedSequence(uint32_t packetSequence);
    
    // Восстановление конкретного пакета в группе
    bool recoverPacketInGroup(int groupId, int missingIndex, qint64 currentTime);
};