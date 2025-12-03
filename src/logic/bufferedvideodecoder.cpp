// bufferedvideodecoder.cpp
#include "bufferedvideodecoder.h"
#include <QDebug>

BufferedVideoDecoder::BufferedVideoDecoder(int width, int height, int fps, 
                                           int bufferDelayFrames, QObject *parent)
    : QObject(parent)
    , m_buffer(static_cast<int>(DEFAULT_BUFFERSECONDS * fps))
    , m_decoder(width, height, this)
    , m_targetFps(fps)
{
    // Устанавливаем задержку
    if (bufferDelayFrames < 0) {
        // Автоматический расчет: задержка = 1/3 от размера буфера
        m_bufferDelayFrames = m_buffer.capacity() / 3;
        m_bufferDelayFrames = qMax(1, m_bufferDelayFrames);  // Минимум 1 кадр
    } else {
        m_bufferDelayFrames = qMax(0, bufferDelayFrames);
    }
    
    connect(&m_decoder, &VideoDecoder::frameDecoded,
            this, &BufferedVideoDecoder::onFrameDecoded);
    connect(&m_decoder, &VideoDecoder::errorOccurred,
            this, &BufferedVideoDecoder::errorOccurred);
    
    qDebug() << "[BufferedVideoDecoder] Created with buffer delay:" 
             << m_bufferDelayFrames << "frames, capacity:" << m_buffer.capacity();
}

void BufferedVideoDecoder::initialize()
{
    m_decoder.initialize();
    m_lastDecodedFrame = -1;
    m_decoderBusy = false;
}

void BufferedVideoDecoder::cleanup()
{
    m_decoder.cleanup();
    m_buffer.clear();
    m_lastDecodedFrame = -1;
    m_decoderBusy = false;
}

void BufferedVideoDecoder::clear()
{
    m_buffer.clear();
    m_lastDecodedFrame = -1;
    m_decoderBusy = false;
}

void BufferedVideoDecoder::setBufferDelay(int delayFrames)
{
    m_bufferDelayFrames = qMax(0, delayFrames);
    qDebug() << "[BufferedVideoDecoder] Buffer delay set to:" << m_bufferDelayFrames << "frames";
}

void BufferedVideoDecoder::addFrame(int streamId, int frameNumber, const QByteArray &frameData)
{
    if (frameData.isEmpty()) {
        emit errorOccurred(QString("Empty frame data for frame %1").arg(frameNumber));
        return;
    }
    
    // Сохраняем кадр в буфере
    m_buffer.insertFrame(frameNumber, frameData);
    
    // Пытаемся обработать следующий кадр
    if (!m_decoderBusy) {
        processNextFrame();
    }
}

void BufferedVideoDecoder::processNextFrame()
{
    if (m_decoderBusy) {
        return;
    }

    // Получаем диапазон кадров в буфере
    int minFrame = m_buffer.getMinFrameNumber();
    int maxFrame = m_buffer.getMaxFrameNumber();
    
    if (minFrame < 0 || maxFrame < 0) {
        return; // Буфер пуст
    }
    
    // Рассчитываем целевой кадр с учетом задержки
    int targetFrame = maxFrame - m_bufferDelayFrames;
    
    // Если задержка слишком большая, берем самый старый кадр
    if (targetFrame < minFrame) {
        targetFrame = minFrame;
    }
    
    // Проверяем, что это новый кадр
    if (targetFrame <= m_lastDecodedFrame) {
        return;
    }
    
    // Ищем ближайший доступный кадр к целевому (вперед)
    int actualFrame = -1;
    for (int frame = targetFrame; frame <= maxFrame; ++frame) {
        if (m_buffer.hasFrame(frame)) {
            actualFrame = frame;
            break;
        }
    }
    
    // Если не нашли вперед, ищем назад
    if (actualFrame == -1) {
        for (int frame = targetFrame - 1; frame >= minFrame; --frame) {
            if (m_buffer.hasFrame(frame)) {
                actualFrame = frame;
                break;
            }
        }
    }
    
    if (actualFrame == -1) {
        return; // Не нашли подходящий кадр
    }
    
    QByteArray frameData;
    if (m_buffer.getFrame(actualFrame, frameData)) {
        qDebug() << "[BufferedVideoDecoder] Decoding frame" << actualFrame 
                 << "(target:" << targetFrame << ", delay:" << m_bufferDelayFrames 
                 << "frames, latest:" << maxFrame << ")";
        
        m_decoderBusy = true;
        m_decoder.decodeFrame(frameData, actualFrame);
    }
}

void BufferedVideoDecoder::onFrameDecoded(const QImage &image, int frameNumber)
{
    // Обновляем номер последнего декодированного кадра
    m_lastDecodedFrame = frameNumber;
    
    // Освобождаем декодер
    m_decoderBusy = false;
    
    // Отправляем кадр наружу
    emit frameReady(image, frameNumber);
    
    // Пытаемся декодировать следующий кадр
    processNextFrame();
}

BufferedVideoDecoder::~BufferedVideoDecoder()
{
    cleanup();
}