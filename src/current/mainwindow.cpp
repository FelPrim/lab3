#include "mainwindow.h"
#include "videoselectiondialog.h"
#include "removevideodialog.h"
#include "darktheme.h"
#include "video_defaults.h"
#include "networkdisplaybuffer.h"  // Будет реализован позже для сетевого эхо
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
    , m_networkManager(new NetworkManager(this))
{
    setupUI();
    setupConnections();
    
    // Инициализируем NetworkManager
    if (m_networkManager->initialize()) {
        m_networkManager->start();
        qDebug() << "NetworkManager initialized successfully";
    } else {
        qWarning() << "Failed to initialize NetworkManager";
    }
    
    refreshDevices();
    
    // Принудительно устанавливаем минимальный размер окна
    setMinimumSize(800, 600);
    
    // Показываем окно ДО обновления layout
    show();
    
    // Небольшая задержка для инициализации GUI, затем обновляем layout
    QTimer::singleShot(100, this, &MainWindow::updateVideoLayout);
}

MainWindow::~MainWindow()
{
    // Останавливаем все захваты видео
    for (auto capture : m_videoCaptures) {
        if (capture) {
            capture->stopCapture();
            capture->wait();
            capture->deleteLater();
        }
    }
    
    // Останавливаем NetworkManager
    if (m_networkManager) {
        m_networkManager->stop();
        m_networkManager->deleteLater();
    }
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
    
    // Соединяем NetworkManager с методом обработки собранных фреймов
    connect(m_networkManager, &NetworkManager::frameAssembled, 
            this, &MainWindow::onFrameAssembled);
    connect(m_networkManager, &NetworkManager::errorOccurred,
            this, &MainWindow::onError);
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
    
    int streamId = m_videoCaptures.size();
    
    // 1. Создаем окно для прямого показа
    VideoDisplay *sourceDisplay = new VideoDisplay(this);
    sourceDisplay->setStreamId(streamId);
    m_sourceDisplays.append(sourceDisplay);
    
    // 2. Создаем окно для сетевого эхо
    VideoDisplay *networkDisplay = new VideoDisplay(this);
    networkDisplay->setStreamId(streamId);
    m_networkDisplays.append(networkDisplay);
	
	// 3. Создаем буфер для сетевого отображения
    NetworkDisplayBuffer *networkBuffer = new NetworkDisplayBuffer(
        streamId, DEFAULT_WIDTH, DEFAULT_HEIGHT, DEFAULT_FPS, this);
    m_networkBuffers.append(networkBuffer);
    
    // 4. Создаем захват видео
    VideoCapture *videoCapture = new VideoCapture(selectedDevice, this);
    m_videoCaptures.append(videoCapture);
    
    // 5. Создаем кодировщик
    VideoEncoder *videoEncoder = new VideoEncoder(streamId, this);
    m_videoEncoders.append(videoEncoder);
    
    // Соединяем сигналы:
    
    // Прямой показ
    connect(videoCapture, &VideoCapture::rawFrameReady,
            sourceDisplay, &VideoDisplay::displayFrame);
    
    // Кодирование
    connect(videoCapture, &VideoCapture::frameForEncodingReady,
            videoEncoder, &VideoEncoder::encodeFrame);
    
    // Отправка по сети
    connect(videoEncoder, &VideoEncoder::encodedPacketReady,
            m_networkManager, &NetworkManager::sendVideoFrame);
    
	// Получение сетевых фреймов
    connect(m_networkManager, &NetworkManager::frameAssembled,
            this, &MainWindow::onFrameAssembled);
    
    // Сетевой показ
    connect(networkBuffer, &NetworkDisplayBuffer::frameReady,
            networkDisplay, &VideoDisplay::displayFrameFromNetwork);
	
    // Обработка ошибок
    connect(videoCapture, &VideoCapture::errorOccurred,
            this, &MainWindow::onError);
    connect(videoEncoder, &VideoEncoder::errorOccurred,
            this, &MainWindow::onError);
    
    // Инициализируем компоненты
    videoEncoder->initialize(DEFAULT_WIDTH, DEFAULT_HEIGHT, DEFAULT_FPS);
    networkBuffer->initialize();
    
    // Запускаем захват
    videoCapture->startCapture();
    m_usedDevices.append(selectedDevice);
    
    updateVideoLayout();
    
    m_infoLabel->setText(QString("Added video from camera #%1").arg(selectedDevice));
    qDebug() << "Added video stream" << streamId << "from device" << selectedDevice;
}

void MainWindow::removeVideo()
{
    if (m_videoCaptures.isEmpty()) {
        QMessageBox::information(this, "No Videos", "No active video streams to remove.");
        return;
    }
    
    // Show selection dialog for multiple videos
    if (m_videoCaptures.size() > 1) {
        RemoveVideoDialog dialog(m_videoCaptures, this);
        if (dialog.exec() == QDialog::Accepted && dialog.selectedIndex() >= 0) {
            removeVideoAtIndex(dialog.selectedIndex());
        }
    } else {
        // Only one video - remove it directly
        removeVideoAtIndex(0);
    }
}

