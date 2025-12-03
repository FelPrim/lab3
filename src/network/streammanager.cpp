#include "streammanager.h"
#include <QDebug>
#include "../ui/id_utils.h"

StreamManager::StreamManager(QObject *parent)
    : QObject(parent)
    , m_networkFacade(nullptr)
    , m_connectedToServer(false)
{
    qDebug() << "StreamManager created (NetworkFacade wrapper)";
}

StreamManager::~StreamManager()
{
    cleanup();
}

void StreamManager::initialize()
{
    qDebug() << "StreamManager initialized";
    
    if (!m_networkFacade) {
        m_networkFacade = new NetworkFacade(this);
        
        // Подключаем все сигналы NetworkFacade
        connect(m_networkFacade, &NetworkFacade::connected, 
                this, &StreamManager::onNetworkConnected);
        connect(m_networkFacade, &NetworkFacade::disconnected, 
                this, &StreamManager::onNetworkDisconnected);
        connect(m_networkFacade, &NetworkFacade::errorOccurred, 
                this, &StreamManager::onNetworkError);
        connect(m_networkFacade, &NetworkFacade::handshakeCompleted, 
                this, &StreamManager::onHandshakeCompleted);
        
        // Сигналы стримов
        connect(m_networkFacade, &NetworkFacade::serverStreamCreated, 
                this, &StreamManager::onServerStreamCreated);
        connect(m_networkFacade, &NetworkFacade::serverStreamDeleted, 
                this, &StreamManager::onServerStreamDeleted);
        connect(m_networkFacade, &NetworkFacade::serverStreamJoined, 
                this, &StreamManager::onServerStreamJoined);
        connect(m_networkFacade, &NetworkFacade::serverStreamStart, 
                this, &StreamManager::onServerStreamStart);
        connect(m_networkFacade, &NetworkFacade::serverStreamEnd, 
                this, &StreamManager::onServerStreamEnd);
        
        // Сигналы конференций
        connect(m_networkFacade, &NetworkFacade::serverCallCreated, 
                this, &StreamManager::onServerCallCreated);
        connect(m_networkFacade, &NetworkFacade::serverCallConnJoined, 
                this, &StreamManager::onServerCallConnJoined);
        connect(m_networkFacade, &NetworkFacade::serverCallConnNew, 
                this, &StreamManager::onServerCallConnNew);
        connect(m_networkFacade, &NetworkFacade::serverCallConnLeft, 
                this, &StreamManager::onServerCallConnLeft);
        connect(m_networkFacade, &NetworkFacade::serverCallStreamNew, 
                this, &StreamManager::onServerCallStreamNew);
        connect(m_networkFacade, &NetworkFacade::serverCallStreamDeleted, 
                this, &StreamManager::onServerCallStreamDeleted);

        // Сигналы ошибок и успехов
        connect(m_networkFacade, &NetworkFacade::serverErrorReceived, 
                this, &StreamManager::onServerErrorReceived);
        connect(m_networkFacade, &NetworkFacade::serverSuccessReceived, 
                this, &StreamManager::onServerSuccessReceived);
    }
}

void StreamManager::cleanup()
{
    qDebug() << "StreamManager cleanup";
    if (m_networkFacade) {
        m_networkFacade->disconnect();
    }
    m_pendingStreamCreates.clear();
    m_pendingStreamJoins.clear();
}

// Старые методы (обратная совместимость)
void StreamManager::setServerAddress(const QString &address, quint16 port)
{
    qDebug() << "StreamManager: setServerAddress" << address << port;
    if (m_networkFacade) {
        m_networkFacade->setServer(address, port, port + 1); 
    }
}

void StreamManager::connectToServer()
{
    qDebug() << "StreamManager: connectToServer";
    if (m_networkFacade) {
        m_networkFacade->connectToServer();
    } else {
        qWarning() << "NetworkFacade not initialized";
    }
}

void StreamManager::disconnectFromServer()
{
    qDebug() << "StreamManager: disconnectFromServer";
    if (m_networkFacade) {
        m_networkFacade->disconnect();
    }
    m_connectedToServer = false;
    emit connectionStatusChanged(false);
}


