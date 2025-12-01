// videogridwidget.cpp
#include "videogridwidget.h"
#include <QScrollArea>
#include <QResizeEvent>
#include <QDebug>
#include <algorithm>

VideoGridWidget::VideoGridWidget(QWidget *parent)
    : QWidget(parent)
    , m_scrollArea(nullptr)
    , m_container(nullptr)
    , m_gridLayout(nullptr)
{
    setupUI();
}

VideoGridWidget::~VideoGridWidget()
{
    cleanupWidgets();
}

void VideoGridWidget::setupUI()
{
    // Main layout
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Scroll area
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setStyleSheet(R"(
        QScrollArea {
            background: #1e1e1e;
            border: none;
        }
        QScrollBar:vertical {
            background: #2d2d2d;
            width: 12px;
            margin: 0px;
        }
        QScrollBar::handle:vertical {
            background: #555;
            border-radius: 6px;
            min-height: 20px;
        }
        QScrollBar::handle:vertical:hover {
            background: #777;
        }
    )");

    // Container for video widgets
    m_container = new QWidget();
    m_container->setStyleSheet("background: #1e1e1e;");
    m_scrollArea->setWidget(m_container);

    mainLayout->addWidget(m_scrollArea);
}

void VideoGridWidget::addStreamerWidget(int deviceIndex)
{
    qDebug() << "Adding streamer widget for device:" << deviceIndex;
    
    // Check if already exists
    if (findStreamerWidget(deviceIndex)) {
        qWarning() << "Streamer widget for device" << deviceIndex << "already exists";
        return;
    }

    StreamerWidget *widget = new StreamerWidget(deviceIndex, m_container);
    m_streamerWidgets.append(widget);
    connectStreamerWidget(widget);
    
    updateLayout();
    qDebug() << "Streamer widget added. Total widgets:" << getTotalWidgetCount();
}

void VideoGridWidget::addViewerWidget(uint32_t streamId, const QString& displayId, uint32_t callId)
{
    qDebug() << "Adding viewer widget for stream:" << displayId << "ID:" << streamId << "CallId:" << callId;
    
    // Check if already exists
    if (findViewerWidget(streamId)) {
        qWarning() << "Viewer widget for stream" << streamId << "already exists";
        return;
    }

    // Передайте callId в конструктор ViewerWidget
    ViewerWidget *widget = new ViewerWidget(streamId, displayId, callId, m_container);
    m_viewerWidgets.append(widget);
    connectViewerWidget(widget);
    
    updateLayout();
    qDebug() << "Viewer widget added. Total widgets:" << getTotalWidgetCount();
}

void VideoGridWidget::removeStreamerWidget(int deviceIndex)
{
    qDebug() << "Removing streamer widget for device:" << deviceIndex;
    
    for (int i = 0; i < m_streamerWidgets.size(); ++i) {
        if (m_streamerWidgets[i]->getDeviceIndex() == deviceIndex) {
            StreamerWidget *widget = m_streamerWidgets.takeAt(i);
            disconnectStreamerWidget(widget);
            widget->deleteLater();
            updateLayout();
            qDebug() << "Streamer widget removed. Total widgets:" << getTotalWidgetCount();
            return;
        }
    }
    
    qWarning() << "Streamer widget for device" << deviceIndex << "not found";
}

void VideoGridWidget::removeViewerWidget(uint32_t streamId)
{
    qDebug() << "Removing viewer widget for stream:" << streamId;
    
    for (int i = 0; i < m_viewerWidgets.size(); ++i) {
        if (m_viewerWidgets[i]->getStreamId() == streamId) {
            ViewerWidget *widget = m_viewerWidgets.takeAt(i);
            disconnectViewerWidget(widget);
            widget->deleteLater();
            updateLayout();
            qDebug() << "Viewer widget removed. Total widgets:" << getTotalWidgetCount();
            return;
        }
    }
    
    qWarning() << "Viewer widget for stream" << streamId << "not found";
}

void VideoGridWidget::onServerStreamDeleted(uint32_t streamId)
{
    removeViewerWidget(streamId);
}

