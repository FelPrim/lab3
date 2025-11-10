#pragma once

#include <QObject>
#include <QtNetwork/QUdpSocket>
#include <QtNetwork/QNetworkDatagram>
#include <QHash>
#include <QPair>
#include <QTimer>
#include <QElapsedTimer>
#include <QSet>
#include <QDateTime> 
#include <QVariant>
#include <zlib.h>
#include "simplefec.h"

// Объявляем константы здесь для networkmanager
constexpr int XOR_FEC_K = 4;  // Data packets
constexpr int XOR_FEC_N = 5;  // Total packets (4 data + 1 XOR)

// Типы пакетов
enum PacketType {
    START_FRAME = 0x01,
    CONTINUE_FRAME = 0x02,
    BOUNDARY_FRAME = 0x03,
    FAILED_BOUNDARY_FRAME = 0x04,
    XOR_FEC_PACKET = 0x06  // Заменяем FEC_PACKET на XOR_FEC_PACKET
};

// Структура для сборки фреймов
struct StreamAssembly {
    int streamId;
    int frameNumber;
    int totalSize;
    int receivedSize;
    QByteArray data;
    qint64 creationTime;
    bool hasStartFrame;
    
    // Конструктор по умолчанию для QHash
    StreamAssembly() 
        : streamId(0), frameNumber(0), totalSize(0), receivedSize(0),
          creationTime(QDateTime::currentMSecsSinceEpoch()), hasStartFrame(false) 
    {}
    
    StreamAssembly(int stream, int frame) 
        : streamId(stream), frameNumber(frame), totalSize(0), receivedSize(0),
          creationTime(QDateTime::currentMSecsSinceEpoch()), hasStartFrame(false) 
    {}
    
    bool isComplete() const { 
        return hasStartFrame && receivedSize >= totalSize; 
    }
};

class NetworkManager : public QObject
{
    Q_OBJECT

public:
    explicit NetworkManager(QObject *parent = nullptr);
    ~NetworkManager() override;

    bool initialize();
    void cleanup();

    // Метод для отправки фрейма (вызывается VideoEncoder)
    void sendVideoFrame(int streamId, int frameNumber, const QByteArray &frameData);

    // Методы для управления сетевыми настройками
    void setServerAddress(const QString &address, quint16 port);
    QHostAddress getServerAddress() const { return m_serverAddress; }
    quint16 getServerPort() const { return m_serverPort; }
    quint16 getLocalPort() const { return m_localPort; }
    void setPort(quint16 port) { m_localPort = port; }

signals:
    // Сигнал о собранном фрейме для FrameBuffer
    void frameAssembled(int streamId, int frameNumber, const QByteArray &frameData);
    void errorOccurred(const QString &message);
    void statisticsUpdated(const QString &stats);

public slots:
    void start();
    void stop();

private slots:
    void onPacketReceived();
    void cleanupOldAssemblies();
    void printStatistics();
    void onXORFECGroupDecoded(int streamId, int groupId, const QVector<QByteArray> &packets);

private:
    // Сетевые методы
    void setupSocket();
    
    // Новые методы для протокола с фиксированными пакетами
    void processPacketNewProtocol(const QNetworkDatagram &datagram);
    void sendPacketNewProtocol(const QByteArray &data, int streamId, PacketType type);
    void sendPacketNewProtocol(const QByteArray &data, int streamId, PacketType type, int customSequence);
    void sendBufferedData();
    
    // Обработчики типов пакетов
    void handleStartFrame(int streamId, const QByteArray& data);
    void handleContinueFrame(int streamId, const QByteArray& data);
    void handleBoundaryFrame(int streamId, const QByteArray& data);
    void handleFailedBoundaryFrame(int streamId, const QByteArray& data);
    void processCompleteFrame(int streamId);
    
    // XOR FEC методы
    void sendXORFECGroup(int streamId, int groupStartSequence);
    void processXORFECPacket(int streamId, int groupId, const QByteArray &data);
    void processRecoveredPacket(const QByteArray &packetData);
    
    // Статистика
    void updateSendStats(int packets, int bytes);
    void updateReceiveStats(int packets, int bytes);
    uint32_t calculateCRC32(const QByteArray &data);

private:
    QUdpSocket *m_udpSocket = nullptr;
    QHostAddress m_serverAddress;
    quint16 m_serverPort;
    quint16 m_localPort;
    
    // Таймеры
    QTimer *m_cleanupTimer = nullptr;
    QTimer *m_statsTimer = nullptr;
    
    // Новые буферы для протокола с фиксированными пакетами
    QHash<int, StreamAssembly> m_streamAssemblies;
    QByteArray m_sendBuffer;
    int m_currentFrameNumber = 0;
    int m_packetSequence = 0;
    int m_streamId = 0;
    
    // XOR FEC
    SimpleFEC *m_xorFEC;
    QVector<QByteArray> m_currentDataPackets;
    int m_currentGroupStartSequence = 0;
    
    // Статистика
    struct Statistics {
        quint64 totalPacketsSent = 0;
        quint64 totalPacketsReceived = 0;
        quint64 totalBytesSent = 0;
        quint64 totalBytesReceived = 0;
        quint64 framesSent = 0;
        quint64 framesReceived = 0;
        quint64 assembliesCompleted = 0;
        quint64 assembliesDropped = 0;
        
        // Для расчета потерь
        QSet<QPair<int, int>> expectedFrames;
        QSet<QPair<int, int>> receivedFrames;
        
        // XOR FEC статистика
        quint64 xorGroupsSent = 0;
        quint64 xorGroupsRecovered = 0;
        quint64 packetsRecoveredByXOR = 0;
    } m_stats;
    
    QElapsedTimer m_operationTimer;
    bool m_initialized = false;
    
    // Константы протокола
    static const int MAX_UDP_PACKET_SIZE = 1200;
    static const int PACKET_HEADER_SIZE = 12;
    static const int MAX_PAYLOAD_SIZE = MAX_UDP_PACKET_SIZE - PACKET_HEADER_SIZE;
    static const int FRAME_HEADER_SIZE = 8;
};
