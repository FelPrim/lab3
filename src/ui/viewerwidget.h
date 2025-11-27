// viewerwidget.h
#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include "../logic/videodisplay.h"
#include "streamcontrolpanel.h"
#include "../network/udp/networkdisplaybuffer.h"
#include "../logic/videodecoder.h"
// УБРАТЬ дублирование: #include "../network/udp/networkdisplaybuffer.h"
#include "../network/udp/networkmanager.h"  // ДОБАВИТЬ

class ViewerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ViewerWidget(uint32_t streamId, const QString &displayId, QWidget *parent = nullptr);
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

private slots:
    void onLeaveButtonClicked();

private:
    void setupUI();
    void setupConnections();
    void updateStatus();

    VideoDisplay *m_videoDisplay;
    StreamControlPanel *m_controlPanel;
    QVBoxLayout *m_mainLayout;
    
    // ОСТАВИТЬ только одно объявление NetworkDisplayBuffer
    NetworkDisplayBuffer *m_displayBuffer;
    
    uint32_t m_streamId;
    QString m_displayId;
    bool m_active;
    
    static const QString PLACEHOLDER_TEXT;
    static const QString STATUS_ACTIVE;
    static const QString STATUS_INACTIVE;

signals:
    void streamLeft(uint32_t streamId);

private:
    VideoDecoder *m_videoDecoder;
    NetworkManager *m_networkManager;
    // УБРАТЬ дублирование: NetworkDisplayBuffer *m_displayBuffer;

public slots:
    void onStreamJoined(uint32_t streamId);
    void onStreamLeft(uint32_t streamId);
    // УБРАТЬ: void onVideoPacketReceived(uint32_t streamId, const QByteArray& packet);

    // ДОБАВИТЬ метод для установки NetworkManager

private:
    NetworkManager* m_networkManager; // ДОБАВИТЬ
};
