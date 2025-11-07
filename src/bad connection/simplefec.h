#pragma once

#include <QObject>
#include <QVector>
#include <QByteArray>
#include <QHash>
#include <QTimer>
#include <QDateTime>

class SimpleFEC : public QObject
{
    Q_OBJECT

public:
    explicit SimpleFEC(QObject *parent = nullptr);
    
    // Кодирование группы из 4 пакетов
    QVector<QByteArray> encodeGroup(const QVector<QByteArray> &packets);
    
    // Добавление пакета для декодирования
    void addPacket(int streamId, int frameNumber, int groupId, int packetType, const QByteArray &data);
    
    // Попытка декодирования группы
    bool tryDecodeGroup(int streamId, int frameNumber, int groupId);
    
    // Получение декодированных пакетов
    QVector<QByteArray> getDecodedPackets(int streamId, int frameNumber, int groupId) const;
    
    // Проверка, полностью ли декодирована группа
    bool isGroupComplete(int streamId, int frameNumber, int groupId) const;
    
    // Очистка группы
    void clearGroup(int streamId, int frameNumber, int groupId);
    
    // Очистка старых данных
    void cleanupOldGroups();

signals:
    void groupDecoded(int streamId, int frameNumber, int groupId, const QVector<QByteArray> &packets);

private:
    // Вычисление XOR для пакетов
    QByteArray xorPackets(const QVector<QByteArray> &packets) const;
    
    // Восстановление потерянных пакетов
    bool recoverLostPackets(int streamId, int frameNumber, int groupId);
    
    // Генерация уникального ID группы
    int generateGroupId(int streamId, int frameNumber, int groupId) const;
    
    // Восстановление одного пакета
    bool recoverSinglePacket(int lostIndex);
    
    // Восстановление двух пакетов  
    bool recoverTwoPackets(int lost1, int lost2);
    
    // Восстановление трех пакетов
    bool recoverThreePackets(int lost1, int lost2, int lost3);

    struct FECGroup {
        QVector<QByteArray> originalPackets;    // P1, P2, P3, P4
        QVector<bool> hasOriginal;              // Флаги наличия оригинальных пакетов
        QByteArray xor12, xor23, xor34, xor14; // XOR пар
        QByteArray xor1234;                     // XOR всех пакетов
        bool hasXor12 = false, hasXor23 = false, hasXor34 = false, hasXor14 = false;
        bool hasXor1234 = false;
        qint64 lastUpdateTime;
        
        FECGroup() : originalPackets(4), hasOriginal(4, false), lastUpdateTime(QDateTime::currentMSecsSinceEpoch()) {}
    };
    
    QHash<int, FECGroup> m_groups;
    QTimer *m_cleanupTimer;
};