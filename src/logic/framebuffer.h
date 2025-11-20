#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <QVector>
#include <QMutex>
#include <QByteArray>
#include "video_defaults.h"

class FrameBuffer
{
public:
    explicit FrameBuffer(int capacity = DEFAULT_BUFFERSZ);
    
    // Основной метод - вставка фрейма по номеру
    void insertFrame(int frameNumber, const QByteArray &frameData);
    
    // Получение фрейма по номеру
    bool getFrame(int frameNumber, QByteArray &out) const;
    bool getLatestFrame(QByteArray &out);
    
    // Управление буфером
    void clear();
    int size() const;
    int capacity() const;
    void setCapacity(int capacity);
    
    int getMinFrameNumber() const;
    int getMaxFrameNumber() const;
    bool hasFrame(int frameNumber) const;

private:
    mutable QMutex m_mutex;
    QVector<QByteArray> m_buffer;
    QVector<int> m_frameNumbers;
    int m_capacity;
    int m_minFrame = 0;
    int m_maxFrame = -1;
    
    int getBufferIndex(int frameNumber) const;
};

#endif