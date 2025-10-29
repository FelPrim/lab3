#pragma once

#include <QObject>
#include <QtNetwork/QUdpSocket>
#include <QtNetwork/QNetworkDatagram>
#include <QHash>
#include <QPair>
#include <QTimer>
#include <QElapsedTimer>
#include <QSet>
#include "video_defaults.h"
#include <QDateTime> 
#include <QVariant>
#include <zlib.h>
#include "reed_solomon.h"

// Структура для представления собираемого фрейма из сетевых пакетов
struct FrameAssembly {
    int streamId;
    int frameNumber;
    int totalParts;
    int receivedParts;
    QVector<QPair<QByteArray, int>> parts;  // Данные + оригинальный размер
    qint64 creationTime;
    
    FrameAssembly() 
        : streamId(0), frameNumber(0), totalParts(0), receivedParts(0), 
          creationTime(QDateTime::currentMSecsSinceEpoch()) 
    {
    }

    FrameAssembly(int stream, int frameNum, int total) 
        : streamId(stream), frameNumber(frameNum), totalParts(total), 
          receivedParts(0), creationTime(QDateTime::currentMSecsSinceEpoch()) 
    {
        parts.resize(total);
        // Инициализируем все части как пустые
        for (int i = 0; i < total; i++) {
            parts[i] = qMakePair(QByteArray(), 0);
        }
    }
    
    bool isComplete() const { 
        return receivedParts >= totalParts; 
    }
    
    QByteArray assembleFrame() const {
        QByteArray result;
        
        for (int i = 0; i < totalParts; i++) {
            const auto& part = parts[i];
            if (!part.first.isEmpty() && part.second > 0) {
                int copySize = qMin(part.second, part.first.size());
                result.append(part.first.constData(), copySize);
            }
        }
        return result;
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

private:
    // Новая конфигурация FEC: N data shards, K = (N+1)/2 parity shards
    // Начальное значение: N=3, K=2, первый байт=4
    ReedSolomonFEC::Config m_fecConfig{3, 6};
    
    // Сетевые методы
    void setupSocket();
    
    // Методы обработки пакетов
    void processIncomingPacket(const QNetworkDatagram &datagram);
    void addPacketToAssembly(int streamId, int frameNumber, int partIndex, int totalParts, const QPair<QByteArray, int>& packetWithSize);
    void completeFrameAssembly(int streamId, int frameNumber);
    
    // Статистика
    void updateSendStats(int packets, int bytes);
    void updateReceiveStats(int packets, int bytes);
    uint32_t calculateCRC32(const QByteArray &data);
    
    void sendPacketWithSize(const QByteArray &data, int streamId, int frameNumber, 
                          int partIndex, int totalParts, int originalSize);
    void processPacketWithSize(const QNetworkDatagram &datagram);
    std::vector<QByteArray> safeEncodeFrame(const QByteArray &frameData);
    bool safeDecodeFrame(FrameAssembly &assembly, QByteArray &result);
    bool canRecoverWithFEC(const FrameAssembly &assembly) const;
    void sendVideoFrameSimple(int streamId, int frameNumber, const QByteArray &frameData);
    
    // Методы для нового протокола
    int calculateOptimalN(int frame_size);
    QByteArray createPacketWithHeader(const QByteArray &data, int streamId, int frameNumber, 
                                    int partIndex, int totalParts, int originalSize);
    bool parsePacketWithHeader(const QByteArray &packetData, int &streamId, int &frameNumber,
                             int &partIndex, int &totalParts, int &originalSize, QByteArray &payload);

private:
    QUdpSocket *m_udpSocket = nullptr;
    QHostAddress m_serverAddress;
    quint16 m_serverPort;
    quint16 m_localPort;
    
    // Таймеры
    QTimer *m_cleanupTimer = nullptr;
    QTimer *m_statsTimer = nullptr;
    
    // Буфер для сборки фреймов: ключ = (streamId, frameNumber)
    QHash<QPair<int, int>, FrameAssembly> m_assemblies;
    
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
    } m_stats;
    
    QElapsedTimer m_operationTimer;
    bool m_initialized = false;
    
    // Константы протокола
    static const int MAX_UDP_PACKET_SIZE = 1200;
    static const int HEADER_SIZE = sizeof(quint8) + sizeof(int) * 5; // firstByte + streamId, frameNumber, partIndex, totalParts, originalSize
    static const int MAX_PAYLOAD_SIZE = MAX_UDP_PACKET_SIZE - HEADER_SIZE;
};
