#include "tcpmanager.h"
#include <QDataStream>
#include <QHostAddress>
#include <QDebug>
#include <QtEndian>
#include <QTimer>

TCPManager::TCPManager(QObject *parent)
    : QObject(parent)
    , m_socket(new QTcpSocket(this))
    , m_serverPort(0)
    , m_localBindAddr(QHostAddress::Any)
    , m_localBindPort(0)
    , m_reconnectTimer(new QTimer(this))
    , m_autoReconnect(false)
    , m_reconnectInterval(5000)
    , m_connected(false)
    , m_bytesSent(0)
    , m_bytesReceived(0)
    , m_messagesSent(0)
    , m_messagesReceived(0)
{
    // Настройка сокета
    connect(m_socket, &QTcpSocket::connected, this, &TCPManager::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &TCPManager::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &TCPManager::onReadyRead);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
            this, &TCPManager::onErrorOccurred);

    // Настройка таймеров
    setupTimers();
}

TCPManager::~TCPManager()
{
    cleanup();
}

void TCPManager::setupTimers()
{
    connect(m_reconnectTimer, &QTimer::timeout, this, &TCPManager::onReconnectTimer);
    m_reconnectTimer->setSingleShot(true);
}

void TCPManager::cleanup()
{
    if (m_socket) {
        m_socket->disconnect();
        if (m_socket->isOpen()) {
            m_socket->close();
        }
        m_socket->deleteLater();
        m_socket = nullptr;
    }
    
    if (m_reconnectTimer) {
        m_reconnectTimer->stop();
    }
    
    m_buffer.clear();
    m_connected = false;
}

void TCPManager::setServer(const QString &host, quint16 port)
{
    m_serverHost = host;
    m_serverPort = port;
}

void TCPManager::setLocalBindAddress(const QHostAddress &addr, quint16 port)
{
    m_localBindAddr = addr;
    m_localBindPort = port;
}

void TCPManager::setAutoReconnect(bool enable)
{
    m_autoReconnect = enable;
}

void TCPManager::setReconnectInterval(int milliseconds)
{
    m_reconnectInterval = milliseconds;
}

void TCPManager::connectToServer()
{
    if (!m_socket || m_connected) {
        return;
    }

    // Останавливаем таймер переподключения
    m_reconnectTimer->stop();

    // Привязка к локальному адресу, если задано
    if (m_localBindPort != 0 || m_localBindAddr != QHostAddress::Any) {
        if (!m_socket->bind(m_localBindAddr, m_localBindPort)) {
            qWarning() << "TCPManager: bind failed:" << m_socket->errorString();
            emit errorOccurred(QString("Bind failed: %1").arg(m_socket->errorString()));
            return;
        }
    }

    qDebug() << "TCPManager: connecting to" << m_serverHost << ":" << m_serverPort;
    m_socket->connectToHost(m_serverHost, m_serverPort);
}

void TCPManager::disconnectFromServer()
{
    if (m_socket) {
        m_socket->disconnectFromHost();
    }
    m_connected = false;
}

bool TCPManager::isConnected() const
{
    return m_connected;
}

void TCPManager::onConnected()
{
    qDebug() << "TCPManager: connected to server";
    m_connected = true;
    m_reconnectTimer->stop();
    emit connected();
}

void TCPManager::onDisconnected()
{
    qDebug() << "TCPManager: disconnected from server";
    m_connected = false;
    
    // Автопереподключение
    if (m_autoReconnect && !m_serverHost.isEmpty() && m_serverPort != 0) {
        qDebug() << "TCPManager: auto-reconnecting in" << m_reconnectInterval << "ms";
        m_reconnectTimer->start(m_reconnectInterval);
    }
    
    emit disconnected();
}

void TCPManager::onErrorOccurred(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError)
    QString err = m_socket->errorString();
    qWarning() << "TCPManager: socket error:" << err;
    emit errorOccurred(err);
}

