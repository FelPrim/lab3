#include "networkcapturethread.h"
#include <QtCore/QDataStream>
#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtCore/QVariant>
#include <QtNetwork/QNetworkInterface>
#include <QtNetwork/QNetworkDatagram>
#include <cmath>

constexpr int MAX_UDP_PACKET_SIZE = 1200;
constexpr int STATS_INTERVAL_MS = 5000;

NetworkCaptureThread::NetworkCaptureThread(QObject *parent)
    : CaptureThread(parent)
    , m_echoServerAddress(DEFAULT_ECHO_SERVER_ADDRESS)
    , m_echoServerPort(DEFAULT_ECHO_SERVER_PORT)
    , m_statsTimer(new QTimer(this))
{
    qDebug() << "NetworkCaptureThread: Constructor called";
    connect(m_statsTimer, &QTimer::timeout, this, &NetworkCaptureThread::printStatistics);
}

NetworkCaptureThread::~NetworkCaptureThread()
{
    qDebug() << "NetworkCaptureThread: Destructor called";
    printStatistics();
    
    if (m_statsTimer) {
        m_statsTimer->stop();
        delete m_statsTimer;
    }
    
    if (m_udpSocket) {
        m_udpSocket->close();
        delete m_udpSocket;
    }
}

void NetworkCaptureThread::startCapture(int deviceIndex)
{
    qDebug() << "NetworkCaptureThread: startCapture called for device" << deviceIndex;
    
    // Сначала настраиваем сеть
    if (!m_udpSocket) {
        setupNetwork();
    }
    
    // Сбрасываем статистику
    m_stats = Statistics();
    m_captureTimer.start();
    
    qDebug() << "NetworkCaptureThread: Starting statistics timer";
    m_statsTimer->start(STATS_INTERVAL_MS);
    
    // Затем вызываем базовую реализацию
    CaptureThread::startCapture(deviceIndex);
}

void NetworkCaptureThread::stopCapture()
{
    qDebug() << "NetworkCaptureThread: stopCapture called";
    
    if (m_statsTimer && m_statsTimer->isActive()) {
        m_statsTimer->stop();
    }
    
    if (m_udpSocket) {
        m_udpSocket->close();
    }
    
    CaptureThread::stopCapture();
}

void NetworkCaptureThread::setupNetwork()
{
    qDebug() << "NetworkCaptureThread: setupNetwork called";
    
    if (m_udpSocket) {
        qDebug() << "Cleaning up existing UDP socket";
        m_udpSocket->deleteLater();
    }
    
    m_udpSocket = new QUdpSocket(this);
    qDebug() << "UDP socket created:" << (void*)m_udpSocket;
    
    // Увеличиваем размеры буферов
    m_udpSocket->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, 1024 * 1024);
    m_udpSocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, 1024 * 1024);
    
    // Пробуем привязаться к любому адресу
    if (!m_udpSocket->bind(QHostAddress::Any, 0)) {
        QString error = QString("Failed to bind UDP socket: %1").arg(m_udpSocket->errorString());
        qCritical() << error;
        emit errorOccurred(error);
        return;
    }
    
    qDebug() << "UDP socket bound to port:" << m_udpSocket->localPort();
    qDebug() << "Echo server:" << m_echoServerAddress.toString() << ":" << m_echoServerPort;
    
    connect(m_udpSocket, &QUdpSocket::readyRead, this, &NetworkCaptureThread::onPacketReceived);
    
    // Обработчики ошибок
    connect(m_udpSocket, &QUdpSocket::errorOccurred, this,
        [this](QAbstractSocket::SocketError error) {
        qCritical() << "UDP Socket error:" << error << m_udpSocket->errorString();
    });
}

void NetworkCaptureThread::run()
{
    qDebug() << "NetworkCaptureThread: run() started";
    
    try {
        CaptureThread::run();
    } catch (const std::exception& e) {
        qCritical() << "Exception in NetworkCaptureThread::run:" << e.what();
        emit errorOccurred(QString("Exception: %1").arg(e.what()));
    } catch (...) {
        qCritical() << "Unknown exception in NetworkCaptureThread::run";
        emit errorOccurred("Unknown exception in network thread");
    }
    
    qDebug() << "NetworkCaptureThread: run() finished";
}

