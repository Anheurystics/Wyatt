#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <fstream>
#include <map>

#include <QActionGroup>
#include <QAction>
#include <QApplication>
#include <QButtonGroup>
#include <QFileDialog>
#include <QFont>
#include <QGridLayout>
#include <QHeaderView>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QOpenGLWidget>
#include <QPushButton>
#include <QSplitter>
#include <QTabWidget>
#include <QVariant>
#include <QVBoxLayout>
#include <QWidget>

#include "customglwidget.h"
#include "codeeditor.h"
#include "highlighter.h"
#include "helper.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0, std::string startupFile = "");
    ~MainWindow();

    void updateCode();

private:
    QAction *actionNew;
    QAction *actionOpen;
    QAction *actionSave;
    QAction *actionSave_As;
    QAction *actionClose_Tab;
    QAction *action1_1;
    QAction *action3_2;
    QAction *action4_3;
    QAction *action16_9;
    QAction *actionRestart_on_Resize;
    QWidget *centralWidget;
    QGridLayout *centralLayout;
    QSplitter *hSplit;
    QSplitter *vSplit;
    QTabWidget *editors;
    QWidget *tab;
    QVBoxLayout *tabLayout;
    CodeEditor *codeEditor;
    LogWindow *logWindow;
    QWidget *hSplitWidget;
    QVBoxLayout *hSplitLayout;
    CustomGLWidget *openGLWidget;
    QPushButton *playButton;
    QMenuBar *menuBar;
    QMenu *menuFile;
    QMenu *menuOptions;
    QMenu *menuAspect_Ratio;

    CodeEditor* currentEditor;
    Highlighter* highlighter;

    map<QString, int> openFiles;

    QString txtFilter;
    QString startupCode;

    int createNewTab(QString);
    int createOpenTab(QString, QString);

    QActionGroup* aspectRatioGroup;

private slots:
    void switchAspectRatio();
    void closeTab(int tab);
    void switchTab(int newTab);

    void newFile();
    void openFile();
    void saveFile();
    void saveAsFile();
    void closeFile();
};

#endif // MAINWINDOW_H
