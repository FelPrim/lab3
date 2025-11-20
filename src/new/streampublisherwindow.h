#pragma once

#include "../videocapture.h"
#include "../videoencoder.h"
#include "../networkmanager.h"
#include "streamwindow.h"
#include "streamcontrolpanel.h"
#include "streamvideodisplaypanel.h"
#include "deviceselectorwidget.h"
#include "streamstatswidget.h"

class StreamPublisherWindow : public StreamWindow
{
    Q_OBJECT

public:
    // Теперь конструктор принимает deviceIndex
    explicit StreamPublisherWindow(int streamId, int deviceIndex, QWidget *parent = nullptr);
    ~StreamPublisherWindow();

    // StreamWindow interface
    void initialize() override;
    void cleanup() override;
    int getStreamId() const override { return m_streamId; }
    QString getDisplayId() const override { return m_displayId; }
    bool isActive() const override { return m_isStreaming; }

    void setStreamId(uint32_t streamId, const QString &displayId);

    void initializeWithRealId(uint32_t streamId, const QString &displayId);
    // Специфичные методы ведущего
    void startStream();
    void stopStream();

signals:
    void streamStopped(int streamId);
    void streamingStateChanged(int streamId, bool enabled);  // УДАЛЕНО ДУБЛИРОВАНИЕ
    void encodedPacketReady(int streamId, int frameNumber, const QByteArray &packet);

public slots:
    void setViewersStatus(bool hasViewers);
    void setStreamingEnabled(bool enabled);

private slots:
    void onStopRequested();
    void onVideoError(const QString &message);
    void onRawFrameReady(const QImage &image);  // ДОБАВЛЕНО
    void onFrameForEncoding(const cv::Mat &frame);  // ДОБАВЛЕНО

private:
    void setupUI();
    void setupConnections();
    void updateStatusLabel();
    void initializeVideoCapture();
    void cleanupVideoCapture();
    void initializeVideoEncoder();  // ДОБАВЛЕНО
    void cleanupVideoEncoder();     // ДОБАВЛЕНО
    cv::Mat qImageToCvMat(const QImage &image);  // ДОБАВЛЕНО

    int m_streamId;
    QString m_displayId;
    bool m_isStreaming;
    bool m_hasViewers;

    // Специфичные для ведущего виджеты
    StreamControlPanel *m_controlPanel;
    StreamVideoDisplayPanel *m_videoDisplay;
    StreamStatsWidget *m_statsWidget;

    // Видео компоненты
    VideoCapture *m_videoCapture;
    VideoEncoder *m_videoEncoder;

    int m_deviceIndex; // выбранный индекс устройства

    static const QString STATUS_NO_VIEWERS;
    static const QString STATUS_HAS_VIEWERS;
    static const QString STATUS_STOPPED;
public slots:
    void onFrameEncoded(int streamId, int frameNumber, const QByteArray &packet);
    void updateStreamInfo(); // или setStreamInfo, если он уже существует
private:
    bool m_streamingEnabled = false;
};
