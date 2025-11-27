#pragma once

#include <QObject>
#include <QHostAddress>
#include <QMap>
#include <QTimer>
#include "udp/udpmanager.h"
#include "tcp/tcpmanager.h"
#include "../video_defaults.h"

class NetworkManager;

class NetworkFacade : public QObject {
    Q_OBJECT
public:
    explicit NetworkFacade(QObject *parent = nullptr);
    ~NetworkFacade() override;

    void setServer(const QString &host, quint16 tcpPort, quint16 udpPort);
    void setLocalUdpInfo(const QHostAddress &localIp, quint16 localUdpPort);

    bool initialize();
    void connectToServer();
    void disconnect();

    // NetworkManager management
    NetworkManager* createNetworkManager(int streamId);
    void removeNetworkManager(int streamId);
    NetworkManager* getNetworkManager(int streamId);

    // UI -> TCP commands - Streams
    void sendStreamCreate(uint32_t callId = 0); // callId = 0 для публичного стрима
    void sendStreamDelete(uint32_t streamId);
    void sendStreamJoin(uint32_t streamId);
    void sendStreamLeave(uint32_t streamId);

    // UI -> TCP commands - Calls
    void sendCallCreate();
    void sendCallJoin(uint32_t callId);
    void sendCallLeave(uint32_t callId);

    // Error/Success handling
    void sendClientError(uint8_t originalMessageType, const QString &errorMessage);
    void sendClientSuccess(uint8_t originalMessageType, const QString &successMessage);

    // Getters
    UDPManager* getUdpManager() const { return m_udpManager; }
    quint16 getLocalUdpPort() const { return m_localUdpPort; }
    uint32_t getConnectionId() const { return m_connectionId; }
    bool isHandshakeCompleted() const { return m_handshakeCompleted; }

signals:
    // Connection status
    void connected();
    void disconnected();
    void errorOccurred(const QString &err);

    // Handshake signals
    void handshakeStarted(uint32_t connectionId);
    void handshakeCompleted(uint32_t connectionId);

    // Error/Success signals
    void serverErrorReceived(uint8_t originalMessageType, const QString &errorMessage);
    void serverSuccessReceived(uint8_t originalMessageType, const QString &successMessage);

    // Server messages - Streams
    void serverStreamCreated(uint32_t id);
    void serverStreamDeleted(uint32_t id);
    void serverStreamJoined(uint32_t id);
    void serverStreamStart(uint32_t id);
    void serverStreamEnd(uint32_t id);

    // Server messages - Calls
    void serverCallCreated(uint32_t callId);
    void serverCallConnJoined(uint32_t callId, const QVector<uint32_t>& participants, const QVector<uint32_t>& streams);
    void serverCallConnNew(uint32_t callId, uint32_t participantId);
    void serverCallConnLeft(uint32_t callId, uint32_t participantId);
    void serverCallStreamNew(uint32_t callId, uint32_t streamId);
    void serverCallStreamDeleted(uint32_t callId, uint32_t streamId);

    // NetworkManager signals (forwarded)
    void frameAssembled(int streamId, int frameNumber, const QByteArray &frameData);
    void networkErrorOccurred(const QString &message);

private slots:
    void onTcpConnected();
    void onTcpDisconnected();
    void onTcpError(const QString &err);
    
    // Handshake handling
    void onServerHandshakeStart(uint32_t connectionId);
    void onServerHandshakeEnd(uint32_t connectionId);
    void onUdpHandshakeTimeout();
    
    // Error/Success handling
    void onServerErrorReceived(uint8_t originalMessageType, const QString &errorMessage);
    void onServerSuccessReceived(uint8_t originalMessageType, const QString &successMessage);
    
    // Stream message handlers
    void onServerStreamCreated(uint32_t id);
    void onServerStreamDeleted(uint32_t id);
    void onServerStreamJoined(uint32_t id);
    void onServerStreamStart(uint32_t id);
    void onServerStreamEnd(uint32_t id);

    // Call message handlers
    void onServerCallCreated(uint32_t callId);
    void onServerCallConnJoined(uint32_t callId, const QVector<uint32_t>& participants, const QVector<uint32_t>& streams);
    void onServerCallConnNew(uint32_t callId, uint32_t participantId);
    void onServerCallConnLeft(uint32_t callId, uint32_t participantId);
    void onServerCallStreamNew(uint32_t callId, uint32_t streamId);
    void onServerCallStreamDeleted(uint32_t callId, uint32_t streamId);

private:
    void sendUdpHandshakePacket();
    void stopUdpHandshake();

    TCPManager *m_tcp;
    UDPManager *m_udpManager;
    
    QString m_serverHost;
    quint16 m_serverTcpPort = DEFAULT_ECHO_SERVER_PORT;
    quint16 m_serverUdpPort = DEFAULT_UDP_SERVER_PORT;

    QHostAddress m_localUdpIp = QHostAddress::AnyIPv4;
    quint16 m_localUdpPort = 0;

    // Handshake state
    uint32_t m_connectionId = 0;
    bool m_handshakeCompleted = false;
    QTimer *m_handshakeTimer = nullptr;
    int m_handshakeAttempts = 0;
    static const int MAX_HANDSHAKE_ATTEMPTS = 50; // 5 seconds at 10 packets/second

    QMap<int, NetworkManager*> m_networkManagers; // streamId -> NetworkManager
                                                  // 

void onHandshakeStart(uint32_t connectionId);
void onHandshakeEnd(uint32_t connectionId);
};
