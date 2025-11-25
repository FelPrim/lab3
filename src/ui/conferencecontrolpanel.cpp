// conferencecontrolpanel.cpp
#include "conferencecontrolpanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QDebug>

ConferenceControlPanel::ConferenceControlPanel(QWidget *parent)
    : QWidget(parent)
    , m_conferenceInfoLabel(nullptr)
    , m_participantsLabel(nullptr)
    , m_streamsLabel(nullptr)
    , m_streamSelector(nullptr)
    , m_addDeviceBtn(nullptr)
    , m_leaveConferenceBtn(nullptr)
    , m_callId(0)
    , m_displayId("---")
    , m_participantsCount(0)
    , m_streamsCount(0)
{
    setupUI();
    setupConnections();
}

void ConferenceControlPanel::setupUI()
{
    setStyleSheet(R"(
        ConferenceControlPanel {
            background: #2d2d2d;
            border: 1px solid #444;
            border-radius: 8px;
            padding: 16px;
        }
        QPushButton {
            background-color: #3d3d3d;
            color: #ffffff;
            border: 1px solid #555;
            border-radius: 6px;
            padding: 10px 14px;
            font-weight: bold;
            min-height: 18px;
        }
        QPushButton:hover {
            background-color: #4d4d4d;
            border: 1px solid #666;
        }
        QPushButton:pressed {
            background-color: #2d2d2d;
        }
        QLabel {
            color: #ffffff;
            background: transparent;
            border: none;
        }
    )");

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Conference info section
    auto infoSection = new QFrame(this);
    infoSection->setStyleSheet("background: transparent;");
    auto infoLayout = new QVBoxLayout(infoSection);
    infoLayout->setSpacing(6);
    infoLayout->setContentsMargins(0, 0, 0, 0);

    m_conferenceInfoLabel = new QLabel("Conference: ---", this);
    m_conferenceInfoLabel->setStyleSheet("font-weight: bold; font-size: 16px; color: #e0e0e0;");

    auto statsLayout = new QHBoxLayout();
    m_participantsLabel = new QLabel("Participants: 0", this);
    m_participantsLabel->setStyleSheet("font-size: 13px; color: #adb5bd;");

    m_streamsLabel = new QLabel("Streams: 0", this);
    m_streamsLabel->setStyleSheet("font-size: 13px; color: #adb5bd;");

    statsLayout->addWidget(m_participantsLabel);
    statsLayout->addWidget(m_streamsLabel);
    statsLayout->addStretch();

    infoLayout->addWidget(m_conferenceInfoLabel);
    infoLayout->addLayout(statsLayout);
    mainLayout->addWidget(infoSection);

    // Stream selector section
    auto streamsSection = new QFrame(this);
    streamsSection->setStyleSheet("background: transparent;");
    auto streamsLayout = new QVBoxLayout(streamsSection);
    streamsLayout->setSpacing(8);
    streamsLayout->setContentsMargins(0, 0, 0, 0);

    auto streamsHeader = new QLabel("Available Streams", this);
    streamsHeader->setStyleSheet("font-weight: bold; font-size: 14px; color: #e0e0e0;");

    m_streamSelector = new StreamSelectorWidget(this);
    m_streamSelector->setStyleSheet(R"(
        StreamSelectorWidget {
            background: #252525;
            border: 1px solid #444;
            border-radius: 6px;
            padding: 8px;
        }
    )");

    streamsLayout->addWidget(streamsHeader);
    streamsLayout->addWidget(m_streamSelector);
    mainLayout->addWidget(streamsSection);

    // Control buttons section
    auto controlsSection = new QFrame(this);
    controlsSection->setStyleSheet("background: transparent;");
    auto controlsLayout = new QVBoxLayout(controlsSection);
    controlsLayout->setSpacing(8);
    controlsLayout->setContentsMargins(0, 0, 0, 0);

    m_addDeviceBtn = new QPushButton("Add Video Device", this);
    m_addDeviceBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #1976D2;
            color: white;
        }
        QPushButton:hover {
            background-color: #1565C0;
        }
        QPushButton:pressed {
            background-color: #0D47A1;
        }
    )");

    m_leaveConferenceBtn = new QPushButton("Leave Conference", this);
    m_leaveConferenceBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #d32f2f;
            color: white;
        }
        QPushButton:hover {
            background-color: #c62828;
        }
        QPushButton:pressed {
            background-color: #b71c1c;
        }
    )");

    controlsLayout->addWidget(m_addDeviceBtn);
    controlsLayout->addWidget(m_leaveConferenceBtn);
    mainLayout->addWidget(controlsSection);

    // Add stretch to push everything to top
    mainLayout->addStretch();
}

