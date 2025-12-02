#pragma once

#include <QObject>
#include <QTimer>
#include <QMap>
#include "../logic/videodecoder.h"
#include "../video_defaults.h"

class BufferedVideoDecoder : public QObject
{
    Q_OBJECT

public:
    // Простой конструктор с минимальными параметрами
    explicit BufferedVideoDecoder(int width, int height, int targetFps = DEFAULT_FPS, QObject *parent = nullptr);
    ~BufferedVideoDecoder();

    // Всего два публичных метода: инициализация и добавление кадра
    void initialize();
    void addEncodedFrame(int frameNumber, const QByteArray &frameData);

    // Простые геттеры для отладки
    int getBufferSize() const { return m_frameMap.size(); }
    int getCurrentFrame() const { return m_currentFrame; }

signals:
    // Единственный важный сигнал - декодированный кадр готов
    void frameDecoded(const QImage &image);

private slots:
    // Внутренний слот для обработки декодированных кадров
    void onFrameDecoded(const QImage &image, int frameNumber);

private:
    // Внутренние методы
    void setupDecoder();
    void processNextFrame();
    void cleanupOldFrames();
    
    // Вычисление, какой кадр декодировать следующим
    int findFrameToDecode();

private:
    // Параметры видео
    int m_width;
    int m_height;
    int m_targetFps;
    
    // Компоненты
    VideoDecoder *m_decoder;
    QTimer *m_decodeTimer;
    
    // Буфер закодированных кадров
    QMap<int, QByteArray> m_frameMap;
    
    // Состояние
    int m_currentFrame;
    bool m_initialized;
};
