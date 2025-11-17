#include "streammanager.h"
#include "streampublisherwindow.h"
#include "streamviewerwindow.h"
#include <QRandomGenerator>
#include <QDebug>
#include <QMessageBox>
#include "networkfacade.h"
#include "video_defaults.h"

// Функция для генерации 6-буквенного ID
QString generateStreamId() {
    const QString letters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    QString result;
    for (int i = 0; i < 6; ++i) {
        int index = QRandomGenerator::global()->bounded(letters.length());
        result.append(letters.at(index));
    }
    return result;
}

StreamManager::StreamManager(QObject *parent)
    : QObject(parent)
    , m_connectedToServer(false)
    , m_nextStreamId(1)
{
    qDebug() << "StreamManager created";
}

// В методе createStream заменяем генерацию ID:
void StreamManager::createStream(int deviceIndex)
{
    int localStreamId = m_nextStreamId++;
    QString displayId = generateStreamId();  // 6 букв

    qDebug() << "Creating stream (local) device:" << deviceIndex << "localId:" << localStreamId << "displayId:" << displayId;

    // Отправляем серверу запрос на создание трансляции
    if (m_networkFacade) {
        qDebug() << "StreamManager: sending CLIENT_STREAM_CREATE to server";
        m_networkFacade->sendStreamCreate();
    } else {
        qWarning() << "StreamManager: no network facade - cannot send CREATE";
    }

    // Создаём локальное окно стримера сразу (preview). Когда сервер пришлёт SERVER_STREAM_CREATED,
    // мы сможем обновить окно реальным id, если потребуется.
    StreamPublisherWindow *window = new StreamPublisherWindow(localStreamId, deviceIndex);
    connect(window, &StreamPublisherWindow::streamStopped, this, &StreamManager::onStreamStopped);
    connect(window, &StreamWindow::windowClosed, this, &StreamManager::onWindowClosed);

    m_openWindows[localStreamId] = window;

    // Текущая локальная заглушка - сразу отображаем окно и симулируем "есть зрители" позже.
    QTimer::singleShot(500, this, [this, window, localStreamId, displayId]() {
        window->setStreamId(localStreamId, displayId);
        window->initialize();
        emit streamWindowCreated(window);

        // Заглушка: через 2 секунды имитируем появление зрителей (если реального SERVER_STREAM_START не придёт)
        QTimer::singleShot(2000, window, [window]() {
            // но если реальный сервер пришлёт событие — оно перезапишет этот статус
            window->setViewersStatus(true);
        });
    });
}


StreamManager::~StreamManager()
{
    cleanup();
}

void StreamManager::initialize()
{
    qDebug() << "StreamManager initialized";
    
    // Заглушка: имитируем успешное подключение через 1 секунду
    QTimer::singleShot(1000, this, [this]() {
        m_connectedToServer = true;
        emit connectionStatusChanged(true);
        qDebug() << "Connected to server (simulated)";
    });
    if (!m_networkFacade) {
        m_networkFacade = new NetworkFacade(this);
        // Используем константы из video_defaults.h (если их изменил на 127.0.0.1) либо явно localhost:
        m_networkFacade->setServer(QString::fromUtf8(DEFAULT_ECHO_SERVER_ADDRESS), DEFAULT_ECHO_SERVER_PORT, 23230);

        // Сообщаем фасаду, что у нас пока нет конкретной локальной UDP информации:
        m_networkFacade->setLocalUdpInfo(QHostAddress::AnyIPv4, 0);

        // Пересылаем сигналы фасада в методы StreamManager
        connect(m_networkFacade, &NetworkFacade::connected, this, [](){ qDebug() << "NetworkFacade connected"; });
        connect(m_networkFacade, &NetworkFacade::errorOccurred, this, [](const QString &err){ qWarning() << "NetworkFacade error:" << err; });
        connect(m_networkFacade, &NetworkFacade::connected, this, [this]() {
            m_connectedToServer = true;
            emit connectionStatusChanged(true);
            qDebug() << "StreamManager: NetworkFacade reports connected";
        });
        connect(m_networkFacade, &NetworkFacade::disconnected, this, [this]() {
            m_connectedToServer = false;
            emit connectionStatusChanged(false);
            qDebug() << "StreamManager: NetworkFacade reports disconnected";
        });
        // Перенаправляем события сервера в StreamManager (реализуй методы-обработчики, если их ещё нет)
        connect(m_networkFacade, &NetworkFacade::serverStreamCreated, this, &StreamManager::onServerStreamCreated);
        connect(m_networkFacade, &NetworkFacade::serverStreamDeleted, this, &StreamManager::onServerStreamDeleted);
        connect(m_networkFacade, &NetworkFacade::serverStreamJoined, this, &StreamManager::onServerStreamJoined);
        connect(m_networkFacade, &NetworkFacade::serverStreamStart, this, &StreamManager::onServerStreamStart);
        connect(m_networkFacade, &NetworkFacade::serverStreamEnd, this, &StreamManager::onServerStreamEnd);
    }
}

