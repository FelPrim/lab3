#include "darktheme.h"

QString DarkTheme::getStylesheet()
{
    return R"(
        * {
            color: #ffffff;
        }
        QMainWindow {
            background-color: #1e1e1e;
        }
        QWidget {
            background-color: #1e1e1e;
        }
        QPushButton {
            background-color: #2d2d2d;
            border: 1px solid #555;
            padding: 8px 16px;
            border-radius: 4px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #3d3d3d;
        }
        QPushButton:pressed {
            background-color: #252525;
        }
        QPushButton:disabled {
            background-color: #1a1a1a;
            color: #666;
            border-color: #333;
        }
        QLabel {
            background-color: #000000;
            border: none;
        }
        QScrollArea {
            background-color: #1e1e1e;
            border: none;
        }
        QListWidget {
            background-color: #2d2d2d;
            border: 1px solid #555;
        }
        QDialog {
            background-color: #1e1e1e;
        }
        QMessageBox {
            background-color: #1e1e1e;
        }
    )";
}

void DarkTheme::applyToApplication()
{
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(30, 30, 30));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(30, 30, 30));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(45, 45, 45));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    
    QApplication::setPalette(darkPalette);
    qApp->setStyleSheet(getStylesheet());
}