// framebuffer.cpp
#include "framebuffer.h"
#include <QDebug>
#include <algorithm>
#include <QCryptographicHash>

#define TESTING_NETCODE
#undef TESTING_NETCODE
FrameBuffer::FrameBuffer(int capacity)
    : m_capacity(qMax(30, capacity))
{
    m_buffer.resize(m_capacity);
    m_frameNumbers.resize(m_capacity);
    
    for (int i = 0; i < m_capacity; ++i) {
        m_frameNumbers[i] = -1;
    }
    
    m_minFrame = 0;
    m_maxFrame = -1;
    
    qDebug() << "FrameBuffer constructed with capacity:" << m_capacity;
}

int FrameBuffer::filledSlotsCount() const
{
    QMutexLocker locker(&m_mutex);
    
    if (m_maxFrame == -1) return 0;
    
    int count = 0;
    for (int i = 0; i < m_capacity; ++i) {
        if (m_frameNumbers[i] != -1) {
            count++;
        }
    }
    return count;
}

int FrameBuffer::findFrameWithDelay(int delayFrames) const
{
    if (m_maxFrame == -1) return -1;
    
    // Целевой кадр = самый новый - задержка
    int targetFrame = m_maxFrame - delayFrames;
    
    // Если целевой кадр выпал из буфера, берем самый старый
    if (targetFrame < m_minFrame) {
        return m_minFrame;
    }
    
    // Проверяем, есть ли целевой кадр в буфере
    if (hasFrame(targetFrame)) {
        return targetFrame;
    }
    
    // Если целевого кадра нет, ищем ближайший доступный
    // Ищем ближайший кадр к целевому (в любую сторону)
    int closestFrame = -1;
    int minDistance = std::numeric_limits<int>::max();
    
    for (int i = 0; i < m_capacity; ++i) {
        int frameNum = m_frameNumbers[i];
        if (frameNum != -1) {
            int distance = std::abs(frameNum - targetFrame);
            if (distance < minDistance) {
                minDistance = distance;
                closestFrame = frameNum;
            }
        }
    }
    
    return closestFrame;
}

bool FrameBuffer::getFrameWithDelay(int delayFrames, QByteArray &out, int &outFrameNumber) const
{
    QMutexLocker locker(&m_mutex);
    
    if (m_maxFrame == -1 || delayFrames < 0) {
        return false;
    }
    
    // Находим кадр с нужной задержкой
    int targetFrame = findFrameWithDelay(delayFrames);
    
    if (targetFrame == -1) {
        return false;
    }
    
    // Получаем данные кадра
    if (getFrame(targetFrame, out)) {
        outFrameNumber = targetFrame;
        return true;
    }
    
    return false;
}

int FrameBuffer::getBufferIndex(int frameNumber) const
{
    if (m_capacity == 0) return -1;
    int idx = frameNumber % m_capacity;
    if (idx < 0) idx += m_capacity;
    return idx;
}

void FrameBuffer::insertFrame(int frameNumber, const QByteArray &frameData)
{
#ifdef TESTING_NETCODE
    QByteArray sha = QCryptographicHash::hash(frameData, QCryptographicHash::Sha256);
    qDebug() << "FrameBuffer: frame" << frameNumber
           << "size:" << frameData.size() << "sha256:" << sha.toHex().left(64);
#endif
    QMutexLocker locker(&m_mutex);

    if (m_capacity == 0) return;

    // Если это первый фрейм
    if (m_maxFrame == -1) {
        m_minFrame = m_maxFrame = frameNumber;
    } else {
        if (frameNumber > m_maxFrame) {
            int oldMin = m_minFrame;

            m_maxFrame = frameNumber;
            // Рассчитываем кандидата на новую m_minFrame (когда окно "переполнится")
            int candidateMin = m_maxFrame - m_capacity + 1;
            if (candidateMin < 0) candidateMin = 0;

            // Не уменьшаем m_minFrame — оно может только увеличиваться, когда окно действительно "съезжает"
            if (candidateMin > m_minFrame) {
                m_minFrame = candidateMin;
                // Очистим слоты для фреймов, которые выпали из буфера (oldMin .. m_minFrame-1)
                for (int f = oldMin; f < m_minFrame; ++f) {
                    int idx = getBufferIndex(f);
                    if (idx >= 0 && idx < m_capacity) {
                        m_frameNumbers[idx] = -1;
                        m_buffer[idx].clear();
                    }
                }
            }
            // В противном случае m_minFrame остаётся старым (буфер ещё не переполнился)
        }

        // Игнорируем слишком старые фреймы
        else if (frameNumber < m_minFrame) {
            return;
        }
        // иначе: вставляем во внутрь текущего окна (overwriting старые / пустые)
    }

    int index = getBufferIndex(frameNumber);
    if (index < 0) return;

    // Заменяем слот — старый QByteArray будет освобождён при присваивании
    m_buffer[index] = frameData;
    m_frameNumbers[index] = frameNumber;

   // qDebug() << "FrameBuffer: inserted frame" << frameNumber << "at index" << index
   //          << "size:" << frameData.size() << "bytes";
}

