#include "tcpmanager.h"
#include <QDataStream>
#include <QHostAddress>
#include <QDebug>
#include <QtEndian> // qToBigEndian / qFromBigEndian

TCPManager::TCPManager(QObject *parent)
    : QObject(parent), m_socket(new QTcpSocket(this))
{
    connect(m_socket, &QTcpSocket::connected, this, &TCPManager::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &TCPManager::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &TCPManager::onReadyRead);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
            this, &TCPManager::onErrorOccured);
}

TCPManager::~TCPManager()
{
    if (m_socket->isOpen()) m_socket->close();
}

void TCPManager::connectToServer()
{
    if (!m_socket) return;

    if (m_localBindPort != 0 || m_localBindAddr != QHostAddress::Any) {
        if (!m_socket->bind(m_localBindAddr, m_localBindPort)) {
            qWarning() << "TCPManager: bind failed:" << m_socket->errorString();
        }
    }

    qDebug() << "TCPManager: connecting to" << m_serverHost << ":" << m_serverPort;
    m_socket->connectToHost(m_serverHost, m_serverPort);
}

void TCPManager::disconnectFromServer()
{
    if (m_socket) m_socket->disconnectFromHost();
}

void TCPManager::onConnected()
{
    qDebug() << "TCPManager: connected to server";
    emit connected();
}

void TCPManager::onDisconnected()
{
    qDebug() << "TCPManager: disconnected from server";
    emit disconnected();
}

void TCPManager::onErrorOccured(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError)
    QString err = m_socket->errorString();
    qWarning() << "TCPManager: socket error:" << err;
    emit errorOccurred(err);
}

void TCPManager::onReadyRead()
{
    QByteArray chunk = m_socket->readAll();
    if (!chunk.isEmpty()) {
        m_buffer.append(chunk);
        processBuffer();
    }
}

void TCPManager::processBuffer()
{
    while (!m_buffer.isEmpty()) {
        if (m_buffer.size() < 1) return;
        uint8_t type = static_cast<uint8_t>(m_buffer.at(0));

        qDebug() << "TCPManager: Processing message, type:" << QString::number(type, 16) 
                 << "buffer size:" << m_buffer.size();

        // Обработка сообщений от сервера
        switch (type) {
            // Общие сообщения
            case SERVER_HANDSHAKE_START: {
                if (m_buffer.size() < 5) return; // 1 + 4 bytes
                uint32_t connectionId = qFromBigEndian<quint32>(
                    reinterpret_cast<const uchar*>(m_buffer.constData() + 1));
                m_buffer.remove(0, 5);
                emit serverHandshakeStart(connectionId);
                break;
            }
            
            case SERVER_HANDSHAKE_END: {
                if (m_buffer.size() < 5) return; // 1 + 4 bytes
                uint32_t connectionId = qFromBigEndian<quint32>(
                    reinterpret_cast<const uchar*>(m_buffer.constData() + 1));
                m_buffer.remove(0, 5);
                emit serverHandshakeEnd(connectionId);
                break;
            }
            
            case SERVER_ERROR:
            case SERVER_SUCCESS: {
                if (m_buffer.size() < 2) return; // 1 + 1 bytes (минимум)
                uint8_t originalMessageType = static_cast<uint8_t>(m_buffer.at(1));
                uint8_t messageLength = static_cast<uint8_t>(m_buffer.at(2));
                
                if (m_buffer.size() < 3 + messageLength) return;
                
                QString message = QString::fromUtf8(m_buffer.constData() + 3, messageLength);
                m_buffer.remove(0, 3 + messageLength);
                
                if (type == SERVER_ERROR) {
                    emit serverErrorReceived(originalMessageType, message);
                } else {
                    emit serverSuccessReceived(originalMessageType, message);
                }
                break;
            }
            
            // Простые сообщения с 4-байтным ID
            case SERVER_STREAM_CREATED:
            case SERVER_STREAM_DELETED:
            case SERVER_STREAM_CONN_JOINED:
            case SERVER_STREAM_START:
            case SERVER_STREAM_END:
            case SERVER_CALL_CREATED: {
                if (m_buffer.size() < 5) return; // 1 + 4 bytes
                uint32_t id = qFromBigEndian<quint32>(
                    reinterpret_cast<const uchar*>(m_buffer.constData() + 1));
                m_buffer.remove(0, 5);
                
                switch (type) {
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
                const int min_size = 1 + 4 + 1 + 1; // type + call_id + participant_count + stream_count
                if (m_buffer.size() < min_size) return;
                
                const uchar *data = reinterpret_cast<const uchar*>(m_buffer.constData());
                uint32_t call_id = qFromBigEndian<quint32>(data + 1);
                uint8_t participant_count = data[5];
                uint8_t stream_count = data[6];
                
                int total_size = min_size + (participant_count * 4) + (stream_count * 4);
                if (m_buffer.size() < total_size) return;
                
                QVector<uint32_t> participants;
                QVector<uint32_t> streams;
                
                int offset = 7;
                for (int i = 0; i < participant_count; ++i) {
                    uint32_t participant_id = qFromBigEndian<quint32>(data + offset);
                    participants.append(participant_id);
                    offset += 4;
                }
                
                for (int i = 0; i < stream_count; ++i) {
                    uint32_t stream_id = qFromBigEndian<quint32>(data + offset);
                    streams.append(stream_id);
                    offset += 4;
                }
                
                m_buffer.remove(0, total_size);
                emit serverCallConnJoined(call_id, participants, streams);
                break;
            }
            
            // Сообщения с call_id + connection_id/stream_id
            case SERVER_CALL_CONN_NEW:
            case SERVER_CALL_CONN_LEFT:
            case SERVER_CALL_STREAM_NEW:  
            case SERVER_CALL_STREAM_DELETED: {
                if (m_buffer.size() < 9) return; // 1 + 4 + 4 bytes
                const uchar *data = reinterpret_cast<const uchar*>(m_buffer.constData());
                uint32_t call_id = qFromBigEndian<quint32>(data + 1);
                uint32_t id = qFromBigEndian<quint32>(data + 5);
                
                m_buffer.remove(0, 9);
                
                switch (type) {
                    case SERVER_CALL_CONN_NEW: emit serverCallConnNew(call_id, id); break;
                    case SERVER_CALL_CONN_LEFT: emit serverCallConnLeft(call_id, id); break;
                    case SERVER_CALL_STREAM_NEW: emit serverCallStreamNew(call_id, id); break;
                    case SERVER_CALL_STREAM_DELETED: emit serverCallStreamDeleted(call_id, id); break;
                }
                break;
            }
            
            default:
                qWarning() << "TCPManager: Unknown message type:" << type;
                m_buffer.remove(0, 1);
                break;
        }
    }
}

// ================ МЕТОДЫ ОТПРАВКИ ================

// Общие сообщения
void TCPManager::sendClientError(uint8_t originalMessageType, const QString &errorMessage)
{
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) return;
    
    QByteArray messageData = errorMessage.toUtf8();
    if (messageData.size() > 255) {
        messageData = messageData.left(255);
    }
    
    QByteArray out;
    out.append(static_cast<char>(CLIENT_ERROR));
    out.append(static_cast<char>(originalMessageType));
    out.append(static_cast<char>(messageData.size()));
    out.append(messageData);
    
    m_socket->write(out);
    m_socket->flush();
    qDebug() << "TCPManager: sent CLIENT_ERROR for message type" << originalMessageType;
}

