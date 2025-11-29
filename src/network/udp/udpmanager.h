#pragma once

#include <QtEndian>
#include <QObject>
#include <QUdpSocket>
#include <QMap>
#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <QHostAddress>
#include "network_packet.h"

class NetworkManager;

class UDPManager : public QObject
{
    Q_OBJECT

public:
    explicit UDPManager(QObject *parent = nullptr);
    ~UDPManager();

    bool initialize(quint16 port = 0);
    void cleanup();
    
    // Регистрация NetworkManager'ов для разных streamId
    void registerNetworkManager(int streamId, NetworkManager *networkManager);
    void unregisterNetworkManager(int streamId);
    
    // Отправка пакетов через общий сокет
    void sendPacket(const QByteArray &data, const QHostAddress &host, quint16 port);
    
    quint16 getLocalPort() const { return m_localPort; }

signals:
    void errorOccurred(const QString &message);

private slots:
    void onPacketReceived();

private:
    void routePacket(const QByteArray &data, const QHostAddress &sender, quint16 port);

    QUdpSocket *m_udpSocket = nullptr;
    quint16 m_localPort = 0;
    
    // Маршрутизация: streamId -> NetworkManager
    QMap<int, NetworkManager*> m_networkManagers;
    QMutex m_managersMutex;
};