void TCPManager::onReconnectTimer()
{
    if (m_autoReconnect && !m_connected) {
        qDebug() << "TCPManager: attempting to reconnect...";
        connectToServer();
    }
}

void TCPManager::onReadyRead()
{
    QByteArray chunk = m_socket->readAll();
    if (chunk.isEmpty()) {
        return;
    }

    m_bytesReceived += chunk.size();
    m_buffer.append(chunk);

    // Защита от переполнения буфера
    if (m_buffer.size() > MAX_BUFFER_SIZE) {
        qWarning() << "TCPManager: buffer overflow, clearing buffer";
        m_buffer.clear();
        return;
    }

    processBuffer();
}

bool TCPManager::validateMessageSize(uint8_t messageType, size_t availableSize, size_t expectedMinSize)
{
    if (availableSize < expectedMinSize) {
        qWarning() << "TCPManager: message too small, type:" << QString::number(messageType, 16)
                   << "available:" << availableSize << "expected:" << expectedMinSize;
        return false;
    }
    return true;
}

void TCPManager::processBuffer()
{
    while (!m_buffer.isEmpty()) {
        if (m_buffer.size() < 1) {
            return; // Ждем хотя бы тип сообщения
        }

        uint8_t messageType = static_cast<uint8_t>(m_buffer.at(0));
        qDebug() << "TCPManager: processing message type:" << QString::number(messageType, 16);

        switch (messageType) {
            // SERVER_HANDSHAKE_START и SERVER_HANDSHAKE_END - 1 + 4 байта
            case SERVER_HANDSHAKE_START:
            case SERVER_HANDSHAKE_END: {
                if (!validateMessageSize(messageType, m_buffer.size(), 5)) return;
                
                uint32_t connectionId = qFromBigEndian<quint32>(
                    reinterpret_cast<const uchar*>(m_buffer.constData() + 1));
                m_buffer.remove(0, 5);
                m_messagesReceived++;
                
                if (messageType == SERVER_HANDSHAKE_START) {
                    emit serverHandshakeStart(connectionId);
                } else {
                    emit serverHandshakeEnd(connectionId);
                }
                break;
            }

            // SERVER_ERROR и SERVER_SUCCESS - 1 + 1 + 1 + N байт
            case SERVER_ERROR:
            case SERVER_SUCCESS: {
                if (!validateMessageSize(messageType, m_buffer.size(), 3)) return;
                
                uint8_t originalMessageType = static_cast<uint8_t>(m_buffer.at(1));
                uint8_t messageLength = static_cast<uint8_t>(m_buffer.at(2));
                
                if (!validateMessageSize(messageType, m_buffer.size(), 3 + messageLength)) return;
                
                QString message = QString::fromUtf8(m_buffer.constData() + 3, messageLength);
                m_buffer.remove(0, 3 + messageLength);
                m_messagesReceived++;
                
                if (messageType == SERVER_ERROR) {
                    emit serverErrorReceived(originalMessageType, message);
                } else {
                    emit serverSuccessReceived(originalMessageType, message);
                }
                break;
            }

            // Простые сообщения с ID - 1 + 4 байта
            case SERVER_STREAM_CREATED:
            case SERVER_STREAM_DELETED:
            case SERVER_STREAM_CONN_JOINED:
            case SERVER_STREAM_START:
            case SERVER_STREAM_END:
            case SERVER_CALL_CREATED: {
                if (!validateMessageSize(messageType, m_buffer.size(), 5)) return;
                
                uint32_t id = qFromBigEndian<quint32>(
                    reinterpret_cast<const uchar*>(m_buffer.constData() + 1));
                m_buffer.remove(0, 5);
                m_messagesReceived++;
                
                switch (messageType) {
                    case SERVER_STREAM_CREATED: emit serverStreamCreated(id); break;
                    case SERVER_STREAM_DELETED: emit serverStreamDeleted(id); break;
                    case SERVER_STREAM_CONN_JOINED: emit serverStreamJoined(id); break;
                    case SERVER_STREAM_START: emit serverStreamStart(id); break;
                    case SERVER_STREAM_END: emit serverStreamEnd(id); break;
                    case SERVER_CALL_CREATED: emit serverCallCreated(id); break;
                }
                break;
            }

            // SERVER_CALL_CONN_JOINED - переменной длины
            case SERVER_CALL_CONN_JOINED: {
                const int minSize = 1 + 4 + 1 + 1; // type + call_id + participant_count + stream_count
                if (!validateMessageSize(messageType, m_buffer.size(), minSize)) return;
                
                const uchar *data = reinterpret_cast<const uchar*>(m_buffer.constData());
                uint32_t callId = qFromBigEndian<quint32>(data + 1);
                uint8_t participantCount = data[5];
                uint8_t streamCount = data[6];
                
                int totalSize = minSize + (participantCount * 4) + (streamCount * 4);
                if (!validateMessageSize(messageType, m_buffer.size(), totalSize)) return;
                
                QVector<uint32_t> participants;
                QVector<uint32_t> streams;
                
                int offset = 7;
                for (int i = 0; i < participantCount; ++i) {
                    uint32_t participantId = qFromBigEndian<quint32>(data + offset);
                    participants.append(participantId);
                    offset += 4;
                }
                
                for (int i = 0; i < streamCount; ++i) {
                    uint32_t streamId = qFromBigEndian<quint32>(data + offset);
                    streams.append(streamId);
                    offset += 4;
                }
                
                m_buffer.remove(0, totalSize);
                m_messagesReceived++;
                emit serverCallConnJoined(callId, participants, streams);
                break;
            }

            // Сообщения с call_id + connection_id/stream_id - 1 + 4 + 4 байта
            case SERVER_CALL_CONN_NEW:
            case SERVER_CALL_CONN_LEFT:
            case SERVER_CALL_STREAM_NEW:  
            case SERVER_CALL_STREAM_DELETED: {
                if (!validateMessageSize(messageType, m_buffer.size(), 9)) return;
                
                const uchar *data = reinterpret_cast<const uchar*>(m_buffer.constData());
                uint32_t callId = qFromBigEndian<quint32>(data + 1);
                uint32_t id = qFromBigEndian<quint32>(data + 5);
                
                m_buffer.remove(0, 9);
                m_messagesReceived++;
                
                switch (messageType) {
                    case SERVER_CALL_CONN_NEW: emit serverCallConnNew(callId, id); break;
                    case SERVER_CALL_CONN_LEFT: emit serverCallConnLeft(callId, id); break;
                    case SERVER_CALL_STREAM_NEW: emit serverCallStreamNew(callId, id); break;
                    case SERVER_CALL_STREAM_DELETED: emit serverCallStreamDeleted(callId, id); break;
                }
                break;
            }

            default:
                qWarning() << "TCPManager: unknown message type:" << QString::number(messageType, 16);
                m_buffer.remove(0, 1); // Удаляем неизвестный байт и продолжаем
                break;
        }
    }
}