void StreamManager::cleanup()
{
    qDebug() << "StreamManager cleanup";
    for (auto window : m_openWindows) {
        if (window) {
            window->close();
            window->deleteLater();
        }
    }
    m_openWindows.clear();
}



void StreamManager::joinStream(const QString &streamId)
{
    if (streamId.length() != 6) {
        emit errorOccurred("Stream ID must be exactly 6 characters");
        return;
    }

    // Преобразуем 6-знаковую строку в число в соответствии с ТЗ:
    int numericId = 0;
    for (int i = 0; i < 6; ++i) {
        numericId *= 26;
        numericId += (streamId[i].unicode() - 'A');
    }

    qDebug() << "Joining stream:" << streamId << "-> numeric ID:" << numericId;

    // Отправляем запрос на сервер
    if (m_networkFacade) {
        qDebug() << "StreamManager: sending CLIENT_STREAM_JOIN id=" << numericId;
        m_networkFacade->sendStreamJoin(static_cast<uint32_t>(numericId));
    } else {
        qWarning() << "StreamManager: no network facade - cannot send JOIN";
    }

    // Локальное окно зрителя
    StreamViewerWindow *window = new StreamViewerWindow();
    connect(window, &StreamViewerWindow::streamLeft, this, &StreamManager::onStreamLeft);
    connect(window, &StreamWindow::windowClosed, this, &StreamManager::onWindowClosed);

    m_openWindows[numericId] = window;

    // Заглушка: после короткой задержки инициализируем окно (реальное подключение начнёт приходить после SERVER_STREAM_JOINED)
    QTimer::singleShot(500, this, [this, window, numericId, streamId]() {
        window->setStreamId(numericId, streamId);
        window->initialize();
        emit streamWindowCreated(window);
    });
}

void StreamManager::deleteStream(int streamId)
{
    qDebug() << "Delete stream requested:" << streamId;

    // Отправляем команду на сервер
    if (m_networkFacade) {
        qDebug() << "StreamManager: sending CLIENT_STREAM_DELETE id=" << streamId;
        m_networkFacade->sendStreamDelete(static_cast<uint32_t>(streamId));
    } else {
        qWarning() << "StreamManager: no network facade - cannot send DELETE";
    }

    // Локальные действия: закрываем окно
    if (m_openWindows.contains(streamId)) {
        m_openWindows[streamId]->close();
        m_openWindows.remove(streamId);
        emit streamWindowClosed(streamId);
    }
}


void StreamManager::leaveStream(int streamId)
{
    qDebug() << "Leave stream requested:" << streamId;

    // Отправляем команду на сервер
    if (m_networkFacade) {
        qDebug() << "StreamManager: sending CLIENT_STREAM_LEAVE id=" << streamId;
        m_networkFacade->sendStreamLeave(static_cast<uint32_t>(streamId));
    } else {
        qWarning() << "StreamManager: no network facade - cannot send LEAVE";
    }

    // Локальные действия: закрываем окно
    if (m_openWindows.contains(streamId)) {
        m_openWindows[streamId]->close();
        m_openWindows.remove(streamId);
        emit streamWindowClosed(streamId);
    }
}


void StreamManager::setServerAddress(const QString &address, quint16 port)
{
    qDebug() << "Set server address:" << address << ":" << port;
    if (!m_networkFacade) {
        m_networkFacade = new NetworkFacade(this);
        // подключим обработчики как в initialize(), если нужно
    }
    m_networkFacade->setServer(address, port, /*udpPort*/ static_cast<quint16>(port - 1)); // если UDP порт = TCP-1, или укажи явный
}


