
#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    auto central = new QWidget(this);
    auto layout = new QVBoxLayout(central);

    auto top = new QHBoxLayout();
    m_comboDevices = new QComboBox(this);
    m_btnRefresh = new QPushButton("Обновить", this);
    m_btnStart = new QPushButton("Старт", this);
    m_btnStop = new QPushButton("Стоп", this);
    top->addWidget(m_comboDevices);
    top->addWidget(m_btnRefresh);
    top->addWidget(m_btnStart);
    top->addWidget(m_btnStop);
    layout->addLayout(top);

    m_videoLabel = new QLabel(this);
    m_videoLabel->setAlignment(Qt::AlignCenter);
    m_videoLabel->setMinimumSize(640, 480);
    layout->addWidget(m_videoLabel);

    m_infoLabel = new QLabel("Ожидание выбора камеры...", this);
    layout->addWidget(m_infoLabel);

    setCentralWidget(central);

    connect(m_btnRefresh, &QPushButton::clicked, this, &MainWindow::refreshDevices);
    connect(m_btnStart, &QPushButton::clicked, this, &MainWindow::startCapture);
    connect(m_btnStop, &QPushButton::clicked, this, &MainWindow::stopCapture);

    refreshDevices();
}

MainWindow::~MainWindow()
{
    stopCapture();
}

void MainWindow::refreshDevices()
{
    m_comboDevices->clear();
    m_deviceIndices.clear();

    qDebug() << "Проверяем видеоустройства через OpenCV...";
    for (int i = 0; i < 10; ++i) {
        cv::VideoCapture cap;
#ifdef _WIN32
        try {
            cap.open(i, cv::CAP_DSHOW);
        } catch (...) {
            continue;
        }
#else
        if (!cap.open(i)) continue;
#endif
        if (cap.isOpened()) {
            QString name = QString("Device #%1").arg(i);
            m_comboDevices->addItem(name);
            m_deviceIndices.append(i);
            qDebug() << "Найдено устройство:" << name;
            cap.release();
        }
    }

    if (m_deviceIndices.isEmpty()) {
        m_comboDevices->addItem("(Нет доступных камер)");
        m_infoLabel->setText("Камеры не найдены!");
    } else {
        m_infoLabel->setText("Выберите устройство и нажмите Старт.");
    }
}

void MainWindow::startCapture()
{
    int index = m_comboDevices->currentIndex();
    if (index < 0 || index >= m_deviceIndices.size()) {
        QMessageBox::warning(this, "Ошибка", "Нет выбранного устройства!");
        return;
    }

    stopCapture();

    int deviceIndex = m_deviceIndices.at(index);
    m_infoLabel->setText(QString("Захват устройства #%1...").arg(deviceIndex));

    m_thread = new CaptureThread(this);
    connect(m_thread, &CaptureThread::frameReady, this, &MainWindow::onFrame);
    connect(m_thread, &CaptureThread::errorOccurred, this, &MainWindow::onError);
    m_thread->startCapture(deviceIndex);
}

void MainWindow::stopCapture()
{
    if (m_thread) {
        m_thread->stopCapture();
        m_thread->wait();
        m_thread->deleteLater();
        m_thread = nullptr;
    }
    m_videoLabel->clear();
    m_infoLabel->setText("Захват остановлен.");
}

void MainWindow::onFrame(const QImage &img)
{
    if (img.isNull()) return;
    m_videoLabel->setPixmap(QPixmap::fromImage(img).scaled(
        m_videoLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void MainWindow::onError(const QString &msg)
{
    QMessageBox::warning(this, "Ошибка захвата", msg);
    stopCapture();
}
