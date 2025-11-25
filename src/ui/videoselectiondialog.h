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
    // Унифицированный конструктор - без параметра availableDevices
    explicit VideoSelectionDialog(QWidget *parent = nullptr);
    int selectedDevice() const { return m_selectedDevice; }

private slots:
    void onDeviceSelected(int index);

private:
    QListWidget *m_listWidget;
    int m_selectedDevice = -1;

public:
    int getSelectedDeviceIndex() const;
    void refreshDevices();
    
signals:
    void deviceSelected(int deviceIndex);
};