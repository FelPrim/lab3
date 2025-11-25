#pragma once

#include <QObject>
#include <QMap>
#include <QTimer>
#include <QVector>
#include <cstdint>
#include <QDebug> 

class StreamManager : public QObject
{
    Q_OBJECT

public:
    explicit StreamManager(QObject *parent = nullptr);
    ~StreamManager();

    void initialize();
    void cleanup();

    // ЗАКОММЕНТИРОВАТЬ все методы, связанные со старым GUI
    /*
    void createStream(int deviceIndex);
    void joinStream(const QString &streamId);
    void deleteStream(uint32_t streamId);
    void leaveStream(uint32_t streamId);

    void setServerAddress(const QString &address, quint16 port);
    void connectToServer();
    void disconnectFromServer();

    bool isStreamActive(uint32_t streamId) const;
    QVector<uint32_t> getActiveStreams() const;
    QString getStreamStatus(uint32_t streamId) const;

    void createViewerWindowForStream(uint32_t streamId);
    */
    
signals:
    // void streamWindowCreated(QWidget *window);
    // void streamWindowClosed(uint32_t streamId);
    void connectionStatusChanged(bool connected);
    void errorOccurred(const QString &message);

private slots:
    // void onStreamStopped(uint32_t streamId);
    // void onStreamLeft(uint32_t streamId);
    // void onWindowClosed(uint32_t streamId);

    // void onServerStreamCreated(uint32_t streamId);
    // void onServerStreamDeleted(uint32_t streamId);
    // void onServerStreamJoined(uint32_t streamId);
    // void onServerStreamStart(uint32_t streamId);
    // void onServerStreamEnd(uint32_t streamId);

private:
    bool m_connectedToServer;
    // QMap<uint32_t, QWidget*> m_openWindows;
    // NetworkFacade *m_networkFacade = nullptr;


public:
    // Заглушки для GUI
    void setServerAddress(const QString &address, quint16 port) {
        qDebug() << "StreamManager: setServerAddress" << address << port;
    }
    
    void connectToServer() {
        qDebug() << "StreamManager: connectToServer";
        m_connectedToServer = true;
        emit connectionStatusChanged(true);
    }
    
    void disconnectFromServer() {
        qDebug() << "StreamManager: disconnectFromServer";
        m_connectedToServer = false;
        emit connectionStatusChanged(false);
    }
    
    void createStream(int deviceIndex) {
        qDebug() << "StreamManager: createStream for device" << deviceIndex;
        // Временная заглушка
    }
    
    void joinStream(const QString &streamId) {
        qDebug() << "StreamManager: joinStream" << streamId;
        // Временная заглушка
    }
    
    void deleteStream(uint32_t streamId) {
        qDebug() << "StreamManager: deleteStream" << streamId;
        // Временная заглушка
    }
    
    void leaveStream(uint32_t streamId) {
        qDebug() << "StreamManager: leaveStream" << streamId;
        // Временная заглушка
    }

// Добавить сигналы, которые ожидает GUI:
signals:
    void streamWindowCreated(QWidget *window);
    void streamWindowClosed(uint32_t streamId);
};