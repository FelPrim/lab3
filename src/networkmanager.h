#pragma once

#include <QObject>
#include <QtNetwork/QUdpSocket>
#include <QtNetwork/QNetworkDatagram>
#include <QHash>
#include <QPair>
#include <QTimer>
#include <QElapsedTimer>
#include <QSet>
#include "video_defaults.h"
#include <QDateTime> 
#include <QVariant>
#include <zlib.h>
#include "myfec.h"
#include "network_packet.h" 

class NetworkManager : public QObject
{
    Q_OBJECT

public:
    explicit NetworkManager(QObject *parent = nullptr);
    ~NetworkManager() override;

    bool initialize();
    void cleanup();
    void sendVideoFrame(int streamId, int frameNumber, const QByteArray &frameData);
    void setServerAddress(const QString &address, quint16 port);
    QHostAddress getServerAddress() const { return m_serverAddress; }
    quint16 getServerPort() const { return m_serverPort; }
    quint16 getLocalPort() const { return m_localPort; }
    void setPort(quint16 port) { m_localPort = port; }

signals:
    void frameAssembled(int streamId, int frameNumber, const QByteArray &frameData);
    void errorOccurred(const QString &message);
    void statisticsUpdated(const QString &stats);

public slots:
    void start();
    void stop();

private slots:
    void onPacketReceived();
    void cleanupOldAssemblies();
    void printStatistics();
    void onFrameAssembled(int streamId, int frameNumber, const QByteArray &frameData);

private:
    void setupSocket();
    void processPacketNewProtocol(const QNetworkDatagram &datagram);
    void sendPacketNewProtocol(const QByteArray &data, int streamId, PacketType type);
    void sendPacketNewProtocol(const QByteArray &data, int streamId, PacketType type, int customSequence);
    
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
    QUdpSocket *m_udpSocket = nullptr;
    QHostAddress m_serverAddress;
    quint16 m_serverPort;
    quint16 m_localPort;
    
    QTimer *m_cleanupTimer = nullptr;
    QTimer *m_statsTimer = nullptr;
    
    FrameAssembler *m_frameAssembler;
    FrameSender *m_frameSender;
    
    int m_currentFrameNumber = 0;
    int m_packetSequence = 0;
    int m_streamId = 0;
   
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
};
