#pragma once

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <QSet>
#include <QHostAddress>
#include <QNetworkDatagram>
#include "video_defaults.h"
#include <QDateTime> 
#include <QVariant>
#include <zlib.h>
#include "myfec.h"
#include "network_packet.h" 

// Forward declaration
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

    // New method for packet processing from UDPManager
    void processPacketFromNetwork(const QByteArray &data, const QHostAddress &sender, quint16 port);

    // Методы для управления отправкой
    void setSendingEnabled(bool enabled) { m_sendingEnabled = enabled; }
    bool isSendingEnabled() const { return m_sendingEnabled; }

signals:
    void frameAssembled(int streamId, int frameNumber, const QByteArray &frameData);
    void errorOccurred(const QString &message);
    void statisticsUpdated(const QString &stats);

public slots:
    void start();
    void stop();

private slots:
    void cleanupOldAssemblies();
    void printStatistics();
    void onFrameAssembled(int streamId, int frameNumber, const QByteArray &frameData);

private:
    // Изменяем сигнатуру: вместо QNetworkDatagram принимаем QByteArray
    void processPacketNewProtocol(const QByteArray &data);
    void sendPacketNewProtocol(const QByteArray &data, PacketType type);
    void sendPacketNewProtocol(const QByteArray &data, PacketType type, int customSequence);
    
    // FEC методы
    void processXorPacket(const NetworkPacket& packet);
    void sendXorPackets();
    QByteArray calculateXorForGroup(int groupId);
    void tryRecoverLostPackets(int groupId);
    
    // FEC буферы хранят данные для XOR (без RouteHeader)
    QHash<int, QVector<QByteArray>> m_fecSendBuffers;    // groupId -> data_parts
    QHash<int, QVector<QByteArray>> m_fecReceiveBuffers; // groupId -> data_parts
    QHash<int, QVector<bool>> m_fecReceived;             // groupId -> received flags

    void updateSendStats(int packets, int bytes);
    void updateReceiveStats(int packets, int bytes);
    uint32_t calculateCRC32(const QByteArray &data);

private:
    UDPManager *m_udpManager = nullptr;  // Используем централизованный UDPManager
    QHostAddress m_serverAddress;
    quint16 m_serverPort;
    
    QTimer *m_cleanupTimer = nullptr;
    QTimer *m_statsTimer = nullptr;
    
    FrameAssembler *m_frameAssembler;
    FrameSender *m_frameSender;
    
    int m_streamId;  // ID видеопотока для идентификации
    int m_currentFrameNumber = 0;
    int m_packetSequence = 0;
   
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

    // УДАЛЕНО ДУБЛИРОВАНИЕ: оставляем только одно объявление
    bool m_sendingEnabled = false;
};
