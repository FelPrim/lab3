#pragma once

#include <QSize>
#include <QPair>
#include <QVector>
#include <cmath>
#include "video_defaults.h"

class VideoLayoutCalculator
{
public:
    struct LayoutResult {
        int rows;
        int cols;
        QSize videoSize;
        QVector<QPair<int, int>> positions;
    };

    static LayoutResult calculateLayout(int videoCount, const QSize& containerSize);

private:
    static const int MARGIN = 5; // 5 пикселей с каждой стороны видео
    static const int TOTAL_MARGIN = 10; // 5 слева + 5 справа = 10
};