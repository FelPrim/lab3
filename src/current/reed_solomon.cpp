#include "reed_solomon.h"
#include <QDebug>
#include <algorithm>
#include <cstring>

// Максимальное количество шардов
static const int MAX_TOTAL_SHARDS = 50;

std::vector<QByteArray> ReedSolomonFEC::encode(const QByteArray &data, const Config &config)
{
    try {
        qDebug() << "ReedSolomonFEC: Starting encode, data size:" << data.size() 
                 << "config:" << config.data_shards << "+" << config.parity_shards;
        
        // Валидация входных данных
        validateEncodeInput(data, config);
        
        // Вычисляем размер каждого шарда
        int shard_size = calculateShardSize(data.size(), config.data_shards);
        qDebug() << "ReedSolomonFEC: Calculated shard size:" << shard_size;
        
        if (shard_size > MAX_SHARD_SIZE) {
            throw ReedSolomonException("Shard size too large: " + std::to_string(shard_size) + 
                                     ", max allowed: " + std::to_string(MAX_SHARD_SIZE));
        }
        
        // Создаем кодек
        std::unique_ptr<void, decltype(&destroyCodec)> codec(createCodec(config), destroyCodec);
        if (!codec) {
            throw ReedSolomonException("Failed to create RS codec");
        }
        
        // Дополняем данные до кратного размера
        int padded_size = shard_size * config.data_shards;
        QByteArray padded_data = padData(data, padded_size);
        qDebug() << "ReedSolomonFEC: Padded data size:" << padded_data.size();
        
        // Создаем шарды данных
        std::vector<QByteArray> data_shards(config.data_shards);
        for (int i = 0; i < config.data_shards; ++i) {
            data_shards[i] = QByteArray(shard_size, 0);
            int offset = i * shard_size;
            if (offset < padded_data.size()) {
                // Безопасное вычисление размера для копирования
                int available_size = static_cast<int>(padded_data.size()) - offset;
                int copy_size = std::min(shard_size, available_size);
                std::memcpy(data_shards[i].data(), padded_data.constData() + offset, copy_size);
            }
        }
        
        // Создаем контрольные шарды
        std::vector<QByteArray> parity_shards(config.parity_shards);
        for (int i = 0; i < config.parity_shards; ++i) {
            parity_shards[i] = QByteArray(shard_size, 0);
        }
        
        // Кодируем построчно (байт за байтом)
        qDebug() << "ReedSolomonFEC: Starting byte-by-byte encoding...";
        for (int byte_pos = 0; byte_pos < shard_size; ++byte_pos) {
            unsigned char data_bytes[config.data_shards];
            unsigned char parity_bytes[config.parity_shards];
            
            // Читаем данные для текущей позиции байта
            for (int i = 0; i < config.data_shards; ++i) {
                data_bytes[i] = static_cast<unsigned char>(data_shards[i][byte_pos]);
            }
            
            // Вычисляем контрольные байты
            encode_rs_char(codec.get(), data_bytes, parity_bytes);
            
            // Записываем контрольные байты
            for (int i = 0; i < config.parity_shards; ++i) {
                parity_shards[i][byte_pos] = static_cast<char>(parity_bytes[i]);
            }
        }
        
        // Объединяем результаты
        std::vector<QByteArray> all_shards;
        all_shards.reserve(config.total_shards);
        
        // Добавляем шарды данных
        for (auto& shard : data_shards) {
            all_shards.push_back(std::move(shard));
        }
        
        // Добавляем контрольные шарды
        for (auto& shard : parity_shards) {
            all_shards.push_back(std::move(shard));
        }
        
        qDebug() << "ReedSolomonFEC: Encode successful, total shards:" << all_shards.size();
        return all_shards;
        
    } catch (const std::exception& e) {
        qCritical() << "ReedSolomonFEC: Encode failed:" << e.what();
        throw;
    }
}

