#include "viewerwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QDebug>
#include "../video_defaults.h"

#define TESTING_NETCODE
#include <QThread>
#undef TESTING_NETCODE
const QString ViewerWidget::PLACEHOLDER_TEXT = "Waiting for video stream...";
const QString ViewerWidget::STATUS_ACTIVE = "● Connected";
const QString ViewerWidget::STATUS_INACTIVE = "● Disconnected";

ViewerWidget::ViewerWidget(uint32_t streamId, const QString &displayId, uint32_t callId, QWidget *parent)
    : QWidget(parent)
    , m_videoDisplay(nullptr)
    , m_controlPanel(nullptr)
    , m_controlLayout(nullptr)
    , m_streamIdLabel(nullptr)
    , m_statusLabel(nullptr)
    , m_leaveButton(nullptr)
    , m_mainLayout(nullptr)
    , m_bufferedDecoder(nullptr)
    , m_networkManager(nullptr)
    , m_streamId(streamId)
    , m_displayId(displayId)
    , m_callId(callId)
    , m_active(false)
{
    qDebug() << "=== ViewerWidget::ViewerWidget() START ===";
    qDebug() << "Stream ID:" << streamId << "Display ID:" << displayId << "Call ID:" << callId;
    
    setupUI();
    qDebug() << "setupUI() completed";
    
    setupConnections();
    qDebug() << "setupConnections() completed";
    
    qDebug() << "=== ViewerWidget::ViewerWidget() END ===";
}

ViewerWidget::~ViewerWidget()
{
    cleanup();
}

void ViewerWidget::setupUI()
{
    qDebug() << "=== ViewerWidget::setupUI() START ===";
    
    setStyleSheet(R"(
        ViewerWidget {
            background: #1e1e1e;
            border: 1px solid #444;
            border-radius: 8px;
            margin: 2px;
        }
    )");
    
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(0);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);

    // Video display
    m_videoDisplay = new VideoDisplay(this);
    m_videoDisplay->setStyleSheet(R"(
        VideoDisplay {
            background: #000000;
            border: none;
            border-radius: 6px;
            margin: 4px;
        }
    )");

    // Создаем простую панель управления для ViewerWidget
    m_controlPanel = new QWidget(this);
    m_controlPanel->setFixedHeight(40);
    m_controlPanel->setStyleSheet(R"(
        QWidget {
            background: #2d2d2d;
            border: none;
            border-top: 1px solid #444;
            border-bottom-left-radius: 6px;
            border-bottom-right-radius: 6px;
        }
    )");
    
    m_controlLayout = new QHBoxLayout(m_controlPanel);
    m_controlLayout->setContentsMargins(8, 4, 8, 4);
    m_controlLayout->setSpacing(8);
    
    // Stream ID label
    m_streamIdLabel = new QLabel(m_controlPanel);
    m_streamIdLabel->setStyleSheet(R"(
        QLabel {
            color: #cccccc;
            font-weight: bold;
            padding: 4px;
        }
    )");
    m_streamIdLabel->setText(m_displayId);
    
    // Status label
    m_statusLabel = new QLabel(m_controlPanel);
    m_statusLabel->setStyleSheet(R"(
        QLabel {
            color: #888888;
            padding: 4px;
        }
    )");
    m_statusLabel->setText(STATUS_INACTIVE);
    
    // Leave button
    m_leaveButton = new QPushButton("Leave", m_controlPanel);
    m_leaveButton->setFixedSize(60, 24);
    m_leaveButton->setStyleSheet(R"(
        QPushButton {
            background: #444;
            color: white;
            border: none;
            border-radius: 4px;
            font-weight: bold;
        }
        QPushButton:hover {
            background: #555;
        }
        QPushButton:pressed {
            background: #333;
        }
        QPushButton:disabled {
            background: #2a2a2a;
            color: #666;
        }
    )");
    m_leaveButton->setEnabled(false);
    
    // Добавляем элементы на панель управления
    m_controlLayout->addWidget(m_streamIdLabel);
    m_controlLayout->addWidget(m_statusLabel);
    m_controlLayout->addStretch();
    m_controlLayout->addWidget(m_leaveButton);
    
    // Добавляем основные виджеты в основной layout
    m_mainLayout->addWidget(m_videoDisplay, 1);
    m_mainLayout->addWidget(m_controlPanel);
    
    qDebug() << "=== ViewerWidget::setupUI() END ===";
}

void ViewerWidget::setupConnections()
{
    qDebug() << "ViewerWidget::setupConnections for" << m_displayId;
    
    if (m_leaveButton) {
        connect(m_leaveButton, &QPushButton::clicked,
                this, &ViewerWidget::onLeaveButtonClicked);
    }
}

