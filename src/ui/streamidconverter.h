#pragma once
#include <QString>
#include <cstdint>

class StreamIdConverter
{
public:
    static QString streamIdToDisplayString(uint32_t streamId);
    static uint32_t displayStringToStreamId(const QString& displayString);
    
    static bool isValidDisplayString(const QString& displayString);
};