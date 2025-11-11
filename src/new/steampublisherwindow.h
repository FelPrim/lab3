#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include "streamcontrolpanel.h"
#include "streamvideodisplaypanel.h"
#include "deviceselectorwidget.h"
#include "streamstatswidget.h"

class NetworkManager;

class StreamPublisherWindow : public QWidget
{
    Q_OBJECT

public:
    explicit StreamPublisherWindow(int streamId, QWidget *parent = nullptr);
    ~StreamPublisherWindow();

    void initialize(NetworkManager *networkManager = nullptr);
    void startStream();
    void stopStream();

    int getStreamId() const { return m_streamId; }
    bool isStreaming() const { return m_isStreaming; }
    bool hasViewers() const { return m_hasViewers; }
    QString getDisplayId() const { return m_displayId; }

signals:
    void streamStopped(int streamId);
    void errorOccurred(const QString &message);

public slots:
    void setViewersStatus(bool hasViewers);
    void setStreamId(int streamId, const QString &displayId);

private slots:
    void onStopRequested();
    void onDeviceSelected(int deviceIndex);

private:
    void setupUI();
    void setupConnections();
    void updateStatusLabel();

    int m_streamId;
    QString m_displayId;
    bool m_isStreaming;
    bool m_hasViewers;

    QVBoxLayout *m_mainLayout;
    QLabel *m_statusLabel;
    QLabel *m_streamIdLabel;
    StreamControlPanel *m_controlPanel;
    DeviceSelectorWidget *m_deviceSelector;
    StreamVideoDisplayPanel *m_videoDisplay;
    StreamStatsWidget *m_statsWidget;

    static const QString STATUS_NO_VIEWERS;
    static const QString STATUS_HAS_VIEWERS;
    static const QString STATUS_STOPPED;
};
