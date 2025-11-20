// YOLOv8 Multi-Camera Tracking System with Qt GUI

#include <QApplication>
#include "MainWindow.h"
#include <iostream>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Set application info
    app.setApplicationName("YOLOv8 Multi-Camera Tracker");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("YOLO Tracking");

    try {
        MainWindow mainWindow;
        mainWindow.show();
        return app.exec();
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return -1;
    }
}
