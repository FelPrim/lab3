#pragma once

#include <QObject>
#include <QMap>
#include <QTimer>
#include <QVector>
#include <cstdint>
#include "networkfacade.h"
#include "udp/networkmanager.h" 

class StreamerWidget;
class ViewerWidget;

class StreamManager : public QObject
{
    Q_OBJECT

public:
    explicit StreamManager(QObject *parent = nullptr);
    ~StreamManager();

    void initialize();
    void cleanup();

    // Управление подключением (совместимость со старым кодом)
    void setServerAddress(const QString &address, quint16 port);
    void connectToServer();
    void disconnectFromServer();
    bool isConnected() const { return m_connectedToServer; }

    // Управление стримами (старый интерфейс)
    void createStream(int deviceIndex);
    void joinStream(const QString &streamId);
    void deleteStream(uint32_t streamId);
    void leaveStream(uint32_t streamId);

    // Новые методы для конференций
    void createCall();
    void joinCall(const QString &callId);
    void leaveCall(uint32_t callId);
    void createStreamInCall(int deviceIndex, uint32_t callId);

    // Регистрация виджетов (старый интерфейс)
    void handleViewerJoined(uint32_t streamId, ViewerWidget* viewer);
    void handleViewerLeft(uint32_t streamId);

signals:
    // Старые сигналы для обратной совместимости
    void connectionStatusChanged(bool connected);
    void errorOccurred(const QString &message);
    void streamWindowCreated(QWidget *window);
    void streamWindowClosed(uint32_t streamId);
    
    // Сигналы стримов
    void serverStreamCreated(uint32_t streamId);
    void serverStreamStart(uint32_t streamId);
    void serverStreamEnd(uint32_t streamId);
    void serverStreamDeleted(uint32_t streamId);
    void serverStreamJoined(uint32_t streamId);

    // Сигналы конференций
    void serverCallCreated(uint32_t callId);
    void serverCallConnJoined(uint32_t callId, const QVector<uint32_t>& participants, 
                             const QVector<uint32_t>& streams);
    void serverCallConnNew(uint32_t callId, uint32_t participantId);
    void serverCallConnLeft(uint32_t callId, uint32_t participantId);
    void serverCallStreamNew(uint32_t callId, uint32_t streamId);
    void serverCallStreamDeleted(uint32_t callId, uint32_t streamId);

public slots:
    // Обработчики серверных событий
    void handleServerStreamCreated(uint32_t streamId);
    void handleServerStreamStart(uint32_t streamId);
    void handleServerStreamEnd(uint32_t streamId);
    void handleServerStreamDeleted(uint32_t streamId);
    void handleServerStreamJoined(uint32_t streamId);

private slots:
    // Обработчики NetworkFacade
    void onNetworkConnected();
    void onNetworkDisconnected();
    void onNetworkError(const QString &error);
    
    void onHandshakeCompleted(uint32_t connectionId);
    
    void onServerStreamCreated(uint32_t id);
    void onServerStreamDeleted(uint32_t id);
    void onServerStreamJoined(uint32_t id);
    void onServerStreamStart(uint32_t id);
    void onServerStreamEnd(uint32_t id);
    
    void onServerCallCreated(uint32_t callId);
    void onServerCallConnJoined(uint32_t callId, const QVector<uint32_t>& participants, 
                               const QVector<uint32_t>& streams);
    void onServerCallConnNew(uint32_t callId, uint32_t participantId);
    void onServerCallConnLeft(uint32_t callId, uint32_t participantId);
    void onServerCallStreamNew(uint32_t callId, uint32_t streamId);
    void onServerCallStreamDeleted(uint32_t callId, uint32_t streamId);

private:
    NetworkFacade* m_networkFacade;
    bool m_connectedToServer;
    
    // Mapping для обратной совместимости
    QMap<int, uint32_t> m_deviceToStreamMap;
    QMap<uint32_t, ViewerWidget*> m_activeViewers;
    
    // Очередь запросов до handshake
    QList<int> m_pendingStreamCreates;
    QList<QString> m_pendingStreamJoins;
public:
    NetworkManager* getNetworkManagerForStream(uint32_t streamId);
    void sendVideoPacket(uint32_t streamId, const QByteArray& packet, int frameNumber);

    void sendVideoFrame(uint32_t streamId, int frameNumber, const QByteArray &frameData);
};