void ConferenceControlPanel::setupConnections()
{
    connect(m_addDeviceBtn, &QPushButton::clicked, 
            this, &ConferenceControlPanel::onAddDeviceClicked);
    connect(m_leaveConferenceBtn, &QPushButton::clicked, 
            this, &ConferenceControlPanel::onLeaveConferenceClicked);
    
    // Stream selector signals
    connect(m_streamSelector, &StreamSelectorWidget::watchStreamRequested,
            this, &ConferenceControlPanel::onWatchStreamClicked);
    connect(m_streamSelector, &StreamSelectorWidget::stopWatchingRequested,
            this, &ConferenceControlPanel::onStopWatchingClicked);
}

void ConferenceControlPanel::setConferenceInfo(uint32_t callId, const QString& displayId)
{
    m_callId = callId;
    m_displayId = displayId;
    
    m_conferenceInfoLabel->setText(QString("Conference: %1").arg(displayId));
    
    qDebug() << "Conference info updated - ID:" << callId << "Display:" << displayId;
}

void ConferenceControlPanel::setParticipantsCount(int count)
{
    m_participantsCount = count;
    m_participantsLabel->setText(QString("Participants: %1").arg(count));
    
    // Update color based on count
    if (count > 0) {
        m_participantsLabel->setStyleSheet("font-size: 13px; color: #4CAF50;");
    } else {
        m_participantsLabel->setStyleSheet("font-size: 13px; color: #adb5bd;");
    }
}

void ConferenceControlPanel::setStreamsCount(int count)
{
    m_streamsCount = count;
    m_streamsLabel->setText(QString("Streams: %1").arg(count));
    
    // Update color based on count
    if (count > 0) {
        m_streamsLabel->setStyleSheet("font-size: 13px; color: #2196F3;");
    } else {
        m_streamsLabel->setStyleSheet("font-size: 13px; color: #adb5bd;");
    }
}

void ConferenceControlPanel::addAvailableStream(uint32_t streamId, const QString& displayId)
{
    if (m_streamSelector) {
        m_streamSelector->addStream(streamId, displayId);
        setStreamsCount(m_streamSelector->getStreamCount());
    }
    
    qDebug() << "Available stream added - ID:" << streamId << "Display:" << displayId;
}

void ConferenceControlPanel::removeAvailableStream(uint32_t streamId)
{
    if (m_streamSelector) {
        m_streamSelector->removeStream(streamId);
        setStreamsCount(m_streamSelector->getStreamCount());
    }
    
    qDebug() << "Available stream removed - ID:" << streamId;
}

void ConferenceControlPanel::onAddDeviceClicked()
{
    qDebug() << "Add device requested in conference";
    emit addDeviceRequested();
}

void ConferenceControlPanel::onLeaveConferenceClicked()
{
    qDebug() << "Leave conference requested";
    emit leaveConferenceRequested();
}

void ConferenceControlPanel::onWatchStreamClicked(uint32_t streamId)
{
    qDebug() << "Watch stream requested:" << streamId;
    emit watchStreamRequested(streamId);
}

void ConferenceControlPanel::onStopWatchingClicked(uint32_t streamId)
{
    qDebug() << "Stop watching requested:" << streamId;
    emit stopWatchingRequested(streamId);
}