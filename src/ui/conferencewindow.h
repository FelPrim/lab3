#pragma once

#include "videogridwidget.h"
#include <QMainWindow>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include "videogridwidget.h"
#include "conferencecontrolpanel.h"
#include "videoselectiondialog.h"
#include "../network/streammanager.h"
#include "id_utils.h"

class ConferenceWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ConferenceWindow(uint32_t callId, const QString& displayId, QWidget* parent = nullptr);
    ~ConferenceWindow();

    void initialize();
    void cleanup();

    void setConferenceInfo(uint32_t callId, const QString& displayId);
    void addParticipant(uint32_t participantId);
    void removeParticipant(uint32_t participantId);
    void addAvailableStream(uint32_t streamId, const QString& displayId);
    void removeAvailableStream(uint32_t streamId);

private slots:
    // Слоты для ConferenceControlPanel
    void onAddDeviceRequested();
    void onLeaveConferenceRequested();
    void onWatchStreamRequested(uint32_t streamId);
    void onStopWatchingRequested(uint32_t streamId);
    
    // Слоты для VideoGridWidget
    void onStreamerDisconnectRequested(int deviceIndex);
    void onViewerLeaveRequested(uint32_t streamId);
    void onStreamStartRequested(int deviceIndex);
    void onStreamStopRequested(uint32_t streamId);
    void onEncodedPacketReady(uint32_t streamId, int frameNumber, const QByteArray& packet);

    // Слоты для VideoSelectionDialog
    void onDeviceSelected(int deviceIndex);
    
    // Слоты для StreamManager
    void onStreamCreated(uint32_t streamId);
    void onStreamDeleted(uint32_t streamId);
    void onStreamJoined(uint32_t streamId);
    void onErrorOccurred(const QString& message);

private:
    void setupUI();
    void setupConnections();
    
    // Вспомогательные методы
    void startStreamInConference(int deviceIndex);
    void joinStreamInConference(uint32_t streamId);
    void updateConferenceInfo();

    // Основные компоненты
    QSplitter* m_mainSplitter;
    ConferenceControlPanel* m_controlPanel;
    VideoGridWidget* m_videoGrid;
    
    // Диалоги
    VideoSelectionDialog* m_videoSelectionDialog;
    
    // Менеджеры
    StreamManager* m_streamManager;
    
    // Состояние конференции
    uint32_t m_callId;
    QString m_displayId;
    bool m_initialized;
    
    // Участники и трансляции
    QVector<uint32_t> m_participants;
    QMap<uint32_t, QString> m_availableStreams;
    QMap<uint32_t, QString> m_watchedStreams;
    int m_nextDeviceIndex;

public slots:
    void onConnectionStatusChanged(bool connected) {
        qDebug() << "Connection status:" << connected; // Заглушка
    }
    
    void onStreamWindowCreated(QWidget* window) {
        qDebug() << "Stream window created:" << window; // Заглушка
    }
    
    void onStreamWindowClosed(uint32_t streamId) {
        qDebug() << "Stream window closed:" << streamId; // Заглушка
    }
    
    void showError(const QString& message) {
        qDebug() << "Error:" << message; // Заглушка
    }
    
protected:
    void closeEvent(QCloseEvent* event) override;
signals:
    void conferenceClosed(uint32_t callId);
    void conferenceJoined(uint32_t callId, const QString& displayId);

};