void NetworkCaptureThread::handleEncodedPacket(const QByteArray &packet, int frameNumber)
{
    qDebug() << "=== handleEncodedPacket called for frame" << frameNumber << "size:" << packet.size() << "bytes ===";
    
    if (!m_udpSocket) {
        qCritical() << "No UDP socket for frame" << frameNumber;
        return;
    }
    
    if (m_udpSocket->state() != QAbstractSocket::BoundState) {
        qCritical() << "UDP socket not bound for frame" << frameNumber << "state:" << m_udpSocket->state();
        return;
    }
    
    if (!m_running) {
        qDebug() << "Not running, skipping frame" << frameNumber;
        return;
    }
    
    try {
        // Используем реальные данные из encoder
        QByteArray realPacket = packet;
        
        // Рассчитываем количество частей динамически
        int packetSize = realPacket.size();
        int partsCount = (packetSize + MAX_PAYLOAD_SIZE - 1) / MAX_PAYLOAD_SIZE;
        
        qDebug() << "Splitting frame" << frameNumber << "of size" << packetSize << "into" << partsCount << "parts";
        
        // Разбиваем реальный пакет на части
        for (int i = 0; i < partsCount; i++) {
            int start = i * MAX_PAYLOAD_SIZE;
            int length = qMin(MAX_PAYLOAD_SIZE, packetSize - start);
            
            QByteArray part = realPacket.mid(start, length);
            qDebug() << "Sending part" << i << "of frame" << frameNumber << "size:" << part.size() << "bytes";
            sendPacketPart(part, frameNumber, i, partsCount);
        }
        
        m_stats.totalSent += partsCount;
        m_stats.packetsSplit++;
        
        m_stats.expectedFrames.insert(frameNumber);
        
        qDebug() << "Successfully processed frame" << frameNumber << "- sent" << partsCount << "parts";
        
    } catch (const std::exception& e) {
        qCritical() << "Exception in handleEncodedPacket for frame" << frameNumber << ":" << e.what();
    } catch (...) {
        qCritical() << "Unknown exception in handleEncodedPacket for frame" << frameNumber;
    }
    
    qDebug() << "=== handleEncodedPacket finished for frame" << frameNumber << "===";
}

void NetworkCaptureThread::sendPacketPart(const QByteArray &data, int frameNumber, int partIndex, int totalParts)
{
    qDebug() << "sendPacketPart: frame" << frameNumber << "part" << partIndex << "/" << totalParts << "data size:" << data.size();
    
    if (!m_udpSocket) {
        qCritical() << "No UDP socket for frame" << frameNumber << "part" << partIndex;
        return;
    }

    if (!m_running) {
        qDebug() << "Not running, skipping frame" << frameNumber << "part" << partIndex;
        return;
    }

    // Проверяем состояние сокета
    if (m_udpSocket->state() != QAbstractSocket::BoundState) {
        qCritical() << "Socket not bound for frame" << frameNumber << "part" << partIndex << "state:" << m_udpSocket->state();
        return;
    }

    try {
        // Создаем datagram с информацией о фрейме и части
        QByteArray datagram;
        QDataStream stream(&datagram, QIODevice::WriteOnly);
        
        // Заголовок: номер фрейма, индекс части, общее количество частей
        stream << frameNumber;
        stream << partIndex;
        stream << totalParts;
        
        // Данные
        stream.writeRawData(data.constData(), data.size());

        qDebug() << "Datagram size:" << datagram.size() << "bytes";

        // Проверяем размер датаграммы
        if (datagram.size() > MAX_UDP_PACKET_SIZE) {
			qCritical() << "Datagram too large:" << datagram.size() << "bytes >" << MAX_UDP_PACKET_SIZE << "bytes";
			return;
		}

        // Отправляем на сервер
        qint64 sent = m_udpSocket->writeDatagram(datagram, m_echoServerAddress, m_echoServerPort);
        
        if (sent == -1) {
            QAbstractSocket::SocketError error = m_udpSocket->error();
            QString errorString = m_udpSocket->errorString();
            
            qCritical() << "Failed to send UDP packet for frame" << frameNumber 
                       << "part" << partIndex << "/" << totalParts
                       << "Error:" << errorString << "(" << error << ")"
                       << "Socket state:" << m_udpSocket->state();
        } else {
            m_stats.totalBytesSent += sent;
            qDebug() << "Successfully sent frame" << frameNumber 
                     << "part" << partIndex << "/" << totalParts
                     << "size:" << sent << "bytes";
        }
        
    } catch (const std::exception& e) {
        qCritical() << "Exception in sendPacketPart for frame" << frameNumber << "part" << partIndex << ":" << e.what();
    } catch (...) {
        qCritical() << "Unknown exception in sendPacketPart for frame" << frameNumber << "part" << partIndex;
    }
}

