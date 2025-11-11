#include "streamvideodisplaypanel.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QDebug>

StreamVideoDisplayPanel::StreamVideoDisplayPanel(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

StreamVideoDisplayPanel::~StreamVideoDisplayPanel()
{
    qDebug() << "StreamVideoDisplayPanel destroyed";
}

void StreamVideoDisplayPanel::setupUI()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    
    // Video display label (скрыт по умолчанию)
    m_videoLabel = new QLabel(this);
    m_videoLabel->setAlignment(Qt::AlignCenter);
    m_videoLabel->setStyleSheet("background-color: #000000; color: #666; border: 2px solid #444;");
    m_videoLabel->setMinimumSize(320, 240);
    m_videoLabel->setVisible(false);
    
    // Placeholder label
    m_placeholderLabel = new QLabel(this);
    m_placeholderLabel->setAlignment(Qt::AlignCenter);
    m_placeholderLabel->setStyleSheet("background-color: #1a1a1a; color: #666; border: 2px dashed #444; padding: 40px;");
    m_placeholderLabel->setMinimumSize(320, 240);
    
    m_layout->addWidget(m_videoLabel);
    m_layout->addWidget(m_placeholderLabel);
}

void StreamVideoDisplayPanel::setPlaceholderText(const QString &text)
{
    m_placeholderLabel->setText(text);
}

void StreamVideoDisplayPanel::showVideo(bool show)
{
    m_videoLabel->setVisible(show);
    m_placeholderLabel->setVisible(!show);
}

void StreamVideoDisplayPanel::setStreamId(int streamId)
{
    // Можно использовать для идентификации потока
    Q_UNUSED(streamId)
}

void StreamVideoDisplayPanel::displayFrame(const QImage &frame)
{
    if (!frame.isNull()) {
        QPixmap pixmap = QPixmap::fromImage(frame.scaled(m_videoLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        m_videoLabel->setPixmap(pixmap);
        showVideo(true);
    }
}

void StreamVideoDisplayPanel::clearDisplay()
{
    m_videoLabel->clear();
    showVideo(false);
}
