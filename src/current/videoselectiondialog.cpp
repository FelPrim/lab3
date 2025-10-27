#include "videoselectiondialog.h"
#include <QListWidgetItem>

VideoSelectionDialog::VideoSelectionDialog(const QList<int> &availableDevices, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Select Video Device");
    setModal(true);
    setMinimumWidth(300);
    
    auto layout = new QVBoxLayout(this);
    m_listWidget = new QListWidget(this);
    
    for (int device : availableDevices) {
        m_listWidget->addItem(QString("Camera #%1").arg(device));
    }
    
    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    
    layout->addWidget(new QLabel("Available devices:"));
    layout->addWidget(m_listWidget);
    layout->addWidget(buttonBox);
    
    connect(m_listWidget, &QListWidget::currentRowChanged, this, &VideoSelectionDialog::onDeviceSelected);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    if (m_listWidget->count() > 0) {
        m_listWidget->setCurrentRow(0);
    }
}

void VideoSelectionDialog::onDeviceSelected(int index)
{
    m_selectedDevice = index;
}