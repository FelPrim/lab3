#include "mainwindow.h"
#include "videoselectiondialog.h"
#include "removevideodialog.h"
#include "darktheme.h"
#include "video_defaults.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QDebug>
#include <cmath>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_btnAddVideo(nullptr)
    , m_btnRemoveVideo(nullptr)
    , m_btnRefresh(nullptr)
    , m_infoLabel(nullptr)
    , m_videoContainer(nullptr)
    , m_videoLayout(nullptr)
{
    setupUI();
    setupConnections();
    refreshDevices();
    updateVideoLayout();
}

void MainWindow::setupUI()
{
    // Apply dark theme
    DarkTheme::applyToApplication();
    
    setWindowTitle("Multi-Video Stream");
    setMinimumSize(800, 600);
    
    // Central widget
    auto central = new QWidget(this);
    auto mainLayout = new QVBoxLayout(central);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    
    // Top control panel
    auto topPanel = new QWidget(this);
    auto topLayout = new QHBoxLayout(topPanel);
    topLayout->setContentsMargins(0, 0, 0, 0);
    
    m_btnAddVideo = new QPushButton("➕ Add Video", this);
    m_btnRemoveVideo = new QPushButton("➖ Remove Video", this);
    m_btnRefresh = new QPushButton("🔄 Refresh", this);
    
    topLayout->addWidget(m_btnAddVideo);
    topLayout->addWidget(m_btnRemoveVideo);
    topLayout->addWidget(m_btnRefresh);
    topLayout->addStretch();
    
    mainLayout->addWidget(topPanel);

    // Video container without scroll area
    m_videoContainer = new QWidget(this);
    m_videoLayout = new QGridLayout(m_videoContainer);
    m_videoLayout->setSpacing(MARGIN);
    m_videoLayout->setContentsMargins(MARGIN, MARGIN, MARGIN, MARGIN);
    
    mainLayout->addWidget(m_videoContainer, 1);

    m_infoLabel = new QLabel("Click 'Add Video' to start capturing", this);
    m_infoLabel->setAlignment(Qt::AlignCenter);
    m_infoLabel->setStyleSheet("color: #888; font-size: 14px; padding: 10px; background: transparent;");
    mainLayout->addWidget(m_infoLabel);

    setCentralWidget(central);
}

void MainWindow::setupConnections()
{
    connect(m_btnRefresh, &QPushButton::clicked, this, &MainWindow::refreshDevices);
    connect(m_btnAddVideo, &QPushButton::clicked, this, &MainWindow::addVideo);
    connect(m_btnRemoveVideo, &QPushButton::clicked, this, &MainWindow::removeVideo);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    updateVideoLayout();
}

void MainWindow::updateVideoLayout()
{
    // Clear existing layout
    for (auto label : m_videoLabels) {
        m_videoLayout->removeWidget(label);
        label->setVisible(false);
    }
    
    int count = m_captureThreads.size();
    
    if (count == 0) {
        m_infoLabel->setText("No active video streams. Click 'Add Video' to start.");
        m_btnRemoveVideo->setEnabled(false);
        return;
    }
    
    // Calculate optimal layout
    auto layout = VideoLayoutCalculator::calculateLayout(count, m_videoContainer->size());
    
    // Ensure we have enough labels
    while (m_videoLabels.size() < count) {
        QLabel *label = new QLabel(m_videoContainer);
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("background-color: #000000;");
        label->setMinimumSize(160, 120);
        m_videoLabels.append(label);
    }
    
    // Clear grid layout
    for (int i = 0; i < m_videoLayout->rowCount(); ++i) {
        m_videoLayout->setRowStretch(i, 0);
    }
    for (int i = 0; i < m_videoLayout->columnCount(); ++i) {
        m_videoLayout->setColumnStretch(i, 0);
    }
    
    // Setup new grid dimensions
    for (int i = 0; i < layout.rows; ++i) {
        m_videoLayout->setRowStretch(i, 1);
    }
    for (int i = 0; i < layout.cols; ++i) {
        m_videoLayout->setColumnStretch(i, 1);
    }
    
    // Position videos according to calculated layout
    for (int i = 0; i < count; ++i) {
        QLabel *label = m_videoLabels[i];
        label->setFixedSize(layout.videoSize);
        label->setVisible(true);
        
        auto pos = layout.positions[i];
        m_videoLayout->addWidget(label, pos.first, pos.second, 1, 1, Qt::AlignCenter);
    }
    
    // Update UI state
    m_btnRemoveVideo->setEnabled(true);
    m_btnAddVideo->setEnabled(m_availableDevices.size() > m_usedDevices.size());
    m_infoLabel->setText(QString("Displaying %1 video stream(s)").arg(count));
}

