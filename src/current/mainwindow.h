#pragma once

#include <QMainWindow>
#include <QPushButton>
#include <QLabel>
#include <QGridLayout>
#include <QResizeEvent>
#include "capturethread.h"
#include "videolayoutcalculator.h"

class VideoSelectionDialog;
class RemoveVideoDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void refreshDevices();
    void addVideo();
    void removeVideo();
    void onFrame(int streamIndex, const QImage &img);
    void onError(const QString &msg);
    void updateVideoLayout();

private:
    void setupUI();
    void setupConnections();
    
    QPushButton *m_btnAddVideo;
    QPushButton *m_btnRemoveVideo;
    QPushButton *m_btnRefresh;
    QLabel *m_infoLabel;
    QWidget *m_videoContainer;
    QGridLayout *m_videoLayout;

    QVector<QLabel*> m_videoLabels;
    QVector<CaptureThread*> m_captureThreads;
    QList<int> m_availableDevices;
    QList<int> m_usedDevices;
    
    const int MARGIN = 5;
};