#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include "../logic/videodisplay.h"
#include "../logic/bufferedvideodecoder.h"
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

    void setNetworkManager(NetworkManager* networkManager);
    void showError(const QString &message);

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
    
    // Простая панель управления для ViewerWidget
    QWidget *m_controlPanel;
    QHBoxLayout *m_controlLayout;
    QLabel *m_streamIdLabel;
    QLabel *m_statusLabel;
    QPushButton *m_leaveButton;
    
    QVBoxLayout *m_mainLayout;
    BufferedVideoDecoder *m_bufferedDecoder;
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