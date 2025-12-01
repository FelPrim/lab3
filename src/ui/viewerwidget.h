#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include "../logic/videodisplay.h"
#include "streamcontrolpanel.h"
#include "../network/udp/networkdisplaybuffer.h"
#include "../logic/videodecoder.h"
#include "../network/udp/networkmanager.h"

class ViewerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ViewerWidget(uint32_t streamId, const QString &displayId, uint32_t callId = 0, QWidget *parent = nullptr);
    ~ViewerWidget();

    void initialize();
    void cleanup();
    
    uint32_t getStreamId() const { return m_streamId; }
    QString getDisplayId() const { return m_displayId; }
    bool isActive() const { return m_active; }
    
    void setActive(bool active);
    void setStreamId(uint32_t streamId, const QString &displayId);

    void displayFrame(const QImage &frame);
    void clearDisplay();

    void setControlPanel(StreamControlPanel* panel);  
    void setNetworkManager(NetworkManager* networkManager);

public slots:
    void onFrameReady(const QImage &frame, int frameNumber);
    void onLeaveRequested();
    void onFrameAssembled(int streamId, int frameNumber, const QByteArray &frameData);
    void onStreamJoined(uint32_t streamId);
    void onStreamLeft(uint32_t streamId);
    void onNetworkError(const QString& error);

private slots:
    void onLeaveButtonClicked();

private:
    void setupUI();
    void setupConnections();
    void updateStatus();

    VideoDisplay *m_videoDisplay;
    StreamControlPanel *m_controlPanel;
    QVBoxLayout *m_mainLayout;
    NetworkDisplayBuffer *m_displayBuffer;
    VideoDecoder *m_videoDecoder;
    NetworkManager *m_networkManager;
    
    uint32_t m_callId;     
    uint32_t m_streamId;
    QString m_displayId;
    bool m_active;
    
    static const QString PLACEHOLDER_TEXT;
    static const QString STATUS_ACTIVE;
    static const QString STATUS_INACTIVE;

signals:
    void streamLeft(uint32_t streamId);
};