#pragma once

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <QMap>
#include <atomic>
#include "../../logic/videodecoder.h"
#include "../../video_defaults.h"

class NetworkDisplayBuffer : public QObject
{
    Q_OBJECT

public:
    explicit NetworkDisplayBuffer(int streamId, int width, int height, int fps, QObject *parent = nullptr);
    ~NetworkDisplayBuffer();

    void initialize();
    void cleanup();
    void forceResync(); 

    // Методы для статистики
    double getCurrentFps() const { return m_currentFps; }
    int getBufferSize() const { return m_frameMap.size(); }
    int getTotalFramesProcessed() const { return m_totalFramesProcessed; }
    int getDroppedFrames() const { return m_droppedFrames; }

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
    std::atomic<bool> m_processingFrame{false};
    
    // Статистика
    QElapsedTimer m_latencyTimer;
    QElapsedTimer m_fpsTimer;
    int m_totalFramesProcessed;
    int m_droppedFrames;
    double m_currentFps;
    int m_frameCountForFps;
};
