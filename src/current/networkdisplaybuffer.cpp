#include "networkdisplaybuffer.h"
#include <QTimer>
#include <QDebug>

NetworkDisplayBuffer::NetworkDisplayBuffer(int streamId, int width, int height, int fps, QObject *parent)
    : QObject(parent)
    , m_streamId(streamId)
    , m_width(width)
    , m_height(height)
    , m_fps(fps)
    , m_frameBuffer(nullptr)
    , m_videoDecoder(nullptr)
    , m_playbackTimer(nullptr)
    , m_currentPlaybackFrame(0)
    , m_playbackActive(false)
{
    int bufferCapacity = qMax(1, static_cast<int>(m_fps * DEFAULT_BUFFERSECONDS * 2));
    m_frameBuffer = new FrameBuffer(bufferCapacity);
}

NetworkDisplayBuffer::~NetworkDisplayBuffer()
{
    stopPlayback();
    cleanup();
}

void NetworkDisplayBuffer::initialize()
{
    setupDecoder();
    setupPlaybackTimer();
    
    qDebug() << "NetworkDisplayBuffer initialized for stream" << m_streamId
             << "buffer capacity:" << m_frameBuffer->capacity();
}

void NetworkDisplayBuffer::cleanup()
{
    stopPlayback();
    
    if (m_videoDecoder) {
        m_videoDecoder->cleanup();
        delete m_videoDecoder;
        m_videoDecoder = nullptr;
    }
    
    if (m_frameBuffer) {
        m_frameBuffer->clear();
        delete m_frameBuffer;
        m_frameBuffer = nullptr;
    }
    
    if (m_playbackTimer) {
        delete m_playbackTimer;
        m_playbackTimer = nullptr;
    }
}

void NetworkDisplayBuffer::setupDecoder()
{
    m_videoDecoder = new VideoDecoder(m_width, m_height, this);
    connect(m_videoDecoder, &VideoDecoder::frameDecoded,
            this, &NetworkDisplayBuffer::onFrameDecoded);
    connect(m_videoDecoder, &VideoDecoder::errorOccurred,
            this, [this](const QString &msg) {
                qWarning() << "VideoDecoder error for stream" << m_streamId << ":" << msg;
            });
    
    m_videoDecoder->initialize();
}

void NetworkDisplayBuffer::setupPlaybackTimer()
{
    m_playbackTimer = new QTimer(this);
    int intervalMs = 1000 / m_fps;
    m_playbackTimer->setInterval(intervalMs);
    connect(m_playbackTimer, &QTimer::timeout, this, &NetworkDisplayBuffer::playbackNextFrame);
}

void NetworkDisplayBuffer::addFrame(int frameNumber, const QByteArray &frameData)
{
    if (!m_frameBuffer) return;
    
    // Вставляем фрейм в буфер (используем существующую логику вставки по номеру)
    m_frameBuffer->insertFrame(frameNumber, frameData);
    
    // Если воспроизведение еще не началось, но буфер достаточно заполнен - начинаем
    if (!m_playbackActive && m_frameBuffer->size() >= m_fps * DEFAULT_BUFFERSECONDS) {
        startPlayback();
    }
    
    qDebug() << "NetworkDisplayBuffer stream" << m_streamId 
             << "added frame" << frameNumber << "buffer size:" << m_frameBuffer->size();
}

void NetworkDisplayBuffer::startPlayback()
{
    if (m_playbackActive || !m_playbackTimer) return;
    
    // Начинаем с минимального доступного фрейма
    m_currentPlaybackFrame = m_frameBuffer->getMinFrameNumber();
    m_playbackActive = true;
    m_playbackTimer->start();
    
    qDebug() << "NetworkDisplayBuffer stream" << m_streamId 
             << "playback started from frame" << m_currentPlaybackFrame;
}

void NetworkDisplayBuffer::stopPlayback()
{
    if (m_playbackTimer) {
        m_playbackTimer->stop();
    }
    m_playbackActive = false;
}

void NetworkDisplayBuffer::playbackNextFrame()
{
    if (!m_frameBuffer || !m_playbackActive) return;
    
    // Получаем следующий фрейм для воспроизведения
    QByteArray frameData;
    if (m_frameBuffer->getFrame(m_currentPlaybackFrame, frameData)) {
        // Декодируем фрейм
        if (m_videoDecoder) {
            m_videoDecoder->decodeFrame(frameData, m_currentPlaybackFrame);
        }
        
        // Переходим к следующему фрейму
        m_currentPlaybackFrame++;
        
        // Если достигли конца буфера, останавливаем воспроизведение
        if (m_currentPlaybackFrame > m_frameBuffer->getMaxFrameNumber()) {
            stopPlayback();
        }
    } else {
        // Фрейм не найден - возможно, пропущен, переходим к следующему
        m_currentPlaybackFrame++;
        
        // Если пропустили слишком много фреймов, перезапускаем с текущего минимума
        if (m_currentPlaybackFrame < m_frameBuffer->getMinFrameNumber()) {
            m_currentPlaybackFrame = m_frameBuffer->getMinFrameNumber();
        }
    }
}

void NetworkDisplayBuffer::onFrameDecoded(const QImage &image, int frameNumber)
{
    // Эмитируем декодированный кадр для отображения
    emit frameReady(image, m_streamId);
    
    qDebug() << "NetworkDisplayBuffer stream" << m_streamId 
             << "frame" << frameNumber << "decoded and ready for display";
}
