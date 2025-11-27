// streamselectorwidget.cpp (заглушка)
#include "streamselectorwidget.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QDebug>

StreamSelectorWidget::StreamSelectorWidget(QWidget *parent)
    : QWidget(parent)
    , m_streamsList(nullptr)
{
    setupUI();
}

// В streamselectorwidget.cpp - добавляем базовую функциональность
void StreamSelectorWidget::setupUI()
{
    auto layout = new QVBoxLayout(this);
    layout->setSpacing(8);
    layout->setContentsMargins(0, 0, 0, 0);

    // Заголовок
    auto header = new QLabel("Available Streams:", this);
    header->setStyleSheet("color: #ffffff; font-weight: bold;");
    layout->addWidget(header);

    m_streamsList = new QListWidget(this);
    m_streamsList->setStyleSheet(R"(
        QListWidget {
            background: #1e1e1e;
            border: 1px solid #444;
            border-radius: 4px;
            color: #ffffff;
            font-size: 12px;
        }
        QListWidget::item {
            padding: 8px;
            border-bottom: 1px solid #333;
        }
        QListWidget::item:selected {
            background: #1976D2;
        }
        QListWidget::item:hover {
            background: #2d2d2d;
        }
    )");

    // Подключаем двойной клик для просмотра стрима
    connect(m_streamsList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
        if (item) {
            uint32_t streamId = item->data(Qt::UserRole).toUInt();
            emit watchStreamRequested(streamId);
        }
    });

    layout->addWidget(m_streamsList);

    // Кнопка "Stop Watching" для текущего выбранного стрима
    auto stopButton = new QPushButton("Stop Watching Selected Stream", this);
    stopButton->setStyleSheet(R"(
        QPushButton {
            background-color: #d32f2f;
            color: white;
            border: none;
            padding: 8px;
            border-radius: 4px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #c62828;
        }
    )");
    
    connect(stopButton, &QPushButton::clicked, this, [this]() {
        QListWidgetItem* currentItem = m_streamsList->currentItem();
        if (currentItem) {
            uint32_t streamId = currentItem->data(Qt::UserRole).toUInt();
            emit stopWatchingRequested(streamId);
        }
    });

    layout->addWidget(stopButton);
}

void StreamSelectorWidget::addStream(uint32_t streamId, const QString& displayId)
{
    // Check if stream already exists
    for (int i = 0; i < m_streamsList->count(); ++i) {
        QListWidgetItem *item = m_streamsList->item(i);
        if (item->data(Qt::UserRole).toUInt() == streamId) {
            item->setText(QString("%1 (ID: %2)").arg(displayId).arg(streamId));
            return;
        }
    }

    // Add new stream
    QListWidgetItem *item = new QListWidgetItem(
        QString("%1 (ID: %2)").arg(displayId).arg(streamId), m_streamsList);
    item->setData(Qt::UserRole, streamId);
    
    // Add watch button to the item (simplified - in real implementation might use custom widget)
    m_streamsList->addItem(item);
}

void StreamSelectorWidget::removeStream(uint32_t streamId)
{
    for (int i = 0; i < m_streamsList->count(); ++i) {
        QListWidgetItem *item = m_streamsList->item(i);
        if (item->data(Qt::UserRole).toUInt() == streamId) {
            delete m_streamsList->takeItem(i);
            break;
        }
    }
}

int StreamSelectorWidget::getStreamCount() const
{
    return m_streamsList->count();
}

// In real implementation, these would be connected to buttons in custom list items
void StreamSelectorWidget::onWatchButtonClicked()
{
    // This would be connected to watch buttons in the list
    // For now, emit signal when item is double-clicked
}

void StreamSelectorWidget::onStopWatchingButtonClicked()
{
    // This would be connected to stop watching buttons
}
