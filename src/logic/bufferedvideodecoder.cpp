#include "bufferedvideodecoder.h"
#include <QDebug>
#include <algorithm>

BufferedVideoDecoder::BufferedVideoDecoder(int width, int height, int targetFps, 
                                           int bufferDelayMs, QObject *parent)
    : QObject(parent)
    , m_width(width)
    , m_height(height)
    , m_targetFps(targetFps)
    , m_bufferDelayMs(bufferDelayMs)
    , m_videoDecoder(nullptr)
    , m_playbackTimer(nullptr)
    , m_bufferCapacity(calculateBufferCapacity())
    , m_currentPlaybackFrame(0)
    , m_playbackActive(false)
    , m_processingFrame(false)
    , m_totalFramesProcessed(0)
    , m_droppedFrames(0)
    , m_currentFps(0)
    , m_frameCountForFps(0)
    , m_lastUpdateTime(0)
{
    qDebug() << "🎬 BufferedVideoDecoder created:"
             << width << "x" << height
             << "FPS:" << targetFps
             << "Buffer delay:" << bufferDelayMs << "ms"
             << "Capacity:" << m_bufferCapacity << "frames";
}

BufferedVideoDecoder::~BufferedVideoDecoder()
{
    cleanup();
}

void BufferedVideoDecoder::initialize()
{
    setupDecoder();
    
    // Создаем таймер воспроизведения с интервалом, соответствующим target FPS
    m_playbackTimer = new QTimer(this);
    int intervalMs = 1000 / m_targetFps;
    m_playbackTimer->setInterval(intervalMs);
    connect(m_playbackTimer, &QTimer::timeout, this, &BufferedVideoDecoder::processNextFrame);
    
    // Таймер для статистики
    m_statisticsTimer.start();
    m_lastUpdateTime = m_statisticsTimer.elapsed();
    
    qDebug() << "✅ BufferedVideoDecoder initialized. Playback interval:" << intervalMs << "ms";
}

void BufferedVideoDecoder::cleanup()
{
    stopPlayback();
    
    if (m_playbackTimer) {
        delete m_playbackTimer;
        m_playbackTimer = nullptr;
    }
    
    if (m_videoDecoder) {
        m_videoDecoder->cleanup();
        delete m_videoDecoder;
        m_videoDecoder = nullptr;
    }
    
    m_frameMap.clear();
    m_playbackActive = false;
    m_processingFrame.store(false);
    
    qDebug() << "🧹 BufferedVideoDecoder cleaned up";
}

void BufferedVideoDecoder::setupDecoder()
{
    m_videoDecoder = new VideoDecoder(m_width, m_height, this);
    connect(m_videoDecoder, &VideoDecoder::frameDecoded,
            this, &BufferedVideoDecoder::onDecoderFrameDecoded);
    connect(m_videoDecoder, &VideoDecoder::errorOccurred,
            this, [this](const QString &error) {
                qWarning() << "Decoder error:" << error;
                stopPlayback();
            });
    
    m_videoDecoder->initialize();
}

int BufferedVideoDecoder::calculateBufferCapacity() const
{
    // Рассчитываем емкость буфера на основе задержки и FPS
    int framesForDelay = (m_bufferDelayMs * m_targetFps) / 1000;
    
    // Минимальная емкость - 5 кадров, максимальная - 2 секунды
    int minFrames = 5;
    int maxFrames = m_targetFps * 2;
    
    int capacity = qBound(minFrames, framesForDelay + 3, maxFrames);
    
    return capacity;
}

void BufferedVideoDecoder::addEncodedFrame(int frameNumber, const QByteArray &frameData)
{
    if (frameData.isEmpty()) {
        qDebug() << "⚠️ Empty frame data for frame" << frameNumber;
        return;
    }
    
    // Проверяем, не является ли кадр слишком старым
    if (!m_frameMap.isEmpty() && frameNumber < m_frameMap.firstKey()) {
        m_droppedFrames++;
        return;
    }
    
    // Сохраняем кадр в буфер
    m_frameMap[frameNumber] = frameData;
    
    // Ограничиваем размер буфера
    while (m_frameMap.size() > m_bufferCapacity) {
        int oldestFrame = m_frameMap.firstKey();
        m_frameMap.erase(m_frameMap.begin());
        m_droppedFrames++;
    }
    
    // Если буфер достаточно заполнен, запускаем воспроизведение
    if (!m_playbackActive && m_frameMap.size() >= 3) {
        startPlayback();
    }
    
    // Обновляем статистику
    updateStatistics();
}

void BufferedVideoDecoder::startPlayback()
{
    if (m_playbackActive) {
        return;
    }
    
    if (!m_playbackTimer) {
        qWarning() << "Cannot start playback - timer not initialized";
        return;
    }
    
    // Находим начальный кадр для воспроизведения
    m_currentPlaybackFrame = findBestFrameToPlay();
    
    m_playbackActive = true;
    m_playbackTimer->start();
    
    emit playbackStarted();
    qDebug() << "▶️ Playback started from frame" << m_currentPlaybackFrame;
}

