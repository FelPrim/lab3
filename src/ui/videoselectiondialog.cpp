#include "videoselectiondialog.h"
#include "../logic/videocapture.h"
#include <QVBoxLayout>
#include <QListWidget>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QLabel>
#include <QDebug>

VideoSelectionDialog::VideoSelectionDialog(QWidget *parent)
    : QDialog(parent)
    , m_listWidget(nullptr)
    , m_selectedDevice(-1)
{
    setWindowTitle("Select Video Device");
    setMinimumSize(350, 300);
    
    auto layout = new QVBoxLayout(this);
    
    // Label
    auto label = new QLabel("Available video devices:", this);
    layout->addWidget(label);
    
    // Refresh button
    auto refreshButton = new QPushButton("🔄 Refresh", this);
    refreshButton->setStyleSheet(R"(
        QPushButton {
            background: #555;
            color: white;
            border: none;
            padding: 6px 12px;
            border-radius: 4px;
            font-weight: bold;
        }
        QPushButton:hover {
            background: #666;
        }
    )");
    layout->addWidget(refreshButton);
    
    // List widget
    m_listWidget = new QListWidget(this);
    m_listWidget->setStyleSheet(R"(
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
    
    // Initial device scan
    refreshDevices();
    
    layout->addWidget(m_listWidget, 1); // Add stretch factor
    
    // Buttons
    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->setStyleSheet(R"(
        QPushButton {
            padding: 8px 16px;
            border-radius: 4px;
            font-weight: bold;
        }
        QPushButton[text="OK"] {
            background: #1976D2;
            color: white;
        }
        QPushButton[text="Cancel"] {
            background: #555;
            color: white;
        }
    )");
    
    layout->addWidget(buttonBox);
    
    // Connections
    connect(refreshButton, &QPushButton::clicked, this, &VideoSelectionDialog::refreshDevices);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &VideoSelectionDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &VideoSelectionDialog::reject);
    connect(m_listWidget, &QListWidget::itemSelectionChanged, this, [this]() {
        if (m_listWidget->currentRow() >= 0) {
            QListWidgetItem* item = m_listWidget->currentItem();
            if (item && item->data(Qt::UserRole).isValid()) {
                m_selectedDevice = item->data(Qt::UserRole).toInt();
                qDebug() << "Device selected in dialog:" << m_selectedDevice;
            }
        }
    });
    
    // Select first device by default
    if (m_listWidget->count() > 0) {
        m_listWidget->setCurrentRow(0);
    }
}

int VideoSelectionDialog::getSelectedDeviceIndex() const
{
    return m_selectedDevice;
}

void VideoSelectionDialog::refreshDevices()
{
    m_listWidget->clear();
    m_selectedDevice = -1;
    
    // Get real available devices using the static method
    QList<int> availableDevices = VideoCapture::getAvailableDevices();
    
    if (availableDevices.isEmpty()) {
        QListWidgetItem *noDevicesItem = new QListWidgetItem("No video devices found");
        noDevicesItem->setFlags(Qt::NoItemFlags); // Make non-selectable
        noDevicesItem->setForeground(Qt::gray);
        m_listWidget->addItem(noDevicesItem);
        qDebug() << "No video devices found during refresh";
        return;
    }
    
    // Add devices to list
    for (int deviceIndex : availableDevices) {
        QListWidgetItem *item = new QListWidgetItem(QString("Camera %1").arg(deviceIndex));
        item->setData(Qt::UserRole, deviceIndex);
        m_listWidget->addItem(item);
    }
    
    qDebug() << "Refreshed device list, found" << availableDevices.size() << "devices";
    
    // Select first device
    if (m_listWidget->count() > 0) {
        m_listWidget->setCurrentRow(0);
    }
}

void VideoSelectionDialog::onDeviceSelected(int index)
{
    if (index >= 0 && index < m_listWidget->count()) {
        QListWidgetItem* item = m_listWidget->item(index);
        if (item && item->data(Qt::UserRole).isValid()) {
            m_selectedDevice = item->data(Qt::UserRole).toInt();
            qDebug() << "Device selected via slot:" << m_selectedDevice;
            emit deviceSelected(m_selectedDevice);
        }
    }
}