bool FrameBuffer::getFrame(int frameNumber, QByteArray &out) const
{
    QMutexLocker locker(&m_mutex);

    if (m_capacity == 0) return false;
    if (m_maxFrame == -1) return false;
    if (frameNumber < m_minFrame || frameNumber > m_maxFrame) {
        return false;
    }
    int index = getBufferIndex(frameNumber);
    if (index < 0) return false;

    if (m_frameNumbers[index] == frameNumber) {
        out = m_buffer[index];
        return true;
    }
    return false;
}

bool FrameBuffer::getLatestFrame(QByteArray &out) const
{
    QMutexLocker locker(&m_mutex);

    if (m_capacity == 0) return false;
    if (m_maxFrame == -1) return false;

    int index = getBufferIndex(m_maxFrame);
    if (index < 0) return false;

    if (m_frameNumbers[index] == m_maxFrame) {
        out = m_buffer[index];
        return true;
    }
    return false;
}

// framebuffer.cpp - метод clear()
void FrameBuffer::clear()
{
    QMutexLocker locker(&m_mutex);
    
    qDebug() << "FrameBuffer::clear() called at address:" << this 
             << "capacity:" << m_capacity;

    m_minFrame = 0;
    m_maxFrame = -1;
    
    if (m_capacity <= 0) {
        qWarning() << "FrameBuffer::clear(): invalid capacity" << m_capacity;
        return;
    }

    // Безопасная очистка с проверкой индексов
    for (int i = 0; i < m_capacity && i < m_buffer.size(); ++i) {
        m_frameNumbers[i] = -1;
        m_buffer[i].clear();
    }
    
    qDebug() << "FrameBuffer::clear(): cleared" << m_capacity << "slots";
}

int FrameBuffer::size() const
{
    QMutexLocker locker(&m_mutex);
    if (m_maxFrame == -1) return 0;
    if (m_maxFrame < m_minFrame) return 0;
    // размер окна (кол-во потенциальных индексов, не фактическое кол-во заполненных)
    return m_maxFrame - m_minFrame + 1;
}

int FrameBuffer::capacity() const { 
    QMutexLocker locker(&m_mutex);
    return m_capacity; 
}

void FrameBuffer::setCapacity(int capacity)
{
    QMutexLocker locker(&m_mutex);

    int newCap = qMax(0, capacity);
    if (newCap == m_capacity) return;

    // Если новая ёмкость == 0 — просто очистим всё и установим пустые вектора
    if (newCap == 0) {
        m_buffer.clear();
        m_frameNumbers.clear();
        m_capacity = 0;
        m_minFrame = 0;
        m_maxFrame = -1;
        return;
    }

    // Новый контейнер
    QVector<QByteArray> newBuffer(newCap);
    QVector<int> newFrameNumbers(newCap, -1);

    // Переносим старые фреймы, если они попадают в новое окно
    if (m_capacity > 0 && m_maxFrame != -1) {
        // Проходим по текущему диапазону и копируем
        for (int f = m_minFrame; f <= m_maxFrame; ++f) {
            int oldIdx = getBufferIndex(f);
            if (oldIdx < 0 || oldIdx >= m_capacity) continue;
            int frameNumAtIdx = m_frameNumbers[oldIdx];
            if (frameNumAtIdx != f) continue; // слот не содержит этот кадр

            // Помещаем в новый буфер, если попадает в новый размер окна
            int newIdx = f % newCap;
            if (newIdx < 0) newIdx += newCap;
            newBuffer[newIdx] = m_buffer[oldIdx];
            newFrameNumbers[newIdx] = f;
        }

        // Обновим границы окна: максимальный номер остаётся тем же, минимальный зависит от ёмкости
        int newMax = m_maxFrame;
        int newMin = newMax - newCap + 1;
        if (newMin < 0) newMin = 0;
        m_minFrame = newMin;
        m_maxFrame = newMax;
    } else {
        // старого содержимого нет
        m_minFrame = 0;
        m_maxFrame = -1;
    }

    // Присваиваем новые контейнеры
    m_capacity = newCap;
    m_buffer.swap(newBuffer);
    m_frameNumbers.swap(newFrameNumbers);
}

int FrameBuffer::getMinFrameNumber() const
{
    QMutexLocker locker(&m_mutex);
    return m_minFrame;
}

int FrameBuffer::getMaxFrameNumber() const
{
    QMutexLocker locker(&m_mutex);
    return m_maxFrame;
}

bool FrameBuffer::hasFrame(int frameNumber) const
{
    QMutexLocker locker(&m_mutex);
    if (m_capacity == 0) return false;
    if (m_maxFrame == -1) return false;
    if (frameNumber < m_minFrame || frameNumber > m_maxFrame) return false;
    int index = getBufferIndex(frameNumber);
    if (index < 0) return false;
    return m_frameNumbers[index] == frameNumber;
}

FrameBuffer::~FrameBuffer()
{
    QMutexLocker locker(&m_mutex);
    qDebug() << "~FrameBuffer() at" << this 
             << "capacity:" << m_capacity;
    
    // Векторы очистятся автоматически при разрушении объекта
}