// networkdiagnostics.cpp
#include "networkcapturethread.h"
#include <QProcess>
#include <QTcpSocket>
#include <QDateTime>

void NetworkCaptureThread::checkNetworkConnection()
{
    qDebug() << "=== Network Connection Check ===";
    
    // Проверяем доступность сервера через ping (ICMP)
    QProcess pingProcess;
    QStringList pingArgs;
    pingArgs << "-c" << "3" << m_echoServerAddress.toString();
    pingProcess.start("ping", pingArgs);
    
    if (pingProcess.waitForFinished(5000)) {
        QString output = pingProcess.readAllStandardOutput();
        qDebug() << "Ping result:" << output;
    } else {
        qDebug() << "Ping failed or timed out";
    }
    
    // Проверяем доступность порта через telnet/nc
    QTcpSocket tcpTest;
    tcpTest.connectToHost(m_echoServerAddress, m_echoServerPort);
    
    if (tcpTest.waitForConnected(3000)) {
        qDebug() << "TCP connection to" << m_echoServerAddress.toString() 
                 << ":" << m_echoServerPort << "SUCCESS";
        tcpTest.close();
    } else {
        qDebug() << "TCP connection to" << m_echoServerAddress.toString() 
                 << ":" << m_echoServerPort << "FAILED:" << tcpTest.errorString();
    }
    
    // Проверяем локальные сетевые интерфейсы
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    qDebug() << "Available network interfaces:";
    for (const QNetworkInterface &interface : interfaces) {
        if (interface.flags().testFlag(QNetworkInterface::IsUp) && 
            !interface.flags().testFlag(QNetworkInterface::IsLoopBack)) {
            qDebug() << "Interface:" << interface.name() << "HW:" << interface.hardwareAddress();
            for (const QNetworkAddressEntry &entry : interface.addressEntries()) {
                if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                    qDebug() << "  IP:" << entry.ip().toString() << "Mask:" << entry.netmask().toString();
                }
            }
        }
    }
    qDebug() << "=================================";
}

void NetworkCaptureThread::checkMTU()
{
    qDebug() << "=== MTU and Packet Size Check ===";
    qDebug() << "Maximum UDP payload size:" << MAX_UDP_PACKET_SIZE << "bytes";
    qDebug() << "Recommended maximum:" << 1472 << "bytes (for Ethernet MTU 1500)";
    
    // Тестируем отправку пакетов разного размера
    QVector<int> testSizes = {100, 500, 1000, 1472, 2000, 5000};
    
    for (int size : testSizes) {
        QByteArray testPacket(size, 'T');
        qint64 sent = m_udpSocket->writeDatagram(testPacket, m_echoServerAddress, m_echoServerPort);
        
        if (sent == -1) {
            qDebug() << "Packet size" << size << "bytes: FAILED -" << m_udpSocket->errorString();
        } else {
            qDebug() << "Packet size" << size << "bytes: SUCCESS - sent" << sent << "bytes";
        }
        
        QThread::msleep(100); // Небольшая задержка между тестами
    }
    qDebug() << "=================================";
}

void NetworkCaptureThread::checkConnection()
{
    if (!m_udpSocket || m_udpSocket->state() != QAbstractSocket::BoundState) {
        qDebug() << "Connection check: Socket not bound";
        return;
    }
    
    // Отправляем тестовый пакет для проверки соединения
    QByteArray testPacket = "CONNECTION_TEST_" + QByteArray::number(QDateTime::currentMSecsSinceEpoch());
    qint64 sent = m_udpSocket->writeDatagram(testPacket, m_echoServerAddress, m_echoServerPort);
    
    if (sent == -1) {
        qDebug() << "Connection test: FAILED -" << m_udpSocket->errorString();
    } else {
        qDebug() << "Connection test: SUCCESS - sent" << sent << "bytes";
    }
}