void StreamManager::createStream(int deviceIndex)
{
    qDebug() << "=== StreamManager::createStream() DEBUG ===";
    qDebug() << "Device index:" << deviceIndex;
    qDebug() << "NetworkFacade pointer:" << m_networkFacade;
    qDebug() << "Connected to server:" << m_connectedToServer;
    qDebug() << "Handshake completed:" << (m_networkFacade ? m_networkFacade->isHandshakeCompleted() : false);
    
    if (!m_networkFacade || !m_networkFacade->isHandshakeCompleted()) {
        qDebug() << "Handshake not completed, queuing stream creation";
        m_pendingStreamCreates.append(deviceIndex);
        qDebug() << "Pending stream creates count:" << m_pendingStreamCreates.count();
    } else {
        // Публичный стрим (callId = 0)
        qDebug() << "Sending stream create request to server...";
        m_networkFacade->sendStreamCreate(0);
        m_deviceToStreamMap[deviceIndex] = 0; // Временно, пока не получим реальный ID
        qDebug() << "Stream create request sent for device:" << deviceIndex;
    }
    qDebug() << "=== StreamManager::createStream() END ===";
}


void StreamManager::joinStream(const QString &streamId)
{
    qDebug() << "StreamManager: joinStream" << streamId;
    
    if (!m_networkFacade || !m_networkFacade->isHandshakeCompleted()) {
        qDebug() << "Handshake not completed, queuing stream join";
        m_pendingStreamJoins.append(streamId);
        return;
    }
    
    uint32_t streamIdNum = string_to_id(streamId.toLatin1().constData());
    m_networkFacade->sendStreamJoin(streamIdNum);
}

void StreamManager::deleteStream(uint32_t streamId)
{
    qDebug() << "StreamManager: deleteStream" << streamId;
    if (m_networkFacade) {
        m_networkFacade->sendStreamDelete(streamId);
    }
    
    // Удаляем из mapping'а
    int deviceIndex = -1;
    for (auto it = m_deviceToStreamMap.begin(); it != m_deviceToStreamMap.end(); ++it) {
        if (it.value() == streamId) {
            deviceIndex = it.key();
            break;
        }
    }
    if (deviceIndex != -1) {
        m_deviceToStreamMap.remove(deviceIndex);
    }
}

void StreamManager::leaveStream(uint32_t streamId)
{
    qDebug() << "StreamManager: leaveStream" << streamId;
    if (m_networkFacade) {
        m_networkFacade->sendStreamLeave(streamId);
    }
}

// Новые методы для конференций
void StreamManager::createCall()
{
    qDebug() << "StreamManager: createCall";
    if (m_networkFacade) {
        m_networkFacade->sendCallCreate();
    }
}

void StreamManager::joinCall(const QString &callId)
{
    qDebug() << "StreamManager: joinCall" << callId;
    uint32_t callIdNum = string_to_id(callId.toLatin1().constData());
    if (m_networkFacade) {
        m_networkFacade->sendCallJoin(callIdNum);
    }
}

void StreamManager::leaveCall(uint32_t callId)
{
    qDebug() << "StreamManager: leaveCall" << callId;
    if (m_networkFacade) {
        m_networkFacade->sendCallLeave(callId);
    }
}

void StreamManager::createStreamInCall(int deviceIndex, uint32_t callId)
{
    qDebug() << "StreamManager: createStreamInCall device" << deviceIndex << "in call" << callId;
    if (m_networkFacade) {
        m_networkFacade->sendStreamCreate(callId);
        m_deviceToStreamMap[deviceIndex] = 0; // Временно
    }
}

// Обработчики NetworkFacade
void StreamManager::onNetworkConnected()
{
    qDebug() << "StreamManager: network connected";
    m_connectedToServer = true;
    emit connectionStatusChanged(true);
}

void StreamManager::onNetworkDisconnected()
{
    qDebug() << "StreamManager: network disconnected";
    m_connectedToServer = false;
    emit connectionStatusChanged(false);
}

void StreamManager::onNetworkError(const QString &error)
{
    qWarning() << "StreamManager: network error:" << error;
    emit errorOccurred(error);
}

