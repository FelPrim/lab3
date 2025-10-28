#include "xorfec.h"
#include <QDebug>
#include <stdexcept>

QByteArray XORFEC::padToSize(const QByteArray& data, size_t targetSize)
{
    if (data.size() >= static_cast<int>(targetSize)) {
        return data;
    }
    
    QByteArray padded = data;
    padded.append(QByteArray(targetSize - data.size(), 0));
    return padded;
}

QByteArray XORFEC::unpadFromSize(const QByteArray& data, size_t originalSize)
{
    if (data.size() <= static_cast<int>(originalSize)) {
        return data;
    }
    
    return data.left(originalSize);
}

size_t XORFEC::findMaxSize(const std::vector<QByteArray>& shards)
{
    size_t max_size = 0;
    for (const auto& shard : shards) {
        if (!shard.isEmpty() && shard.size() > static_cast<int>(max_size)) {
            max_size = shard.size();
        }
    }
    return max_size;
}

QByteArray XORFEC::computeXOR(const std::vector<QByteArray>& shards)
{
    if (shards.empty()) {
        return QByteArray();
    }
    
    size_t max_size = findMaxSize(shards);
    
    if (max_size == 0) {
        return QByteArray();
    }
    
    QByteArray xor_packet(max_size, 0);
    
    for (const auto& shard : shards) {
        QByteArray padded_shard = padToSize(shard, max_size);
        for (int i = 0; i < padded_shard.size(); i++) {
            xor_packet[i] ^= padded_shard[i];
        }
    }
    
    return xor_packet;
}

bool XORFEC::canRecover(const std::vector<QByteArray>& shards)
{
    if (shards.empty()) {
        return false;
    }
    
    int missing_count = 0;
    for (const auto& shard : shards) {
        if (shard.isEmpty()) {
            missing_count++;
        }
    }
    
    return missing_count == 1;
}

QByteArray XORFEC::recover(const std::vector<QByteArray>& shards, int missingIndex)
{
    if (!canRecover(shards)) {
        throw std::runtime_error("Cannot recover - too many missing shards");
    }
    
    if (missingIndex < 0 || missingIndex >= static_cast<int>(shards.size())) {
        throw std::runtime_error("Invalid missing index");
    }
    
    size_t max_size = findMaxSize(shards);
    
    // Восстанавливаем недостающий сегмент
    QByteArray recovered(max_size, 0);
    
    for (size_t i = 0; i < shards.size(); i++) {
        if (i != static_cast<size_t>(missingIndex) && !shards[i].isEmpty()) {
            QByteArray padded_shard = padToSize(shards[i], max_size);
            for (int j = 0; j < padded_shard.size(); j++) {
                recovered[j] ^= padded_shard[j];
            }
        }
    }
    
    qDebug() << "✅ Recovered missing shard" << missingIndex << "using XOR";
    return recovered;
}