void TCPManager::sendClientSuccess(uint8_t originalMessageType, const QString &successMessage)
{
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) return;
    
    QByteArray messageData = successMessage.toUtf8();
    if (messageData.size() > 255) {
        messageData = messageData.left(255);
    }
    
    QByteArray out;
    out.append(static_cast<char>(CLIENT_SUCCESS));
    out.append(static_cast<char>(originalMessageType));
    out.append(static_cast<char>(messageData.size()));
    out.append(messageData);
    
    m_socket->write(out);
    m_socket->flush();
    qDebug() << "TCPManager: sent CLIENT_SUCCESS for message type" << originalMessageType;
}

// Сообщения стримов
void TCPManager::sendClientStreamCreate(uint32_t callId)
{
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) return;
    
    QByteArray out;
    out.append(static_cast<char>(CLIENT_STREAM_CREATE));
    quint32 be_callId = qToBigEndian<quint32>(callId);
    out.append(reinterpret_cast<const char*>(&be_callId), 4);
    
    m_socket->write(out);
    m_socket->flush();
    qDebug() << "TCPManager: sent CLIENT_STREAM_CREATE callId=" << callId;
}

void TCPManager::sendClientStreamDelete(uint32_t streamId)
{
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) return;
    
    QByteArray out;
    out.append(static_cast<char>(CLIENT_STREAM_DELETE));
    quint32 be_streamId = qToBigEndian<quint32>(streamId);
    out.append(reinterpret_cast<const char*>(&be_streamId), 4);
    
    m_socket->write(out);
    m_socket->flush();
    qDebug() << "TCPManager: sent CLIENT_STREAM_DELETE streamId=" << streamId;
}

void TCPManager::sendClientStreamJoin(uint32_t streamId)
{
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) return;
    
    QByteArray out;
    out.append(static_cast<char>(CLIENT_STREAM_CONN_JOIN));
    quint32 be_streamId = qToBigEndian<quint32>(streamId);
    out.append(reinterpret_cast<const char*>(&be_streamId), 4);
    
    m_socket->write(out);
    m_socket->flush();
    qDebug() << "TCPManager: sent CLIENT_STREAM_JOIN streamId=" << streamId;
}

void TCPManager::sendClientStreamLeave(uint32_t streamId)
{
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) return;
    
    QByteArray out;
    out.append(static_cast<char>(CLIENT_STREAM_CONN_LEAVE));
    quint32 be_streamId = qToBigEndian<quint32>(streamId);
    out.append(reinterpret_cast<const char*>(&be_streamId), 4);
    
    m_socket->write(out);
    m_socket->flush();
    qDebug() << "TCPManager: sent CLIENT_STREAM_LEAVE streamId=" << streamId;
}

// Сообщения звонков
void TCPManager::sendClientCallCreate()
{
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) return;
    
    QByteArray out;
    out.append(static_cast<char>(CLIENT_CALL_CREATE));
    m_socket->write(out);
    m_socket->flush();
    qDebug() << "TCPManager: sent CLIENT_CALL_CREATE";
}

void TCPManager::sendClientCallJoin(uint32_t callId)
{
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) return;
    
    QByteArray out;
    out.append(static_cast<char>(CLIENT_CALL_CONN_JOIN));
    quint32 be_callId = qToBigEndian<quint32>(callId);
    out.append(reinterpret_cast<const char*>(&be_callId), 4);
    
    m_socket->write(out);
    m_socket->flush();
    qDebug() << "TCPManager: sent CLIENT_CALL_JOIN callId=" << callId;
}

void TCPManager::sendClientCallLeave(uint32_t callId)
{
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) return;
    
    QByteArray out;
    out.append(static_cast<char>(CLIENT_CALL_CONN_LEAVE));
    quint32 be_callId = qToBigEndian<quint32>(callId);
    out.append(reinterpret_cast<const char*>(&be_callId), 4);
    
    m_socket->write(out);
    m_socket->flush();
    qDebug() << "TCPManager: sent CLIENT_CALL_LEAVE callId=" << callId;
}
