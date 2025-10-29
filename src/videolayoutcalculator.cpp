#include "videolayoutcalculator.h"
#include <QDebug>

VideoLayoutCalculator::LayoutResult VideoLayoutCalculator::calculateLayout(int videoCount, const QSize& containerSize)
{
    LayoutResult result;
    
    if (videoCount <= 0) {
        return {0, 0, QSize(), {}};
    }

    // Размеры одного видео без учета отступов
    const int videoWidth = DEFAULT_WIDTH;
    const int videoHeight = DEFAULT_HEIGHT;
    const double aspectRatio = static_cast<double>(videoWidth) / videoHeight;

    // Доступная площадь контейнера (учитываем отступы по краям)
    const int availableWidth = containerSize.width() - TOTAL_MARGIN;
    const int availableHeight = containerSize.height() - TOTAL_MARGIN;

    if (availableWidth <= 0 || availableHeight <= 0) {
        // Fallback: минимальный размер
        result.videoSize = QSize(320, 240);
        result.rows = 1;
        result.cols = 1;
        result.positions = {{0, 0}};
        return result;
    }

    double bestK = 0;
    int bestRows = 1;
    int bestCols = 1;

    // Перебираем возможные конфигурации сетки
    for (int rows = 1; rows <= videoCount; ++rows) {
        int cols = std::ceil(static_cast<double>(videoCount) / rows);
        
        // Рассчитываем k для горизонтального расположения
        double k_horizontal = std::min(
            static_cast<double>(availableWidth - (cols - 1) * TOTAL_MARGIN) / (cols * videoWidth),
            static_cast<double>(availableHeight - (rows - 1) * TOTAL_MARGIN) / (rows * videoHeight)
        );

        // Рассчитываем k для вертикального расположения  
        double k_vertical = std::min(
            static_cast<double>(availableWidth - (cols - 1) * TOTAL_MARGIN) / (cols * videoWidth),
            static_cast<double>(availableHeight - (rows - 1) * TOTAL_MARGIN) / (rows * videoHeight)
        );

        // Выбираем лучший k для этой конфигурации
        double k = std::max(k_horizontal, k_vertical);

        if (k > bestK) {
            bestK = k;
            bestRows = rows;
            bestCols = cols;
        }
    }

    // Специальные случаи для лучшего визуального восприятия
    if (videoCount == 2) {
        // Для 2 видео проверяем, что лучше: горизонтально или вертикально
        double k_horizontal = std::min(
            static_cast<double>(availableWidth - TOTAL_MARGIN) / (2 * videoWidth),
            static_cast<double>(availableHeight) / videoHeight
        );
        
        double k_vertical = std::min(
            static_cast<double>(availableWidth) / videoWidth,
            static_cast<double>(availableHeight - TOTAL_MARGIN) / (2 * videoHeight)
        );

        if (k_vertical > k_horizontal) {
            bestRows = 2;
            bestCols = 1;
            bestK = k_vertical;
        } else {
            bestRows = 1;
            bestCols = 2;
            bestK = k_horizontal;
        }
    }
    else if (videoCount == 3) {
        // Для 3 видео проверяем разные варианты
        double k1 = std::min( // 3 в ряд
            static_cast<double>(availableWidth - 2 * TOTAL_MARGIN) / (3 * videoWidth),
            static_cast<double>(availableHeight) / videoHeight
        );
        
        double k2 = std::min( // 2 вверху, 1 внизу
            static_cast<double>(availableWidth - TOTAL_MARGIN) / (2 * videoWidth),
            static_cast<double>(availableHeight - TOTAL_MARGIN) / (2 * videoHeight)
        );
        
        double k3 = std::min( // 3 в столбик
            static_cast<double>(availableWidth) / videoWidth,
            static_cast<double>(availableHeight - 2 * TOTAL_MARGIN) / (3 * videoHeight)
        );

        bestK = std::max({k1, k2, k3});
        
        if (bestK == k1) {
            bestRows = 1;
            bestCols = 3;
        } else if (bestK == k2) {
            bestRows = 2;
            bestCols = 2;
        } else {
            bestRows = 3;
            bestCols = 1;
        }
    }

    // Рассчитываем финальный размер видео
    int finalWidth = static_cast<int>(bestK * videoWidth);
    int finalHeight = static_cast<int>(bestK * videoHeight);
    
    // Гарантируем минимальный размер
    finalWidth = std::max(finalWidth, 160);
    finalHeight = std::max(finalHeight, 120);

    result.videoSize = QSize(finalWidth, finalHeight);
    result.rows = bestRows;
    result.cols = bestCols;

    // Генерируем позиции с центрированием
    result.positions.clear();
    for (int i = 0; i < videoCount; ++i) {
        int row = i / bestCols;
        int col = i % bestCols;
        
        // Центрируем последнюю строку если она неполная
        if (row == bestRows - 1) {
            int videosInLastRow = videoCount - (bestRows - 1) * bestCols;
            if (videosInLastRow < bestCols) {
                col = (bestCols - videosInLastRow) / 2 + col;
            }
        }
        
        result.positions.append({row, col});
    }

    qDebug() << "Layout calculated:" << videoCount << "videos, size:" << result.videoSize 
             << "grid:" << bestRows << "x" << bestCols << "k:" << bestK;

    return result;
}