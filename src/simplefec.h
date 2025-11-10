#pragma once

#include <QObject>
#include <QVector>
#include <QByteArray>
#include <QHash>
#include <QTimer>
#include <QDateTime>

// Объявляем константы здесь, чтобы избежать циклических зависимостей
constexpr int XOR_FEC_K = 4;  // Data packets
constexpr int XOR_FEC_N = 5;  // Total packets (4 data + 1 XOR)

class SimpleFEC : public QObject
{
    Q_OBJECT

public:
    explicit SimpleFEC(QObject *parent = nullptr);
    
    // Кодирование XOR для группы пакетов
    QByteArray encodeXORGroup(const QVector<QByteArray> &packets);
    
    // Добавление пакета для декодирования
    void addPacket(int streamId, int groupId, int packetIndex, const QByteArray &data);
    
    // Попытка декодирования группы
    bool tryDecodeGroup(int streamId, int groupId);
    
    // Получение декодированных пакетов
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
    // Вычисление XOR для пакетов
    QByteArray xorPackets(const QVector<QByteArray> &packets) const;
    
    // Восстановление потерянных пакетов
    bool recoverLostPackets(int streamId, int groupId);
    
    // Генерация уникального ID группы
    int generateGroupId(int streamId, int groupId) const;

    struct XORFECGroup {
        QVector<QByteArray> dataPackets;    // P0, P1, P2, P3
        QVector<bool> hasData;              // Флаги наличия пакетов данных
        QByteArray xorPacket;               // XOR пакет (P0 ^ P1 ^ P2 ^ P3)
        bool hasXor = false;
        qint64 lastUpdateTime;
        
        XORFECGroup() : dataPackets(XOR_FEC_K), hasData(XOR_FEC_K, false), 
                       lastUpdateTime(QDateTime::currentMSecsSinceEpoch()) {}
    };
    
    QHash<int, XORFECGroup> m_groups;
    QTimer *m_cleanupTimer;
};
