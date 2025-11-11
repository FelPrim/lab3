#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>

class StreamControlPanel : public QWidget
{
    Q_OBJECT

public:
    enum Role {
        Publisher,  // Ведущий трансляцию
        Viewer      // Зритель
    };

    explicit StreamControlPanel(Role role, QWidget *parent = nullptr);

    void setStreamId(const QString &streamId);
    void setStatus(const QString &status);
    void setActive(bool active);
    void setViewers(bool hasViewers);

    QString getStreamId() const { return m_streamId; }
    bool isActive() const { return m_active; }

signals:
    void stopRequested();
    void leaveRequested();

private slots:
    void onStopClicked();
    void onLeaveClicked();

private:
    void setupUI();
    void updateUI();

    Role m_role;
    QLabel *m_statusLabel;
    QLabel *m_streamIdLabel;
    QPushButton *m_actionButton;
    
    QString m_streamId;
    bool m_active;
    bool m_hasViewers;
};
