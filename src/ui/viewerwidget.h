#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include "../logic/videodisplay.h"
#include "streamcontrolpanel.h"
#include "../network/udp/networkdisplaybuffer.h"

class ViewerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ViewerWidget(uint32_t streamId, const QString &displayId, QWidget *parent = nullptr);
    ~ViewerWidget();

    // Управление виджетом
    void initialize();
    void cleanup();
    
    // StreamWindow-like interface
    uint32_t getStreamId() const { return m_streamId; }
    QString getDisplayId() const { return m_displayId; }
    bool isActive() const { return m_active; }
    
    // Специфичные методы зрителя
    void setActive(bool active);
    void setStreamId(uint32_t streamId, const QString &displayId);

    // Видео методы
    void displayFrame(const QImage &frame);
    void clearDisplay();

    void setControlPanel(StreamControlPanel* panel);  

public slots:
    void onFrameAssembled(int streamId, int frameNumber, const QByteArray &frameData);
    void onLeaveRequested();
public slots:
    void onFrameReady(const QImage &frame, int frameNumber) {
        Q_UNUSED(frame)
        Q_UNUSED(frameNumber)
        qDebug() << "ViewerWidget::onFrameReady - frame:" << frameNumber;
    }
signals:
    void streamLeft(uint32_t streamId);

private slots:
    void onLeaveButtonClicked();

private:
    void setupUI();
    void setupConnections();
    void updateStatus();

    // Компоненты
    VideoDisplay *m_videoDisplay;
    StreamControlPanel *m_controlPanel;
    QVBoxLayout *m_mainLayout;
    
    // КРИТИЧЕСКИ ВАЖНО: NetworkDisplayBuffer для буферизации видео
    NetworkDisplayBuffer *m_displayBuffer;
    
    // Состояние
    uint32_t m_streamId;
    QString m_displayId;
    bool m_active;
    
    // Константы
    static const QString PLACEHOLDER_TEXT;
    static const QString STATUS_ACTIVE;
    static const QString STATUS_INACTIVE;
signals:
    void frameReady(const QImage &frame, int frameNumber);
};