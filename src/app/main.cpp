#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("CloverCog"));
    QApplication::setApplicationName(QStringLiteral("Clover-Cog"));


    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
