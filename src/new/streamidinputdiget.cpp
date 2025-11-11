#include "streamidinputwidget.h"
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QRegExpValidator>
#include <QDebug>

const QRegExp StreamIdInputWidget::STREAM_ID_REGEX("[A-Z]{6}");

StreamIdInputWidget::StreamIdInputWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

void StreamIdInputWidget::setupUI()
{
    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    
    // Input field
    m_streamIdInput = new QLineEdit(this);
    m_streamIdInput->setPlaceholderText("Enter Stream ID (6 capital letters)");
    m_streamIdInput->setMaxLength(6);
    m_streamIdInput->setStyleSheet("QLineEdit { padding: 8px; border: 1px solid #555; border-radius: 4px; background: #2d2d2d; }");
    
    // Validator
    m_validator = new QRegExpValidator(STREAM_ID_REGEX, this);
    m_streamIdInput->setValidator(m_validator);
    
    // Join button
    m_joinButton = new QPushButton("Join", this);
    m_joinButton->setEnabled(false);
    m_joinButton->setStyleSheet("QPushButton { padding: 8px 16px; background: #1976D2; color: white; border-radius: 4px; }"
                              "QPushButton:disabled { background: #555; color: #888; }");
    
    layout->addWidget(m_streamIdInput, 1);
    layout->addWidget(m_joinButton);
    
    setupConnections();
}

void StreamIdInputWidget::setupConnections()
{
    connect(m_streamIdInput, &QLineEdit::textChanged, this, &StreamIdInputWidget::onTextChanged);
    connect(m_joinButton, &QPushButton::clicked, this, &StreamIdInputWidget::onJoinClicked);
}

QString StreamIdInputWidget::getStreamId() const
{
    return m_streamIdInput->text().toUpper();
}

bool StreamIdInputWidget::isValid() const
{
    return m_streamIdInput->hasAcceptableInput();
}

void StreamIdInputWidget::clear()
{
    m_streamIdInput->clear();
}

void StreamIdInputWidget::setEnabled(bool enabled)
{
    m_streamIdInput->setEnabled(enabled);
    m_joinButton->setEnabled(enabled && isValid());
}

void StreamIdInputWidget::setStreamId(const QString &streamId)
{
    m_streamIdInput->setText(streamId);
}

void StreamIdInputWidget::onTextChanged(const QString &text)
{
    // Автоматически поднимаем регистр
    if (text != text.toUpper()) {
        m_streamIdInput->setText(text.toUpper());
        return;
    }
    
    bool valid = isValid();
    m_joinButton->setEnabled(valid);
    
    if (valid) {
        m_streamIdInput->setStyleSheet("QLineEdit { padding: 8px; border: 2px solid #4CAF50; border-radius: 4px; background: #2d2d2d; }");
    } else {
        m_streamIdInput->setStyleSheet("QLineEdit { padding: 8px; border: 1px solid #555; border-radius: 4px; background: #2d2d2d; }");
    }
    
    emit streamIdChanged(text);
}

void StreamIdInputWidget::onJoinClicked()
{
    if (isValid()) {
        QString streamId = getStreamId();
        qDebug() << "Join clicked with stream ID:" << streamId;
        emit joinRequested(streamId);
    }
}