void ViewerWidget::initialize()
{
    qDebug() << "=== ViewerWidget::initialize() START ===";
    qDebug() << "Stream:" << m_displayId << "ID:" << m_streamId;
    qDebug() << "NetworkManager ptr:" << m_networkManager;
    qDebug() << "BufferedDecoder ptr:" << m_bufferedDecoder;

    if (!m_networkManager) {
        qWarning() << "ViewerWidget::initialize: NetworkManager not set for" << m_displayId;
        m_videoDisplay->setPlaceholderText("Waiting for network connection...");
        return;
    }

    m_networkManager->setCallId(m_callId);

    if (!m_bufferedDecoder) {
        m_bufferedDecoder = new BufferedVideoDecoder(
            DEFAULT_WIDTH, DEFAULT_HEIGHT, DEFAULT_FPS, -1, this
        );

        connect(m_bufferedDecoder, &BufferedVideoDecoder::frameReady,
                this, &ViewerWidget::onFrameReady, Qt::QueuedConnection);

        connect(m_bufferedDecoder, &BufferedVideoDecoder::errorOccurred,
                this, [this](const QString& error) {
                    qCritical() << "BufferedVideoDecoder error for stream" << m_displayId << ":" << error;
                    showError(QString("Decoder error: %1").arg(error));
                }, Qt::QueuedConnection);

        m_bufferedDecoder->initialize();
        qDebug() << "BufferedVideoDecoder created for" << m_displayId;
    }

    disconnect(m_networkManager, nullptr, this, nullptr);
    bool ok = connect(m_networkManager, &NetworkManager::frameAssembled,
                      this, &ViewerWidget::onFrameAssembled, Qt::QueuedConnection);
    if (!ok) {
        qWarning() << "Failed to connect NetworkManager::frameAssembled for" << m_displayId;
    }

    setActive(true);
    updateStatus();
   // connect(m_networkManager, &NetworkManager::frameAssembled,
   //     m_bufferedDecoder, &BufferedVideoDecoder::addFrame);
    qDebug() << "ViewerWidget initialized with BufferedVideoDecoder for stream:" << m_displayId;
}

void ViewerWidget::cleanup()
{
    qDebug() << "Cleaning up ViewerWidget for stream:" << m_displayId;

    setActive(false);

    if (m_networkManager) {
        m_networkManager->disconnect(this);
    }

    if (m_bufferedDecoder) {
        m_bufferedDecoder->cleanup();
        m_bufferedDecoder->clear();
        delete m_bufferedDecoder;
        m_bufferedDecoder = nullptr;
        qDebug() << "BufferedVideoDecoder cleaned up";
    }

    clearDisplay();
}

void ViewerWidget::setNetworkManager(NetworkManager* networkManager)
{
    if (m_networkManager == networkManager) return;

    if (m_networkManager) {
        m_networkManager->disconnect(this);
    }

    m_networkManager = networkManager;

    if (!m_networkManager) {
        qDebug() << "ViewerWidget::setNetworkManager called with nullptr for" << m_displayId;
        m_videoDisplay->setPlaceholderText("Waiting for network connection...");
        return;
    }

    if (!m_bufferedDecoder) {
        m_bufferedDecoder = new BufferedVideoDecoder(
            DEFAULT_WIDTH, DEFAULT_HEIGHT, DEFAULT_FPS, -1, this
        );

        connect(m_bufferedDecoder, &BufferedVideoDecoder::frameReady,
                this, &ViewerWidget::onFrameReady, Qt::QueuedConnection);

        connect(m_bufferedDecoder, &BufferedVideoDecoder::errorOccurred,
                this, [this](const QString& error) {
                    qCritical() << "BufferedVideoDecoder error for stream" << m_displayId << ":" << error;
                    showError(QString("Decoder error: %1").arg(error));
                }, Qt::QueuedConnection);

        m_bufferedDecoder->initialize();
    }

    bool ok = connect(m_networkManager, &NetworkManager::frameAssembled,
                      this, &ViewerWidget::onFrameAssembled, Qt::QueuedConnection);
    if (!ok) {
        qWarning() << "ViewerWidget: failed to connect frameAssembled signal for stream:" << m_displayId;
    } else {
        qDebug() << "ViewerWidget connected to NetworkManager for stream:" << m_streamId;
    }

    if (!m_active) {
        setActive(true);
        updateStatus();
    }
}

void ViewerWidget::setActive(bool active)
{
    if (m_active == active) return;

    m_active = active;
    updateStatus();
    
    if (active && m_bufferedDecoder) {
        m_bufferedDecoder->clear();
    }
}

void ViewerWidget::setStreamId(uint32_t streamId, const QString &displayId)
{
    m_streamId = streamId;
    m_displayId = displayId;

    if (m_streamIdLabel) {
        m_streamIdLabel->setText(displayId);
    }

    updateStatus();
}

