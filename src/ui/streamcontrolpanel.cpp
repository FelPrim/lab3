// streamcontrolpanel.cpp
#include "streamcontrolpanel.h"
#include <QDebug>

StreamControlPanel::StreamControlPanel(Mode mode, QWidget *parent)
    : QWidget(parent)
    , m_mode(mode)
    , m_streamIdLabel(nullptr)
    , m_statusLabel(nullptr)
    , m_viewersLabel(nullptr)
    , m_startStopButton(nullptr)
    , m_leaveButton(nullptr)
    , m_disconnectButton(nullptr)
    , m_active(false)
    , m_streaming(false)
    , m_viewersCount(0)
    , m_streamState(StateNoStream)
    , m_viewersConnected(false)
{
    setupUI();
    updateUI();
}

void StreamControlPanel::setupUI()
{
    setStyleSheet(R"(
        StreamControlPanel {
            background-color: #2d2d2d;
            border: 1px solid #555;
            border-radius: 6px;
            padding: 8px;
        }
        QLabel {
            color: #ffffff;
            background: transparent;
            border: none;
        }
        QPushButton {
            background-color: #3d3d3d;
            color: #ffffff;
            border: 1px solid #555;
            border-radius: 4px;
            padding: 6px 12px;
            font-weight: bold;
            min-width: 80px;
        }
        QPushButton:hover {
            background-color: #4d4d4d;
        }
        QPushButton:pressed {
            background-color: #2d2d2d;
        }
        QPushButton:disabled {
            background-color: #2a2a2a;
            color: #666;
            border-color: #333;
        }
    )");

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(6);
    mainLayout->setContentsMargins(8, 8, 8, 8);

    // Stream ID label
    m_streamIdLabel = new QLabel("Stream ID: ---");
    m_streamIdLabel->setStyleSheet("font-weight: bold; font-size: 12px;");
    mainLayout->addWidget(m_streamIdLabel);

    // Status label
    m_statusLabel = new QLabel("Status: Inactive");
    m_statusLabel->setStyleSheet("font-size: 11px; color: #adb5bd;");
    mainLayout->addWidget(m_statusLabel);

    // Viewers label (only for streamer mode)
    if (m_mode == StreamerMode) {
        m_viewersLabel = new QLabel("Viewers: 0");
        m_viewersLabel->setStyleSheet("font-size: 11px; color: #adb5bd;");
        mainLayout->addWidget(m_viewersLabel);
    }

    // Button layout
    auto buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(6);

    // Start/Stop button (streamer mode) or status (viewer mode)
    if (m_mode == StreamerMode) {
        m_startStopButton = new QPushButton("Start Stream");
        m_startStopButton->setStyleSheet(R"(
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
            QPushButton:disabled {
                background-color: #424242;
                color: #757575;
            }
        )");
        buttonLayout->addWidget(m_startStopButton);
    } else {
        // Viewer mode - connection status indicator
        auto statusWidget = new QWidget();
        auto statusLayout = new QHBoxLayout(statusWidget);
        statusLayout->setContentsMargins(0, 0, 0, 0);
        
        auto statusIndicator = new QLabel("●");
        statusIndicator->setStyleSheet("color: #f44336; font-size: 14px;");
        auto statusText = new QLabel("Disconnected");
        statusText->setStyleSheet("color: #adb5bd; font-size: 11px;");
        
        statusLayout->addWidget(statusIndicator);
        statusLayout->addWidget(statusText);
        statusLayout->addStretch();
        
        buttonLayout->addWidget(statusWidget);
    }

    // Leave button (viewer mode) or Disconnect button (streamer mode)
    if (m_mode == ViewerMode) {
        m_leaveButton = new QPushButton("Leave");
        m_leaveButton->setStyleSheet(R"(
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
        buttonLayout->addWidget(m_leaveButton);
    } else {
        m_disconnectButton = new QPushButton("Disconnect");
        m_disconnectButton->setStyleSheet(R"(
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
        buttonLayout->addWidget(m_disconnectButton);
    }

    mainLayout->addLayout(buttonLayout);

    // Connect signals
    if (m_startStopButton) {
        connect(m_startStopButton, &QPushButton::clicked, this, &StreamControlPanel::onStartStopClicked);
    }
    if (m_leaveButton) {
        connect(m_leaveButton, &QPushButton::clicked, this, &StreamControlPanel::onLeaveClicked);
    }
    if (m_disconnectButton) {
        connect(m_disconnectButton, &QPushButton::clicked, this, &StreamControlPanel::onDisconnectClicked);
    }
}

// Новый метод для установки состояния трансляции
void StreamControlPanel::setStreamState(StreamState state)
{
    if (m_streamState == state) return;
    
    m_streamState = state;
    updateUI();
}


// Полностью перерабатываем updateUI для отражения состояний
void StreamControlPanel::updateUI()
{
    // Обновляем stream ID
    if (!m_streamId.isEmpty()) {
        m_streamIdLabel->setText(QString("Stream ID: %1").arg(m_streamId));
    }
    m_startStopButton->setEnabled(true);

    if (m_mode == StreamerMode) {
        // Streamer mode - отображаем состояния трансляции
        switch (m_streamState) {
        case StateNoStream:
            m_statusLabel->setText("Status: Ready to stream");
            m_statusLabel->setStyleSheet("font-size: 11px; color: #FFA000;"); // Оранжевый
            if (m_startStopButton) {
                m_startStopButton->setText("Start Stream");
                m_startStopButton->setEnabled(m_active);
                m_startStopButton->setStyleSheet(R"(
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
            }
            break;
            
        case StateStreamCreated:
            m_statusLabel->setText("Status: Waiting for viewers...");
            m_statusLabel->setStyleSheet("font-size: 11px; color: #FFA000;"); // Оранжевый
            if (m_startStopButton) {
                m_startStopButton->setText("Stop Stream");
                m_startStopButton->setEnabled(true);
                m_startStopButton->setStyleSheet(R"(
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
            }
            break;
            
        case StateStreamActive:
            if (m_viewersConnected) {
                m_statusLabel->setText("Status: Streaming ● Viewers connected");
                m_statusLabel->setStyleSheet("font-size: 11px; color: #4CAF50;"); // Зеленый
            } else {
                m_statusLabel->setText("Status: Streaming ● No viewers");
                m_statusLabel->setStyleSheet("font-size: 11px; color: #FFA000;"); // Оранжевый
            }
            if (m_startStopButton) {
                m_startStopButton->setText("Stop Stream");
                m_startStopButton->setEnabled(true);
                m_startStopButton->setStyleSheet(R"(
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
            }
            break;
            
        case StateStreamError:
            m_statusLabel->setText("Status: Error ● Check connection");
            m_statusLabel->setStyleSheet("font-size: 11px; color: #f44336;"); // Красный
            if (m_startStopButton) {
                m_startStopButton->setText("Restart Stream");
                m_startStopButton->setEnabled(true);
                m_startStopButton->setStyleSheet(R"(
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
            }
            break;
        }

        // Обновляем статус зрителей
        if (m_viewersLabel) {
            if (m_viewersConnected) {
                m_viewersLabel->setText("Viewers: ● Connected");
                m_viewersLabel->setStyleSheet("font-size: 11px; color: #4CAF50;");
            } else {
                m_viewersLabel->setText("Viewers: ● None");
                m_viewersLabel->setStyleSheet("font-size: 11px; color: #adb5bd;");
            }
        }

        // Кнопка Disconnect всегда активна
        if (m_disconnectButton) {
            m_disconnectButton->setEnabled(true);
        }

    } else {
        // Viewer mode - оставляем существующую логику
        if (m_active) {
            m_statusLabel->setText("Status: Connected");
            m_statusLabel->setStyleSheet("font-size: 11px; color: #4CAF50;");
        } else {
            m_statusLabel->setText("Status: Disconnected");
            m_statusLabel->setStyleSheet("font-size: 11px; color: #f44336;");
        }
        if (m_leaveButton) m_leaveButton->setEnabled(m_active);
    }
}

void StreamControlPanel::setStreamId(const QString &streamId)
{
    m_streamId = streamId;
    updateUI();
}

void StreamControlPanel::setActive(bool active)
{
    m_active = active;
    updateUI();
}

void StreamControlPanel::setStreaming(bool streaming)
{
    m_streaming = streaming;
    updateUI();
}

void StreamControlPanel::setConnectionStatus(bool connected)
{
    if (m_mode == ViewerMode) {
        setActive(connected);
    }
}

void StreamControlPanel::onStartStopClicked()
{
    if (m_streaming) {
        qDebug() << "Stop stream requested for:" << m_streamId;
        emit stopStreamRequested();
    } else {
        qDebug() << "Start stream requested for:" << m_streamId;
        emit startStreamRequested();
    }
}

void StreamControlPanel::onLeaveClicked()
{
    qDebug() << "Leave stream requested for:" << m_streamId;
    emit leaveStreamRequested();
}

void StreamControlPanel::onDisconnectClicked()
{
    qDebug() << "Disconnect requested for device with stream:" << m_streamId;
    emit disconnectRequested();
}

void StreamControlPanel::setViewersStatus(bool hasViewers)
{
    if (m_viewersConnected == hasViewers) return;
    
    m_viewersConnected = hasViewers;
    updateUI();
}
