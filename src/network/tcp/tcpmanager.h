#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QHostAddress>
#include <QByteArray>
#include <QVector>
#include <QTimer>
#include <QElapsedTimer>

class TCPManager : public QObject {
    Q_OBJECT

public:
    explicit TCPManager(QObject *parent = nullptr);
    ~TCPManager() override;

    // Настройки соединения
    void setServer(const QString &host, quint16 port);
    void setLocalBindAddress(const QHostAddress &addr = QHostAddress::Any, quint16 port = 0);
    void setAutoReconnect(bool enable);
    void setReconnectInterval(int milliseconds);

    // Управление соединением
    void connectToServer();
    void disconnectFromServer();
    bool isConnected() const;

    // Отправка сообщений - общие
    void sendClientError(uint8_t originalMessageType, const QString &errorMessage);
    void sendClientSuccess(uint8_t originalMessageType, const QString &successMessage);

    // Отправка сообщений - стримы
    void sendClientStreamCreate(uint32_t callId = 0);
    void sendClientStreamDelete(uint32_t streamId);
    void sendClientStreamJoin(uint32_t streamId);
    void sendClientStreamLeave(uint32_t streamId);

    // Отправка сообщений - звонки
    void sendClientCallCreate();
    void sendClientCallJoin(uint32_t callId);
    void sendClientCallLeave(uint32_t callId);

    // Статистика
    quint64 getBytesSent() const { return m_bytesSent; }
    quint64 getBytesReceived() const { return m_bytesReceived; }
    quint64 getMessagesSent() const { return m_messagesSent; }
    quint64 getMessagesReceived() const { return m_messagesReceived; }

signals:
    // Состояние соединения
    void connected();
    void disconnected();
    void errorOccurred(const QString &error);

    // Общие сообщения от сервера
    void serverHandshakeStart(uint32_t connectionId);
    void serverHandshakeEnd(uint32_t connectionId);
    void serverErrorReceived(uint8_t originalMessageType, const QString &errorMessage);
    void serverSuccessReceived(uint8_t originalMessageType, const QString &successMessage);

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
    void onErrorOccurred(QAbstractSocket::SocketError socketError);
    void onReconnectTimer();

private:
    // Типы сообщений протокола - ДОЛЖНЫ СОВПАДАТЬ С СЕРВЕРОМ
    enum MessageType {
        // Общие сообщения
        CLIENT_ERROR              = 0x01,
        SERVER_ERROR              = 0x02,
        CLIENT_SUCCESS            = 0x03,
        SERVER_SUCCESS            = 0x04,
        SERVER_HANDSHAKE_START    = 0x05,
        SERVER_HANDSHAKE_END      = 0x06,
        
        // Сообщения стримов
        CLIENT_STREAM_CREATE      = 0x10,
        CLIENT_STREAM_DELETE      = 0x11,
        CLIENT_STREAM_CONN_JOIN   = 0x12,
        CLIENT_STREAM_CONN_LEAVE  = 0x13,
        
        SERVER_STREAM_CREATED     = 0x90,
        SERVER_STREAM_DELETED     = 0x91,
        SERVER_STREAM_CONN_JOINED = 0x92,
        SERVER_STREAM_START       = 0x93,
        SERVER_STREAM_END         = 0x94,
        
        // Сообщения звонков
        CLIENT_CALL_CREATE        = 0x20,
        CLIENT_CALL_CONN_JOIN     = 0x21,
        CLIENT_CALL_CONN_LEAVE    = 0x22,
        
        SERVER_CALL_CREATED       = 0xA0,
        SERVER_CALL_CONN_JOINED   = 0xA1,
        SERVER_CALL_CONN_NEW      = 0xA2,
        SERVER_CALL_CONN_LEFT     = 0xA3,
        SERVER_CALL_STREAM_NEW    = 0xA4,
        SERVER_CALL_STREAM_DELETED = 0xA5
    };

    // Вспомогательные методы
    void setupTimers();
    void cleanup();
    void processBuffer();
    bool validateMessageSize(uint8_t messageType, size_t availableSize, size_t expectedMinSize);
    void sendMessage(const QByteArray &message);

    // TCP сокет и настройки
    QTcpSocket *m_socket;
    QString m_serverHost;
    quint16 m_serverPort;
    QHostAddress m_localBindAddr;
    quint16 m_localBindPort;

    // Таймеры
    QTimer *m_reconnectTimer;
    bool m_autoReconnect;
    int m_reconnectInterval;

    // Буфер и состояние
    QByteArray m_buffer;
    bool m_connected;

    // Статистика
    quint64 m_bytesSent;
    quint64 m_bytesReceived;
    quint64 m_messagesSent;
    quint64 m_messagesReceived;

    // Константы
    static const int MAX_MESSAGE_SIZE = 1024;
    static const int MAX_BUFFER_SIZE = 8192;
};