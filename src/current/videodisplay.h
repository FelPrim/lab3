#pragma once

#include <QLabel>
#include <QImage>

class VideoDisplay : public QLabel
{
    Q_OBJECT

public:
    explicit VideoDisplay(QWidget *parent = nullptr);
    void setStreamId(int streamId) { m_streamId = streamId; }

public slots:
    void displayFrame(const QImage &frame);
    void displayFrameFromNetwork(const QImage &frame);

private:
    int m_streamId = -1;
};
