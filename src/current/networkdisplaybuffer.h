#pragma once

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <QMap>
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
    void forceResync(); 

public slots:
    void addFrame(int frameNumber, const QByteArray &frameData);

signals:
    void frameReady(const QImage &image, int streamId);
    void errorOccurred(const QString &message);

private slots:
    void onFrameDecoded(const QImage &image, int frameNumber);
    void processNextFrameImmediately();

private:
    void setupDecoder();
    int findBestFrameToPlay();
    void cleanupOldFrames();
    int calculateDelayFrames() const;

    int m_streamId;
    int m_width;
    int m_height;
    int m_fps;
    
    QMap<int, QByteArray> m_frameMap;
    int m_bufferCapacity;
    
    VideoDecoder *m_videoDecoder;
    
    int m_currentPlaybackFrame;
    bool m_playbackActive;
    bool m_processingFrame;
    
    // Статистика
    QElapsedTimer m_latencyTimer;
    int m_totalFramesProcessed;
    int m_droppedFrames;
};