#pragma once

#include <QObject>
#include <QTimer>
#include <QHostAddress>

class UDPManager;

class HandshakeService : public QObject {
    Q_OBJECT

public:
    explicit HandshakeService(UDPManager *udpManager, QObject *parent = nullptr);
    ~HandshakeService();

    void startHandshake(uint32_t connectionId, const QHostAddress &serverAddress, quint16 serverPort);
    void stopHandshake();
    void onHandshakeConfirmed();

    bool isHandshakeCompleted() const { return m_handshakeCompleted; }
    uint32_t getConnectionId() const { return m_connectionId; }  // ✅ Реализован

signals:
    void handshakeStarted(uint32_t connectionId);
    void handshakeCompleted(uint32_t connectionId);
    void handshakeFailed(const QString &error);

private slots:
    void sendHandshakePacket();

private:
    UDPManager *m_udpManager;
    QTimer *m_handshakeTimer;
    
    uint32_t m_connectionId = 0;
    QHostAddress m_serverAddress;
    quint16 m_serverPort = 0;
    bool m_handshakeCompleted = false;
    int m_attempts = 0;
    
    static const int MAX_ATTEMPTS = 50;
    static const int HANDSHAKE_INTERVAL_MS = 100; // 10 packets per second
};