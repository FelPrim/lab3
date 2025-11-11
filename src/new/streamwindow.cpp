#include "streamwindow.h"
#include <QCloseEvent>
#include <QHBoxLayout>
#include <QTimer>

StreamWindow::StreamWindow(QWidget *parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_statusLabel(nullptr)
    , m_streamInfoLabel(nullptr)
    , m_errorLabel(nullptr)
{
}

void StreamWindow::setupCommonUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(10);
    m_mainLayout->setContentsMargins(15, 15, 15, 15);
    
    // Stream info (ID, название и т.д.)
    auto infoLayout = new QHBoxLayout();
    m_streamInfoLabel = new QLabel("Stream Info", this);
    m_streamInfoLabel->setStyleSheet("font-weight: bold; font-size: 16px; color: #4FC3F7;");
    infoLayout->addWidget(m_streamInfoLabel);
    infoLayout->addStretch();
    m_mainLayout->addLayout(infoLayout);
    
    // Status label
    m_statusLabel = new QLabel("Status: Unknown", this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet("font-size: 14px; padding: 10px; background: #2d2d2d; border-radius: 5px;");
    m_mainLayout->addWidget(m_statusLabel);
    
    // Error label (скрыт по умолчанию)
    setupErrorLabel();
}

void StreamWindow::setupCommonConnections()
{
    // Общие соединения могут быть добавлены здесь
}

void StreamWindow::setupErrorLabel()
{
    m_errorLabel = new QLabel(this);
    m_errorLabel->setAlignment(Qt::AlignCenter);
    m_errorLabel->setStyleSheet("color: #F44336; background: #330000; padding: 8px; border-radius: 4px; margin: 5px;");
    m_errorLabel->setVisible(false);
    m_mainLayout->addWidget(m_errorLabel);
}

void StreamWindow::setStreamInfo(const QString &info)
{
    if (m_streamInfoLabel) {
        m_streamInfoLabel->setText(info);
    }
}

void StreamWindow::setStatus(const QString &status, const QString &color)
{
    if (m_statusLabel) {
        m_statusLabel->setText(status);
        m_statusLabel->setStyleSheet(QString(
            "font-size: 14px; padding: 10px; background: %1; color: white; border-radius: 5px;"
        ).arg(color));
    }
}

void StreamWindow::showError(const QString &error)
{
    if (m_errorLabel) {
        m_errorLabel->setText("❌ " + error);
        m_errorLabel->setVisible(true);
        
        // Автоматически скрываем ошибку через 5 секунд
        QTimer::singleShot(5000, this, [this]() {
            if (m_errorLabel) {
                m_errorLabel->setVisible(false);
            }
        });
        
        emit errorOccurred(error);
    }
}

void StreamWindow::cleanup()
{
    // Базовая реализация cleanup - может быть переопределена
    qDebug() << "StreamWindow cleanup for stream:" << getStreamId();
}

void StreamWindow::closeEvent(QCloseEvent *event)
{
    cleanup();
    emit windowClosed(getStreamId());
    QWidget::closeEvent(event);
}
