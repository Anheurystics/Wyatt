#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QFont>
#include <QVBoxLayout>
#include <QOpenGLWidget>
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
    CustomGLWidget* glWidget;
    CodeEditor* currentEditor;
    Highlighter* highlighter;
    QTabWidget* tabs;

    QString txtFilter;
    QString startupCode;

    int createNewTab(QString);
    int createOpenTab(QString, QString);

private slots:
    void closeTab(int tab) {
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
            QObject::disconnect(currentEditor, SIGNAL(textChanged()), 0, 0);
        }
        tabs->setCurrentIndex(newTab);
        currentEditor = (CodeEditor*)tabs->currentWidget()->findChild<QPlainTextEdit*>();
        highlighter->setDocument(currentEditor->document());
        QObject::connect(currentEditor, SIGNAL(textChanged()), glWidget, SLOT(updateCode()));

        emit currentEditor->textChanged();
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

        saveFile();
    }
};

#endif // MAINWINDOW_H
