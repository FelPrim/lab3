#include "streammanager.h"
#include "streampublisherwindow.h"
#include "streamviewerwindow.h"
#include <QRandomGenerator>
#include <QDebug>
#include <QMessageBox>

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
    int streamId = m_nextStreamId++;
    QString displayId = generateStreamId();  // Исправлено: 6 букв
    
    qDebug() << "Creating stream with device:" << deviceIndex << "-> ID:" << streamId << "Display ID:" << displayId;
    
    StreamPublisherWindow *window = new StreamPublisherWindow(streamId);
    connect(window, &StreamPublisherWindow::streamStopped, this, &StreamManager::onStreamStopped);
    connect(window, &StreamWindow::windowClosed, this, &StreamManager::onWindowClosed);
    
    m_openWindows[streamId] = window;
    
    // Заглушка: имитируем успешное создание трансляции
    QTimer::singleShot(500, this, [this, window, streamId, displayId]() {
        window->setStreamId(streamId, displayId);
        window->initialize();
        emit streamWindowCreated(window);
        
        // Заглушка: через 2 секунды имитируем появление зрителей
        QTimer::singleShot(2000, window, [window]() {
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
    
    int numericId = streamId.length(); // Простая заглушка для ID
    qDebug() << "Joining stream:" << streamId << "-> numeric ID:" << numericId;
    
    StreamViewerWindow *window = new StreamViewerWindow();
    connect(window, &StreamViewerWindow::streamLeft, this, &StreamManager::onStreamLeft);
    connect(window, &StreamWindow::windowClosed, this, &StreamManager::onWindowClosed);
    
    m_openWindows[numericId] = window;
    
    // Заглушка: имитируем успешное присоединение
    QTimer::singleShot(500, this, [this, window, numericId, streamId]() {
        window->setStreamId(numericId, streamId);
        window->initialize();  // Исправлено: без параметров
        emit streamWindowCreated(window);
    });
}

void StreamManager::deleteStream(int streamId)
{
    qDebug() << "Delete stream:" << streamId;
    if (m_openWindows.contains(streamId)) {
        m_openWindows[streamId]->close();
        m_openWindows.remove(streamId);
        emit streamWindowClosed(streamId);
    }
}

void StreamManager::leaveStream(int streamId)
{
    qDebug() << "Leave stream:" << streamId;
    if (m_openWindows.contains(streamId)) {
        m_openWindows[streamId]->close();
        m_openWindows.remove(streamId);
        emit streamWindowClosed(streamId);
    }
}

void StreamManager::setServerAddress(const QString &address, quint16 port)
{
    qDebug() << "Set server address:" << address << ":" << port;
}

void StreamManager::connectToServer()
{
    qDebug() << "Connect to server";
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
