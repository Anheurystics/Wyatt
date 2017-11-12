#include <QApplication>
#include <QDesktopWidget>

#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    std::string startfile = "main.txt";
    if(argc > 1) {
        startfile = argv[1];
    }

    MainWindow win(NULL, startfile);
    QRect screenRect = QApplication::desktop()->screenGeometry();
    win.move((screenRect.width() - win.width()) / 2, (screenRect.height() - win.height()) / 2);
    win.show();

    return app.exec();
}
