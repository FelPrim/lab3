#pragma once
#include <QObject>
#include <QHostAddress>

class TCPManager;

class NetworkFacade : public QObject {
    Q_OBJECT
public:
    explicit NetworkFacade(QObject *parent = nullptr);
    ~NetworkFacade() override;

    void setServer(const QString &host, quint16 tcpPort, quint16 udpPort);
    // call this after you created your UDP-socket and obtained a port
    void setLocalUdpInfo(const QHostAddress &localIp, quint16 localUdpPort);

    bool initialize(); // no assumptions about UDP class
    void connectToServer();
    void disconnect();

    // UI -> TCP
    void sendStreamCreate();
    void sendStreamDelete(uint32_t id);
    void sendStreamJoin(uint32_t id);
    void sendStreamLeave(uint32_t id);
    void sendDisconnect();

signals:
    // facade -> UI
    void connected();
    void disconnected();
    void errorOccurred(const QString &err);

    void serverStreamCreated(uint32_t id);
    void serverStreamDeleted(uint32_t id);
    void serverStreamJoined(uint32_t id);
    void serverStreamStart(uint32_t id);
    void serverStreamEnd(uint32_t id);

    // If you have an existing UDP manager, connect it to this signal to send datagrams
    void sendUdpDatagram(const QByteArray &data, const QHostAddress &host, quint16 port);

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
    QString m_serverHost;
    quint16 m_serverTcpPort = 23231;
    quint16 m_serverUdpPort = 23230;

    QHostAddress m_localUdpIp = QHostAddress::AnyIPv4;
    quint16 m_localUdpPort = 0;
};
