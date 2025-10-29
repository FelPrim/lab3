#pragma once

#include <QMainWindow>
#include <QPushButton>
#include <QLabel>
#include <QGridLayout>
#include <QResizeEvent>
#include "videocapture.h"
#include "videodisplay.h"
#include "videoencoder.h"
#include "networkmanager.h"
#include "videolayoutcalculator.h"
#include "video_defaults.h"
#include "networkdisplaybuffer.h"

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
    void onFrameAssembled(int streamId, int frameNumber, const QByteArray &frameData);
    void onError(const QString &msg);
    void updateVideoLayout();

private:
    void setupUI();
    void setupConnections();
    void removeVideoAtIndex(int index);
    
    // UI elements
    QPushButton *m_btnAddVideo;
    QPushButton *m_btnRemoveVideo;
    QPushButton *m_btnRefresh;
    QLabel *m_infoLabel;
    QWidget *m_videoContainer;
    QGridLayout *m_videoLayout;

    // Video components
    QVector<VideoCapture*> m_videoCaptures;
    QVector<VideoDisplay*> m_sourceDisplays;    // Прямой показ с камеры
    QVector<VideoDisplay*> m_networkDisplays;   // Показ через сеть (эхо)
    QVector<VideoEncoder*> m_videoEncoders;
    QVector<NetworkDisplayBuffer*> m_networkBuffers;
    // Network
    NetworkManager *m_networkManager = nullptr;
    
    // Device management
    QList<int> m_availableDevices;
    QList<int> m_usedDevices;
    
    const int MARGIN = MARGIN;
};