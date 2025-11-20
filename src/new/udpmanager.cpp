#include "udpmanager.h"
#include "networkmanager.h"
#include <QDebug>
#include <QNetworkInterface>

UDPManager::UDPManager(QObject *parent)
    : QObject(parent)
{
}

UDPManager::~UDPManager()
{
    cleanup();
}

bool UDPManager::initialize(quint16 port)
{
    if (m_udpSocket) {
        return true;
    }

    m_udpSocket = new QUdpSocket(this);
    
    // Увеличиваем размеры буферов
    m_udpSocket->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, QVariant(1024 * 1024));
    m_udpSocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, QVariant(1024 * 1024));
    
    if (!m_udpSocket->bind(QHostAddress::Any, port)) {
        QString error = QString("Failed to bind UDP socket to port %1: %2")
                          .arg(port).arg(m_udpSocket->errorString());
        qCritical() << error;
        emit errorOccurred(error);
        return false;
    }
    
    m_localPort = m_udpSocket->localPort();
    connect(m_udpSocket, &QUdpSocket::readyRead, this, &UDPManager::onPacketReceived);
    
    // Обработчики ошибок
    connect(m_udpSocket, &QUdpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError error) {
        qWarning() << "UDP Socket error:" << error << m_udpSocket->errorString();
    });
    
    qDebug() << "UDPManager: Socket bound to port" << m_localPort;
    return true;
}

void UDPManager::cleanup()
{
    if (m_udpSocket) {
        m_udpSocket->close();
        delete m_udpSocket;
        m_udpSocket = nullptr;
    }
    
    QMutexLocker locker(&m_managersMutex);
    m_networkManagers.clear();
}

void UDPManager::registerNetworkManager(int streamId, NetworkManager *networkManager)
{
    if (!networkManager) {
        qWarning() << "UDPManager: Attempt to register null NetworkManager for stream" << streamId;
        return;
    }
    
    QMutexLocker locker(&m_managersMutex);
    
    if (m_networkManagers.contains(streamId)) {
        qWarning() << "UDPManager: Stream" << streamId << "already registered, replacing";
    }
    
    m_networkManagers[streamId] = networkManager;
    qDebug() << "UDPManager: Registered NetworkManager for stream" << streamId;
}

void UDPManager::unregisterNetworkManager(int streamId)
{
    QMutexLocker locker(&m_managersMutex);
    m_networkManagers.remove(streamId);
    qDebug() << "UDPManager: Unregistered NetworkManager for stream" << streamId;
}

void UDPManager::sendPacket(const QByteArray &data, const QHostAddress &host, quint16 port)
{
    if (!m_udpSocket) {
        qWarning() << "UDPManager: Socket not initialized";
        return;
    }

    if (data.isEmpty()) {
        qWarning() << "UDPManager: Attempt to send empty packet";
        return;
    }

    qint64 sent = m_udpSocket->writeDatagram(data, host, port);
    if (sent == -1) {
        QString error = QString("Failed to send UDP packet to %1:%2: %3")
                          .arg(host.toString()).arg(port).arg(m_udpSocket->errorString());
        qWarning() << error;
        emit errorOccurred(error);
    } else if (sent != data.size()) {
        qWarning() << "UDPManager: Partial send:" << sent << "of" << data.size() << "bytes";
    }
}

void UDPManager::onPacketReceived()
{
    if (!m_udpSocket) return;
    
    while (m_udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(m_udpSocket->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;

        qint64 read = m_udpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
        if (read == -1) {
            qWarning() << "UDPManager: Failed to read datagram";
            continue;
        }

        routePacket(datagram, sender, senderPort);
    }
}

void UDPManager::routePacket(const QByteArray &data, const QHostAddress &sender, quint16 port)
{
    if (data.size() < sizeof(RouteHeader)) {
        qDebug() << "UDPManager: Packet too small for routing:" << data.size();
        return;
    }

    // Извлекаем streamId из RouteHeader (первые 4 байта)
    RouteHeader routeHeader;
    memcpy(&routeHeader, data.constData(), sizeof(RouteHeader));

    int streamId = static_cast<int>(qFromBigEndian(routeHeader.streamId)); 
    QMutexLocker locker(&m_managersMutex);
    
    if (m_networkManagers.contains(streamId)) {
        NetworkManager *manager = m_networkManagers[streamId];
        // Вызываем метод обработки пакета в соответствующем NetworkManager
        QMetaObject::invokeMethod(manager, "onPacketReceived", 
                          Qt::QueuedConnection,
                          Q_ARG(QByteArray, data));
    } else {
        qDebug() << "UDPManager: No NetworkManager registered for stream" << streamId;
    }
}
