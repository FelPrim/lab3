// bufferedvideodecoder.h
#pragma once

#include <QObject>
#include "framebuffer.h"
#include "videodecoder.h"
#include "../video_defaults.h"

class BufferedVideoDecoder : public QObject
{
    Q_OBJECT

public:
    explicit BufferedVideoDecoder(int width, int height, int fps, 
                                 int bufferDelayFrames = -1, // -1 = автоматический расчет
                                 QObject *parent = nullptr);
    ~BufferedVideoDecoder();

    // Основные методы
    void initialize();
    void cleanup();
    void clear();
    
    // Установка задержки (в кадрах)
    void setBufferDelay(int delayFrames);
    
    // Единственный публичный метод для добавления кадров
    void addFrame(int streamId, int frameNumber, const QByteArray &frameData); // Добавить frameNumber

signals:
    void frameReady(const QImage &image, int frameNumber);
    void errorOccurred(const QString &message);

private slots:
    void onFrameDecoded(const QImage &image, int frameNumber);

private:
    void processNextFrame();

private:
    FrameBuffer m_buffer;
    VideoDecoder m_decoder;
    int m_targetFps = 0;
    int m_bufferDelayFrames = 0;  // Задержка в кадрах
    int m_lastDecodedFrame = -1;
    bool m_decoderBusy = false;
};