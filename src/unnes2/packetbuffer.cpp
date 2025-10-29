#include "packetbuffer.h"

PacketBuffer::PacketBuffer(int capacity)
    : m_capacity(capacity), m_buffer(capacity), m_frameNumbers(capacity, -1)
{
}

void PacketBuffer::insertFrame(int frameNumber, const QByteArray &packet)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_capacity == 0) return;

    // Если это первый фрейм
    if (m_maxFrame == -1) {
        m_minFrame = m_maxFrame = frameNumber;
    } else {
        // Обновляем максимальный фрейм
        if (frameNumber > m_maxFrame) {
            m_maxFrame = frameNumber;
            // Обновляем минимальный фрейм чтобы сохранить capacity
            m_minFrame = m_maxFrame - m_capacity + 1;
            if (m_minFrame < 0) m_minFrame = 0;
        }
        // Игнорируем слишком старые фреймы
        else if (frameNumber < m_minFrame) {
            return;
        }
    }

    // Вставляем в кольцевой буфер
    int index = frameNumber % m_capacity;
    m_buffer[index] = packet;
    m_frameNumbers[index] = frameNumber;
}

bool PacketBuffer::getFrame(int frameNumber, QByteArray &out) const
{
    QMutexLocker locker(&m_mutex);
    
    if (frameNumber < m_minFrame || frameNumber > m_maxFrame) {
        return false;
    }

    int index = frameNumber % m_capacity;
    if (m_frameNumbers[index] == frameNumber) {
        out = m_buffer[index];
        return true;
    }
    
    return false;
}

bool PacketBuffer::getLatestPacket(QByteArray &out)
{
    QMutexLocker locker(&m_mutex);
    return getFrame(m_maxFrame, out);
}

void PacketBuffer::clear()
{
    QMutexLocker locker(&m_mutex);
    m_minFrame = 0;
    m_maxFrame = -1;
    for (int i = 0; i < m_capacity; ++i) {
        m_frameNumbers[i] = -1;
    }
}

int PacketBuffer::size() const
{
    QMutexLocker locker(&m_mutex);
    if (m_maxFrame < m_minFrame) return 0;
    return m_maxFrame - m_minFrame + 1;
}

int PacketBuffer::capacity() const { return m_capacity; }

void PacketBuffer::setCapacity(int capacity)
{
    QMutexLocker locker(&m_mutex);
    if (capacity == m_capacity) return;
    
    // Сохраняем данные и пересоздаем буфер
    QVector<QByteArray> oldBuffer = m_buffer;
    QVector<int> oldFrameNumbers = m_frameNumbers;
    int oldCapacity = m_capacity;
    
    m_capacity = capacity;
    m_buffer.resize(m_capacity);
    m_frameNumbers.resize(m_capacity);
    clear();
    
    // Переinsert старые фреймы
    for (int i = 0; i < oldCapacity; ++i) {
        if (oldFrameNumbers[i] != -1) {
            insertFrame(oldFrameNumbers[i], oldBuffer[i]);
        }
    }
}

int PacketBuffer::getMinFrameNumber() const { return m_minFrame; }
int PacketBuffer::getMaxFrameNumber() const { return m_maxFrame; }

bool PacketBuffer::hasFrame(int frameNumber) const
{
    QMutexLocker locker(&m_mutex);
    if (frameNumber < m_minFrame || frameNumber > m_maxFrame) return false;
    int index = frameNumber % m_capacity;
    return m_frameNumbers[index] == frameNumber;
}

int PacketBuffer::getBufferIndex(int frameNumber) const
{
    return frameNumber % m_capacity;
}

void PacketBuffer::cloneFrom(const PacketBuffer* source, int maxFrames)
{
    QMutexLocker locker(&m_mutex);
    if (!source) return;
    
    // Блокируем исходный буфер для чтения
    QMutexLocker sourceLocker(&source->m_mutex);
    
    clear();
    
    int sourceMin = source->getMinFrameNumber();
    int sourceMax = source->getMaxFrameNumber();
    
    if (sourceMax < sourceMin) return; // Буфер пуст
    
    // Определяем диапазон фреймов для клонирования
    int startFrame = sourceMin;
    int endFrame = sourceMax;
    if (maxFrames > 0 && (endFrame - startFrame + 1) > maxFrames) {
        startFrame = endFrame - maxFrames + 1;
    }
    
    // Копируем фреймы
    for (int frameNum = startFrame; frameNum <= endFrame; ++frameNum) {
        QByteArray packet;
        if (source->getFrame(frameNum, packet)) {
            insertFrame(frameNum, packet);
        }
    }
}