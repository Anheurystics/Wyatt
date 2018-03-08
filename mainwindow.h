#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QFont>
#include <QVBoxLayout>
#include <QOpenGLWidget>
#include <fstream>
#include <map>

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
    CustomGLWidget* glWidget;
    CodeEditor* currentEditor;
    Highlighter* highlighter;
    QTabWidget* tabs;

    map<QString, int> openFiles;

    QString txtFilter;
    QString startupCode;

    int createNewTab(QString);
    int createOpenTab(QString, QString);

private slots:
    void closeTab(int tab) {
        CodeEditor* editor = qobject_cast<CodeEditor*>(tabs->widget(tab)->findChild<QPlainTextEdit*>());
        QString file = editor->fileInfo.fileName();
        openFiles.erase(file);

        if(tabs->count() == 1) {
            switchTab(createNewTab(startupCode));
        } else 
        if(tabs->currentIndex() == tab) {
            if(tab + 1 < tabs->count()) {
                switchTab(tab + 1);
            } else {
                switchTab(tab - 1);
            }
        }

        tabs->removeTab(tab);
    }

    void switchTab(int newTab) {
        if(currentEditor != nullptr) {
            disconnect(currentEditor, SIGNAL(textChanged()), 0, 0);
        }
        tabs->setCurrentIndex(newTab);
        currentEditor = qobject_cast<CodeEditor*>(tabs->currentWidget()->findChild<QPlainTextEdit*>());
        highlighter->setDocument(currentEditor->document());
        connect(currentEditor, SIGNAL(textChanged()), glWidget, SLOT(updateCode()));

        //interpreter->reparse = false;
        emit currentEditor->textChanged();
        //interpreter->reparse = true;
    }

    void newFile() {
        switchTab(createNewTab(startupCode));
    }

    void openFile() {
        glWidget->setUpdatesEnabled(false);
        QString selected = QFileDialog::getOpenFileName(this, tr("Open File"), Q_NULLPTR, txtFilter);
        glWidget->setUpdatesEnabled(true);
        if(selected == Q_NULLPTR) {
            return;
        }

        QString selectedFileName = QFileInfo(selected).fileName();
        if(openFiles.find(selectedFileName) != openFiles.end()) {
            switchTab(openFiles[selectedFileName]);
            return;
        }

        ifstream file;
        file.open(selected.toStdString());
        string contents = "";
        string line = "";
        while(getline(file, line)) {
            contents += line + '\n';
        }
        file.close();

        if(currentEditor->fileInfo.fileName().size() == 0) {
            currentEditor->fileInfo.setFile(selected);
            currentEditor->setPlainText(QString::fromStdString(contents));
            tabs->setTabText(tabs->currentIndex(), currentEditor->fileInfo.fileName());
        } else {
            switchTab(createOpenTab(QString::fromStdString(contents), selected));
        }

        openFiles.insert(pair<QString, int>(selectedFileName, tabs->currentIndex()));
    }

    void closeFile() {
        closeTab(tabs->currentIndex());
    }

    void saveFile() {
        QString fileName = currentEditor->fileInfo.fileName();
        if(fileName.size() == 0) {
            saveAsFile();
            return;
        }

        ofstream file;
        file.open(fileName.toStdString());
        file << currentEditor->document()->toPlainText().toStdString();
        file.close();

        tabs->setTabText(tabs->currentIndex(), fileName);
    }

    void saveAsFile() {
        QString selected = QFileDialog::getSaveFileName(this, tr("Save As"), Q_NULLPTR, txtFilter);
        if(selected == Q_NULLPTR) {
            return;
        }

        currentEditor->fileInfo.setFile(selected);

        saveFile();
    }
};

#endif // MAINWINDOW_H
