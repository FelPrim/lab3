// maincontrolpanel.cpp
#include "maincontrolpanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QDebug>

MainControlPanel::MainControlPanel(QWidget *parent)
    : QWidget(parent)
    , m_addDeviceBtn(nullptr)
    , m_createConferenceBtn(nullptr)
    , m_conferenceIdInput(nullptr)
    , m_streamIdInput(nullptr)
    , m_connectionStatusLabel(nullptr)
{
    setupUI();
    setupConnections();
}

void MainControlPanel::setupUI()
{
    setStyleSheet(R"(
        MainControlPanel {
            background: #2d2d2d;
            border-right: 1px solid #444;
        }
        QGroupBox {
            color: #ffffff;
            font-weight: bold;
            border: 1px solid #555;
            border-radius: 6px;
            margin-top: 10px;
            padding-top: 10px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px 0 5px;
            color: #bb86fc;
        }
    )");

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(15, 15, 15, 15);

    // Connection status
    m_connectionStatusLabel = new QLabel("🔴 Disconnected", this);
    m_connectionStatusLabel->setStyleSheet(R"(
        QLabel {
            color: #ff5252;
            font-weight: bold;
            font-size: 14px;
            padding: 8px;
            background: #1e1e1e;
            border-radius: 4px;
            border: 1px solid #444;
        }
    )");
    mainLayout->addWidget(m_connectionStatusLabel);

    // Device management group
    auto deviceGroup = new QGroupBox("Video Devices", this);
    auto deviceLayout = new QVBoxLayout(deviceGroup);
    deviceLayout->setSpacing(8);

    m_addDeviceBtn = new QPushButton("➕ Add Video Device", deviceGroup);
    m_addDeviceBtn->setStyleSheet(R"(
        QPushButton {
            background: #1976D2;
            color: white;
            border: none;
            padding: 10px;
            border-radius: 4px;
            font-weight: bold;
        }
        QPushButton:hover {
            background: #1565C0;
        }
        QPushButton:pressed {
            background: #0D47A1;
        }
    )");
    deviceLayout->addWidget(m_addDeviceBtn);
    mainLayout->addWidget(deviceGroup);

    // Conference management group
    auto conferenceGroup = new QGroupBox("Conferences", this);
    auto conferenceLayout = new QVBoxLayout(conferenceGroup);
    conferenceLayout->setSpacing(8);

    m_createConferenceBtn = new QPushButton("🎯 Create Conference", conferenceGroup);
    m_createConferenceBtn->setStyleSheet(R"(
        QPushButton {
            background: #388E3C;
            color: white;
            border: none;
            padding: 10px;
            border-radius: 4px;
            font-weight: bold;
        }
        QPushButton:hover {
            background: #2E7D32;
        }
        QPushButton:pressed {
            background: #1B5E20;
        }
    )");
    conferenceLayout->addWidget(m_createConferenceBtn);

    // Conference join section - ТОЛЬКО поле ввода (с встроенной кнопкой)
    m_conferenceIdInput = new StreamIdInputWidget(conferenceGroup);
    m_conferenceIdInput->setPlaceholderText("Conference ID");
    conferenceLayout->addWidget(m_conferenceIdInput);
    
    mainLayout->addWidget(conferenceGroup);

    // Public streams group - ТОЛЬКО поле ввода (с встроенной кнопкой)
    auto streamGroup = new QGroupBox("Public Streams", this);
    auto streamLayout = new QVBoxLayout(streamGroup);
    streamLayout->setSpacing(8);

    m_streamIdInput = new StreamIdInputWidget(streamGroup);
    m_streamIdInput->setPlaceholderText("Stream ID");
    streamLayout->addWidget(m_streamIdInput);
    
    mainLayout->addWidget(streamGroup);

    // Add stretch to push everything to top
    mainLayout->addStretch(1);
}

void MainControlPanel::setupConnections()
{
    // Основные кнопки
    connect(m_addDeviceBtn, &QPushButton::clicked, this, &MainControlPanel::onAddDeviceClicked);
    connect(m_createConferenceBtn, &QPushButton::clicked, this, &MainControlPanel::onCreateConferenceClicked);
    
    // ТОЛЬКО прямые сигналы от полей ввода (больше нет дублирующих кнопок)
    connect(m_conferenceIdInput, &StreamIdInputWidget::joinRequested, this, &MainControlPanel::joinConferenceRequested);
    connect(m_streamIdInput, &StreamIdInputWidget::joinRequested, this, &MainControlPanel::joinPublicStreamRequested);
}

void MainControlPanel::setConnectionStatus(bool connected)
{
    if (connected) {
        m_connectionStatusLabel->setText("🟢 Connected");
        m_connectionStatusLabel->setStyleSheet(R"(
            QLabel {
                color: #69f0ae;
                font-weight: bold;
                font-size: 14px;
                padding: 8px;
                background: #1e1e1e;
                border-radius: 4px;
                border: 1px solid #444;
            }
        )");
    } else {
        m_connectionStatusLabel->setText("🔴 Disconnected");
        m_connectionStatusLabel->setStyleSheet(R"(
            QLabel {
                color: #ff5252;
                font-weight: bold;
                font-size: 14px;
                padding: 8px;
                background: #1e1e1e;
                border-radius: 4px;
                border: 1px solid #444;
            }
        )");
    }
}

void MainControlPanel::onAddDeviceClicked()
{
    qDebug() << "Add device button clicked";
    emit addDeviceRequested();
}

void MainControlPanel::onCreateConferenceClicked()
{
    qDebug() << "Create conference button clicked";
    emit createConferenceRequested();
}