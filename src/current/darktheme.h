#pragma once

#include <QString>
#include <QApplication>
#include <QPalette>
#include <QStyleFactory>

class DarkTheme
{
public:
    static QString getStylesheet();
    static void applyToApplication();
};