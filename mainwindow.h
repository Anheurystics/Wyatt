#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <fstream>

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
    explicit MainWindow(QWidget *parent = 0, std::string startfile = "main.txt");
    ~MainWindow();

    void updateCode();

private:
    Ui::MainWindow *ui;
    CodeEditor* codeEditor;
    Highlighter* highlighter;
    QString openFileName;

private slots:
    void newFile() {
        openFileName = QFileDialog::getSaveFileName(this, tr("New File"), "/");
        ofstream file;
        file.open(openFileName.toStdString());
        file.close();
        codeEditor->setPlainText("");
    }

    void openFile() {
        openFileName = QFileDialog::getOpenFileName(this, tr("Open File"), "/");
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
        ofstream file;
        file.open(openFileName.toStdString());
        file << codeEditor->document()->toPlainText().toStdString();
        file.close();
    }

    void saveAsFile() {
        openFileName = QFileDialog::getSaveFileName(this, tr("Save As"), "/");
        saveFile();
    }
};

#endif // MAINWINDOW_H
