#pragma once

#include <QObject>
#include <QMap>
#include <QTimer>
#include <QVector>
#include <cstdint>

#include "tcpclient.h"
#include "../networkmanager.h"
#include "streamwindow.h"
#include "networkfacade.h"
#include "streamidconverter.h"  // Добавляем

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
    void deleteStream(uint32_t streamId);  // Изменено на uint32_t
    void leaveStream(uint32_t streamId);   // Изменено на uint32_t

    void setServerAddress(const QString &address, quint16 port);
    void connectToServer();
    void disconnectFromServer();

    bool isStreamActive(uint32_t streamId) const;  // Изменено на uint32_t
    QVector<uint32_t> getActiveStreams() const;    // Изменено на uint32_t
    QString getStreamStatus(uint32_t streamId) const; // Изменено на uint32_t

    void createViewerWindowForStream(uint32_t streamId);
signals:
    void streamWindowCreated(StreamWindow *window);
    void streamWindowClosed(uint32_t streamId);    // Изменено на uint32_t
    void connectionStatusChanged(bool connected);
    void errorOccurred(const QString &message);

private slots:
    // Existing UI-related handlers
    void onStreamStopped(uint32_t streamId);       // Изменено на uint32_t
    void onStreamLeft(uint32_t streamId);          // Изменено на uint32_t
    void onWindowClosed(uint32_t streamId);        // Изменено на uint32_t

    // Handlers for server messages
    void onServerStreamCreated(uint32_t streamId);
    void onServerStreamDeleted(uint32_t streamId);
    void onServerStreamJoined(uint32_t streamId);
    void onServerStreamStart(uint32_t streamId);
    void onServerStreamEnd(uint32_t streamId);

private:
    bool m_connectedToServer;
    QMap<uint32_t, StreamWindow*> m_openWindows;  // Изменено на uint32_t
    NetworkFacade *m_networkFacade = nullptr;
    
    // Для ожидающих создания стримов
    StreamPublisherWindow *m_pendingWindow = nullptr;
    int m_pendingDeviceIndex = -1;
    
    // Вспомогательные методы
    void setupStreamConnections(StreamPublisherWindow* window, uint32_t streamId);
};
