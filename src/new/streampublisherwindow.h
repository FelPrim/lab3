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
    explicit StreamPublisherWindow(int streamId, QWidget *parent = nullptr);
    ~StreamPublisherWindow();

    // StreamWindow interface
    void initialize() override;
    void cleanup() override;
    int getStreamId() const override { return m_streamId; }
    QString getDisplayId() const override { return m_displayId; }
    bool isActive() const override { return m_isStreaming; }

    // Специфичные методы ведущего
    void startStream();
    void stopStream();

signals:
    void streamStopped(int streamId);

public slots:
    void setViewersStatus(bool hasViewers);
    void setStreamId(int streamId, const QString &displayId);

private slots:
    void onStopRequested();
    void onDeviceSelected(int deviceIndex);
    void onVideoError(const QString &message);

private:
    void setupUI();
    void setupConnections();
    void updateStatusLabel();

    int m_streamId;
    QString m_displayId;
    bool m_isStreaming;
    bool m_hasViewers;

    // Специфичные для ведущего виджеты
    StreamControlPanel *m_controlPanel;
    DeviceSelectorWidget *m_deviceSelector;
    StreamVideoDisplayPanel *m_videoDisplay;
    StreamStatsWidget *m_statsWidget;

    // Видео компоненты
    VideoCapture *m_videoCapture;
    VideoEncoder *m_videoEncoder;

    static const QString STATUS_NO_VIEWERS;
    static const QString STATUS_HAS_VIEWERS;
    static const QString STATUS_STOPPED;
};
