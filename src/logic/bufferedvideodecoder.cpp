// bufferedvideodecoder.cpp
#include "bufferedvideodecoder.h"
#include <QDebug>
#include <QCryptographicHash>
#include "../video_defaults.h"

#define TESTING_NETCODE
#undef TESTING_NETCODE

BufferedVideoDecoder::BufferedVideoDecoder(int width, int height, int fps, 
                                           int bufferDelayFrames, QObject *parent)
    : QObject(parent)
    , m_buffer(static_cast<int>(10*DEFAULT_BUFFERSECONDS * fps))
    , m_decoder(width, height, this)
    , m_targetFps(fps)
{
    // Устанавливаем задержку
    if (bufferDelayFrames < 0) {
        // Автоматический расчет: задержка = 1/3 от размера буфера
        m_bufferDelayFrames = DEFAULT_BUFFERSECONDS*DEFAULT_FPS;
        m_bufferDelayFrames = qMax(1, m_bufferDelayFrames);  // Минимум 1 кадр
    } else {
        m_bufferDelayFrames = qMax(0, bufferDelayFrames);
    }
    
    connect(&m_decoder, &VideoDecoder::frameDecoded,
            this, &BufferedVideoDecoder::onFrameDecoded);
    connect(&m_decoder, &VideoDecoder::errorOccurred,
        this, &BufferedVideoDecoder::onDecoderError);
    
    qDebug() << "[BufferedVideoDecoder] Created with buffer delay:" 
             << m_bufferDelayFrames << "frames, capacity:" << m_buffer.capacity();
}

void BufferedVideoDecoder::initialize()
{
    m_decoder.initialize();
    m_lastDecodedFrame = -1;
    m_lastAttemptedFrame = -1;
    m_decoderBusy = false;
}

void BufferedVideoDecoder::cleanup()
{
    m_decoder.cleanup();
    m_buffer.clear();
    m_lastDecodedFrame = -1;
    m_lastAttemptedFrame = -1;
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
#ifdef TESTING_NETCODE
   // QByteArray sha = QCryptographicHash::hash(frameData, QCryptographicHash::Sha256);
   // qDebug() << "BufferedVideoDecoder: received frame" << frameNumber
   //         << "size:" << frameData.size() << "sha256:" << sha.toHex().left(64);
   // const int BUF_SIZE = 4000;
   // int copyLen = qMin<int>(frameData.size(), BUF_SIZE - 1); // оставляем 1 байт под '\0'
   // char buffer[BUF_SIZE];
   // if (copyLen > 0) {
   //     memcpy(buffer, frameData.constData(), copyLen);
   // }
   // buffer[copyLen] = '\0';
   // printf("BufferedVideoDecoder: RECVD (size=%d, shown=%d): %s\n",
   //        static_cast<int>(frameData.size()), copyLen, buffer);
#endif
    qDebug() << "[BufferedVideoDecoder] ADD frame" << frameNumber 
         << "size:" << frameData.size();
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

    int minFrame = m_buffer.getMinFrameNumber();
    int maxFrame = m_buffer.getMaxFrameNumber();
    if (minFrame < 0 || maxFrame < 0) {
        return; // Буфер пуст
    }

    qDebug() << "[BufferedVideoDecoder] Buffer state: min=" << minFrame
             << "max=" << maxFrame << "lastDecoded=" << m_lastDecodedFrame;

    int targetFrame = maxFrame - m_bufferDelayFrames;

    // Если задержка слишком большая, берем самый старый кадр
    if (targetFrame < minFrame) {
        targetFrame = minFrame;
    }

    // Вместо немедленного выхода, если targetFrame <= m_lastDecodedFrame,
    // начинаем поиск с (m_lastDecodedFrame + 1) — чтобы гарантировать прогресс.
    // Но всё ещё даём приоритет кадрам, близким к targetFrame (если они > lastDecoded).
    int searchStart = qMax(targetFrame, m_lastDecodedFrame + 1);

    // Ищем ближайший доступный кадр вперёд, начиная с searchStart
    int actualFrame = -1;
    for (int frame = searchStart; frame <= maxFrame; ++frame) {
        if (m_buffer.hasFrame(frame)) {
            actualFrame = frame;
            break;
        }
    }

    // Если не нашли вперёд, попробуем найти ближайший доступный кадр в пределах (m_lastDecodedFrame+1 .. targetFrame-1),
    // чтобы не застревать если нет новых кадров, но есть более старые, ещё не декодированные.
    if (actualFrame == -1) {
        for (int frame = targetFrame - 1; frame >= qMax(minFrame, m_lastDecodedFrame + 1); --frame) {
            if (m_buffer.hasFrame(frame)) {
                actualFrame = frame;
                break;
            }
        }
    }

    if (actualFrame == -1) {
        // Не найдено подходящего кадра для декодирования сейчас
        return;
    }

    QByteArray frameData;
    if (m_buffer.getFrame(actualFrame, frameData)) {
        qDebug() << "[BufferedVideoDecoder] Decoding frame" << actualFrame
                 << "(target:" << targetFrame << ", delay:" << m_bufferDelayFrames
                 << "frames, latest:" << maxFrame << ")";

        m_decoderBusy = true;
        m_lastAttemptedFrame = actualFrame; // <-- помним, какой кадр отправили
        m_decoder.decodeFrame(frameData, actualFrame);
    }
}

void BufferedVideoDecoder::onDecoderError(const QString &message)
{
    // Логируем ошибку
    qDebug() << "[BufferedVideoDecoder] Decoder error for frame"
             << m_lastAttemptedFrame << ":" << message;

    // Форвардим внешний сигнал об ошибке
    emit errorOccurred(message);

    // Сбрасываем busy-флаг, иначе декодирование остановится навсегда
    m_decoderBusy = false;

    // Решаем, как обходить проблемный кадр:
    // Вариант A (безопасный): помечаем кадр как "последний декодированный",
    // чтобы не пытаться декодировать его снова:
    if (m_lastAttemptedFrame > m_lastDecodedFrame) {
        m_lastDecodedFrame = m_lastAttemptedFrame;
    }

    // Вариант B (если FrameBuffer поддерживает удаление) — удалить кадр из буфера,
    // чтобы релизнуть память и не пытаться снова:
    // if (m_lastAttemptedFrame >= 0) {
    //     m_buffer.removeFrame(m_lastAttemptedFrame);
    // }

    m_lastAttemptedFrame = -1;

    // Попытаться декодировать дальше
    processNextFrame();
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