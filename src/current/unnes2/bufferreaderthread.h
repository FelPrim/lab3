#ifndef BUFFERREADERTHREAD_H
#define BUFFERREADERTHREAD_H

#include <QThread>
#include <QByteArray>
#include <QElapsedTimer>
#include "packetbuffer.h"
#include "video_defaults.h"
#include <QDebug>

class BufferReaderThread : public QThread
{
    Q_OBJECT
public:
    // Конструктор с float delaySeconds
    BufferReaderThread(PacketBuffer *buffer, 
                      int fps = DEFAULT_FPS, 
                      float delaySeconds = DEFAULT_BUFFERSECONDS,  // float вместо int
                      QObject *parent = nullptr)
        : QThread(parent), m_buffer(buffer), m_fps(fps) 
    {
        // Вычисляем задержку в кадрах
        m_delayFrames = qMax(1, static_cast<int>(delaySeconds * fps));
        m_intervalMs = (fps > 0) ? (1000 / fps) : (1000 / DEFAULT_FPS);
        
        qDebug() << "BufferReaderThread: delaySeconds =" << delaySeconds 
                 << "-> delayFrames =" << m_delayFrames;
    }

    void stop() { m_running = false; }
    void setDelaySeconds(float delaySeconds) { 
        m_delayFrames = qMax(1, static_cast<int>(delaySeconds * m_fps)); 
    }
    void setFps(int fps) { 
        m_fps = fps; 
        m_intervalMs = (fps > 0) ? (1000 / fps) : (1000 / DEFAULT_FPS);
    }

signals:
    void packetReady(const QByteArray &packet, int frameNumber);
    void bufferUnderrun();
    void currentDelay(int frames);
    void frameDropped();

protected:
    void run() override {
        if (!m_buffer) {
            qWarning() << "BufferReaderThread: no buffer provided";
            return;
        }

        QByteArray packet;
        QElapsedTimer timer;
        
        m_running = true;
        int decoderFrameCount = 0;
        int consecutiveDrops = 0;

        qDebug() << "BufferReaderThread started with delay:" << m_delayFrames 
                 << "frames, FPS:" << m_fps << ", interval:" << m_intervalMs << "ms";

        // Ждем пока буфер наполнится до задержки
        while (m_running && m_buffer->size() < m_delayFrames) {
            QThread::msleep(10);
        }
        qDebug() << "BufferReaderThread: buffer ready, size:" << m_buffer->size();

        while (m_running) {
            timer.restart();
            
            // ПРАВИЛЬНЫЙ РАСЧЕТ: текущий максимальный фрейм минус задержка
            int targetFrame = m_buffer->getMaxFrameNumber() - m_delayFrames;
            if (targetFrame < m_buffer->getMinFrameNumber()) {
                targetFrame = m_buffer->getMinFrameNumber();
            }
            
            if (m_buffer->getFrame(targetFrame, packet)) {
                emit packetReady(packet, targetFrame);
                
                // Отслеживаем реальную задержку
                int currentMaxFrame = m_buffer->getMaxFrameNumber();
                int actualDelay = currentMaxFrame - targetFrame;
                
                if (actualDelay != m_lastReportedDelay) {
                    qDebug() << "Current delay:" << actualDelay << "frames (" 
                             << (actualDelay * 1000 / m_fps) << "ms)";
                    emit currentDelay(actualDelay);
                    m_lastReportedDelay = actualDelay;
                }
            } else {
                // Если не нашли нужный фрейм, используем самый новый
                if (m_buffer->getLatestPacket(packet)) {
                    emit packetReady(packet, m_buffer->getMaxFrameNumber());
                    emit bufferUnderrun();
                }
            }
            
            // Точный интервал
            int elapsed = timer.elapsed();
            int remaining = m_intervalMs - elapsed;
            
            if (remaining > 0) {
                QThread::msleep(remaining);
            }
        }

        qDebug() << "BufferReaderThread: finished";
    }

private:
    PacketBuffer *m_buffer = nullptr;
    int m_fps;
    int m_delayFrames;  // Всегда целое число кадров
    int m_intervalMs;
    bool m_running = false;
    int m_lastReportedDelay = -1;
};

#endif // BUFFERREADERTHREAD_H