#pragma once

#include "streamwindow.h"
#include "streamcontrolpanel.h"
#include "streamvideodisplaypanel.h"
#include "streamstatswidget.h"
#include "networkdisplaybuffer.h"

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
    bool isActive() const override { return m_isActive; }

    // Специфичные методы зрителя
    void setStreamId(int streamId, const QString &displayId);
    void setActive(bool active);

public slots:
    void onFrameAssembled(int streamId, int frameNumber, const QByteArray &frameData);
    void onLeaveRequested();

signals:
    void streamLeft(int streamId);

private:
    void setupUI();
    void setupConnections();
    void updateStatusLabel(); // ДОБАВЛЕНО объявление

    int m_streamId;
    QString m_displayId;
    bool m_isActive;

    // Специфичные для зрителя виджеты
    StreamControlPanel *m_controlPanel;
    StreamVideoDisplayPanel *m_videoDisplay;
    StreamStatsWidget *m_statsWidget;
    NetworkDisplayBuffer *m_displayBuffer;
};
