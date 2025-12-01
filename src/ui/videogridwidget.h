// videogridwidget.h
#pragma once

#include <QWidget>
#include <QGridLayout>
#include <QScrollArea>
#include <QVector>
#include <QPair>
#include <QSize>
#include <cmath>
#include "streamerwidget.h"
#include "viewerwidget.h"
#include "../video_defaults.h"

class VideoGridWidget : public QWidget
{
    Q_OBJECT

public:
    // Структура для результатов расчета компоновки (из VideoLayoutCalculator)
    struct LayoutResult {
        int rows;
        int cols;
        QSize videoSize;
        QVector<QPoint> positions;
    };

    explicit VideoGridWidget(QWidget *parent = nullptr);
    ~VideoGridWidget();

    // Управление подокнами
    void addStreamerWidget(int deviceIndex);
    void addViewerWidget(uint32_t streamId, const QString& displayId, uint32_t callId = 0);
    void removeStreamerWidget(int deviceIndex);
    void removeViewerWidget(uint32_t streamId);
    
    // Поиск
    StreamerWidget* findStreamerWidget(int deviceIndex) const;
    ViewerWidget* findViewerWidget(uint32_t streamId) const;
    
    // Информация
    int getStreamerWidgetCount() const { return m_streamerWidgets.size(); }
    int getViewerWidgetCount() const { return m_viewerWidgets.size(); }
    int getTotalWidgetCount() const { return m_streamerWidgets.size() + m_viewerWidgets.size(); }
    
    // Обновление компоновки
    void updateLayout();

public slots:
    void onStreamerDisconnectRequested(int deviceIndex);
    void onViewerLeaveRequested(uint32_t streamId);
   void onServerStreamDeleted(uint32_t streamId);

signals:
    // Проброс сигналов от внутренних виджетов
    void streamerDisconnectRequested(int deviceIndex);
    void viewerLeaveRequested(uint32_t streamId);

private:
    // Методы из VideoLayoutCalculator
    LayoutResult calculateLayout(int videoCount, const QSize& containerSize);
    int calculateOptimalColumns(int videoCount, const QSize& containerSize) const;
    QSize calculateVideoSize(int columns, int rows, const QSize& containerSize) const;
    
    // Вспомогательные методы
    void setupUI();
    void cleanupWidgets();
    void connectStreamerWidget(StreamerWidget* widget);
    void connectViewerWidget(ViewerWidget* widget);
    void applyLayout(const LayoutResult& layout);

    // Компоненты UI
    QScrollArea* m_scrollArea;
    QWidget* m_container;
    QGridLayout* m_gridLayout;
    
    // Коллекции виджетов
    QVector<StreamerWidget*> m_streamerWidgets;
    QVector<ViewerWidget*> m_viewerWidgets;
    
    // Константы из VideoLayoutCalculator
    static const int MARGIN = 5;
    static const int TOTAL_MARGIN = 10;
    inline static const int MIN_VIDEO_WIDTH = 160;
    inline static const int MIN_VIDEO_HEIGHT = 120;
    static const int PREFERRED_ASPECT_NUMERATOR = 16;
    static const int PREFERRED_ASPECT_DENOMINATOR = 9;

public:
    QVector<ViewerWidget*> getViewerWidgets() const { return m_viewerWidgets; }
    void disconnectStreamerWidget(StreamerWidget* widget);
    void disconnectViewerWidget(ViewerWidget* widget);
    void resizeEvent(QResizeEvent* event);
    QVector<StreamerWidget*> getStreamerWidgets() const { return m_streamerWidgets; }
};