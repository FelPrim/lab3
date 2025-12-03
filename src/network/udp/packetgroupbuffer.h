#pragma once

#include <QObject>
#include <QVector>
#include <QByteArray>
#include <QHash>
#include <QList>
#include <QDateTime>
#include <cstdint>
#include <limits>
#include "network_packet.h"
#include "../../video_defaults.h"

class PacketGroupBuffer : public QObject
{
    Q_OBJECT
public:
    explicit PacketGroupBuffer(int streamId, QObject *parent = nullptr);

    void addPacket(const NetworkPacket &packet);
    void cleanup();
    void cleanupOldFramesByTimeout(qint64 maxAgeMs = 2000); // НОВЫЙ МЕТОД

    int getFrameCount() const { return m_groups.size(); }
    int getCompletedCount() const { return m_completedCount; }
    int getLatestFrameNumber() const { return m_latestFrameNumber; }
    qint64 getLastActivityTime() const { return m_lastActivityTime; }

signals:
    void frameComplete(int streamId, int frameNumber, const QByteArray &frameData);
    void frameDropped(int streamId, int frameNumber, const QString &reason); // НОВЫЙ СИГНАЛ

private:
    struct TempPacket {
        uint32_t packetSequence;
        uint8_t rawType; // raw type byte stored in dataPacket.type (without FEC bit)
        QByteArray payload; // full payload (1187 bytes)
    };

    struct FrameGroup {
        int frameNumber = -1;
        uint32_t startSequence = std::numeric_limits<uint32_t>::max();
        int frameSize = 0;
        int totalPackets = 0;
        int packetsReceived = 0;
        qint64 creationTimeMs = 0;
        qint64 lastUpdateTimeMs = 0; // НОВОЕ ПОЛЕ: время последнего обновления

        QVector<QByteArray> packets; // index -> data (packet body without frameNumber/size prefix)
        QVector<char> received;      // index -> 0/1
        QList<TempPacket> tempPackets;

        // В FrameGroup структуре:
bool isStale(qint64 currentTime, qint64 maxAgeMs) const {
    // Кадр считается устаревшим если:
    // 1. Прошло слишком много времени с момента создания (даже если пакеты приходят)
    // 2. И он не собран
    if (isComplete()) return false;
    return (currentTime - creationTimeMs) > maxAgeMs;
}

        FrameGroup() : 
            creationTimeMs(QDateTime::currentMSecsSinceEpoch()),
            lastUpdateTimeMs(QDateTime::currentMSecsSinceEpoch()) 
        {}
        
        bool hasStart() const { return startSequence != std::numeric_limits<uint32_t>::max(); }
        bool isComplete() const { return totalPackets > 0 && packetsReceived == totalPackets; }
        bool isExpired(qint64 currentTime, qint64 maxAgeMs) const {
            return (currentTime - lastUpdateTimeMs) > maxAgeMs;
        }
        void updateLastActivity() {
            lastUpdateTimeMs = QDateTime::currentMSecsSinceEpoch();
        }
    };

    int m_streamId;
    QHash<int, FrameGroup> m_groups; // frameNumber -> FrameGroup
    int m_completedCount = 0;
    int m_latestFrameNumber = -1;
    qint64 m_lastActivityTime = 0; // Время последней активности (получения пакета)

    static constexpr int START_FIRST_DATA = DATA_PAYLOAD_SIZE - 8;   // 1187 - 8 = 1179
    static constexpr int CONTINUE_DATA = DATA_PAYLOAD_SIZE - 4;     // 1187 - 4 = 1183

    int getRelativePacketIndex(uint32_t currentSequence, uint32_t startSequence) const;
    int calculateTotalPackets(int frameSize, int firstPacketDataSize) const;
    int extractFrameNumber(const QByteArray &payload, int offset = 0) const;
    int extractFrameSize(const QByteArray &payload, int offset = 4) const;

    void handleStart(FrameGroup &group, const TempPacket &tp, uint32_t packetSequence);
    void handleContinue(FrameGroup &group, const TempPacket &tp, uint32_t packetSequence);
    void handleEnd(FrameGroup &group, const TempPacket &tp, uint32_t packetSequence);
    void tryAssemble(FrameGroup &group);
    void cleanupOldFrames(); // по количеству кадров
    void cleanupExpiredFrames(qint64 maxAgeMs); // по таймауту (НОВЫЙ МЕТОД)
    void cleanupOldestIncompleteFrame();
};