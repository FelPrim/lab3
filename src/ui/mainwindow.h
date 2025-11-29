#pragma once

#include "videogridwidget.h"
#include <QMainWindow>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include "videogridwidget.h"
#include "maincontrolpanel.h"
#include "videoselectiondialog.h"
#include "../network/streammanager.h"
#include "id_utils.h"
#include "conferencewindow.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void initialize();
    void cleanup();



    // Слоты для MainControlPanel
    void onAddDeviceRequested();
    void onCreateConferenceRequested();
    void onJoinConferenceRequested(const QString& conferenceId);
    void onJoinPublicStreamRequested(const QString& streamId);
    
    // Слоты для VideoGridWidget
    void onStreamerDisconnectRequested(int deviceIndex);
    void onViewerLeaveRequested(uint32_t streamId);
    void onStreamStartRequested(int deviceIndex);
    void onStreamStopRequested(uint32_t streamId);

    // Слоты для VideoSelectionDialog
    void onDeviceSelected(int deviceIndex);
    
    // Слоты для StreamManager
    void onConnectionStatusChanged(bool connected);
    void onErrorOccurred(const QString& message);
private slots:
    // Слоты для StreamManager - ОБНОВИТЬ сигнатуры:
    void onServerStreamCreated(uint32_t streamId);
    void onServerStreamStart(uint32_t streamId);
    void onServerStreamEnd(uint32_t streamId);
    void onServerStreamJoined(uint32_t streamId);     
    void onServerStreamDeleted(uint32_t streamId); 

private:
    void setupUI();
    void setupConnections();
    
    // Вспомогательные методы
    void createConferenceWindow(uint32_t callId, const QString& displayId);
    void showError(const QString& message);
    void updateStatus();

    // Основные компоненты
    QSplitter* m_mainSplitter;
    MainControlPanel* m_controlPanel;
    VideoGridWidget* m_videoGrid;
    
    // Диалоги
    VideoSelectionDialog* m_videoSelectionDialog;
    
    // Менеджеры
    StreamManager* m_streamManager;
    
    // Состояние
    bool m_initialized;
    bool m_connectedToServer;
    
    // Счетчики
    int m_nextDeviceIndex;
    QMap<uint32_t, QString> m_activeConferences;

public slots:
    void onStreamWindowCreated(QWidget* window) {
        qDebug() << "Stream window created:" << window; // Заглушка
    }
    
    void onStreamWindowClosed(uint32_t streamId) {
        qDebug() << "Stream window closed:" << streamId; // Заглушка
    }

private slots:
    void onViewerWidgetCreated(uint32_t streamId) {
        qDebug() << "MainWindow: viewer widget created for stream" << streamId;
    }
    
    void onViewerWidgetClosed(uint32_t streamId) {
        qDebug() << "MainWindow: viewer widget closed for stream" << streamId;
    }
    
    void onStreamerWidgetCreated(int deviceIndex, uint32_t streamId) {
        qDebug() << "MainWindow: streamer widget created for device" << deviceIndex << "stream" << streamId;
    }
    
    void onStreamerWidgetClosed(int deviceIndex) {
        qDebug() << "MainWindow: streamer widget closed for device" << deviceIndex;
    }
private:
    // Вспомогательные методы для управления устройствами
    void refreshAvailableDevices();
    void updateDeviceAvailability();
    void initializeStreamerForDevice(int deviceIndex);
    
    // Состояние
    QList<int> m_availableDevices;    // Все доступные устройства
    QList<int> m_usedDevices;         // Используемые устройства
    
private slots:
    void onConferenceClosed(uint32_t callId);
    void onConferenceJoined(uint32_t callId, const QString& displayId);
};
