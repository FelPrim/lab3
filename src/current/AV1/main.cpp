#include <QApplication>
#include "mainwindow.h"
#include <opencv2/opencv.hpp> 

int main(int argc, char *argv[])
{
    qRegisterMetaType<cv::Mat>("cv::Mat");
    QApplication app(argc, argv);
    MainWindow w;
    w.show();
    return app.exec();
}