void TCPManager::sendMessage(const QByteArray &message)
{
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) {
        qWarning() << "TCPManager: cannot send message - not connected";
        return;
    }

    qint64 bytesWritten = m_socket->write(message);
    if (bytesWritten == -1) {
        qWarning() << "TCPManager: failed to write message:" << m_socket->errorString();
        return;
    }

    m_bytesSent += bytesWritten;
    m_messagesSent++;
}

// ==================== РЕАЛИЗАЦИЯ ОТПРАВКИ СООБЩЕНИЙ ====================

void TCPManager::sendClientError(uint8_t originalMessageType, const QString &errorMessage)
{
    QByteArray messageData = errorMessage.toUtf8();
    if (messageData.size() > 255) {
        messageData = messageData.left(255);
        qWarning() << "TCPManager: error message truncated to 255 bytes";
    }
    
    QByteArray out;
    out.append(static_cast<char>(CLIENT_ERROR));
    out.append(static_cast<char>(originalMessageType));
    out.append(static_cast<char>(messageData.size()));
    out.append(messageData);
    
    sendMessage(out);
    qDebug() << "TCPManager: sent CLIENT_ERROR for message type" << originalMessageType;
}

void TCPManager::sendClientSuccess(uint8_t originalMessageType, const QString &successMessage)
{
    QByteArray messageData = successMessage.toUtf8();
    if (messageData.size() > 255) {
        messageData = messageData.left(255);
        qWarning() << "TCPManager: success message truncated to 255 bytes";
    }
    
    QByteArray out;
    out.append(static_cast<char>(CLIENT_SUCCESS));
    out.append(static_cast<char>(originalMessageType));
    out.append(static_cast<char>(messageData.size()));
    out.append(messageData);
    
    sendMessage(out);
    qDebug() << "TCPManager: sent CLIENT_SUCCESS for message type" << originalMessageType;
}