void BufferedVideoDecoder::stopPlayback()
{
    if (!m_playbackActive) {
        return;
    }
    
    m_playbackActive = false;
    
    if (m_playbackTimer) {
        m_playbackTimer->stop();
    }
    
    emit playbackStopped();
    qDebug() << "⏹️ Playback stopped";
}

void BufferedVideoDecoder::pausePlayback()
{
    if (!m_playbackActive) {
        return;
    }
    
    m_playbackActive = false;
    
    if (m_playbackTimer) {
        m_playbackTimer->stop();
    }
    
    qDebug() << "⏸️ Playback paused";
}

void BufferedVideoDecoder::onDecoderFrameDecoded(const QImage &image, int frameNumber)
{
    // Обновляем статистику FPS
    m_frameCountForFps++;
    m_totalFramesProcessed++;
    
    // Излучаем сигнал с декодированным кадром
    emit frameDecoded(image, frameNumber);
    
    // Обновляем статистику каждую секунду
    updateStatistics();
}

void BufferedVideoDecoder::processNextFrame()
{
    // Проверяем, не обрабатывается ли уже кадр
    bool expected = false;
    if (!m_processingFrame.compare_exchange_strong(expected, true)) {
        return;
    }
    
    if (!m_playbackActive || !m_videoDecoder) {
        m_processingFrame.store(false);
        return;
    }
    
    // Находим лучший кадр для воспроизведения
    int targetFrame = findBestFrameToPlay();
    
    if (targetFrame != -1) {
        // Пытаемся получить кадр из буфера
        auto it = m_frameMap.find(targetFrame);
        if (it != m_frameMap.end()) {
            QByteArray frameData = it.value();
            if (!frameData.isEmpty()) {
                m_currentPlaybackFrame = targetFrame;
                m_videoDecoder->decodeFrame(frameData, targetFrame);
            }
        } else {
            // Кадр не найден - увеличиваем счетчик пропущенных
            m_droppedFrames++;
            m_currentPlaybackFrame = targetFrame + 1;
        }
    }
    
    m_processingFrame.store(false);
}

int BufferedVideoDecoder::findBestFrameToPlay()
{
    if (m_frameMap.isEmpty()) {
        return -1;
    }
    
    // Сортируем номера кадров
    QList<int> frameNumbers = m_frameMap.keys();
    std::sort(frameNumbers.begin(), frameNumbers.end());
    
    // Минимальный размер буфера для начала воспроизведения
    const int MIN_BUFFER_SIZE = 3;
    if (frameNumbers.size() < MIN_BUFFER_SIZE) {
        return -1;
    }
    
    // Рассчитываем, какой кадр должен воспроизводиться сейчас
    // на основе задержки (в кадрах)
    int framesDelay = (m_bufferDelayMs * m_targetFps) / 1000;
    framesDelay = qMax(1, framesDelay);
    
    int newestFrame = frameNumbers.last();
    int targetFrame = newestFrame - framesDelay;
    
    // Если целевой кадр есть в буфере - используем его
    if (m_frameMap.contains(targetFrame)) {
        return targetFrame;
    }
    
    // Ищем ближайший доступный кадр к целевому
    int closestFrame = -1;
    int minDistance = std::numeric_limits<int>::max();
    
    for (int frame : frameNumbers) {
        int distance = std::abs(frame - targetFrame);
        if (distance < minDistance) {
            minDistance = distance;
            closestFrame = frame;
        }
    }
    
    return closestFrame;
}

void BufferedVideoDecoder::updateStatistics()
{
    qint64 currentTime = m_statisticsTimer.elapsed();
    
    // Обновляем FPS каждую секунду
    if (currentTime - m_lastUpdateTime >= 1000) {
        m_currentFps = (m_frameCountForFps * 1000.0) / (currentTime - m_lastUpdateTime);
        m_frameCountForFps = 0;
        m_lastUpdateTime = currentTime;
        
        // Излучаем статистику
        int bufferFillPercent = (m_frameMap.size() * 100) / m_bufferCapacity;
        emit statisticsUpdated(m_currentFps, m_droppedFrames, bufferFillPercent);
        
        // Периодический вывод в консоль (опционально)
        if (m_totalFramesProcessed % 60 == 0) {
            qDebug() << "📊 BufferedVideoDecoder stats:"
                     << "FPS:" << QString::number(m_currentFps, 'f', 1)
                     << "Buffer:" << m_frameMap.size() << "/" << m_bufferCapacity
                     << "Dropped:" << m_droppedFrames
                     << "Processed:" << m_totalFramesProcessed;
        }
    }
    
    // Излучаем состояние буфера
    emit bufferStateChanged(m_frameMap.size(), m_bufferCapacity);
}

void BufferedVideoDecoder::cleanupOldFrames()
{
    if (m_frameMap.size() <= m_bufferCapacity) {
        return;
    }
    
    // Удаляем самые старые кадры сверх емкости
    int framesToRemove = m_frameMap.size() - m_bufferCapacity;
    auto it = m_frameMap.begin();
    
    for (int i = 0; i < framesToRemove && it != m_frameMap.end(); ++i) {
        it = m_frameMap.erase(it);
        m_droppedFrames++;
    }
}
