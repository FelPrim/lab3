#pragma once

#include <QLabel>
#include <QImage>
#include <QPixmap>  // Добавляем для QPixmap

class VideoDisplay : public QLabel
{
    Q_OBJECT

public:
    explicit VideoDisplay(QWidget *parent = nullptr);
    void setStreamId(int streamId) { m_streamId = streamId; }

public slots:
    void displayFrame(const QImage &frame);
    void displayFrameFromNetwork(const QImage &frame);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void updateDisplay();  // Добавляем объявление приватного метода

    int m_streamId = -1;
    QPixmap m_currentPixmap;  // Добавляем объявление переменной

public:
    void setPlaceholderText(const QString& text); 
    
private:
    QString m_placeholderText;
};
