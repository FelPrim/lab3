#pragma once

#include <QObject>
#include <QHostAddress>
#include <QMap>
#include <QSet>
#include <QVector>
#include "udp/udpmanager.h"
#include "tcp/tcpmanager.h"
#include "handshake.h"
#include "../video_defaults.h"

class NetworkManager;

// Структура для отслеживания состояния стрима
struct StreamState {
    uint32_t streamId;
    uint32_t callId;
    bool isOwner; // true если мы владелец (стример), false если зритель
    bool isActive; // true если стрим активен (для стримера)
    QString status; // дополнительный статус
};

// Структура для отслеживания состояния звонка
struct CallState {
    uint32_t callId;
    QSet<uint32_t> streams; // стримы в этом звонке
    QSet<uint32_t> participants; // участники звонка
};

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

    // UI -> TCP commands - Стримы
    void sendStreamCreate(uint32_t callId = 0);
    void sendStreamDelete(uint32_t streamId);
    void sendStreamJoin(uint32_t streamId);
    void sendStreamLeave(uint32_t streamId);

    // ✅ ДОБАВЛЕНО: UI -> TCP commands - Звонки
    void sendCallCreate();
    void sendCallJoin(uint32_t callId);
    void sendCallLeave(uint32_t callId);

    // Call management
    void setCallIdForStream(int streamId, uint32_t callId);

    // ✅ ДОБАВЛЕНО: Client state tracking
    void printClientState() const;
    QVector<uint32_t> getCallIds() const;
    QVector<uint32_t> getStreamIds() const;
    StreamState getStreamState(uint32_t streamId) const;
    CallState getCallState(uint32_t callId) const;

    // Getters
    UDPManager* getUdpManager() const { return m_udpManager; }
    quint16 getLocalUdpPort() const { return m_localUdpPort; }
    uint32_t getConnectionId() const { return m_handshakeService->getConnectionId(); }
    bool isHandshakeCompleted() const { return m_handshakeService->isHandshakeCompleted(); }

signals:
    // Connection status
    void connected();
    void disconnected();
    void errorOccurred(const QString &err);

    // Handshake signals
    void handshakeStarted(uint32_t connectionId);
    void handshakeCompleted(uint32_t connectionId);

    // Server messages - Стримы
    void serverStreamCreated(uint32_t id);
    void serverStreamDeleted(uint32_t id);
    void serverStreamJoined(uint32_t id);
    void serverStreamStart(uint32_t id);
    void serverStreamEnd(uint32_t id);

    // ✅ ДОБАВЛЕНО: Server messages - Звонки
    void serverCallCreated(uint32_t callId);
    void serverCallConnJoined(uint32_t callId, const QVector<uint32_t>& participants, const QVector<uint32_t>& streams);
    void serverCallConnNew(uint32_t callId, uint32_t participantId);
    void serverCallConnLeft(uint32_t callId, uint32_t participantId);
    void serverCallStreamNew(uint32_t callId, uint32_t streamId);
    void serverCallStreamDeleted(uint32_t callId, uint32_t streamId);
    void serverErrorReceived(uint8_t originalMessageType, const QString &errorMessage);
    void serverSuccessReceived(uint8_t originalMessageType, const QString &successMessage);

    // NetworkManager signals (forwarded)
    void frameAssembled(int streamId, int frameNumber, const QByteArray &frameData);
    void networkErrorOccurred(const QString &message);

private slots:
    void onTcpConnected();
    void onTcpDisconnected();
    void onTcpError(const QString &err);
    
    // Handshake events
    void onHandshakeStarted(uint32_t connectionId);
    void onHandshakeCompleted(uint32_t connectionId);
    void onHandshakeFailed(const QString &error);
    
    // Server message handlers
    void onServerHandshakeStart(uint32_t connectionId);
    void onServerHandshakeEnd(uint32_t connectionId);
    void onServerStreamCreated(uint32_t id);
    void onServerStreamDeleted(uint32_t id);
    void onServerStreamJoined(uint32_t id);
    void onServerStreamStart(uint32_t id);
    void onServerStreamEnd(uint32_t id);

    // ✅ ДОБАВЛЕНО: Server message handlers - Звонки
    void onServerCallCreated(uint32_t callId);
    void onServerCallConnJoined(uint32_t callId, const QVector<uint32_t>& participants, const QVector<uint32_t>& streams);
    void onServerCallConnNew(uint32_t callId, uint32_t participantId);
    void onServerCallConnLeft(uint32_t callId, uint32_t participantId);
    void onServerCallStreamNew(uint32_t callId, uint32_t streamId);
    void onServerCallStreamDeleted(uint32_t callId, uint32_t streamId);

private:
    TCPManager *m_tcp;
    UDPManager *m_udpManager;
    HandshakeService *m_handshakeService;
    
    QString m_serverHost;
    quint16 m_serverTcpPort = DEFAULT_ECHO_SERVER_PORT;
    quint16 m_serverUdpPort = DEFAULT_UDP_SERVER_PORT;

    QHostAddress m_localUdpIp = QHostAddress::AnyIPv4;
    quint16 m_localUdpPort = 0;

    QMap<int, NetworkManager*> m_networkManagers;
    QMap<int, uint32_t> m_streamCallIds;

    // ✅ ДОБАВЛЕНО: Client state tracking
    QMap<uint32_t, StreamState> m_streamStates;
    QMap<uint32_t, CallState> m_callStates;
    QSet<uint32_t> m_ownedStreams; // стримы, которые мы создали
    QSet<uint32_t> m_joinedStreams; // стримы, к которым присоединились как зритель
    QSet<uint32_t> m_activeStreams; // активные стримы (для которых включена отправка)
    QSet<uint32_t> m_joinedCalls; // звонки, в которых мы участвуем
    void sendNatTraversalPackets(uint32_t connectionId);

};