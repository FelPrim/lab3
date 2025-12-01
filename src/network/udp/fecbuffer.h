#pragma once

#include <QObject>
#include <QHash>
#include <QVector>
#include <QByteArray>
#include <QDateTime>
#include "network_packet.h"
#include "../../video_defaults.h"
#include "network_packet.h" 

class FecBuffer : public QObject
{
    Q_OBJECT

public:
    explicit FecBuffer(int streamId, QObject *parent = nullptr);
    
    // Добавить пакет для FEC обработки
    bool addPacket(const NetworkPacket &packet);
    
    // Попытаться восстановить потерянные пакеты
    void tryRecoverLostPackets();
    
    // Очистить старые данные (старше maxAgeMs миллисекунд)
    void cleanup(qint64 maxAgeMs = 1000);
    
    // Получить готовые пакеты (включая восстановленные)
    QList<NetworkPacket> getReadyPackets();
    
    // Получить статистику
    int getRecoveredCount() const { return m_recoveredCount; }
    int getGroupCount() const { return m_groups.size(); }

signals:
    void packetRecovered(uint32_t packetSequence);
    void packetReady(const NetworkPacket &packet);

private:
    struct FecGroup {
        int groupId;
        QVector<QByteArray> packets;  // 5 элементов: 0-3 - данные, 4 - XOR
        QVector<bool> received;       // Флаги получения
        qint64 timestamp;
        
        FecGroup() : groupId(-1), timestamp(0) {
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
    QList<NetworkPacket> m_readyPackets;
    int m_recoveredCount = 0;
    
    // Методы восстановления
    bool recoverPacket(FecGroup &group, int missingIndex);
    NetworkPacket createRecoveredPacket(const FecGroup &group, int missingIndex, const QByteArray &data);
    
    // Утилиты
    int calculateGroupId(uint32_t packetSequence) const;
    int calculatePositionInGroup(uint32_t packetSequence) const;
};