void MainWindow::refreshDevices()
{
    m_availableDevices.clear();
    
    qDebug() << "Scanning for video devices...";
    for (int i = 0; i < 10; ++i) {
        cv::VideoCapture cap;
#ifdef _WIN32
        try {
            cap.open(i, cv::CAP_DSHOW);
        } catch (...) {
            continue;
        }
#else
#ifdef __linux__
    if (!cap.open(i, cv::CAP_V4L2)) continue;
#else
#ifdef __APPLE__
    if (!cap.open(i, cv::CAP_AVFOUNDATION)) continue;
#endif
    if (!cap.open(i)) continue;
#endif
#endif
        if (cap.isOpened()) {
            m_availableDevices.append(i);
            qDebug() << "Found device:" << i;
            cap.release();
        }
    }

    if (m_availableDevices.isEmpty()) {
        m_infoLabel->setText("No cameras found. Click 'Refresh' to scan again.");
    } else {
        m_infoLabel->setText(QString("Found %1 camera(s). Click 'Add Video' to start.").arg(m_availableDevices.size()));
    }
    
    updateVideoLayout();
}

void MainWindow::addVideo()
{
    // Determine available devices (excluding already used ones)
    QList<int> available;
    for (int device : m_availableDevices) {
        if (!m_usedDevices.contains(device)) {
            available.append(device);
        }
    }
    
    if (available.isEmpty()) {
        QMessageBox::information(this, "No Devices", "No available devices to add.");
        return;
    }
    
    int selectedDevice;
    if (available.size() == 1) {
        selectedDevice = available.first();
    } else {
        VideoSelectionDialog dialog(available, this);
        if (dialog.exec() == QDialog::Accepted) {
            selectedDevice = available[dialog.selectedDevice()];
        } else {
            return;
        }
    }
    
    int streamIndex = m_captureThreads.size();
    
    CaptureThread *thread = new CaptureThread(this);
    connect(thread, &CaptureThread::frameReady, this, 
            [this, streamIndex](const QImage &img) { onFrame(streamIndex, img); });
    connect(thread, &CaptureThread::errorOccurred, this, &MainWindow::onError);
    
    thread->startCapture(selectedDevice);
    m_captureThreads.append(thread);
    m_usedDevices.append(selectedDevice);
    
    updateVideoLayout();
    
    m_infoLabel->setText(QString("Added video from camera #%1").arg(selectedDevice));
}

void MainWindow::removeVideo()
{
    if (m_captureThreads.isEmpty()) {
        QMessageBox::information(this, "No Videos", "No active video streams to remove.");
        return;
    }
    
    // Show selection dialog for multiple videos
    if (m_captureThreads.size() > 1) {
        RemoveVideoDialog dialog(m_captureThreads, this);
        if (dialog.exec() == QDialog::Accepted && dialog.selectedIndex() >= 0) {
            int removeIndex = dialog.selectedIndex();
            CaptureThread *thread = m_captureThreads[removeIndex];
            int deviceIndex = thread->getDeviceIndex();
            
            thread->stopCapture();
            thread->wait();
            thread->deleteLater();
            
            m_captureThreads.removeAt(removeIndex);
            m_usedDevices.removeAll(deviceIndex);
            
            updateVideoLayout();
            
            m_infoLabel->setText(QString("Removed video from camera #%1").arg(deviceIndex));
        }
    } else {
        // Only one video - remove it directly
        CaptureThread *thread = m_captureThreads[0];
        int deviceIndex = thread->getDeviceIndex();
        
        thread->stopCapture();
        thread->wait();
        thread->deleteLater();
        
        m_captureThreads.clear();
        m_usedDevices.removeAll(deviceIndex);
        
        updateVideoLayout();
        
        m_infoLabel->setText(QString("Removed video from camera #%1").arg(deviceIndex));
    }
}

void MainWindow::onFrame(int streamIndex, const QImage &img)
{
    if (img.isNull() || streamIndex < 0 || streamIndex >= m_videoLabels.size()) 
        return;
        
    QLabel *label = m_videoLabels[streamIndex];
    
    // Scale image to fit the label while maintaining aspect ratio
    QPixmap pixmap = QPixmap::fromImage(img);
    QSize labelSize = label->size();
    
    // Scale to fill the entire label area (will crop if aspect ratios don't match)
    QPixmap scaledPixmap = pixmap.scaled(labelSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    
    // Center the image
    int x = (scaledPixmap.width() - labelSize.width()) / 2;
    int y = (scaledPixmap.height() - labelSize.height()) / 2;
    scaledPixmap = scaledPixmap.copy(x, y, labelSize.width(), labelSize.height());
    
    label->setPixmap(scaledPixmap);
}

void MainWindow::onError(const QString &msg)
{
    QMessageBox::warning(this, "Capture Error", msg);
}

MainWindow::~MainWindow()
{
    for (auto thread : m_captureThreads) {
        if (thread) {
            thread->stopCapture();
            thread->wait();
            thread->deleteLater();
        }
    }
}