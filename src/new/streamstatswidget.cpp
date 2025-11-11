#include "streamstatswidget.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QTimer>
#include <QRandomGenerator>
#include <QDebug>

StreamStatsWidget::StreamStatsWidget(StatsType type, QWidget *parent)
    : QWidget(parent)
    , m_type(type)
    , m_currentFps(0)
    , m_currentBitrate(0)
    , m_packetsSent(0)
    , m_packetsTotal(0)
    , m_currentPacketLoss(0)
    , m_recoveredPackets(0)
{
    setupUI();
    
    // Таймер для обновления статистики
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, &StreamStatsWidget::updateStats);
    m_updateTimer->start(1000); // Обновление каждую секунду
}

void StreamStatsWidget::setupUI()
{
    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(15);
    
    if (m_type == PublisherStats) {
        setupPublisherUI();
    } else {
        setupViewerUI();
    }
}

void StreamStatsWidget::setupPublisherUI()
{
    auto layout = qobject_cast<QHBoxLayout*>(this->layout());
    
    m_fpsLabel = new QLabel("FPS: --", this);
    m_bitrateLabel = new QLabel("Bitrate: -- kbps", this);
    m_packetsLabel = new QLabel("Packets: --/--", this);
    
    m_fpsLabel->setStyleSheet("color: #90CAF9; font-size: 11px;");
    m_bitrateLabel->setStyleSheet("color: #80CBC4; font-size: 11px;");
    m_packetsLabel->setStyleSheet("color: #FFCC80; font-size: 11px;");
    
    layout->addWidget(m_fpsLabel);
    layout->addWidget(m_bitrateLabel);
    layout->addWidget(m_packetsLabel);
    layout->addStretch();
}

void StreamStatsWidget::setupViewerUI()
{
    auto layout = qobject_cast<QHBoxLayout*>(this->layout());
    
    m_fpsLabel = new QLabel("FPS: --", this);
    m_bitrateLabel = new QLabel("Bitrate: -- kbps", this);
    m_packetLossLabel = new QLabel("Loss: --%", this);
    m_recoveredLabel = new QLabel("Recovered: --", this);  // Новый показатель
    
    m_fpsLabel->setStyleSheet("color: #90CAF9; font-size: 11px;");
    m_bitrateLabel->setStyleSheet("color: #80CBC4; font-size: 11px;");
    m_packetLossLabel->setStyleSheet("color: #EF9A9A; font-size: 11px;");
    m_recoveredLabel->setStyleSheet("color: #A5D6A7; font-size: 11px;");  // Светло-зеленый для восстановленных пакетов
    
    layout->addWidget(m_fpsLabel);
    layout->addWidget(m_bitrateLabel);
    layout->addWidget(m_packetLossLabel);
    layout->addWidget(m_recoveredLabel);
    layout->addStretch();
}

void StreamStatsWidget::setFps(double fps)
{
    m_currentFps = fps;
}

void StreamStatsWidget::setBitrate(int bitrateKbps)
{
    m_currentBitrate = bitrateKbps;
}

void StreamStatsWidget::setSentPackets(int sent, int total)
{
    m_packetsSent = sent;
    m_packetsTotal = total;
}

void StreamStatsWidget::setPacketLoss(double lossPercent)
{
    m_currentPacketLoss = lossPercent;
}

void StreamStatsWidget::setRecoveredPackets(int recovered)  // Новый метод
{
    m_recoveredPackets = recovered;
}

void StreamStatsWidget::updateStats()
{
    auto* generator = QRandomGenerator::global();
    
    // Заглушка: генерируем случайные значения для демонстрации
    if (m_type == PublisherStats) {
        m_currentFps = 14 + (generator->bounded(5)); // 14-19 FPS
        m_currentBitrate = 800 + (generator->bounded(400)); // 800-1200 kbps
        m_packetsSent += 10 + (generator->bounded(20));
        m_packetsTotal = m_packetsSent + (generator->bounded(5));
        
        m_fpsLabel->setText(QString("FPS: %1").arg(m_currentFps, 0, 'f', 1));
        m_bitrateLabel->setText(QString("Bitrate: %1 kbps").arg(m_currentBitrate));
        m_packetsLabel->setText(QString("Packets: %1/%2").arg(m_packetsSent).arg(m_packetsTotal));
    } else {
        m_currentFps = 14 + (generator->bounded(5));
        m_currentBitrate = 750 + (generator->bounded(300));
        m_currentPacketLoss = (generator->bounded(50)) / 10.0; // 0.0-5.0%
        m_recoveredPackets += generator->bounded(5); // Увеличиваем количество восстановленных пакетов
        
        m_fpsLabel->setText(QString("FPS: %1").arg(m_currentFps, 0, 'f', 1));
        m_bitrateLabel->setText(QString("Bitrate: %1 kbps").arg(m_currentBitrate));
        m_packetLossLabel->setText(QString("Loss: %1%").arg(m_currentPacketLoss, 0, 'f', 1));
        m_recoveredLabel->setText(QString("Recovered: %1").arg(m_recoveredPackets));
        
        // Меняем цвет recovered в зависимости от значения (чем больше - тем лучше)
        if (m_recoveredPackets > 50) {
            m_recoveredLabel->setStyleSheet("color: #4CAF50; font-size: 11px;"); // Ярко-зеленый для высоких значений
        } else if (m_recoveredPackets > 20) {
            m_recoveredLabel->setStyleSheet("color: #A5D6A7; font-size: 11px;"); // Светло-зеленый для средних значений
        } else {
            m_recoveredLabel->setStyleSheet("color: #C8E6C9; font-size: 11px;"); // Очень светлый зеленый для низких значений
        }
    }
}
