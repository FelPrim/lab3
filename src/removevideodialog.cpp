#include "removevideodialog.h"
#include <QListWidgetItem>

RemoveVideoDialog::RemoveVideoDialog(const QVector<VideoCapture*>& videoCaptures, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Remove Video Stream");
    setModal(true);
    
    auto layout = new QVBoxLayout(this);
    
    auto label = new QLabel("Select video stream to remove:", this);
    layout->addWidget(label);
    
    m_listWidget = new QListWidget(this);
    
    // Заполняем список видеопотоков
    for (int i = 0; i < videoCaptures.size(); ++i) {
        VideoCapture *capture = videoCaptures[i];
        QString itemText = QString("Camera #%1 (Stream %2)").arg(capture->getDeviceIndex()).arg(i);
        m_listWidget->addItem(itemText);
    }
    
    layout->addWidget(m_listWidget);
    
    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttonBox);
    
    connect(m_listWidget, &QListWidget::currentRowChanged, this, &RemoveVideoDialog::onItemSelected);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    // Выбираем первый элемент по умолчанию
    if (m_listWidget->count() > 0) {
        m_listWidget->setCurrentRow(0);
    }
}

void RemoveVideoDialog::onItemSelected(int index)
{
    m_selectedIndex = index;
}


