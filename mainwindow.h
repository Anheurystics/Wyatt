#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <fstream>

#include "customglwidget.h"
#include "codeeditor.h"
#include "highlighter.h"
#include "helper.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0, std::string startupFile = "");
    ~MainWindow();

    void updateCode();

private:
    Ui::MainWindow *ui;
    QOpenGLWidget* openGLWidget;
    CodeEditor* codeEditor;
    Highlighter* highlighter;
    QString openFileName;

    QFileInfo openFileInfo;
    QString txtFilter;
    QString startupCode;

    void setOpenedFile(QString fileName) {
        openFileName = fileName;
        openFileInfo.setFile(fileName);
        setWindowTitle("Prototype - " + openFileInfo.fileName());
    }

private slots:
    void newFile() {
        openFileName.clear();
        openFileInfo.setFile(openFileName);
        setWindowTitle("Prototype - untitled");

        codeEditor->setPlainText(startupCode);
    }

    void openFile() {
        openGLWidget->setUpdatesEnabled(false);
        QString selected = QFileDialog::getOpenFileName(this, tr("Open File"), Q_NULLPTR, txtFilter);
        openGLWidget->setUpdatesEnabled(true);
        if(selected == Q_NULLPTR) {
            return;
        }

        setOpenedFile(selected);

        ifstream file;
        file.open(openFileName.toStdString());
        string contents = "";
        string line = "";
        while(getline(file, line)) {
            contents += line + '\n';
        }
        file.close();
        codeEditor->setPlainText(QString::fromStdString(contents));
    }

    void saveFile() {
        if(openFileName.length() == 0) {
            saveAsFile();
            return;
        }

        ofstream file;
        file.open(openFileName.toStdString());
        file << codeEditor->document()->toPlainText().toStdString();
        file.close();
    }

    void saveAsFile() {
        QString selected = QFileDialog::getSaveFileName(this, tr("Save As"), Q_NULLPTR, txtFilter);
        if(selected == Q_NULLPTR) {
            return;
        }

        setOpenedFile(selected);
        saveFile();
    }
};

#endif // MAINWINDOW_H
