#pragma once

#include <QObject>
#include <QMap>
#include <QTimer>
#include "tcpclient.h"
#include "../networkmanager.h"
#include "streamwindow.h"

// Forward declarations
class StreamPublisherWindow;
class StreamViewerWindow;

class StreamManager : public QObject
{
    Q_OBJECT

public:
    explicit StreamManager(QObject *parent = nullptr);
    ~StreamManager();

    void initialize();
    void cleanup();

    void createStream(int deviceIndex);
    void joinStream(const QString &streamId);
    void deleteStream(int streamId);
    void leaveStream(int streamId);

    void setServerAddress(const QString &address, quint16 port);
    void connectToServer();
    void disconnectFromServer();

    bool isStreamActive(int streamId) const;
    QVector<int> getActiveStreams() const;
    QString getStreamStatus(int streamId) const;

signals:
    void streamWindowCreated(StreamWindow *window);
    void streamWindowClosed(int streamId);
    void connectionStatusChanged(bool connected);
    void errorOccurred(const QString &message);

private slots:
    void onStreamStopped(int streamId);
    void onStreamLeft(int streamId);
    void onWindowClosed(int streamId);

private:
    bool m_connectedToServer;
    int m_nextStreamId;
    QMap<int, StreamWindow*> m_openWindows;
};
