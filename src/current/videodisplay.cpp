#include "videodisplay.h"
#include <QDebug>

VideoDisplay::VideoDisplay(QWidget *parent)
    : QLabel(parent)
{
    setAlignment(Qt::AlignCenter);
    setStyleSheet("background-color: #000000; border: 1px solid #444;");
    setMinimumSize(320, 240);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void VideoDisplay::displayFrame(const QImage &frame)
{
    if (frame.isNull()) return;

    // Масштабируем изображение для отображения с сохранением пропорций
    QPixmap pixmap = QPixmap::fromImage(frame);
    QSize labelSize = size();
    
    QPixmap scaledPixmap = pixmap.scaled(labelSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    setPixmap(scaledPixmap);
}

void VideoDisplay::displayFrameFromNetwork(const QImage &frame)
{
    displayFrame(frame);
}
