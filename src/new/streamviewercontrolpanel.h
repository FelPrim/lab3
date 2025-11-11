#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>

class StreamViewerControlPanel : public QWidget
{
    Q_OBJECT

public:
    explicit StreamViewerControlPanel(QWidget *parent = nullptr);

    void setStreamId(const QString &streamId);
    void setStatus(const QString &status);
    void setJoined(bool joined);

    QString getStreamId() const { return m_streamId; }

signals:
    void joinRequested(const QString &streamId);
    void leaveRequested();

private slots:
    void onJoinClicked();
    void onLeaveClicked();

private:
    void setupUI();
    void setupConnections();
    void updateUI();

    QLabel *m_statusLabel;
    QLabel *m_streamIdLabel;
    QPushButton *m_joinButton;
    QPushButton *m_leaveButton;
    
    QString m_streamId;
    bool m_joined;
    
    static const QString STATUS_DISCONNECTED;
    static const QString STATUS_CONNECTING;
    static const QString STATUS_CONNECTED;
    static const QString STATUS_ENDED;
};
