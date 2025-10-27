#include "removevideodialog.h"
#include <QListWidgetItem>

RemoveVideoDialog::RemoveVideoDialog(const QVector<CaptureThread*>& captureThreads, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Удалить видео");
    setModal(true);
    setMinimumWidth(300);
    
    auto layout = new QVBoxLayout(this);
    m_listWidget = new QListWidget(this);
    
    for (int i = 0; i < captureThreads.size(); ++i) {
        CaptureThread* thread = captureThreads[i];
        m_listWidget->addItem(QString("Видео %1 (Камера #%2)").arg(i + 1).arg(thread->getDeviceIndex()));
    }
    
    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    
    layout->addWidget(new QLabel("Выберите видео для удаления:"));
    layout->addWidget(m_listWidget);
    layout->addWidget(buttonBox);
    
    connect(m_listWidget, &QListWidget::currentRowChanged, this, &RemoveVideoDialog::onItemSelected);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    if (m_listWidget->count() > 0) {
        m_listWidget->setCurrentRow(0);
        m_selectedIndex = 0;
    }
}

void RemoveVideoDialog::onItemSelected(int index)
{
    m_selectedIndex = index;
}