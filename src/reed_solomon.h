#pragma once

#include <QByteArray>
#include <vector>
#include <memory>
#include <stdexcept>

extern "C" {
    #include <fec.h>
}

class ReedSolomonException : public std::exception {
private:
    std::string m_message;
public:
    explicit ReedSolomonException(const std::string& message) : m_message(message) {}
    const char* what() const noexcept override { return m_message.c_str(); }
};

class ReedSolomonFEC
{
public:
    // Конфигурация FEC
    struct Config {
    int data_shards;    // N - количество пакетов данных
    int parity_shards;  // K - количество контрольных пакетов (K = 2*N)
    int total_shards;   // N + K
    
    Config(int n, int k) : data_shards(n), parity_shards(k), total_shards(n + k) {
        if (n < 3) {
            throw ReedSolomonException("Invalid FEC configuration: N must be at least 3");
        }
        // Проверяем, что K = 2*N
        int expected_k = 2 * n;
        if (k != expected_k) {
            throw ReedSolomonException("Invalid FEC configuration: K must equal 2*N");
        }
        if (total_shards > 255) {
            throw ReedSolomonException("FEC configuration error: total shards cannot exceed 255");
        }
    }
};
    // Кодирование данных с защитой от ошибок
    static std::vector<QByteArray> encode(const QByteArray &data, const Config &config);
    
    // Декодирование данных с защитой от ошибок
    static bool decode(std::vector<QByteArray> &shards, const Config &config);

    // Метод для вычисления максимального размера фрейма
    static int calculateMaxFrameSize(const Config &config) {
        return config.data_shards * MAX_SHARD_SIZE;
    }
    
    // Метод для проверки совместимости размера фрейма
    static bool isFrameSizeCompatible(int frame_size, const Config &config) {
        int shard_size = calculateShardSize(frame_size, config.data_shards);
        return shard_size <= MAX_SHARD_SIZE;
    }

private:
    // Безопасное создание и удаление кодера/декодера
    static void* createCodec(const Config &config);
    static void destroyCodec(void* codec);
    
    // Валидация входных данных
    static void validateEncodeInput(const QByteArray &data, const Config &config);
    static void validateDecodeInput(const std::vector<QByteArray> &shards, const Config &config);
    
    // Вычисление размера шарда
    static int calculateShardSize(int data_size, int data_shards);
    
    // Дополнение данных до нужного размера
    static QByteArray padData(const QByteArray &data, int target_size);
    
    // Удаление дополнения
    static QByteArray unpadData(const QByteArray &padded_data, int original_size);

    // Максимальный размер шарда
    static const int MAX_SHARD_SIZE = 1179; // 1200 - 21 байт заголовка
};
