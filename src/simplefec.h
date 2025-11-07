#pragma once

#include <QObject>
#include <QVector>
#include <QByteArray>
#include <QHash>
#include <QTimer>
#include <QDateTime>
#include "video_defaults.h"

class SimpleFEC : public QObject
{
    Q_OBJECT

public:
    explicit SimpleFEC(QObject *parent = nullptr);
    
    // Кодирование группы из 2 пакетов - УПРОЩЕНО
    QByteArray encodeGroup(const QVector<QByteArray> &packets);
    
    // Добавление пакета для декодирования - УПРОЩЕНО
    void addPacket(int streamId, int groupId, int packetType, const QByteArray &data);
    
    // Попытка декодирования группы - УПРОЩЕНО
    bool tryDecodeGroup(int streamId, int groupId);
    
    // Получение декодированных пакетов - УПРОЩЕНО
    QVector<QByteArray> getDecodedPackets(int streamId, int groupId) const;
    
    // Проверка, полностью ли декодирована группа
    bool isGroupComplete(int streamId, int groupId) const;
    
    // Очистка группы
    void clearGroup(int streamId, int groupId);
    
    // Очистка старых данных
    void cleanupOldGroups();

signals:
    void groupDecoded(int streamId, int groupId, const QVector<QByteArray> &packets);

private:
    // Вычисление XOR для пакетов - УПРОЩЕНО
    QByteArray xorPackets(const QVector<QByteArray> &packets) const;
    
    // Восстановление потерянных пакетов - УПРОЩЕНО
    bool recoverLostPackets(int streamId, int groupId);
    
    // Генерация уникального ID группы
    int generateGroupId(int streamId, int groupId) const;

    struct FECGroup {
        QVector<QByteArray> originalPackets;    // P1, P2
        QVector<bool> hasOriginal;              // Флаги наличия оригинальных пакетов
        QByteArray xorPacket;                   // XOR пакет (P1 ^ P2)
        bool hasXor = false;
        qint64 lastUpdateTime;
        
        FECGroup() : originalPackets(2), hasOriginal(2, false), 
                    lastUpdateTime(QDateTime::currentMSecsSinceEpoch()) {}
    };
    
    QHash<int, FECGroup> m_groups;
    QTimer *m_cleanupTimer;
};