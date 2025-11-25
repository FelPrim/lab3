// streamselectorwidget.h
#pragma once

#include <QWidget>
#include <QListWidget>

class StreamSelectorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit StreamSelectorWidget(QWidget *parent = nullptr);
    
    void addStream(uint32_t streamId, const QString& displayId);
    void removeStream(uint32_t streamId);
    int getStreamCount() const;

signals:
    void watchStreamRequested(uint32_t streamId);
    void stopWatchingRequested(uint32_t streamId);

private slots:
    void onWatchButtonClicked();
    void onStopWatchingButtonClicked();

private:
    void setupUI();

    QListWidget *m_streamsList;
};