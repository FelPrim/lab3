#include "networkdisplaybuffer.h"
#include <QDebug>

NetworkDisplayBuffer::NetworkDisplayBuffer(int streamId, int width, int height, int fps, QObject *parent)
    : QObject(parent)
    , m_streamId(streamId)
    , m_width(width)
    , m_height(height)
    , m_fps(fps)
    , m_bufferCapacity(calculateDelayFrames() + 5) // Задержка + небольшой запас
    , m_videoDecoder(nullptr)
    , m_currentPlaybackFrame(0)
    , m_playbackActive(false)
    , m_processingFrame(false)
    , m_totalFramesProcessed(0)
    , m_droppedFrames(0)
{
    qDebug() << "🎯 Delayed playback buffer for stream:" << streamId 
             << "Capacity:" << m_bufferCapacity 
             << "Delay frames:" << calculateDelayFrames();
}

NetworkDisplayBuffer::~NetworkDisplayBuffer()
{
    cleanup();
}

void NetworkDisplayBuffer::initialize()
{
    setupDecoder();
    m_latencyTimer.start();
    
    qDebug() << "✅ NetworkDisplayBuffer initialized for stream" << m_streamId;
}

void NetworkDisplayBuffer::cleanup()
{
    if (m_videoDecoder) {
        m_videoDecoder->cleanup();
        delete m_videoDecoder;
        m_videoDecoder = nullptr;
    }
    
    m_frameMap.clear();
    m_playbackActive = false;
    m_processingFrame = false;
}

void NetworkDisplayBuffer::setupDecoder()
{
    m_videoDecoder = new VideoDecoder(m_width, m_height, this);
    connect(m_videoDecoder, &VideoDecoder::frameDecoded,
            this, &NetworkDisplayBuffer::onFrameDecoded);
    connect(m_videoDecoder, &VideoDecoder::errorOccurred,
            this, &NetworkDisplayBuffer::errorOccurred);
    
    m_videoDecoder->initialize();
}

int NetworkDisplayBuffer::calculateDelayFrames() const
{
    // Вычисляем задержку в кадрах: FPS * секунды задержки
    int delayFrames = static_cast<int>(m_fps * DEFAULT_BUFFERSECONDS);
    return qMax(1, delayFrames); // Минимум 1 кадр задержки
}

void NetworkDisplayBuffer::addFrame(int frameNumber, const QByteArray &frameData)
{
    if (frameData.isEmpty()) {
        return;
    }
    
    m_frameMap[frameNumber] = frameData;
    
    while (m_frameMap.size() > m_bufferCapacity) {
        int oldestFrame = m_frameMap.firstKey();
        m_frameMap.erase(m_frameMap.begin());
    }
    
    if (!m_playbackActive) {
        m_playbackActive = true;
        m_currentPlaybackFrame = m_frameMap.firstKey();
    }
    
    processNextFrameImmediately();
}

void NetworkDisplayBuffer::onFrameDecoded(const QImage &image, int frameNumber)
{
    emit frameReady(image, m_streamId);
    
    QTimer::singleShot(0, this, &NetworkDisplayBuffer::processNextFrameImmediately);
    
    // Периодическая статистика каждые 30 кадров
    if (m_totalFramesProcessed % 30 == 0) {
        int currentMaxFrame = m_frameMap.isEmpty() ? 0 : m_frameMap.lastKey();
        int currentDelay = currentMaxFrame - m_currentPlaybackFrame;
        float delaySeconds = static_cast<float>(currentDelay) / m_fps;
        
        qDebug() << "Stream" << m_streamId 
                 << "- Processed:" << m_totalFramesProcessed 
                 << "- Buffer:" << m_frameMap.size() << "/" << m_bufferCapacity
                 << "- Delay:" << currentDelay << "frames (" << delaySeconds << "s)";
    }
}

