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

        if (type >= 0x80) {
            const int need = 1 + 4;
            if (m_buffer.size() < need) return;
            uint8_t t = static_cast<uint8_t>(m_buffer.at(0));
            const uchar *p = reinterpret_cast<const uchar*>(m_buffer.constData()+1);
            uint32_t id = qFromBigEndian<quint32>(p);

            m_buffer.remove(0, need);

            switch (t) {
                case 0x81:
                    qDebug() << "TCPManager: SERVER_STREAM_CREATED id=" << id;
                    emit serverStreamCreated(id);
                    break;
                case 0x82:
                    qDebug() << "TCPManager: SERVER_STREAM_DELETED id=" << id;
                    emit serverStreamDeleted(id);
                    break;
                case 0x83:
                    qDebug() << "TCPManager: SERVER_STREAM_JOINED id=" << id;
                    emit serverStreamJoined(id);
                    break;
                case 0x84:
                    qDebug() << "TCPManager: SERVER_STREAM_START id=" << id;
                    emit serverStreamStart(id);
                    break;
                case 0x85:
                    qDebug() << "TCPManager: SERVER_STREAM_END id=" << id;
                    emit serverStreamEnd(id);
                    break;
                default:
                    qWarning() << "TCPManager: Unknown server message type:" << t;
                    break;
            }
            continue;
        } else {
            qWarning() << "TCPManager: unexpected message type from server:" << type << " — dropping byte";
            m_buffer.remove(0,1);
        }
    }
}

// -------------- outgoing --------------

void TCPManager::sendClientUdpAddr(const QByteArray &sockaddr_in_bytes)
{
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) {
        qWarning() << "TCPManager: not connected, cannot send CLIENT_UDP_ADDR";
        return;
    }
    if (sockaddr_in_bytes.size() != 16) {
        qWarning() << "TCPManager: CLIENT_UDP_ADDR expects 16 bytes (sockaddr_in)";
        return;
    }
    QByteArray out;
    out.append(static_cast<char>(0x01)); // CLIENT_UDP_ADDR
    out.append(sockaddr_in_bytes);
    m_socket->write(out);
    m_socket->flush();
    qDebug() << "TCPManager: sent CLIENT_UDP_ADDR bytes:" << out.size();
}

void TCPManager::sendClientDisconnect()
{
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) return;
    QByteArray out; out.append(static_cast<char>(0x02));
    m_socket->write(out); m_socket->flush();
    qDebug() << "TCPManager: sent CLIENT_DISCONNECT";
}

void TCPManager::sendClientStreamCreate()
{
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) return;
    QByteArray out; out.append(static_cast<char>(0x03));
    m_socket->write(out); m_socket->flush();
    qDebug() << "TCPManager: sent CLIENT_STREAM_CREATE";
}

void TCPManager::sendClientStreamDelete(uint32_t streamId)
{
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) return;
    QByteArray out; out.append(static_cast<char>(0x04));
    quint32 be = qToBigEndian<quint32>(streamId);
    out.append(reinterpret_cast<const char*>(&be), 4);
    m_socket->write(out); m_socket->flush();
    qDebug() << "TCPManager: sent CLIENT_STREAM_DELETE id=" << streamId;
}

void TCPManager::sendClientStreamJoin(uint32_t streamId)
{
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) return;
    QByteArray out; out.append(static_cast<char>(0x05));
    quint32 be = qToBigEndian<quint32>(streamId);
    out.append(reinterpret_cast<const char*>(&be), 4);
    m_socket->write(out); m_socket->flush();
    qDebug() << "TCPManager: sent CLIENT_STREAM_JOIN id=" << streamId;
}

void TCPManager::sendClientStreamLeave(uint32_t streamId)
{
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) return;
    QByteArray out; out.append(static_cast<char>(0x06));
    quint32 be = qToBigEndian<quint32>(streamId);
    out.append(reinterpret_cast<const char*>(&be), 4);
    m_socket->write(out); m_socket->flush();
    qDebug() << "TCPManager: sent CLIENT_STREAM_LEAVE id=" << streamId;
}
