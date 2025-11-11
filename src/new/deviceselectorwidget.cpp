#include "deviceselectorwidget.h"
#include <QHBoxLayout>
#include <QComboBox>
#include <QPushButton>
#include <QDebug>

DeviceSelectorWidget::DeviceSelectorWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    populateDevices();
}

void DeviceSelectorWidget::setupUI()
{
    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    
    // Device combo box
    m_deviceComboBox = new QComboBox(this);
    m_deviceComboBox->setStyleSheet("QComboBox { padding: 6px; border: 1px solid #555; border-radius: 4px; background: #2d2d2d; }");
    
    // Refresh button
    m_refreshButton = new QPushButton("🔄", this);
    m_refreshButton->setStyleSheet("QPushButton { padding: 6px 12px; border: 1px solid #555; border-radius: 4px; background: #2d2d2d; }");
    m_refreshButton->setToolTip("Refresh devices");
    
    layout->addWidget(m_deviceComboBox, 1);
    layout->addWidget(m_refreshButton);
    
    connect(m_deviceComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DeviceSelectorWidget::onDeviceSelected);
    connect(m_refreshButton, &QPushButton::clicked, this, &DeviceSelectorWidget::onRefreshClicked);
}

void DeviceSelectorWidget::populateDevices()
{
    m_deviceComboBox->clear();
    
    // Заглушка: имитируем обнаруженные устройства
    m_deviceComboBox->addItem("Camera #0 (Default)", 0);
    m_deviceComboBox->addItem("Camera #1", 1);
    m_deviceComboBox->addItem("Camera #2", 2);
    m_deviceComboBox->addItem("Virtual Camera", 3);
    
    qDebug() << "Populated device list with" << m_deviceComboBox->count() << "devices";
}

int DeviceSelectorWidget::getSelectedDeviceIndex() const
{
    return m_deviceComboBox->currentData().toInt();
}

void DeviceSelectorWidget::refreshDevices()
{
    populateDevices();
    emit refreshRequested();
}

void DeviceSelectorWidget::setCurrentDevice(int deviceIndex)
{
    for (int i = 0; i < m_deviceComboBox->count(); ++i) {
        if (m_deviceComboBox->itemData(i).toInt() == deviceIndex) {
            m_deviceComboBox->setCurrentIndex(i);
            break;
        }
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
    qDebug() << "Refresh devices clicked";
    refreshDevices();
}