void StreamManager::connectToServer()
{
    qDebug() << "Connect to server";
    
    if (!m_networkFacade) {
        qWarning() << "StreamManager: network facade is null; calling initialize()";
        initialize();
    }
    if (m_networkFacade) {
        m_networkFacade->connectToServer();
    } else {
        qWarning() << "StreamManager: cannot connect - no NetworkFacade";
    }
    
    m_connectedToServer = true;
    emit connectionStatusChanged(true);
}

void StreamManager::disconnectFromServer()
{
    qDebug() << "Disconnect from server";
    m_connectedToServer = false;
    emit connectionStatusChanged(false);
}

bool StreamManager::isStreamActive(int streamId) const
{
    return m_openWindows.contains(streamId);
}

QVector<int> StreamManager::getActiveStreams() const
{
    QVector<int> keys;
    for (auto it = m_openWindows.begin(); it != m_openWindows.end(); ++it) {
        keys.append(it.key());
    }
    return keys;
}

QString StreamManager::getStreamStatus(int streamId) const
{
    return m_openWindows.contains(streamId) ? "Active" : "Inactive";
}

void StreamManager::onStreamStopped(int streamId)
{
    qDebug() << "Stream stopped by user, ID:" << streamId;
    deleteStream(streamId);
}

void StreamManager::onStreamLeft(int streamId)
{
    qDebug() << "Stream left by user, ID:" << streamId;
    leaveStream(streamId);
}

void StreamManager::onWindowClosed(int streamId)
{
    qDebug() << "Stream window closed, ID:" << streamId;
    if (m_openWindows.contains(streamId)) {
        m_openWindows.remove(streamId);
        emit streamWindowClosed(streamId);
    }
}

void StreamManager::onServerStreamCreated(uint32_t streamId)
{
    qDebug() << "StreamManager: SERVER_STREAM_CREATED id=" << streamId;
    // Заглушка: при получении информации о созданной трансляции просто логируем.
    // В будущем: связать с конкретным окном стримера, установить streamId/displayId.
}

void StreamManager::onServerStreamDeleted(uint32_t streamId)
{
    qDebug() << "StreamManager: SERVER_STREAM_DELETED id=" << streamId;
    // Если у нас есть окно с таким streamId — закроем его.
    if (m_openWindows.contains((int)streamId)) {
        m_openWindows[(int)streamId]->close();
        m_openWindows.remove((int)streamId);
        emit streamWindowClosed((int)streamId);
    }
}

void StreamManager::onServerStreamJoined(uint32_t streamId)
{
    qDebug() << "StreamManager: SERVER_STREAM_JOINED id=" << streamId;
    // Заглушка: можно создать окно viewer или обновить существующее — сейчас просто логируем.
}

void StreamManager::onServerStreamStart(uint32_t streamId)
{
    qDebug() << "StreamManager: SERVER_STREAM_START id=" << streamId;
    // Найдём окно-публикатор (publisher) с этим streamId и поставим флаг "есть зрители".
    if (m_openWindows.contains((int)streamId)) {
        auto w = m_openWindows[(int)streamId];
        // Попробуем привести к StreamPublisherWindow и установить статус
        StreamPublisherWindow *pub = qobject_cast<StreamPublisherWindow*>(w);
        if (pub) {
            pub->setViewersStatus(true);
        } else {
            qDebug() << "StreamManager: window is not a publisher for id" << streamId;
        }
    } else {
        qDebug() << "StreamManager: no open window for stream" << streamId;
    }
}

void StreamManager::onServerStreamEnd(uint32_t streamId)
{
    qDebug() << "StreamManager: SERVER_STREAM_END id=" << streamId;
    if (m_openWindows.contains((int)streamId)) {
        auto w = m_openWindows[(int)streamId];
        StreamPublisherWindow *pub = qobject_cast<StreamPublisherWindow*>(w);
        if (pub) {
            pub->setViewersStatus(false);
        } else {
            qDebug() << "StreamManager: window is not a publisher for id" << streamId;
        }
    } else {
        qDebug() << "StreamManager: no open window for stream" << streamId;
    }
}