StreamerWidget* VideoGridWidget::findStreamerWidget(int deviceIndex) const
{
    for (StreamerWidget *widget : m_streamerWidgets) {
        if (widget->getDeviceIndex() == deviceIndex) {
            return widget;
        }
    }
    return nullptr;
}

ViewerWidget* VideoGridWidget::findViewerWidget(uint32_t streamId) const
{
    for (ViewerWidget *widget : m_viewerWidgets) {
        if (widget->getStreamId() == streamId) {
            return widget;
        }
    }
    return nullptr;
}

void VideoGridWidget::updateLayout()
{
    if (!m_container) return;

    int totalWidgets = getTotalWidgetCount();
    if (totalWidgets == 0) {
        m_container->setMinimumSize(1, 1);
        return;
    }

    // Get available size (scroll area viewport minus margins)
    QSize containerSize = m_scrollArea->viewport()->size();
    containerSize -= QSize(20, 20); // Small margins

    if (containerSize.width() <= 0 || containerSize.height() <= 0) {
        return;
    }

    // Calculate optimal layout
    LayoutResult layout = calculateLayout(totalWidgets, containerSize);
    
    // Apply layout to widgets
    applyLayout(layout);

    qDebug() << "Layout updated:" << totalWidgets << "widgets, grid:" 
             << layout.rows << "x" << layout.cols << "video size:" << layout.videoSize;
}

VideoGridWidget::LayoutResult VideoGridWidget::calculateLayout(int videoCount, const QSize& containerSize)
{
    LayoutResult result;

    if (videoCount <= 0) {
        return {0, 0, QSize(), {}};
    }

    // Use the same logic as VideoLayoutCalculator
    const int videoWidth = DEFAULT_WIDTH;
    const int videoHeight = DEFAULT_HEIGHT;
    const int gap = TOTAL_MARGIN;

    const int containerW = containerSize.width();
    const int containerH = containerSize.height();

    if (containerW <= 0 || containerH <= 0) {
        // Fallback to minimum size
        result.videoSize = QSize(MIN_VIDEO_WIDTH, MIN_VIDEO_HEIGHT);
        result.rows = 1;
        result.cols = 1;
        result.positions = { QPoint(0, 0) };
        return result;
    }

    double bestK = 0.0;
    int bestRows = 1;
    int bestCols = 1;

    // Find optimal grid configuration
    for (int rows = 1; rows <= videoCount; ++rows) {
        int cols = static_cast<int>(std::ceil(static_cast<double>(videoCount) / rows));

        int totalGapsW = std::max(0, cols - 1) * gap;
        int totalGapsH = std::max(0, rows - 1) * gap;

        if (containerW - totalGapsW <= 0 || containerH - totalGapsH <= 0) {
            continue;
        }

        double perBlockW = static_cast<double>(containerW - totalGapsW) / cols;
        double perBlockH = static_cast<double>(containerH - totalGapsH) / rows;

        double k = std::min(perBlockW / videoWidth, perBlockH / videoHeight);

        if (k <= 0.0) continue;

        if (k > bestK) {
            bestK = k;
            bestRows = rows;
            bestCols = cols;
        }
    }

    // Fallback if no suitable configuration found
    if (bestK <= 0.0) {
        bestK = 1.0;
        bestRows = 1;
        bestCols = 1;
    }

    // Calculate final video size
    int finalWidth = static_cast<int>(std::floor(bestK * videoWidth));
    int finalHeight = static_cast<int>(std::floor(bestK * videoHeight));

    // Ensure minimum size
    finalWidth = std::max(finalWidth, MIN_VIDEO_WIDTH);
    finalHeight = std::max(finalHeight, MIN_VIDEO_HEIGHT);

    result.videoSize = QSize(finalWidth, finalHeight);
    result.rows = bestRows;
    result.cols = bestCols;

    // Calculate positions with centering
    result.positions.clear();

    int totalUsedW = bestCols * finalWidth + std::max(0, bestCols - 1) * gap;
    int totalUsedH = bestRows * finalHeight + std::max(0, bestRows - 1) * gap;

    int startX = (containerW - totalUsedW) / 2;
    int startY = (containerH - totalUsedH) / 2;

    for (int r = 0; r < bestRows; ++r) {
        // Handle last row which might have fewer widgets
        int videosInRow = bestCols;
        if (r == bestRows - 1) {
            videosInRow = videoCount - (bestRows - 1) * bestCols;
            if (videosInRow <= 0) videosInRow = bestCols;
        }

        int rowTotalW = videosInRow * finalWidth + std::max(0, videosInRow - 1) * gap;
        int rowStartX = startX;
        
        // Center incomplete rows
        if (videosInRow < bestCols) {
            int fullRowWidth = bestCols * finalWidth + std::max(0, bestCols - 1) * gap;
            rowStartX = startX + (fullRowWidth - rowTotalW) / 2;
        }

        for (int c = 0; c < videosInRow; ++c) {
            int idx = r * bestCols + c;
            if (idx >= videoCount) break;

            int x = rowStartX + c * (finalWidth + gap);
            int y = startY + r * (finalHeight + gap);
            result.positions.append(QPoint(x, y));
        }
    }

    return result;
}