void TCPManager::sendClientStreamCreate(uint32_t callId)
{
    QByteArray out;
    out.append(static_cast<char>(CLIENT_STREAM_CREATE));
    quint32 beCallId = qToBigEndian<quint32>(callId);
    out.append(reinterpret_cast<const char*>(&beCallId), 4);
    
    sendMessage(out);
    qDebug() << "TCPManager: sent CLIENT_STREAM_CREATE callId=" << callId;
}

void TCPManager::sendClientStreamDelete(uint32_t streamId)
{
    QByteArray out;
    out.append(static_cast<char>(CLIENT_STREAM_DELETE));
    quint32 beStreamId = qToBigEndian<quint32>(streamId);
    out.append(reinterpret_cast<const char*>(&beStreamId), 4);
    
    sendMessage(out);
    qDebug() << "TCPManager: sent CLIENT_STREAM_DELETE streamId=" << streamId;
}

void TCPManager::sendClientStreamJoin(uint32_t streamId)
{
    QByteArray out;
    out.append(static_cast<char>(CLIENT_STREAM_CONN_JOIN));
    quint32 beStreamId = qToBigEndian<quint32>(streamId);
    out.append(reinterpret_cast<const char*>(&beStreamId), 4);
    
    sendMessage(out);
    qDebug() << "TCPManager: sent CLIENT_STREAM_JOIN streamId=" << streamId;
}

void TCPManager::sendClientStreamLeave(uint32_t streamId)
{
    QByteArray out;
    out.append(static_cast<char>(CLIENT_STREAM_CONN_LEAVE));
    quint32 beStreamId = qToBigEndian<quint32>(streamId);
    out.append(reinterpret_cast<const char*>(&beStreamId), 4);
    
    sendMessage(out);
    qDebug() << "TCPManager: sent CLIENT_STREAM_LEAVE streamId=" << streamId;
}

void TCPManager::sendClientCallCreate()
{
    QByteArray out;
    out.append(static_cast<char>(CLIENT_CALL_CREATE));
    
    sendMessage(out);
    qDebug() << "TCPManager: sent CLIENT_CALL_CREATE";
}

void TCPManager::sendClientCallJoin(uint32_t callId)
{
    QByteArray out;
    out.append(static_cast<char>(CLIENT_CALL_CONN_JOIN));
    quint32 beCallId = qToBigEndian<quint32>(callId);
    out.append(reinterpret_cast<const char*>(&beCallId), 4);
    
    sendMessage(out);
    qDebug() << "TCPManager: sent CLIENT_CALL_JOIN callId=" << callId;
}

void TCPManager::sendClientCallLeave(uint32_t callId)
{
    QByteArray out;
    out.append(static_cast<char>(CLIENT_CALL_CONN_LEAVE));
    quint32 beCallId = qToBigEndian<quint32>(callId);
    out.append(reinterpret_cast<const char*>(&beCallId), 4);
    
    sendMessage(out);
    qDebug() << "TCPManager: sent CLIENT_CALL_LEAVE callId=" << callId;
}