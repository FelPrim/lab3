#pragma once

#include "../logic/videocapture.h"
#include "../logic/videoencoder.h"
#include "../network/udp/networkmanager.h" 
#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include "../logic/videodisplay.h"
#include "streamcontrolpanel.h"


class StreamerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit StreamerWidget(int deviceIndex, QWidget *parent = nullptr);
    ~StreamerWidget();

    // Управление виджетом
    void initialize();
    void cleanup();
    
    // StreamWindow-like interface
    uint32_t getStreamId() const { return m_streamId; }
    QString getDisplayId() const { return m_displayId; }
    bool isActive() const { return m_isStreaming; }
    int getDeviceIndex() const { return m_deviceIndex; }

    void setStreamId(uint32_t streamId, const QString &displayId);
    void initializeWithRealId(uint32_t streamId, const QString &displayId);
    
    // Специфичные методы ведущего
    void startStream();
    void stopStream();
    void setStreamingEnabled(bool enabled);
    void setViewersStatus(bool hasViewers);

    void setControlPanel(StreamControlPanel* panel); 
    void showError(const QString &message);

signals:
    void streamStopped(uint32_t streamId);
    void streamingStateChanged(uint32_t streamId, bool enabled);
    void encodedPacketReady(uint32_t streamId, int frameNumber, const QByteArray &packet);
    void disconnectRequested(int deviceIndex);

public slots:
    void onVideoError(const QString &message);
    void onRawFrameReady(const QImage &image);
    void onFrameForEncoding(const cv::Mat &frame);
    void onFrameEncoded(int streamId, int frameNumber, const QByteArray &packet);

private slots:
    void onStartStreamRequested();
    void onStopStreamRequested();
    void onDisconnectRequested();

private:
    void setupUI();
    void setupConnections();
    void updateStatus();
    void initializeVideoCapture();
    void cleanupVideoCapture();
    void initializeVideoEncoder();
    void cleanupVideoEncoder();
    cv::Mat qImageToCvMat(const QImage &image);

    // Состояние
    uint32_t m_streamId;
    QString m_displayId;
    int m_deviceIndex;
    bool m_isStreaming;
    bool m_hasViewers;
    bool m_streamingEnabled;

    // Компоненты
    VideoDisplay *m_videoDisplay;
    StreamControlPanel *m_controlPanel;
    QVBoxLayout *m_mainLayout;
    
    // Видео компоненты (критически важны!)
    VideoCapture *m_videoCapture;
    VideoEncoder *m_videoEncoder;

    // Константы
    static const QString STATUS_NO_VIEWERS;
    static const QString STATUS_HAS_VIEWERS;
    static const QString STATUS_STOPPED;
    static const QString PLACEHOLDER_TEXT;
public:
    void forceDisconnect();
private:
    bool m_disconnecting = false;
};