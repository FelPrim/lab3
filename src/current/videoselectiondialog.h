#pragma once

#include <QDialog>
#include <QListWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>

class VideoSelectionDialog : public QDialog
{
    Q_OBJECT
public:
    explicit VideoSelectionDialog(const QList<int> &availableDevices, QWidget *parent = nullptr);
    int selectedDevice() const { return m_selectedDevice; }

private slots:
    void onDeviceSelected(int index);

private:
    QListWidget *m_listWidget;
    int m_selectedDevice = -1;
};
