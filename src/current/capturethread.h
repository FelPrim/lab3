#pragma once
#include "bufferreaderthread.h"
#include "video_defaults.h"
#include <QThread>
#include <QImage>
#include <atomic>
#include <opencv2/opencv.hpp>
#include "EncoderWorker.h"
#include "DecoderWorker.h"

class CaptureThread : public QThread
{
    Q_OBJECT
public:
    explicit CaptureThread(QObject *parent = nullptr);
    ~CaptureThread() override;

    void startCapture(int deviceIndex);
    void stopCapture();
    
    // Новый метод для получения индекса устройства
    int getDeviceIndex() const { return m_deviceIndex; }

    void setBufferSeconds(int seconds) { 
        m_bufferSeconds = seconds; 
        if (m_packetBuffer) {
            int bufferCapacity = m_fps * m_bufferSeconds * 2;
            m_packetBuffer->setCapacity(bufferCapacity);
        }
    }
    float getBufferSeconds() const { return m_bufferSeconds; }
    
signals:
    void frameReady(const QImage &img);
    void errorOccurred(const QString &msg);

    // to encoder
    void frameCaptured(const cv::Mat &frame);

protected:
    void run() override;

private:
    int m_bufferSeconds = DEFAULT_BUFFERSECONDS;
    PacketBuffer* m_packetBuffer = nullptr;
    int m_encoderFrameCount = 0;
    int m_frameSequence = 0;
    std::atomic<bool> m_running{false};
    int m_deviceIndex = -1;
    float m_fps = DEFAULT_FPS;
    bool m_initialized = false;

    // threads and workers
    BufferReaderThread *m_bufferReaderThread = nullptr;
    QThread *m_encoderThread = nullptr;
    QThread *m_decoderThread = nullptr;
    EncoderWorker *m_encoderWorker = nullptr;
    DecoderWorker *m_decoderWorker = nullptr;
};