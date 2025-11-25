#pragma once

#include <QObject>
#include <QHostAddress>
#include <QMap>
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

    // UI -> TCP commands
    void sendStreamCreate();
    void sendStreamDelete(uint32_t id);
    void sendStreamJoin(uint32_t id);
    void sendStreamLeave(uint32_t id);
    void sendDisconnect();

    // Getters
    UDPManager* getUdpManager() const { return m_udpManager; }
    quint16 getLocalUdpPort() const { return m_localUdpPort; }

signals:
    // Connection status
    void connected();
    void disconnected();
    void errorOccurred(const QString &err);

    // Server messages
    void serverStreamCreated(uint32_t id);
    void serverStreamDeleted(uint32_t id);
    void serverStreamJoined(uint32_t id);
    void serverStreamStart(uint32_t id);
    void serverStreamEnd(uint32_t id);

    // NetworkManager signals (forwarded)
    void frameAssembled(int streamId, int frameNumber, const QByteArray &frameData);
    void networkErrorOccurred(const QString &message);

private slots:
    void onTcpConnected();
    void onTcpDisconnected();
    void onTcpError(const QString &err);
    
    void onServerStreamCreated(uint32_t id);
    void onServerStreamDeleted(uint32_t id);
    void onServerStreamJoined(uint32_t id);
    void onServerStreamStart(uint32_t id);
    void onServerStreamEnd(uint32_t id);

private:
    TCPManager *m_tcp;
    UDPManager *m_udpManager;
    
    QString m_serverHost;
    quint16 m_serverTcpPort = DEFAULT_ECHO_SERVER_PORT;
    quint16 m_serverUdpPort = DEFAULT_UDP_SERVER_PORT;

    QHostAddress m_localUdpIp = QHostAddress::AnyIPv4;
    quint16 m_localUdpPort = 0;

    QMap<int, NetworkManager*> m_networkManagers; // streamId -> NetworkManager
};