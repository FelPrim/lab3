#include "deviceselectorwidget.h"
#include <QHBoxLayout>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QDebug>

#include <opencv2/opencv.hpp>

DeviceSelectorWidget::DeviceSelectorWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    populateDevices();
}

void DeviceSelectorWidget::setupUI()
{
    m_deviceComboBox = new QComboBox(this);
    m_refreshButton = new QPushButton("Refresh", this);

    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(new QLabel("Device:", this));
    layout->addWidget(m_deviceComboBox);
    layout->addWidget(m_refreshButton);

    connect(m_deviceComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DeviceSelectorWidget::onDeviceSelected);
    connect(m_refreshButton, &QPushButton::clicked, this, &DeviceSelectorWidget::onRefreshClicked);
}

void DeviceSelectorWidget::populateDevices()
{
    m_deviceComboBox->clear();
    m_availableDevices.clear();

    qDebug() << "DeviceSelectorWidget: scanning for video devices...";

    // Проверяем индексы 0..9 (как в старом проекте)
    for (int i = 0; i < 10; ++i) {
        cv::VideoCapture cap;
#ifdef _WIN32
        try {
            if (!cap.open(i, cv::CAP_DSHOW)) continue;
        } catch (...) {
            continue;
        }
#elif __linux__
        if (!cap.open(i, cv::CAP_V4L2)) continue;
#elif __APPLE__
        if (!cap.open(i, cv::CAP_AVFOUNDATION)) continue;
#else
        if (!cap.open(i)) continue;
#endif
        if (cap.isOpened()) {
            m_availableDevices.append(i);
            QString name = QString("Camera #%1").arg(i);
            m_deviceComboBox->addItem(name, i);
            qDebug() << "DeviceSelectorWidget: found device:" << i;
            cap.release();
        }
    }

    if (m_availableDevices.isEmpty()) {
        m_deviceComboBox->addItem("No cameras found", -1);
        m_deviceComboBox->setEnabled(false);
    } else {
        m_deviceComboBox->setEnabled(true);
    }
}

int DeviceSelectorWidget::getSelectedDeviceIndex() const
{
    if (m_deviceComboBox->count() == 0) return -1;
    int data = m_deviceComboBox->currentData().toInt();
    return data;
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
            return;
        }
    }
}

void DeviceSelectorWidget::onDeviceSelected(int index)
{
    Q_UNUSED(index)
    int device = getSelectedDeviceIndex();
    emit deviceSelected(device);
}

void DeviceSelectorWidget::onRefreshClicked()
{
    refreshDevices();
}