void NetworkCaptureThread::onPacketReceived()
{
    if (!m_udpSocket) {
        return;
    }
    
    qDebug() << "onPacketReceived: pending datagrams:" << m_udpSocket->pendingDatagramSize();
    
    while (m_udpSocket->hasPendingDatagrams()) {
        try {
            QNetworkDatagram datagram = m_udpSocket->receiveDatagram();
            
            if (!datagram.isValid()) {
                qWarning() << "Received invalid datagram";
                continue;
            }

            QByteArray data = datagram.data();
            if (data.size() < HEADER_SIZE) {
                qWarning() << "Received datagram too small:" << data.size() << "bytes";
                continue;
            }

            QDataStream stream(data);
            int frameNumber, partIndex, totalParts;
            
            stream >> frameNumber;
            stream >> partIndex;
            stream >> totalParts;

            QByteArray packetData = data.mid(HEADER_SIZE);
            
            qDebug() << "Received frame" << frameNumber << "part" << partIndex << "/" << totalParts << "size:" << packetData.size() << "bytes";
            
            // Обрабатываем полученную часть пакета
            processNetworkPacket(packetData, frameNumber, partIndex, totalParts);
            
        } catch (const std::exception& e) {
            qCritical() << "Exception in onPacketReceived:" << e.what();
        } catch (...) {
            qCritical() << "Unknown exception in onPacketReceived";
        }
    }
}

void NetworkCaptureThread::processNetworkPacket(const QByteArray &packet, int frameNumber, int partIndex, int totalParts)
{
    qDebug() << "processNetworkPacket: frame" << frameNumber << "part" << partIndex << "/" << totalParts;
    
    if (!m_packetBuffer || !m_running) {
        qWarning() << "No packet buffer or not running for frame" << frameNumber;
        return;
    }

    try {
        // ВСТАВЛЯЕМ ПАКЕТ В БУФЕР
        int uniqueFrameId = frameNumber * 1000 + partIndex;
        m_packetBuffer->insertFrame(uniqueFrameId, packet);
        
        m_stats.totalReceived++;
        m_stats.totalBytesReceived += packet.size();
        updatePacketSizeStats(packet);

        // Отмечаем, что получили хотя бы одну часть этого фрейма
        m_stats.receivedFrames.insert(frameNumber);
        
        qDebug() << "Successfully processed network packet for frame" << frameNumber << "part" << partIndex;
        
    } catch (const std::exception& e) {
        qCritical() << "Exception in processNetworkPacket for frame" << frameNumber << ":" << e.what();
    } catch (...) {
        qCritical() << "Unknown exception in processNetworkPacket for frame" << frameNumber;
    }
}

void NetworkCaptureThread::updatePacketSizeStats(const QByteArray &packet)
{
    int size = packet.size();
    
    if (m_stats.minPacketSize == 0 || size < m_stats.minPacketSize) {
        m_stats.minPacketSize = size;
    }
    if (size > m_stats.maxPacketSize) {
        m_stats.maxPacketSize = size;
    }
    
    if (m_stats.totalReceived == 0) {
        m_stats.averagePacketSize = size;
    } else {
        m_stats.averagePacketSize = (m_stats.averagePacketSize * m_stats.totalReceived + size) / (m_stats.totalReceived + 1);
    }
}

void NetworkCaptureThread::printStatistics()
{
    qDebug() << "=== Network Capture Statistics ===";
    
    if (m_stats.totalSent == 0) {
        qDebug() << "No packets sent yet";
        return;
    }
    
    double elapsedSeconds = m_captureTimer.elapsed() / 1000.0;
    double sendRate = (m_stats.totalBytesSent / 1024.0) / elapsedSeconds;
    double receiveRate = (m_stats.totalBytesReceived / 1024.0) / elapsedSeconds;
    
    int expectedDataFrames = m_stats.expectedFrames.size();
    int receivedDataFrames = m_stats.receivedFrames.size();
    double dataLossRate = expectedDataFrames > 0 ? 
        (1.0 - (double)receivedDataFrames / expectedDataFrames) * 100.0 : 0.0;
    
    qDebug() << "Time elapsed:" << elapsedSeconds << "seconds";
    qDebug() << "Total packets - Sent:" << m_stats.totalSent << "Received:" << m_stats.totalReceived;
    qDebug() << "Frames processed:" << m_stats.packetsSplit;
    qDebug() << "Data loss rate:" << QString::number(dataLossRate, 'f', 2) << "%";
    qDebug() << "Data rate - Send:" << QString::number(sendRate, 'f', 2) << "KB/s"
             << "Receive:" << QString::number(receiveRate, 'f', 2) << "KB/s";
    
    if (m_stats.totalReceived > 0) {
        qDebug() << "Packet sizes - Min:" << m_stats.minPacketSize << "Max:" << m_stats.maxPacketSize
                 << "Avg:" << QString::number(m_stats.averagePacketSize, 'f', 1);
    }
    
    qDebug() << "UDP Socket state:" << (m_udpSocket ? m_udpSocket->state() : QAbstractSocket::UnconnectedState);
    qDebug() << "Running state:" << m_running;
    qDebug() << "==================================";
}