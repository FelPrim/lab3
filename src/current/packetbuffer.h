#ifndef PACKETBUFFER_H
#define PACKETBUFFER_H

#include <QVector>
#include <QMutex>
#include <QByteArray>
#include "video_defaults.h"

class PacketBuffer
{
public:
    explicit PacketBuffer(int capacity = DEFAULT_FPS * DEFAULT_BUFFERSECONDS * 2);
    
    // Основной метод - вставка по любому индексу
    void insertFrame(int frameNumber, const QByteArray &packet);
    
    // Получение фрейма по номеру
    bool getFrame(int frameNumber, QByteArray &out);
    bool getLatestPacket(QByteArray &out);
    
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