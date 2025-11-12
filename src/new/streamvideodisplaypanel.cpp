#include "streamvideodisplaypanel.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QDebug>

StreamVideoDisplayPanel::StreamVideoDisplayPanel(QWidget *parent)
    : QWidget(parent)
    , m_videoDisplayWidget(nullptr)
    , m_placeholderLabel(nullptr)
    , m_layout(nullptr)
{
    setupUI();
}

StreamVideoDisplayPanel::~StreamVideoDisplayPanel()
{
}

void StreamVideoDisplayPanel::setupUI()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);

    // VideoDisplay - делегат для реального показа
    m_videoDisplayWidget = new VideoDisplay(this);
    m_videoDisplayWidget->setMinimumSize(320, 240);
    m_videoDisplayWidget->setVisible(false);

    // Placeholder label
    m_placeholderLabel = new QLabel(this);
    m_placeholderLabel->setAlignment(Qt::AlignCenter);
    m_placeholderLabel->setStyleSheet("background-color: #1a1a1a; color: #666; border: 2px dashed #444; padding: 40px;");
    m_placeholderLabel->setMinimumSize(320, 240);

    m_layout->addWidget(m_videoDisplayWidget);
    m_layout->addWidget(m_placeholderLabel);
}

void StreamVideoDisplayPanel::setPlaceholderText(const QString &text)
{
    m_placeholderLabel->setText(text);
}

void StreamVideoDisplayPanel::showVideo(bool show)
{
    m_videoDisplayWidget->setVisible(show);
    m_placeholderLabel->setVisible(!show);
}

void StreamVideoDisplayPanel::setStreamId(int streamId)
{
    Q_UNUSED(streamId)
}

void StreamVideoDisplayPanel::displayFrame(const QImage &frame)
{
    if (frame.isNull()) return;

    // Делегируем отображение VideoDisplay
    if (m_videoDisplayWidget) {
        m_videoDisplayWidget->displayFrame(frame);
        showVideo(true);
        clearPlaceholder();
    }
}

void StreamVideoDisplayPanel::clearDisplay()
{
    if (m_videoDisplayWidget) {
        m_videoDisplayWidget->clearDisplay();
    }
    showVideo(false);
}

void StreamVideoDisplayPanel::clearPlaceholder()
{
    if (m_placeholderLabel) {
        m_placeholderLabel->clear();
        m_placeholderLabel->setVisible(false);
    }
}
