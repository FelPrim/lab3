#pragma once

#include "capturethread.h"
#include "video_defaults.h"
#include <QtNetwork/QUdpSocket>
#include <QtNetwork/QNetworkDatagram>
#include <QMap>
#include <QElapsedTimer>
#include <QTimer>
#include <exception>

class NetworkCaptureThread : public CaptureThread
{
    Q_OBJECT

public:
    explicit NetworkCaptureThread(QObject *parent = nullptr);
    ~NetworkCaptureThread() override;

    void startCapture(int deviceIndex) override;
    void stopCapture() override;

protected:
    void run() override;
    
    // Переопределяем обработку пакетов от энкодера
    virtual void handleEncodedPacket(const QByteArray &packet, int frameNumber) override;

private slots:
    void onPacketReceived();
    void printStatistics();

private:
    void setupNetwork();
    void sendPacketPart(const QByteArray &data, int frameNumber, int partIndex, int totalParts);
    void processNetworkPacket(const QByteArray &packet, int frameNumber, int partIndex, int totalParts);
    void updatePacketSizeStats(const QByteArray &packet);

    QUdpSocket *m_udpSocket = nullptr;
    QHostAddress m_echoServerAddress;
    quint16 m_echoServerPort;
    
    QTimer *m_statsTimer = nullptr;
    
    // Статистика
    struct Statistics {
        quint64 totalSent = 0;
        quint64 totalReceived = 0;
        quint64 totalBytesSent = 0;
        quint64 totalBytesReceived = 0;
        quint32 minPacketSize = 0;
        quint32 maxPacketSize = 0;
        double averagePacketSize = 0;
        
        // Для расчета потерь
        QSet<int> expectedFrames;
        QSet<int> receivedFrames;
        
        // Статистика по частям пакетов
        quint64 packetsSplit = 0;
    } m_stats;
    
    int m_networkFrameCount = 0;
    QElapsedTimer m_captureTimer;
    
    // Константы протокола
    static const int MAX_UDP_PACKET_SIZE = 1200; // Безопасный размер для UDP через интернет
    static const int HEADER_SIZE = sizeof(int) * 3; // frameNumber + partIndex + totalParts
	static const int MAX_PAYLOAD_SIZE = MAX_UDP_PACKET_SIZE - HEADER_SIZE;
};