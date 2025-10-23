#pragma once

#include <QMainWindow>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <opencv2/opencv.hpp>
#include "capturethread.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void refreshDevices();
    void startCapture();
    void stopCapture();
    void onFrame(const QImage &img);
    void onError(const QString &msg);

private:
    QComboBox *m_comboDevices;
    QPushButton *m_btnStart;
    QPushButton *m_btnStop;
    QPushButton *m_btnRefresh;
    QLabel *m_videoLabel;
    QLabel *m_infoLabel;

    CaptureThread *m_thread = nullptr;
    QList<int> m_deviceIndices;
};
