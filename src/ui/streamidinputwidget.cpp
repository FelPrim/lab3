// streamidinputwidget.cpp
#include "streamidinputwidget.h"
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpressionValidator>
#include <QDebug>

const QRegularExpression StreamIdInputWidget::STREAM_ID_REGEX(QStringLiteral("^[A-Z]{6}$"));

StreamIdInputWidget::StreamIdInputWidget(QWidget *parent)
    : QWidget(parent)
    , m_streamIdInput(nullptr)
    , m_joinButton(nullptr)
    , m_validator(nullptr)
{
    setupUI();
    setupConnections();
}

void StreamIdInputWidget::setupUI()
{
    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    
    // Input field
    m_streamIdInput = new QLineEdit(this);
    m_streamIdInput->setPlaceholderText("Enter ID (6 capital letters)");
    m_streamIdInput->setMaxLength(6);
    m_streamIdInput->setStyleSheet(R"(
        QLineEdit {
            padding: 8px;
            border: 1px solid #555;
            border-radius: 4px;
            background: #2d2d2d;
            color: white;
            font-size: 12px;
        }
        QLineEdit:focus {
            border: 1px solid #1976D2;
        }
    )");
    
    // Validator for 6 capital letters
    m_validator = new QRegularExpressionValidator(STREAM_ID_REGEX, this);
    m_streamIdInput->setValidator(m_validator);
    
    // Join button
    m_joinButton = new QPushButton("Join", this);
    m_joinButton->setEnabled(false);
    m_joinButton->setStyleSheet(R"(
        QPushButton {
            padding: 8px 16px;
            background: #1976D2;
            color: white;
            border-radius: 4px;
            font-weight: bold;
            min-width: 60px;
        }
        QPushButton:hover {
            background: #1565C0;
        }
        QPushButton:pressed {
            background: #0D47A1;
        }
        QPushButton:disabled {
            background: #555;
            color: #888;
        }
    )");
    
    layout->addWidget(m_streamIdInput, 1);
    layout->addWidget(m_joinButton);
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
    // Auto-convert to uppercase
    if (text != text.toUpper()) {
        m_streamIdInput->setText(text.toUpper());
        return;
    }
    
    bool valid = isValid();
    m_joinButton->setEnabled(valid);
    
    // Visual feedback
    if (valid) {
        m_streamIdInput->setStyleSheet(R"(
            QLineEdit {
                padding: 8px;
                border: 2px solid #4CAF50;
                border-radius: 4px;
                background: #2d2d2d;
                color: white;
                font-size: 12px;
            }
        )");
    } else {
        m_streamIdInput->setStyleSheet(R"(
            QLineEdit {
                padding: 8px;
                border: 1px solid #555;
                border-radius: 4px;
                background: #2d2d2d;
                color: white;
                font-size: 12px;
            }
        )");
    }
    
    emit streamIdChanged(text);
}

void StreamIdInputWidget::onJoinClicked()
{
    if (isValid()) {
        QString streamId = getStreamId();
        qDebug() << "Join clicked with ID:" << streamId;
        emit joinRequested(streamId);
    }
}