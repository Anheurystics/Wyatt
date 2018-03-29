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
    void switchAspectRatio() {
        string selected = aspectRatioGroup->checkedAction()->text().toStdString();
        if(selected == "1:1") {
            openGLWidget->aspectRatio = 1.0f;
        }
        if(selected == "3:2") {
            openGLWidget->aspectRatio = 1.5f;
        }
        if(selected == "4:3") {
            openGLWidget->aspectRatio = 4.0f/3.0f;
        }
        if(selected == "16:9") {
            openGLWidget->aspectRatio = 16.0/9.0f;
        }
        openGLWidget->resizeGL(openGLWidget->width(), 0);
    }

    void closeTab(int tab) {
        CodeEditor* editor = qobject_cast<CodeEditor*>(editors->widget(tab)->findChild<QPlainTextEdit*>());
        QString file = editor->fileInfo.fileName();
        openFiles.erase(file);

        if(editors->count() == 1) {
            switchTab(createNewTab(startupCode));
        } else 
        if(editors->currentIndex() == tab) {
            if(tab + 1 < editors->count()) {
                switchTab(tab + 1);
            } else {
                switchTab(tab - 1);
            }
        }

        editors->removeTab(tab);
    }

    void switchTab(int newTab) {
        if(currentEditor != nullptr) {
            disconnect(currentEditor, SIGNAL(textChanged()), 0, 0);
        }
        editors->setCurrentIndex(newTab);
        currentEditor = qobject_cast<CodeEditor*>(editors->currentWidget()->findChild<QPlainTextEdit*>());
        highlighter->setDocument(currentEditor->document());
        connect(currentEditor, SIGNAL(textChanged()), openGLWidget, SLOT(updateCode()));

        openGLWidget->interpreter->workingDir = currentEditor->fileInfo.absolutePath().toStdString();

        //interpreter->reparse = false;
        emit currentEditor->textChanged();
        //interpreter->reparse = true;
    }

    void newFile() {
        switchTab(createNewTab(startupCode));
    }

    void openFile() {
        openGLWidget->setUpdatesEnabled(false);
        QString selected = QFileDialog::getOpenFileName(this, tr("Open File"), Q_NULLPTR, txtFilter);
        openGLWidget->setUpdatesEnabled(true);
        if(selected == Q_NULLPTR) {
            return;
        }

        QFileInfo selectedFile = QFileInfo(selected);
        QString selectedFileName = selectedFile.fileName();
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
            editors->setTabText(editors->currentIndex(), currentEditor->fileInfo.fileName());
        } else {
            switchTab(createOpenTab(QString::fromStdString(contents), selected));
        }
        
        openGLWidget->interpreter->workingDir = selectedFile.absolutePath().toStdString();

        openFiles.insert(pair<QString, int>(selectedFileName, editors->currentIndex()));
    }

    void closeFile() {
        closeTab(editors->currentIndex());
    }

    void saveFile() {
        QString filePath = currentEditor->fileInfo.absoluteFilePath();
        if(filePath.size() == 0) {
            saveAsFile();
            return;
        }

        ofstream file;
        file.open(filePath.toStdString());
        file << currentEditor->document()->toPlainText().toStdString();
        file.close();

        editors->setTabText(editors->currentIndex(), currentEditor->fileInfo.fileName());
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
