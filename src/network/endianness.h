#pragma once

#include <cstdint>
static_assert(sizeof(short) == 2 && sizeof(int) == 4 && sizeof(long long) == 8);
// Типы для стандартных типов в network byte ordering (BigEndian)
typedef  int16_t  nint16_t;
typedef  int32_t  nint32_t;
typedef  int64_t  nint64_t;
typedef uint16_t nuint16_t;
typedef uint32_t nuint32_t;
typedef uint64_t nuint64_t;