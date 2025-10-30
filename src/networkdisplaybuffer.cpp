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
    int delayFrames = static_cast<int>(m_fps * DEFAULT_BUFFERSECONDS);
    
    // Минимальная задержка для стабильного старта
    if (m_totalFramesProcessed < 100) {
        delayFrames = qMin(delayFrames, 10); // Меньшая задержка в начале
    }
    
    return qMax(1, delayFrames);
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

int NetworkDisplayBuffer::findBestFrameToPlay()
{
    if (m_frameMap.isEmpty()) {
        return -1;
    }
    
    QList<int> frameNumbers = m_frameMap.keys();
    std::sort(frameNumbers.begin(), frameNumbers.end());
    
    // Минимальный размер буфера для начала воспроизведения
    const int MIN_BUFFER_SIZE = 3;
    if (frameNumbers.size() < MIN_BUFFER_SIZE) {
        qDebug() << "⏳ Buffer too small:" << frameNumbers.size() << "frames, waiting...";
        return -1;
    }
    
    int bestFrame = -1;
    int maxContinuousSequence = 0;
    int currentSequence = 0;
    
    // Ищем самый длинный непрерывный сегмент
    for (int i = 0; i < frameNumbers.size() - 1; ++i) {
        if (frameNumbers[i + 1] == frameNumbers[i] + 1) {
            currentSequence++;
            // Предпоследний фрейм в последовательности - кандидат на воспроизведение
            if (currentSequence >= 1) { // Минимум 2 последовательных фрейма
                bestFrame = frameNumbers[i];
            }
        } else {
            currentSequence = 0;
        }
        
        if (currentSequence > maxContinuousSequence) {
            maxContinuousSequence = currentSequence;
        }
    }
    
    if (bestFrame != -1) {
        qDebug() << "🎯 Playing frame" << bestFrame 
                 << "(continuous sequence:" << (maxContinuousSequence + 1) << "frames)";
        return bestFrame;
    }
    
    // Fallback: 6-й самый новый фрейм
    int targetPositionFromEnd = 6;
    if (frameNumbers.size() > targetPositionFromEnd) {
        int fallbackFrame = frameNumbers[frameNumbers.size() - targetPositionFromEnd];
        qDebug() << "🔄 No good continuous frames, using 6th newest:" << fallbackFrame;
        return fallbackFrame;
    }
    
    return frameNumbers.first();
}

void NetworkDisplayBuffer::processNextFrameImmediately()
{
    if (!m_playbackActive || m_processingFrame || !m_videoDecoder) {
        return;
    }
    
    m_processingFrame = true;
    
    // Находим 6-й самый новый фрейм
    int targetFrame = findBestFrameToPlay();
    
    if (targetFrame != -1 && targetFrame != m_currentPlaybackFrame) {
        QByteArray frameData = m_frameMap[targetFrame];
        
        if (!frameData.isEmpty()) {
            m_currentPlaybackFrame = targetFrame;
            m_videoDecoder->decodeFrame(frameData, targetFrame);
            m_totalFramesProcessed++;
            
        }
    }
    
    m_processingFrame = false;
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