// streamcontrolpanel.h
#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>

class StreamControlPanel : public QWidget
{
    Q_OBJECT

public:
    enum Mode {
        StreamerMode,  // Режим стримера
        ViewerMode     // Режим зрителя
    };

    explicit StreamControlPanel(Mode mode, QWidget *parent = nullptr);

    // Общие методы
    void setStreamId(const QString &streamId);
    void setActive(bool active);
    
    // Специфичные для StreamerMode
    void setStreaming(bool streaming);
    void setViewersCount(int count);
    
    // Специфичные для ViewerMode
    void setConnectionStatus(bool connected);

    QString getStreamId() const { return m_streamId; }

signals:
    void startStreamRequested();
    void stopStreamRequested();
    void leaveStreamRequested();
    void disconnectRequested();

private slots:
    void onStartStopClicked();
    void onLeaveClicked();
    void onDisconnectClicked();

private:
    void setupUI();
    void updateUI();

    Mode m_mode;
    QLabel *m_streamIdLabel;
    QLabel *m_statusLabel;
    QLabel *m_viewersLabel;
    QPushButton *m_startStopButton;
    QPushButton *m_leaveButton;
    QPushButton *m_disconnectButton;
    
    QString m_streamId;
    bool m_active;
    bool m_streaming;
    int m_viewersCount;
    
};