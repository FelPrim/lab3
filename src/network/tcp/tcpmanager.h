#pragma once
#include <QObject>
#include <QTcpSocket>
#include <QHostAddress>
#include <QByteArray>
#include <QVector>

class TCPManager : public QObject {
    Q_OBJECT
public:
    explicit TCPManager(QObject *parent = nullptr);
    ~TCPManager() override;

    void setServer(const QString &host, quint16 port) { m_serverHost = host; m_serverPort = port; }
    void setLocalBindAddress(const QHostAddress &addr = QHostAddress::Any, quint16 port = 0) { m_localBindAddr = addr; m_localBindPort = port; }

    void connectToServer();
    void disconnectFromServer();

    // Общие сообщения
    void sendClientError(uint8_t originalMessageType, const QString &errorMessage);
    void sendClientSuccess(uint8_t originalMessageType, const QString &successMessage);

    // Сообщения стримов
    void sendClientStreamCreate(uint32_t callId = 0); // callId = 0 для публичного стрима
    void sendClientStreamDelete(uint32_t streamId);
    void sendClientStreamJoin(uint32_t streamId);
    void sendClientStreamLeave(uint32_t streamId);

    // Сообщения звонков
    void sendClientCallCreate();
    void sendClientCallJoin(uint32_t callId);
    void sendClientCallLeave(uint32_t callId);

    QTcpSocket *m_socket;
    QString m_serverHost;
    quint16 m_serverPort = 0;

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString &error);

    // Общие сообщения
    void serverErrorReceived(uint8_t originalMessageType, const QString &errorMessage);
    void serverSuccessReceived(uint8_t originalMessageType, const QString &successMessage);
    void serverHandshakeStart(uint32_t connectionId);
    void serverHandshakeEnd(uint32_t connectionId);

    // Сообщения стримов
    void serverStreamCreated(uint32_t streamId);
    void serverStreamDeleted(uint32_t streamId);
    void serverStreamJoined(uint32_t streamId);
    void serverStreamStart(uint32_t streamId);
    void serverStreamEnd(uint32_t streamId);

    // Сообщения звонков
    void serverCallCreated(uint32_t callId);
    void serverCallConnJoined(uint32_t callId, const QVector<uint32_t>& participants, const QVector<uint32_t>& streams);
    void serverCallConnNew(uint32_t callId, uint32_t participantId);
    void serverCallConnLeft(uint32_t callId, uint32_t participantId);
    void serverCallStreamNew(uint32_t callId, uint32_t streamId);
    void serverCallStreamDeleted(uint32_t callId, uint32_t streamId);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onErrorOccured(QAbstractSocket::SocketError socketError);

private:
    void processBuffer();

    // Константы типов сообщений
    enum MessageType {
        // Общие сообщения
        CLIENT_ERROR = 0x01,
        SERVER_ERROR = 0x02,
        CLIENT_SUCCESS = 0x03,
        SERVER_SUCCESS = 0x04,
        SERVER_HANDSHAKE_START = 0x05,
        SERVER_HANDSHAKE_END = 0x06,
        
        // Сообщения стримов
        CLIENT_STREAM_CREATE = 0x10,
        CLIENT_STREAM_DELETE = 0x11,
        CLIENT_STREAM_CONN_JOIN = 0x12,
        CLIENT_STREAM_CONN_LEAVE = 0x13,
        SERVER_STREAM_CREATED = 0x90,
        SERVER_STREAM_DELETED = 0x91,
        SERVER_STREAM_CONN_JOINED = 0x92,
        SERVER_STREAM_START = 0x93,
        SERVER_STREAM_END = 0x94,
        
        // Сообщения звонков
        CLIENT_CALL_CREATE = 0x20,
        CLIENT_CALL_CONN_JOIN = 0x21,
        CLIENT_CALL_CONN_LEAVE = 0x22,
        SERVER_CALL_CREATED = 0xA0,
        SERVER_CALL_CONN_JOINED = 0xA1,
        SERVER_CALL_CONN_NEW = 0xA2,
        SERVER_CALL_CONN_LEFT = 0xA3,
        SERVER_CALL_STREAM_NEW = 0xA4,
        SERVER_CALL_STREAM_DELETED = 0xA5
    };

    QHostAddress m_localBindAddr = QHostAddress::Any;
    quint16 m_localBindPort = 0;

    QByteArray m_buffer;
};
