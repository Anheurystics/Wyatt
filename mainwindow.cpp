#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    CodeEditor* vEditor = ui->codeEditor;
    QObject::connect(vEditor, SIGNAL(textChanged()), ui->openGLWidget, SLOT(updateCode()));

    vEditor->setPlainText("init {\n\tallocate a;\n\ta <- [-1,-1,0] [1,-1,0] [0,1,0];\n}\n\nloop {\n\tdraw a;\n}\n");
    QTextCharFormat deffmt = vEditor->currentCharFormat();
    deffmt.setUnderlineColor(QColor(Qt::red));
    deffmt.setUnderlineStyle(QTextCharFormat::WaveUnderline);
    QTextCursor cursor = vEditor->textCursor();
    cursor.movePosition(QTextCursor::End);
    vEditor->setCurrentCharFormat(deffmt);
    vEditor->setType("vertex");
}

MainWindow::~MainWindow()
{
    delete ui;
}
