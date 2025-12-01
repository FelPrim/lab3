#pragma once
#define TEST_DECODER
#ifdef TEST_DECODER
#include "../logic/videodecoder.h"
#include "../logic/framebuffer.h"
#endif
#include "../logic/videocapture.h"
#include "../logic/videoencoder.h"
#include "../network/udp/networkmanager.h" 
#include <QWidget>  // ДОБАВЬТЕ этот include!
#include <QVBoxLayout>
#include <QLabel>
#include "../logic/videodisplay.h"
#include "streamcontrolpanel.h"
class StreamManager; 

class StreamerWidget : public QWidget  // ИЗМЕНИТЕ: QWidget вместо QObject
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
    void cleanupTestObjects();
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
    
    // Видео компоненты
    VideoCapture *m_videoCapture;
    VideoEncoder *m_videoEncoder;

    // Константы
    static const QString STATUS_NO_VIEWERS;
    static const QString STATUS_HAS_VIEWERS;
    static const QString STATUS_STOPPED;
    static const QString PLACEHOLDER_TEXT;
    
public:
    void setStreamManager(StreamManager* streamManager);
    void forceDisconnect();
    
private:
    bool m_disconnecting = false;
    
    enum StreamState {
        State_NoStream,      // Только превью, нет трансляции
        State_StreamCreated, // Трансляция создана, кодировщик работает, но пакеты не отправляются
        State_StreamActive,  // Трансляция активна, пакеты отправляются (есть зрители)
        State_StreamError    // Ошибка трансляции
    };

    StreamState m_streamState;
    bool m_encoderInitialized;
    bool m_sendingPackets;

public slots:
    void onServerStreamCreated(uint32_t streamId);
    void onServerStreamStart(uint32_t streamId);
    void onServerStreamEnd(uint32_t streamId);
    void onServerStreamDeleted(uint32_t streamId);
    
private:
    void setStreamState(StreamState newState);
    StreamManager* m_streamManager; 

public:
    void onNetworkError(const QString& error);

#ifdef TEST_DECODER
    VideoDecoder* m_testDecoder = nullptr;
    FrameBuffer* m_frameBuffer = nullptr;
#endif

};