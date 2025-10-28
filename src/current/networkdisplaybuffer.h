#pragma once

#include <QObject>
#include "framebuffer.h"
#include "videodecoder.h"
#include "video_defaults.h"

class NetworkDisplayBuffer : public QObject
{
    Q_OBJECT

public:
    explicit NetworkDisplayBuffer(int streamId, int width, int height, int fps, QObject *parent = nullptr);
    ~NetworkDisplayBuffer();

    void initialize();
    void cleanup();

public slots:
    void addFrame(int frameNumber, const QByteArray &frameData);
    void startPlayback();
    void stopPlayback();

signals:
    void frameReady(const QImage &image, int streamId);

private slots:
    void onFrameDecoded(const QImage &image, int frameNumber);
    void playbackNextFrame();

private:
    void setupDecoder();
    void setupPlaybackTimer();

    int m_streamId;
    int m_width;
    int m_height;
    int m_fps;
    
    FrameBuffer *m_frameBuffer;
    VideoDecoder *m_videoDecoder;
    QTimer *m_playbackTimer;
    
    int m_currentPlaybackFrame;
    bool m_playbackActive;
};
