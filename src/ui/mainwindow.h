#pragma once

#include <QMainWindow>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QMap>
#include "new/streammanager.h"
#include "new/streamwindow.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onStartStreamClicked();
    void onJoinStreamClicked();
    void onStreamWindowCreated(StreamWindow *window);
    void onStreamWindowClosed(int streamId);
    void onConnectionStatusChanged(bool connected);

private:
    void setupUI();
    void setupConnections();
    
    // UI elements
    QPushButton *m_btnStartStream;
    QPushButton *m_btnJoinStream;
    QLabel *m_connectionStatusLabel;
    QLabel *m_infoLabel;
    
    // Stream management
    StreamManager *m_streamManager;
    
    // Track open windows
    QMap<int, StreamWindow*> m_openWindows;
};
