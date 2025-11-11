#pragma once

#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QTimer>

class StreamStatsWidget : public QWidget
{
    Q_OBJECT

public:
    enum StatsType {
        PublisherStats,
        ViewerStats
    };

    explicit StreamStatsWidget(StatsType type, QWidget *parent = nullptr);

    void setFps(double fps);
    void setBitrate(int bitrateKbps);
    void setSentPackets(int sent, int total);
    void setPacketLoss(double lossPercent);
    void setRecoveredPackets(int recovered);  // Новый метод

public slots:
    void updateStats();

private:
    void setupUI();
    void setupPublisherUI();
    void setupViewerUI();
    
    StatsType m_type;
    
    QLabel *m_fpsLabel;
    QLabel *m_bitrateLabel;
    QLabel *m_packetsLabel;
    QLabel *m_packetLossLabel;
    QLabel *m_recoveredLabel;  // Заменяем QProgressBar на QLabel
    
    QTimer *m_updateTimer;
    
    double m_currentFps;
    int m_currentBitrate;
    int m_packetsSent;
    int m_packetsTotal;
    double m_currentPacketLoss;
    int m_recoveredPackets;  // Заменяем m_currentLatency и m_currentBufferLevel
};
