#include "mainwindow.h"
#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("Моделирование распространения вирусного заболевания");
    MainWindow window;
    window.resize(1280, 840);
    window.show();
    return app.exec();
}
