#include <QApplication>
#include <QDesktopWidget>

#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    MainWindow win;
    QRect screenRect = QApplication::desktop()->screenGeometry();
    win.move((screenRect.width() - win.width()) / 2, (screenRect.height() - win.height()) / 2);
    win.show();

    return app.exec();
}
