#include <QApplication>
#include "mainwindow.h"
#include <opencv2/opencv.hpp> 
#include <exception>
#include "darktheme.h"

int main(int argc, char *argv[])
{
	try {
    qRegisterMetaType<cv::Mat>("cv::Mat");
    QApplication app(argc, argv);
	DarkTheme::applyToApplication();
    MainWindow w;
    w.show();
    return app.exec();
	}
	 catch (const std::exception& e) {
        qCritical() << "Exception caught:" << e.what();
        return -1;
    } catch (...) {
        qCritical() << "Unknown exception caught";
        return -1;
    }
}