void StreamManager::onHandshakeCompleted(uint32_t connectionId)
{
    qDebug() << "StreamManager: handshake completed, connectionId:" << connectionId;
    
    // Обрабатываем отложенные запросы
    for (int deviceIndex : m_pendingStreamCreates) {
        createStream(deviceIndex);
    }
    m_pendingStreamCreates.clear();
    
    for (const QString& streamId : m_pendingStreamJoins) {
        joinStream(streamId);
    }
    m_pendingStreamJoins.clear();
}

// Проксирование сигналов NetworkFacade
void StreamManager::onServerStreamCreated(uint32_t id)
{
    qDebug() << "StreamManager: server stream created - ID:" << id;
    
    // Создаем NetworkManager для исходящего стрима
    if (m_networkFacade) {
        NetworkManager* manager = m_networkFacade->createNetworkManager(static_cast<int>(id));
        if (manager) {
            qDebug() << "NetworkManager created for outgoing stream:" << id;
            // Для исходящих стримов включаем возможность отправки
            manager->setSendingEnabled(true);
            
            // Обновляем mapping: находим устройство, которое создавало стрим
            for (auto it = m_deviceToStreamMap.begin(); it != m_deviceToStreamMap.end(); ++it) {
                if (it.value() == 0) { // Находим устройства, ожидающие streamId
                    m_deviceToStreamMap[it.key()] = id;
                    qDebug() << "Mapped device" << it.key() << "to stream" << id;
                    break;
                }
            }
        }
    }
    
    emit serverStreamCreated(id);
}

void StreamManager::onServerStreamDeleted(uint32_t id)
{
    qDebug() << "StreamManager: server stream deleted - ID:" << id;
    emit serverStreamDeleted(id);
}

void StreamManager::onServerStreamJoined(uint32_t id)
{
    qDebug() << "StreamManager: server stream joined - ID:" << id;
    qDebug() << "=== StreamManager::onServerStreamJoined ===";
    qDebug() << "Stream ID:" << id;
    // Создаем NetworkManager для входящего стрима
    if (m_networkFacade) {
        NetworkManager* manager = m_networkFacade->createNetworkManager(static_cast<int>(id));
        if (manager) {
            qDebug() << "NetworkManager created for incoming stream:" << id;
            // Для входящих стримов отправка не нужна, только прием
            manager->setSendingEnabled(false);
        }
    }
    
    emit serverStreamJoined(id);
}

void StreamManager::onServerStreamStart(uint32_t id)
{
    qDebug() << "StreamManager: server stream start - ID:" << id;
    
    NetworkManager* netManager = getNetworkManagerForStream(id);
    if (netManager) {
        netManager->setSendingEnabled(true);
        netManager->start();
        qDebug() << "NetworkManager enabled for sending, stream:" << id;
    }
    
    emit serverStreamStart(id);
}

void StreamManager::onServerStreamEnd(uint32_t id)
{
    qDebug() << "StreamManager: server stream end - ID:" << id;
    
    // ВЫКЛЮЧАЕМ отправку пакетов в NetworkManager
    NetworkManager* netManager = getNetworkManagerForStream(id);
    if (netManager) {
        netManager->setSendingEnabled(false);
        netManager->stop();
        qDebug() << "NetworkManager disabled for sending, stream:" << id;
    }
    
    emit serverStreamEnd(id);
}

void StreamManager::onServerCallCreated(uint32_t callId)
{
    qDebug() << "StreamManager: server call created - ID:" << callId;
    emit serverCallCreated(callId);
}

void StreamManager::onServerCallConnJoined(uint32_t callId, const QVector<uint32_t>& participants, 
                                          const QVector<uint32_t>& streams)
{
    qDebug() << "StreamManager: server call conn joined - callId:" << callId;
    emit serverCallConnJoined(callId, participants, streams);
}

void StreamManager::onServerCallConnNew(uint32_t callId, uint32_t participantId)
{
    qDebug() << "StreamManager: server call conn new - callId:" << callId << "participant:" << participantId;
    emit serverCallConnNew(callId, participantId);
}

void StreamManager::onServerCallConnLeft(uint32_t callId, uint32_t participantId)
{
    qDebug() << "StreamManager: server call conn left - callId:" << callId << "participant:" << participantId;
    emit serverCallConnLeft(callId, participantId);
}