void ViewerWidget::displayFrame(const QImage &frame)
{
    if (!m_videoDisplay || !m_active) return;

    if (frame.isNull()) {
        qWarning() << "Received null frame for stream:" << m_displayId;
        return;
    }

    m_videoDisplay->displayFrame(frame);
}

void ViewerWidget::clearDisplay()
{
    if (m_videoDisplay) {
        m_videoDisplay->clear();
        m_videoDisplay->setPlaceholderText(PLACEHOLDER_TEXT);
    }
}

void ViewerWidget::updateStatus()
{
    if (m_statusLabel) {
        if (m_active) {
            m_statusLabel->setText(STATUS_ACTIVE);
            m_statusLabel->setStyleSheet(R"(
                QLabel {
                    color: #4CAF50;
                    padding: 4px;
                }
            )");
        } else {
            m_statusLabel->setText(STATUS_INACTIVE);
            m_statusLabel->setStyleSheet(R"(
                QLabel {
                    color: #888888;
                    padding: 4px;
                }
            )");
        }
    }
    
    if (m_leaveButton) {
        m_leaveButton->setEnabled(m_active);
    }
}

void ViewerWidget::onFrameAssembled(int streamId, int frameNumber, const QByteArray &frameData)
{
    if (streamId != static_cast<int>(m_streamId)) return;
    if (!m_active) return;
    if (!m_bufferedDecoder) {
        qWarning() << "Received frame but bufferedDecoder is null for stream:" << m_displayId;
        return;
    }
//#ifdef TESTING_NETCODE
//    const int BUF_SIZE = 4000;
//    int copyLen = qMin<int>(frameData.size(), BUF_SIZE - 1); // оставляем 1 байт под '\0'
//    char buffer[BUF_SIZE];
//    if (copyLen > 0) {
//        memcpy(buffer, frameData.constData(), copyLen);
//    }
//    buffer[copyLen] = '\0';
//    printf("ViewerWidget: RECVD (size=%d, shown=%d): %s\n",
//           static_cast<int>(frameData.size()), copyLen, buffer);
//    qDebug() << "ViewerWidget::onFrameAssembled thread:" << QThread::currentThread()
//         << "decoder thread:" << (m_bufferedDecoder ? m_bufferedDecoder->thread() : nullptr)
//         << "networkmanager thread:" << (m_networkManager->thread());
//
//
//#endif
    m_bufferedDecoder->addFrame(streamId, frameNumber, frameData);

    if (frameNumber % 30 == 0) {
        qDebug() << "ViewerWidget: Added frame" << frameNumber
                 << "size:" << frameData.size() << "bytes to decoder for stream:" << m_displayId;
    }
}

void ViewerWidget::onFrameReady(const QImage &frame, int frameNumber)
{
    Q_UNUSED(frameNumber)
    displayFrame(frame);
}

void ViewerWidget::onLeaveRequested()
{
    qDebug() << "Leave requested for stream:" << m_displayId;
    
    setActive(false);
    clearDisplay();
    
    if (m_bufferedDecoder) {
        m_bufferedDecoder->clear();
    }
    
    emit streamLeft(m_streamId);
}

void ViewerWidget::onLeaveButtonClicked()
{
    qDebug() << "Leave button clicked for stream:" << m_displayId;
    onLeaveRequested();
}

void ViewerWidget::onStreamJoined(uint32_t streamId)
{
    if (streamId == m_streamId) {
        setActive(true);
        qDebug() << "ViewerWidget: successfully joined stream" << m_streamId;
    }
}

void ViewerWidget::onStreamLeft(uint32_t streamId)
{
    if (streamId == m_streamId) {
        setActive(false);
        clearDisplay();
        
        if (m_bufferedDecoder) {
            m_bufferedDecoder->clear();
        }
        
        qDebug() << "ViewerWidget: left stream" << m_streamId;
    }
}

void ViewerWidget::onNetworkError(const QString& error)
{
    qCritical() << "Network error for viewer stream" << m_streamId << ":" << error;
    setActive(false);
    
    if (m_videoDisplay) {
        m_videoDisplay->setPlaceholderText(QString("Network error: %1").arg(error));
    }
    
    if (m_statusLabel) {
        m_statusLabel->setText("● Error");
        m_statusLabel->setStyleSheet(R"(
            QLabel {
                color: #f44336;
                padding: 4px;
            }
        )");
    }
}

void ViewerWidget::showError(const QString &message)
{
    qDebug() << "ViewerWidget Error for stream" << m_displayId << ":" << message;
    
    if (m_statusLabel) {
        m_statusLabel->setText("● Error");
        m_statusLabel->setStyleSheet(R"(
            QLabel {
                color: #f44336;
                padding: 4px;
            }
        )");
    }
}