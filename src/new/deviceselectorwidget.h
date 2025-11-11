#pragma once

#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QHBoxLayout>

class DeviceSelectorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DeviceSelectorWidget(QWidget *parent = nullptr);

    int getSelectedDeviceIndex() const;
    void refreshDevices();
    void setCurrentDevice(int deviceIndex);

signals:
    void deviceSelected(int deviceIndex);
    void refreshRequested();

private slots:
    void onDeviceSelected(int index);
    void onRefreshClicked();

private:
    void setupUI();
    void populateDevices();

    QComboBox *m_deviceComboBox;
    QPushButton *m_refreshButton;
};
