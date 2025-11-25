// deviceselectorwidget.cpp
#include <opencv2/opencv.hpp>
#include "deviceselectorwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QDebug>

DeviceSelectorWidget::DeviceSelectorWidget(QWidget *parent)
    : QWidget(parent)
    , m_deviceComboBox(nullptr)
    , m_refreshButton(nullptr)
{
    setupUI();
    populateDevices();
}

void DeviceSelectorWidget::setupUI()
{
    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    
    m_deviceComboBox = new QComboBox(this);
    m_deviceComboBox->setStyleSheet(R"(
        QComboBox {
            background: #2d2d2d;
            border: 1px solid #555;
            border-radius: 4px;
            padding: 6px;
            color: white;
            min-width: 120px;
        }
        QComboBox::drop-down {
            border: none;
            width: 20px;
        }
        QComboBox::down-arrow {
            image: none;
            border-left: 1px solid #555;
        }
        QComboBox QAbstractItemView {
            background: #2d2d2d;
            border: 1px solid #555;
            color: white;
            selection-background-color: #1976D2;
        }
    )");
    
    m_refreshButton = new QPushButton("Refresh", this);
    m_refreshButton->setStyleSheet(R"(
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
    )");
    
    layout->addWidget(m_deviceComboBox);
    layout->addWidget(m_refreshButton);
    
    connect(m_deviceComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DeviceSelectorWidget::onDeviceSelected);
    connect(m_refreshButton, &QPushButton::clicked,
            this, &DeviceSelectorWidget::onRefreshClicked);
}

void DeviceSelectorWidget::populateDevices()
{
    m_deviceComboBox->clear();
    m_availableDevices = {0, 1, 2}; // Example devices
    
    for (int device : m_availableDevices) {
        m_deviceComboBox->addItem(QString("Camera #%1").arg(device), device);
    }
}

int DeviceSelectorWidget::getSelectedDeviceIndex() const
{
    if (m_deviceComboBox->currentIndex() >= 0) {
        return m_deviceComboBox->currentData().toInt();
    }
    return -1;
}

void DeviceSelectorWidget::refreshDevices()
{
    populateDevices();
    emit refreshRequested();
}

void DeviceSelectorWidget::setCurrentDevice(int deviceIndex)
{
    int index = m_deviceComboBox->findData(deviceIndex);
    if (index >= 0) {
        m_deviceComboBox->setCurrentIndex(index);
    }
}

void DeviceSelectorWidget::onDeviceSelected(int index)
{
    if (index >= 0) {
        int deviceIndex = m_deviceComboBox->itemData(index).toInt();
        qDebug() << "Device selected:" << deviceIndex;
        emit deviceSelected(deviceIndex);
    }
}

void DeviceSelectorWidget::onRefreshClicked()
{
    refreshDevices();
}