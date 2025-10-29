#pragma once

#include <QByteArray>
#include <vector>

class XORFEC
{
public:
    static QByteArray computeXOR(const std::vector<QByteArray>& shards);
    static bool canRecover(const std::vector<QByteArray>& shards);
    static QByteArray recover(const std::vector<QByteArray>& shards, int missingIndex);
    
private:
    static QByteArray padToSize(const QByteArray& data, size_t targetSize);
    static QByteArray unpadFromSize(const QByteArray& data, size_t originalSize);
    static size_t findMaxSize(const std::vector<QByteArray>& shards);
};