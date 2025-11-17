#include "streamidconverter.h"
#include <QRegularExpression>

QString StreamIdConverter::streamIdToDisplayString(uint32_t streamId)
{
    char name[7] = {0};
    uint32_t temp = streamId;
    
    // Правильная реализация: от старших разрядов к младшим
    for (int i = 5; i >= 0; --i) {
        name[i] = 'A' + (temp % 26);
        temp /= 26;
    }
    
    return QString::fromLatin1(name, 6);
}

uint32_t StreamIdConverter::displayStringToStreamId(const QString& displayString)
{
    if (displayString.length() != 6) {
        return 0;
    }
    
    uint32_t id = 0;
    for (int i = 0; i < 6; ++i) {
        QChar ch = displayString[i];
        if (ch < 'A' || ch > 'Z') {
            return 0;
        }
        
        // Проверка на переполнение
        if (id > (UINT32_MAX - 25) / 26) {
            qWarning() << "StreamIdConverter: ID would overflow";
            return 0;
        }
        
        id = id * 26 + (ch.unicode() - 'A');
    }
    
    return id;
}

bool StreamIdConverter::isValidDisplayString(const QString& displayString)
{
    QRegularExpression regex("^[A-Z]{6}$");
    return regex.match(displayString).hasMatch();
}