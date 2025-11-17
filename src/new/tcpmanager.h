#pragma once
#include <QObject>
#include <QTcpSocket>
#include <QHostAddress>
#include <QByteArray>

class TCPManager : public QObject {
    Q_OBJECT
public:
    explicit TCPManager(QObject *parent = nullptr);
    ~TCPManager() override;

    void setServer(const QString &host, quint16 port) { m_serverHost = host; m_serverPort = port; }
    void setLocalBindAddress(const QHostAddress &addr = QHostAddress::Any, quint16 port = 0) { m_localBindAddr = addr; m_localBindPort = port; }

    void connectToServer();
    void disconnectFromServer();

    void sendClientUdpAddr(const QByteArray &sockaddr_in_bytes); // 16 bytes sockaddr_in
    void sendClientDisconnect();
    void sendClientStreamCreate();
    void sendClientStreamDelete(uint32_t streamId);
    void sendClientStreamJoin(uint32_t streamId);
    void sendClientStreamLeave(uint32_t streamId);

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString &error);

    void serverStreamCreated(uint32_t streamId);
    void serverStreamDeleted(uint32_t streamId);
    void serverStreamJoined(uint32_t streamId);
    void serverStreamStart(uint32_t streamId);
    void serverStreamEnd(uint32_t streamId);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onErrorOccured(QAbstractSocket::SocketError socketError);

private:
    void processBuffer();

    QTcpSocket *m_socket;
    QString m_serverHost;
    quint16 m_serverPort = 0;

    QHostAddress m_localBindAddr = QHostAddress::Any;
    quint16 m_localBindPort = 0;

    QByteArray m_buffer;
};
