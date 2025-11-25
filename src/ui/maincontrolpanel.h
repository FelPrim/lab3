// maincontrolpanel.h
#pragma once

#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include "streamidinputwidget.h"

class MainControlPanel : public QWidget
{
    Q_OBJECT

public:
    explicit MainControlPanel(QWidget *parent = nullptr);

    void setConnectionStatus(bool connected);

signals:
    void addDeviceRequested();
    void createConferenceRequested();
    void joinConferenceRequested(const QString& conferenceId);
    void joinPublicStreamRequested(const QString& streamId);

private slots:
    void onAddDeviceClicked();
    void onCreateConferenceClicked();

private:
    void setupUI();
    void setupConnections();

    // Кнопки управления
    QPushButton *m_addDeviceBtn;
    QPushButton *m_createConferenceBtn;

    // Поля ввода (с встроенными кнопками Join)
    StreamIdInputWidget *m_conferenceIdInput;
    StreamIdInputWidget *m_streamIdInput;

    // Статус
    QLabel *m_connectionStatusLabel;
};