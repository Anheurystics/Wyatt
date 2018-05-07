#include <QApplication>
#include <QDesktopWidget>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    std::string startupFile = "";
    if (argc > 1)
    {
        startupFile = argv[1];
    }

    MainWindow win(0, startupFile);
    QRect screenRect = QApplication::desktop()->screenGeometry();
    win.move((screenRect.width() - win.width()) / 2, (screenRect.height() - win.height()) / 2);
    win.show();

    return app.exec();
}