void MainWindow::removeVideoAtIndex(int index)
{
    if (index < 0 || index >= m_videoCaptures.size()) return;
    
    qDebug() << "Removing video stream at index:" << index;
    
    // Останавливаем и удаляем захват
    VideoCapture *capture = m_videoCaptures[index];
    int deviceIndex = capture->getDeviceIndex();
    
    capture->stopCapture();
    capture->wait();
    capture->deleteLater();
    m_videoCaptures.removeAt(index);
    
    // Удаляем кодировщик
    VideoEncoder *encoder = m_videoEncoders[index];
    encoder->cleanup();
    encoder->deleteLater();
    m_videoEncoders.removeAt(index);
    
    // Удаляем буфер сетевого отображения
    NetworkDisplayBuffer *buffer = m_networkBuffers[index];
    buffer->cleanup();
    buffer->deleteLater();
    m_networkBuffers.removeAt(index);
    
    // Удаляем окна показа
    VideoDisplay *sourceDisplay = m_sourceDisplays[index];
    VideoDisplay *networkDisplay = m_networkDisplays[index];
    sourceDisplay->deleteLater();
    networkDisplay->deleteLater();
    m_sourceDisplays.removeAt(index);
    m_networkDisplays.removeAt(index);
    
    // Обновляем used devices
    m_usedDevices.removeAll(deviceIndex);
    
    updateVideoLayout();
    
    m_infoLabel->setText(QString("Removed video from camera #%1").arg(deviceIndex));
    qDebug() << "Removed video stream" << index << "from device" << deviceIndex;
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    updateVideoLayout();
}

void MainWindow::updateVideoLayout()
{
    // Clear existing layout
    QLayoutItem* item;
    while ((item = m_videoLayout->takeAt(0)) != nullptr) {
        delete item;
    }
    
    int sourceCount = m_sourceDisplays.size();
    int totalCount = sourceCount * 2;
    
    if (totalCount == 0) {
        m_infoLabel->setText("No active video streams. Click 'Add Video' to start.");
        m_btnRemoveVideo->setEnabled(false);
        
        // Принудительно обновляем контейнер когда нет видео
        m_videoContainer->updateGeometry();
        return;
    }
    
    // Calculate optimal layout - используем минимальные размеры для расчета
    QSize containerSize = m_videoContainer->size();
    if (containerSize.isEmpty()) {
        // Если контейнер еще не имеет размера, используем размер окна
        containerSize = size() - QSize(40, 120); // Отступы
    }
    
    auto layout = VideoLayoutCalculator::calculateLayout(totalCount, containerSize);
    
    // Setup new grid dimensions
    for (int i = 0; i < layout.rows; ++i) {
        m_videoLayout->setRowStretch(i, 1);
    }
    for (int i = 0; i < layout.cols; ++i) {
        m_videoLayout->setColumnStretch(i, 1);
    }
    
    // Position videos according to calculated layout
    int displayIndex = 0;
    
    // Сначала размещаем прямые показы (source displays)
    for (int i = 0; i < sourceCount; i++) {
        VideoDisplay *display = m_sourceDisplays[i];
        display->setVisible(true);
        
        auto pos = layout.positions[displayIndex];
        m_videoLayout->addWidget(display, pos.first, pos.second, 1, 1);
        displayIndex++;
    }
    
    // Затем размещаем сетевые эхо (network displays)
    for (int i = 0; i < sourceCount; i++) {
        VideoDisplay *display = m_networkDisplays[i];
        display->setVisible(true);
        
        auto pos = layout.positions[displayIndex];
        m_videoLayout->addWidget(display, pos.first, pos.second, 1, 1);
        displayIndex++;
    }
    
    // Принудительные обновления геометрии
    m_videoLayout->update();
    m_videoLayout->activate();  // Важно: активируем layout
    
    m_videoContainer->updateGeometry();
    m_videoContainer->update();
    
    // Update UI state
    m_btnRemoveVideo->setEnabled(true);
    m_btnAddVideo->setEnabled(m_availableDevices.size() > m_usedDevices.size());
    m_infoLabel->setText(QString("Displaying %1 video streams (%2 sources + %2 echoes)")
                        .arg(totalCount).arg(sourceCount));
    
    qDebug() << "Video layout updated. Container size:" << m_videoContainer->size()
             << "Video size:" << layout.videoSize
             << "Total displays:" << totalCount;
}

void MainWindow::onFrameAssembled(int streamId, int frameNumber, const QByteArray &frameData)
{
    qDebug() << "Frame assembled - Stream:" << streamId << "Frame:" << frameNumber << "Size:" << frameData.size();
    
    // Передаем собранный фрейм в соответствующий буфер для отображения
    if (streamId >= 0 && streamId < m_networkBuffers.size()) {
        m_networkBuffers[streamId]->addFrame(frameNumber, frameData);
    } else {
        qWarning() << "Received frame for unknown stream:" << streamId;
    }
}

void MainWindow::onError(const QString &msg)
{
    QMessageBox::warning(this, "Error", msg);
    qWarning() << "Error occurred:" << msg;
}