#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QHostAddress>
#include <QTimer>
#include <QElapsedTimer>

class TcpClient : public QObject
{
    Q_OBJECT

public:
    explicit TcpClient(QObject *parent = nullptr);
    ~TcpClient();

    // Подключение к серверу
    bool connectToServer(const QString &host, quint16 port);
    void disconnectFromServer();
    bool isConnected() const;

    // Отправка сообщений
    void sendUdpAddress(const QHostAddress &address, quint16 port);
    void sendDisconnect();
    void sendStreamCreate();
    void sendStreamDelete(quint32 streamId);
    void sendStreamJoin(quint32 streamId);
    void sendStreamLeave(quint32 streamId);

    // Настройки
    void setAutoReconnect(bool enable);
    void setReconnectInterval(int milliseconds);

    // Статистика
    quint64 getBytesSent() const { return m_bytesSent; }
    quint64 getBytesReceived() const { return m_bytesReceived; }
    quint64 getMessagesSent() const { return m_messagesSent; }
    quint64 getMessagesReceived() const { return m_messagesReceived; }

signals:
    // Состояние соединения
    void connected();
    void disconnected();
    void connectionError(const QString &errorString);

    // Входящие сообщения от сервера
    void streamCreated(quint32 streamId);
    void streamDeleted(quint32 streamId);
    void streamJoined(quint32 streamId);
    void streamStart(quint32 streamId);
    void streamEnd(quint32 streamId);

    // Ошибки
    void protocolError(const QString &error);
    void networkError(const QString &error);

private slots:
    // Слоты для TCP сокета
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onErrorOccurred(QAbstractSocket::SocketError error);
    void onStateChanged(QAbstractSocket::SocketState state);

    // Таймеры
    void onReconnectTimer();
    void onKeepAliveTimer();

private:
    // Вспомогательные методы
    void setupSocket();
    void setupTimers();
    void cleanup();
    
    // Обработка входящих данных
    void processIncomingData();
    void parseMessage(const QByteArray &message);
    void handleProtocolMessage(quint8 messageType, const QByteArray &data);
    
    // Отправка сообщений
    void sendMessage(quint8 messageType, const QByteArray &data = QByteArray());
    QByteArray createMessage(quint8 messageType, const QByteArray &data = QByteArray()) const;
    
    // Преобразование данных
    QByteArray uint32ToBytes(quint32 value) const;
    quint32 bytesToUint32(const QByteArray &data, int offset = 0) const;
    QByteArray addressToBytes(const QHostAddress &address, quint16 port) const;

private:
    // TCP сокет
    QTcpSocket *m_socket;
    
    // Буфер для входящих данных
    QByteArray m_readBuffer;
    qint32 m_expectedMessageSize;
    
    // Настройки подключения
    QHostAddress m_serverAddress;
    quint16 m_serverPort;
    QString m_serverHost;
    
    // Таймеры
    QTimer *m_reconnectTimer;
    QTimer *m_keepAliveTimer;
    bool m_autoReconnect;
    int m_reconnectInterval;
    
    // Статистика
    quint64 m_bytesSent;
    quint64 m_bytesReceived;
    quint64 m_messagesSent;
    quint64 m_messagesReceived;
    
    // Состояние
    bool m_connected;
    bool m_initialized;
    
    // Константы
    static const int KEEP_ALIVE_INTERVAL = 30000; // 30 секунд
    static const int MAX_MESSAGE_SIZE = 1024;
    static const quint8 PROTOCOL_VERSION = 0x01;
    
    // Типы сообщений
    enum MessageType {
        // Клиент -> Сервер
        CLIENT_UDP_ADDR = 0x01,
        CLIENT_DISCONNECT = 0x02,
        CLIENT_STREAM_CREATE = 0x03,
        CLIENT_STREAM_DELETE = 0x04,
        CLIENT_STREAM_JOIN = 0x05,
        CLIENT_STREAM_LEAVE = 0x06,
        
        // Сервер -> Клиент
        SERVER_STREAM_CREATED = 0x81,
        SERVER_STREAM_DELETED = 0x82,
        SERVER_STREAM_JOINED = 0x83,
        SERVER_STREAM_START = 0x84,
        SERVER_STREAM_END = 0x85
    };
};
