#pragma once

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <QSet>
#include <QHostAddress>
#include <QDateTime>
#include <QVariant>
#include <zlib.h>
#include <QtEndian>
#include <QMutex>
#include <QQueue>
#include "network_packet.h"
#include "../../video_defaults.h"
#include "fecbuffer.h"
#include "packetgroupbuffer.h"
#include <QTimer>

class UDPManager;

class NetworkManager : public QObject
{
    Q_OBJECT

public:
    explicit NetworkManager(int streamId, QObject *parent = nullptr);
    ~NetworkManager() override;

    bool initialize(UDPManager *udpManager);
    void cleanup();
    void sendVideoFrame(int frameNumber, const QByteArray &frameData);
    void setServerAddress(const QString &address, quint16 port);
    QHostAddress getServerAddress() const { return m_serverAddress; }
    quint16 getServerPort() const { return m_serverPort; }
    int getStreamId() const { return m_streamId; }

    // Call ID management
    void setCallId(uint32_t callId) { m_callId = callId; }
    uint32_t getCallId() const { return m_callId; }

    // Methods for managing sending
    void setSendingEnabled(bool enabled) { m_sendingEnabled = enabled; }
    bool isSendingEnabled() const { return m_sendingEnabled; }

signals:
    void frameAssembled(int streamId, int frameNumber, const QByteArray &frameData);
    void errorOccurred(const QString &message);
    void statisticsUpdated(const QString &stats);

public slots:
    void start();
    void stop();
    void processPacket(const QByteArray &data, const QHostAddress &sender, quint16 port);

private slots:
    void cleanupOldAssemblies();
    void printStatistics();
    void onFrameComplete(int streamId, int frameNumber, const QByteArray &frameData);
    void onPacketRecovered(uint32_t packetSequence);

private:
    // Основные методы
    void processPacketNewProtocol(const QByteArray &data);
    void sendPacketNewProtocol(const char *data, const uint32_t size, PacketType type);
    
    // Статистика
    void updateSendStats(int packets, int bytes);
    void updateReceiveStats(int packets, int bytes);
    uint32_t calculateCRC32(const QByteArray &data);

private:
    UDPManager *m_udpManager = nullptr;
    QHostAddress m_serverAddress;
    quint16 m_serverPort;
    
    QTimer *m_cleanupTimer = nullptr;
    QTimer *m_statsTimer = nullptr;
    
    // Компоненты трехслойной архитектуры
    FecBuffer *m_fecBuffer = nullptr;           // Слой 3: FEC восстановление
    PacketGroupBuffer *m_packetBuffer = nullptr; // Слой 2: Группировка пакетов
    
    // FEC буфер для отправки (используется при создании XOR пакетов)
    uint8_t m_fecSendBuffer[4][1188];
    int m_fecSendBufferCount = 0;
    
    // Идентификаторы
    int m_streamId;
    uint32_t m_callId = 0;
    int m_packetSequence = 0;

    struct Statistics {
        quint32 totalPacketsSent = 0;
        quint32 totalPacketsReceived = 0;
        quint32 fecGroupsSent = 0;
        quint32 fecGroupsRecovered = 0;
        quint32 packetsRecoveredByFEC = 0;
        quint32 framesSent = 0;
        quint32 framesReceived = 0;
        
    } m_stats;
    
    QElapsedTimer m_operationTimer;
    bool m_initialized = false;
    bool m_sendingEnabled = false;
    
private:
    static const int MAX_FEC_BUFFER_SIZE = 128;
    uint32_t frameNumber = 0;
    uint32_t frameSize = 0;
};
