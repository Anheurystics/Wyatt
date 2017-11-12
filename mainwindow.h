#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "highlighter.h"
#include "helper.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0, std::string startfile = "main.txt");
    ~MainWindow();

    void updateCode();

private:
    Ui::MainWindow *ui;
    Highlighter* highlighter;
};

#endif // MAINWINDOW_H
