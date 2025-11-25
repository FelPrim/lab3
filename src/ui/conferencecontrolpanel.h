// conferencecontrolpanel.h
#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include "streamselectorwidget.h"

class ConferenceControlPanel : public QWidget
{
    Q_OBJECT

public:
    explicit ConferenceControlPanel(QWidget *parent = nullptr);

    void setConferenceInfo(uint32_t callId, const QString& displayId);
    void setParticipantsCount(int count);
    void setStreamsCount(int count);

    void addAvailableStream(uint32_t streamId, const QString& displayId);
    void removeAvailableStream(uint32_t streamId);

signals:
    void addDeviceRequested();
    void leaveConferenceRequested();
    void watchStreamRequested(uint32_t streamId);
    void stopWatchingRequested(uint32_t streamId);

private slots:
    void onAddDeviceClicked();
    void onLeaveConferenceClicked();
    void onWatchStreamClicked(uint32_t streamId);
    void onStopWatchingClicked(uint32_t streamId);

private:
    void setupUI();
    void setupConnections();

    // Информация о конференции
    QLabel *m_conferenceInfoLabel;
    QLabel *m_participantsLabel;
    QLabel *m_streamsLabel;

    // Управление трансляциями
    StreamSelectorWidget *m_streamSelector;

    // Кнопки управления
    QPushButton *m_addDeviceBtn;
    QPushButton *m_leaveConferenceBtn;

    // Состояние
    uint32_t m_callId;
    QString m_displayId;
    int m_participantsCount;
    int m_streamsCount;
};