void VideoGridWidget::applyLayout(const LayoutResult& layout)
{
    // Apply to streamer widgets
    for (int i = 0; i < m_streamerWidgets.size(); ++i) {
        if (i < layout.positions.size()) {
            QPoint pos = layout.positions[i];
            StreamerWidget *widget = m_streamerWidgets[i];
            widget->setFixedSize(layout.videoSize);
            widget->move(pos);
            widget->show();
        }
    }

    // Apply to viewer widgets
    int streamerCount = m_streamerWidgets.size();
    for (int i = 0; i < m_viewerWidgets.size(); ++i) {
        int layoutIndex = streamerCount + i;
        if (layoutIndex < layout.positions.size()) {
            QPoint pos = layout.positions[layoutIndex];
            ViewerWidget *widget = m_viewerWidgets[i];
            widget->setFixedSize(layout.videoSize);
            widget->move(pos);
            widget->show();
        }
    }

    // Update container size to ensure proper scrolling
    int totalUsedW = layout.cols * layout.videoSize.width() + std::max(0, layout.cols - 1) * TOTAL_MARGIN;
    int totalUsedH = layout.rows * layout.videoSize.height() + std::max(0, layout.rows - 1) * TOTAL_MARGIN;
    
    m_container->setMinimumSize(totalUsedW, totalUsedH);
}

void VideoGridWidget::cleanupWidgets()
{
    // Clean up streamer widgets
    for (StreamerWidget *widget : m_streamerWidgets) {
        disconnectStreamerWidget(widget);
        widget->deleteLater();
    }
    m_streamerWidgets.clear();

    // Clean up viewer widgets
    for (ViewerWidget *widget : m_viewerWidgets) {
        disconnectViewerWidget(widget);
        widget->deleteLater();
    }
    m_viewerWidgets.clear();
}

void VideoGridWidget::connectStreamerWidget(StreamerWidget* widget)
{
    if (!widget) return;

    connect(widget, &StreamerWidget::disconnectRequested,
            this, &VideoGridWidget::onStreamerDisconnectRequested);

}

void VideoGridWidget::disconnectStreamerWidget(StreamerWidget* widget)
{
    if (!widget) return;
    widget->disconnect(this);
}

void VideoGridWidget::connectViewerWidget(ViewerWidget* widget)
{
    if (!widget) return;

    connect(widget, &ViewerWidget::streamLeft,
            this, &VideoGridWidget::onViewerLeaveRequested);
}

void VideoGridWidget::disconnectViewerWidget(ViewerWidget* widget)
{
    if (!widget) return;
    widget->disconnect(this);
}

void VideoGridWidget::onStreamerDisconnectRequested(int deviceIndex)
{
    qDebug() << "Streamer disconnect requested for device:" << deviceIndex;
    emit streamerDisconnectRequested(deviceIndex);
}

void VideoGridWidget::onViewerLeaveRequested(uint32_t streamId)
{
    qDebug() << "Viewer leave requested for stream:" << streamId;
    emit viewerLeaveRequested(streamId);
}

// Handle resize events to update layout
void VideoGridWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    updateLayout();
}
