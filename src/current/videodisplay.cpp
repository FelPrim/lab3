#include "videodisplay.h"
#include <QDebug>
#include <QResizeEvent>

VideoDisplay::VideoDisplay(QWidget *parent)
    : QLabel(parent)
{
    setAlignment(Qt::AlignCenter);
    setStyleSheet("background-color: #000000; border: 1px solid #444;");
    // Убираем фиксированный минимальный размер или делаем его очень маленьким
    setMinimumSize(1, 1);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void VideoDisplay::displayFrame(const QImage &frame)
{
    if (frame.isNull()) return;

    // Сохраняем оригинальное изображение
    m_currentPixmap = QPixmap::fromImage(frame);
    
    // Немедленно обновляем отображение с правильным масштабированием
    updateDisplay();
}

void VideoDisplay::displayFrameFromNetwork(const QImage &frame)
{
    displayFrame(frame);
}

void VideoDisplay::resizeEvent(QResizeEvent *event)
{
    QLabel::resizeEvent(event);
    // При изменении размера сразу обновляем отображение
    updateDisplay();
}

void VideoDisplay::updateDisplay()
{
    if (m_currentPixmap.isNull()) return;

    QSize labelSize = size();
    
    // Масштабируем с сохранением пропорций для заполнения области
    QPixmap scaledPixmap = m_currentPixmap.scaled(labelSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    setPixmap(scaledPixmap);
    
    // Принудительно обновляем виджет
    update();
}