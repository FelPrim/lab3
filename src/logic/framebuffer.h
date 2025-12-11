#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <QVector>
#include <QMutex>
#include <QByteArray>
#include "../video_defaults.h"

class FrameBuffer
{
public:
    explicit FrameBuffer(int capacity = DEFAULT_BUFFERSZ);
    ~FrameBuffer();
    
    // Основной метод - вставка фрейма по номеру
    void insertFrame(int frameNumber, const QByteArray &frameData);

    // Получение фреймов
    bool getFrame(int frameNumber, QByteArray &out) const;
    bool getLatestFrame(QByteArray &out) const;
    
    // Новый метод: получение кадра с задержкой
    bool getFrameWithDelay(int delayFrames, QByteArray &out, int &outFrameNumber) const;

    // Управление буфером
    void clear();
    int size() const;
    int capacity() const;
    void setCapacity(int capacity);

    int getMinFrameNumber() const;
    int getMaxFrameNumber() const;
    bool hasFrame(int frameNumber) const;
    
    // Статистика заполнения
    int filledSlotsCount() const;
    void removeFrame(int frameNumber);

private:  

    // Возвращает индекс в m_buffer для заданного номера кадра
    int getBufferIndex(int frameNumber) const;
    
    // Внутренний метод для поиска кадра с определенной задержкой
    int findFrameWithDelay(int delayFrames) const;

private:
    mutable QMutex m_mutex;
    QVector<QByteArray> m_buffer;
    QVector<int> m_frameNumbers;
    int m_capacity;
    int m_minFrame = 0;
    int m_maxFrame = -1;
};

#endif