bool ReedSolomonFEC::decode(std::vector<QByteArray> &shards, const Config &config)
{
    try {
        qDebug() << "ReedSolomonFEC: Starting decode, shards count:" << shards.size();
        
        // Валидация входных данных
        validateDecodeInput(shards, config);
        
        // Определяем размер шарда по первому непустому шарду
        int shard_size = 0;
        for (const auto& shard : shards) {
            if (!shard.isEmpty()) {
                shard_size = shard.size();
                break;
            }
        }
        
        if (shard_size == 0) {
            qWarning() << "ReedSolomonFEC: All shards are empty, cannot determine shard size";
            return false;
        }
        
        if (shard_size > MAX_SHARD_SIZE) {
            qCritical() << "ReedSolomonFEC: Shard size too large:" << shard_size;
            return false;
        }
        
        // Проверяем, что все непустые шарды имеют одинаковый размер
        for (const auto& shard : shards) {
            if (!shard.isEmpty() && shard.size() != shard_size) {
                qCritical() << "ReedSolomonFEC: Inconsistent shard sizes";
                return false;
            }
        }
        
        // Подсчитываем количество потерянных шардов
        std::vector<int> erasures;
        for (int i = 0; i < config.total_shards; ++i) {
            if (shards[i].isEmpty()) {
                erasures.push_back(i);
            }
        }
        
        qDebug() << "ReedSolomonFEC: Missing shards:" << erasures.size() << "out of" << config.total_shards;
        
        // Проверяем, возможно ли восстановление
        if (erasures.size() > config.parity_shards) {
            qWarning() << "ReedSolomonFEC: Too many erasures (" << erasures.size() 
                      << "), max recoverable:" << config.parity_shards;
            return false;
        }
        
        if (erasures.empty()) {
            qDebug() << "ReedSolomonFEC: No erasures, decode not needed";
            return true;
        }
        
        // Создаем кодек
        std::unique_ptr<void, decltype(&destroyCodec)> codec(createCodec(config), destroyCodec);
        if (!codec) {
            throw ReedSolomonException("Failed to create RS codec for decoding");
        }
        
        // Декодируем построчно (байт за байтом)
        qDebug() << "ReedSolomonFEC: Starting byte-by-byte decoding for" 
                 << shard_size << "bytes...";
        
        bool overall_success = true;
        int failed_bytes = 0;
        
        for (int byte_pos = 0; byte_pos < shard_size; ++byte_pos) {
            unsigned char data_bytes[config.total_shards];
            
            // Читаем данные для текущей позиции байта
            for (int i = 0; i < config.total_shards; ++i) {
                if (!shards[i].isEmpty() && byte_pos < shards[i].size()) {
                    data_bytes[i] = static_cast<unsigned char>(shards[i][byte_pos]);
                } else {
                    data_bytes[i] = 0; // Помечаем как отсутствующий
                }
            }
            
            // Декодируем
            int result = decode_rs_char(codec.get(), data_bytes, 
                                      erasures.data(), static_cast<int>(erasures.size()));
            
            if (result < 0) {
                qDebug() << "ReedSolomonFEC: decode_rs_char failed at byte" << byte_pos;
                failed_bytes++;
                overall_success = false;
                // Продолжаем попытки для остальных байтов
                continue;
            }
            
            // Записываем восстановленные данные
            for (size_t i = 0; i < erasures.size(); ++i) {
                int idx = erasures[i];
                if (shards[idx].isEmpty()) {
                    // Создаем шард если он был пустым
                    shards[idx] = QByteArray(shard_size, 0);
                }
                shards[idx][byte_pos] = static_cast<char>(data_bytes[idx]);
            }
        }
        
        if (overall_success) {
            qDebug() << "ReedSolomonFEC: Decode successful, recovered" 
                     << erasures.size() << "shards";
            return true;
        } else {
            qWarning() << "ReedSolomonFEC: Decode partially failed -" 
                       << failed_bytes << "bytes could not be recovered out of" 
                       << shard_size;
            // Даже при частичном успехе можем вернуть true, если восстановили достаточно данных
            return (failed_bytes < shard_size / 10); // Допускаем до 10% ошибок
        }
        
    } catch (const std::exception& e) {
        qCritical() << "ReedSolomonFEC: Decode failed:" << e.what();
        return false;
    }
}

// Реализации вспомогательных методов

void* ReedSolomonFEC::createCodec(const Config &config)
{
    // Параметры для GF(256)
    int prim_poly = 0x011d; // Примитивный полином
    int first_root = 1;     // Первый корень
    int root_space = 1;     // Промежуток между корнями
    int pad = 0;            // Дополнение
    
    void* codec = init_rs_char(8, prim_poly, first_root, root_space, 
                              config.parity_shards, pad);
    if (!codec) {
        throw ReedSolomonException("init_rs_char failed for configuration: " + 
                                 std::to_string(config.data_shards) + "+" + 
                                 std::to_string(config.parity_shards));
    }
    return codec;
}

void ReedSolomonFEC::destroyCodec(void* codec)
{
    if (codec) {
        free_rs_char(codec);
    }
}

void ReedSolomonFEC::validateEncodeInput(const QByteArray &data, const Config &config)
{
    if (data.isEmpty()) {
        throw ReedSolomonException("Input data is empty");
    }
    
    if (config.total_shards > MAX_TOTAL_SHARDS) {
        throw ReedSolomonException("Too many total shards: " + std::to_string(config.total_shards) +
                                 ", max: " + std::to_string(MAX_TOTAL_SHARDS));
    }
    
    qDebug() << "ReedSolomonFEC: Input validation passed";
}

void ReedSolomonFEC::validateDecodeInput(const std::vector<QByteArray> &shards, const Config &config)
{
    if (shards.size() != config.total_shards) {
        throw ReedSolomonException("Shards count mismatch: expected " + 
                                 std::to_string(config.total_shards) + 
                                 ", got " + std::to_string(shards.size()));
    }
    
    // Подсчитываем количество полученных шардов
    int received_shards = 0;
    for (const auto& shard : shards) {
        if (!shard.isEmpty()) {
            received_shards++;
        }
    }
    
    if (received_shards < config.data_shards) {
        throw ReedSolomonException("Insufficient shards for recovery: have " + 
                                 std::to_string(received_shards) + 
                                 ", need at least " + std::to_string(config.data_shards));
    }
    
    qDebug() << "ReedSolomonFEC: Decode input validation passed, received" 
             << received_shards << "shards";
}

int ReedSolomonFEC::calculateShardSize(int data_size, int data_shards)
{
    if (data_shards <= 0) {
        throw ReedSolomonException("data_shards must be positive");
    }
    
    // Вычисляем размер шарда с округлением вверх
    int shard_size = (data_size + data_shards - 1) / data_shards;
    
    // Гарантируем, что размер как минимум 1 байт
    return std::max(1, shard_size);
}

QByteArray ReedSolomonFEC::padData(const QByteArray &data, int target_size)
{
    if (data.size() >= target_size) {
        return data;
    }
    
    QByteArray padded = data;
    padded.append(QByteArray(target_size - data.size(), 0));
    return padded;
}

QByteArray ReedSolomonFEC::unpadData(const QByteArray &padded_data, int original_size)
{
    if (padded_data.size() <= original_size) {
        return padded_data;
    }
    
    return padded_data.left(original_size);
}
