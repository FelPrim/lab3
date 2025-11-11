#include "streamcontrolpanel.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDebug>

StreamControlPanel::StreamControlPanel(Role role, QWidget *parent)
    : QWidget(parent)
    , m_role(role)
    , m_active(false)
    , m_hasViewers(false)
{
    setupUI();
}

void StreamControlPanel::setupUI()
{
    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    
    // Stream ID label
    m_streamIdLabel = new QLabel("ID: ---", this);
    m_streamIdLabel->setStyleSheet("color: #adb5bd; font-size: 13px;");
    layout->addWidget(m_streamIdLabel);
    
    layout->addStretch();
    
    // Status label
    m_statusLabel = new QLabel("", this);
    m_statusLabel->setStyleSheet("color: #6c757d; font-size: 13px;");
    layout->addWidget(m_statusLabel);
    
    layout->addStretch();
    
    // Action button
    if (m_role == Publisher) {
        m_actionButton = new QPushButton("Stop Stream", this);
        m_actionButton->setStyleSheet(
            "QPushButton {"
            "   background: #dc3545;"
            "   color: white;"
            "   padding: 10px 20px;"
            "   border: none;"
            "   border-radius: 6px;"
            "   font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "   background: #c82333;"
            "}"
            "QPushButton:pressed {"
            "   background: #bd2130;"
            "}"
        );
        connect(m_actionButton, &QPushButton::clicked, this, &StreamControlPanel::onStopClicked);
    } else {
        m_actionButton = new QPushButton("Leave Stream", this);
        m_actionButton->setStyleSheet(
            "QPushButton {"
            "   background: #6c757d;"
            "   color: white;"
            "   padding: 10px 20px;"
            "   border: none;"
            "   border-radius: 6px;"
            "   font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "   background: #5a6268;"
            "}"
            "QPushButton:pressed {"
            "   background: #545b62;"
            "}"
        );
        connect(m_actionButton, &QPushButton::clicked, this, &StreamControlPanel::onLeaveClicked);
    }
    
    layout->addWidget(m_actionButton);
    
    updateUI();
}

void StreamControlPanel::setStreamId(const QString &streamId)
{
    m_streamId = streamId;
    m_streamIdLabel->setText(QString("ID: %1").arg(streamId));
}

void StreamControlPanel::setStatus(const QString &status)
{
    m_statusLabel->setText(status);
}

void StreamControlPanel::setActive(bool active)
{
    m_active = active;
    updateUI();
}

void StreamControlPanel::setViewers(bool hasViewers)
{
    m_hasViewers = hasViewers;
    updateUI();
}

void StreamControlPanel::onStopClicked()
{
    qDebug() << "Stop button clicked";
    emit stopRequested();
}

void StreamControlPanel::onLeaveClicked()
{
    qDebug() << "Leave button clicked";
    emit leaveRequested();
}

void StreamControlPanel::updateUI()
{
    if (m_role == Publisher) {
        if (m_hasViewers) {
            m_statusLabel->setText("Viewers connected");
            m_statusLabel->setStyleSheet("color: #28a745; font-weight: bold; font-size: 13px;");
        } else {
            m_statusLabel->setText("No viewers");
            m_statusLabel->setStyleSheet("color: #ffc107; font-weight: bold; font-size: 13px;");
        }
    } else {
        if (m_active) {
            m_statusLabel->setText("Connected");
            m_statusLabel->setStyleSheet("color: #28a745; font-weight: bold; font-size: 13px;");
        } else {
            m_statusLabel->setText("Disconnected");
            m_statusLabel->setStyleSheet("color: #dc3545; font-weight: bold; font-size: 13px;");
        }
    }
}