void NetworkDisplayBuffer::processNextFrameImmediately()
{
    if (!m_playbackActive || m_processingFrame || !m_videoDecoder) {
        return;
    }
    
    // Защита от рекурсии
    m_processingFrame = true;
    
    // Ищем лучший кадр для воспроизведения с учетом задержки
    int targetFrame = findBestFrameToPlay();
    
    if (targetFrame != -1 && targetFrame != m_currentPlaybackFrame) {
        QByteArray frameData = m_frameMap[targetFrame];
        
        if (!frameData.isEmpty()) {
            qDebug() << "🎬 Decoding delayed frame:" << targetFrame 
                     << "(current max:" << m_frameMap.lastKey() << ")";
            
            m_currentPlaybackFrame = targetFrame;
            m_videoDecoder->decodeFrame(frameData, targetFrame);
            m_totalFramesProcessed++;
            
            // Удаляем обработанный кадр чтобы освободить место для новых
            // Но только если он не нужен для других потоков воспроизведения
            if (targetFrame < m_frameMap.firstKey() + calculateDelayFrames()) {
                m_frameMap.remove(targetFrame);
            }
        }
    }
    
    m_processingFrame = false;
}

int NetworkDisplayBuffer::findBestFrameToPlay()
{
    if (m_frameMap.isEmpty()) {
        return -1;
    }
    
    int maxFrame = m_frameMap.lastKey();
    int delayFrames = calculateDelayFrames();
    int targetFrame = maxFrame - delayFrames;
    
    qDebug() << "🔍 Looking for frame:" << targetFrame 
             << "(max:" << maxFrame << ", delay:" << delayFrames << "frames)";
    
    // Если целевой фрейм существует в буфере, воспроизводим его
    if (m_frameMap.contains(targetFrame)) {
        return targetFrame;
    }
    
    // Если целевой фрейм еще не получен (слишком новый), воспроизводим самый старый доступный
    if (targetFrame > maxFrame) {
        qDebug() << "⏳ Target frame too new, playing oldest:" << m_frameMap.firstKey();
        return m_frameMap.firstKey();
    }
    
    // Если целевой фрейм уже устарел (удален из буфера), воспроизводим самый старый доступный
    if (targetFrame < m_frameMap.firstKey()) {
        qDebug() << "📜 Target frame too old, playing oldest:" << m_frameMap.firstKey();
        return m_frameMap.firstKey();
    }
    
    // Ищем ближайший доступный фрейм к целевому
    int closestFrame = -1;
    int minDistance = std::numeric_limits<int>::max();
    
    for (auto it = m_frameMap.begin(); it != m_frameMap.end(); ++it) {
        int frameNum = it.key();
        int distance = std::abs(frameNum - targetFrame);
        
        if (distance < minDistance) {
            minDistance = distance;
            closestFrame = frameNum;
        }
    }
    
    if (closestFrame != -1) {
        qDebug() << "🎯 Using closest frame:" << closestFrame << "(target:" << targetFrame << ")";
        return closestFrame;
    }
    
    // Фолбэк: воспроизводим самый старый фрейм
    return m_frameMap.firstKey();
}

void NetworkDisplayBuffer::forceResync()
{
    qDebug() << "🔄 Force resync for stream" << m_streamId;
    
    if (!m_frameMap.isEmpty()) {
        int maxFrame = m_frameMap.lastKey();
        int targetFrame = maxFrame - calculateDelayFrames();
        
        // Очищаем буфер, оставляя только фреймы вокруг целевого
        QMap<int, QByteArray> newFrameMap;
        
        for (auto it = m_frameMap.begin(); it != m_frameMap.end(); ++it) {
            int frameNum = it.key();
            // Сохраняем фреймы в окрестности целевого
            if (std::abs(frameNum - targetFrame) <= calculateDelayFrames()) {
                newFrameMap[frameNum] = it.value();
            }
        }
        
        m_frameMap = newFrameMap;
        m_currentPlaybackFrame = targetFrame;
        qDebug() << "🔄 Resynced to frame:" << targetFrame << "Buffer size:" << m_frameMap.size();
    }
    
    m_processingFrame = false;
}

void NetworkDisplayBuffer::cleanupOldFrames()
{
    // Автоматически обрабатывается в addFrame через ограничение размера m_frameMap
}