#pragma once

#include "../networkmanager.h"
#include "../networkdisplaybuffer.h"
#include "../videodecoder.h"
#include "streamwindow.h"
#include "streamcontrolpanel.h"
#include "streamvideodisplaypanel.h"
#include "streamidinputwidget.h"
#include "streamstatswidget.h"

class StreamViewerWindow : public StreamWindow
{
    Q_OBJECT

public:
    explicit StreamViewerWindow(QWidget *parent = nullptr);
    ~StreamViewerWindow();

    // StreamWindow interface
    void initialize() override;
    void cleanup() override;
    int getStreamId() const override { return m_streamId; }
    QString getDisplayId() const override { return m_displayId; }
    bool isActive() const override { return m_isJoined; }

    // Специфичные методы зрителя
    void joinStream(const QString &streamId = "");
    void leaveStream();

signals:
    void streamJoined(int streamId, const QString &displayId);
    void streamLeft(int streamId);

public slots:
    void setStreamId(int streamId, const QString &displayId);

private slots:
    void onJoinRequested(const QString &streamId);
    void onLeaveRequested();
    void onFrameReady(const QImage &image, int streamId);
    void onDecoderError(const QString &message);

private:
    void setupUI();
    void setupConnections();
    void updateStatusLabel();

    int m_streamId;
    QString m_displayId;
    bool m_isJoined;

    // Специфичные для зрителя виджеты
    StreamIdInputWidget *m_idInputWidget;
    StreamControlPanel *m_controlPanel;
    StreamVideoDisplayPanel *m_videoDisplay;
    StreamStatsWidget *m_statsWidget;

    // Сетевые компоненты
    NetworkDisplayBuffer *m_displayBuffer;
    VideoDecoder *m_videoDecoder;

    static const QString STATUS_DISCONNECTED;
    static const QString STATUS_CONNECTING;
    static const QString STATUS_CONNECTED;
    static const QString STATUS_ENDED;
};