void StreamManager::onServerCallStreamNew(uint32_t callId, uint32_t streamId)
{
    qDebug() << "StreamManager: server call stream new - callId:" << callId << "stream:" << streamId;
    emit serverCallStreamNew(callId, streamId);
}

void StreamManager::onServerCallStreamDeleted(uint32_t callId, uint32_t streamId)
{
    qDebug() << "StreamManager: server call stream deleted - callId:" << callId << "stream:" << streamId;
    emit serverCallStreamDeleted(callId, streamId);
}

// Обработчики ошибок и успехов
void StreamManager::onServerErrorReceived(uint8_t originalMessageType, const QString &errorMessage)
{
    qWarning() << "StreamManager: Server error for message type" << originalMessageType 
               << ":" << errorMessage;
    
    // Определяем тип операции по originalMessageType
    QString operation;
    switch (originalMessageType) {
        case CLIENT_STREAM_CREATE: operation = "create stream"; break;
        case CLIENT_STREAM_DELETE: operation = "delete stream"; break;
        case CLIENT_STREAM_CONN_JOIN: operation = "join stream"; break;
        case CLIENT_STREAM_CONN_LEAVE: operation = "leave stream"; break;
        case CLIENT_CALL_CREATE: operation = "create call"; break;
        case CLIENT_CALL_CONN_JOIN: operation = "join call"; break;
        case CLIENT_CALL_CONN_LEAVE: operation = "leave call"; break;
        default: operation = QString("operation (type %1)").arg(originalMessageType);
    }
    
    emit errorOccurred(QString("Failed to %1: %2").arg(operation).arg(errorMessage));
    emit serverErrorReceived(originalMessageType, errorMessage);
}

void StreamManager::onServerSuccessReceived(uint8_t originalMessageType, const QString &successMessage)
{
    qDebug() << "StreamManager: Server success for message type" << originalMessageType 
             << ":" << successMessage;
    emit serverSuccessReceived(originalMessageType, successMessage);
}

// Старые методы для обратной совместимости
void StreamManager::handleViewerJoined(uint32_t streamId, ViewerWidget* viewer)
{
    if (m_activeViewers.contains(streamId)) {
        qWarning() << "Viewer already exists for stream:" << streamId;
        return;
    }
    m_activeViewers[streamId] = viewer;
    qDebug() << "Viewer registered for stream:" << streamId;
}

void StreamManager::handleViewerLeft(uint32_t streamId)
{
    if (m_activeViewers.remove(streamId)) {
        qDebug() << "Viewer removed for stream:" << streamId;
    } else {
        qWarning() << "Viewer not found for stream:" << streamId;
    }
}

// Публичные слоты для обратной совместимости
void StreamManager::handleServerStreamCreated(uint32_t streamId)
{
    onServerStreamCreated(streamId);
}

void StreamManager::handleServerStreamStart(uint32_t streamId)
{
    onServerStreamStart(streamId);
}

void StreamManager::handleServerStreamEnd(uint32_t streamId)
{
    onServerStreamEnd(streamId);
}

void StreamManager::handleServerStreamDeleted(uint32_t streamId)
{
    onServerStreamDeleted(streamId);
}

void StreamManager::handleServerStreamJoined(uint32_t streamId)
{
    onServerStreamJoined(streamId);
}

NetworkManager* StreamManager::getNetworkManagerForStream(uint32_t streamId)
{
    if (m_networkFacade) {
        return m_networkFacade->getNetworkManager(static_cast<int>(streamId));
    }
    return nullptr;
}

void StreamManager::sendVideoFrame(uint32_t streamId, int frameNumber, const QByteArray &frameData)
{
    NetworkManager* netManager = getNetworkManagerForStream(streamId);
    if (netManager && netManager->isSendingEnabled()) {
        netManager->sendVideoFrame(frameNumber, frameData);
     //   qDebug() << "StreamManager: Sent video frame" << frameNumber << "for stream" << streamId;
    } else {
        if (!netManager) {
            qWarning() << "No NetworkManager for stream:" << streamId;
        } else {
            qDebug() << "StreamManager: Sending disabled for stream:" << streamId;
        }
    }
}