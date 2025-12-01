#pragma once

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <QSet>
#include <QHostAddress>
#include <QDateTime>
#include <QVariant>
#include <zlib.h>
#include <QtEndian>
#include <QMutex>
#include <QQueue>
#include "myfec.h"
#include "network_packet.h"
#include "../../video_defaults.h"

class UDPManager;

class NetworkManager : public QObject
{
    Q_OBJECT

public:
    explicit NetworkManager(int streamId, QObject *parent = nullptr);
    ~NetworkManager() override;

    bool initialize(UDPManager *udpManager);
    void cleanup();
    void sendVideoFrame(int frameNumber, const QByteArray &frameData);
    void setServerAddress(const QString &address, quint16 port);
    QHostAddress getServerAddress() const { return m_serverAddress; }
    quint16 getServerPort() const { return m_serverPort; }
    int getStreamId() const { return m_streamId; }

    // Call ID management
    void setCallId(uint32_t callId) { m_callId = callId; }
    uint32_t getCallId() const { return m_callId; }

    // Methods for managing sending
    void setSendingEnabled(bool enabled) { m_sendingEnabled = enabled; }
    bool isSendingEnabled() const { return m_sendingEnabled; }

signals:
    void frameAssembled(int streamId, int frameNumber, const QByteArray &frameData);
    void errorOccurred(const QString &message);
    void statisticsUpdated(const QString &stats);

public slots:
    void start();
    void stop();
    void processPacket(const QByteArray &data, const QHostAddress &sender, quint16 port);

private slots:
    void cleanupOldAssemblies();
    void printStatistics();
    void onFrameAssembled(int streamId, int frameNumber, const QByteArray &frameData);

private:
    // Основные методы из старой реализации
    void processPacketNewProtocol(const QByteArray &data);
    void sendPacketNewProtocol(const QByteArray &data, PacketType type);
    void sendPacketNewProtocol(const QByteArray &data, PacketType type, int customSequence);
    
    // FEC методы из старой реализации
    void processXorPacket(const NetworkPacket& packet);
    void tryRecoverLostPackets(int groupId);
    
    void updateSendStats(int packets, int bytes);
    void updateReceiveStats(int packets, int bytes);
    uint32_t calculateCRC32(const QByteArray &data);

private:
    UDPManager *m_udpManager = nullptr;
    QHostAddress m_serverAddress;
    quint16 m_serverPort;
    
    QTimer *m_cleanupTimer = nullptr;
    QTimer *m_statsTimer = nullptr;
    
    // Компоненты из старой реализации
    FrameAssembler *m_frameAssembler = nullptr;
    FrameSender *m_frameSender = nullptr;
    
    int m_streamId;
    uint32_t m_callId = 0;
    int m_packetSequence = 0;
   
    // FEC буферы из старой реализации

    QHash<int, QVector<QByteArray>> m_fecReceiveBuffers;
    QHash<int, QVector<bool>> m_fecReceived;

    uint8_t m_fecBuffer[4][1188];
    int m_fecBufferCount;

    struct Statistics {
        quint64 totalPacketsSent = 0;
        quint64 totalPacketsReceived = 0;
        quint64 totalBytesSent = 0;
        quint64 totalBytesReceived = 0;
        quint64 framesSent = 0;
        quint64 framesReceived = 0;
        quint64 assembliesCompleted = 0;
        quint64 assembliesDropped = 0;
        
        QSet<QPair<int, int>> expectedFrames;
        QSet<QPair<int, int>> receivedFrames;
        
        quint64 fecGroupsSent = 0;
        quint64 fecGroupsRecovered = 0;
        quint64 packetsRecoveredByFEC = 0;
    } m_stats;
    
    QElapsedTimer m_operationTimer;
    bool m_initialized = false;
    bool m_sendingEnabled = false;
private:
    static const int MAX_FEC_GROUPS = 50;
    QHash<int, qint64> m_fecGroupTimestamps; 
private:
    static const int MAX_FEC_BUFFER_SIZE = 128;


};