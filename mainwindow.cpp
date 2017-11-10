#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    CodeEditor* codeEditor = ui->codeEditor;
    QObject::connect(codeEditor, SIGNAL(textChanged()), ui->openGLWidget, SLOT(updateCode()));

    codeEditor->setPlainText(QString::fromStdString(str_from_file("main.txt")));
    QTextCharFormat deffmt = codeEditor->currentCharFormat();
    deffmt.setUnderlineColor(QColor(Qt::red));
    deffmt.setUnderlineStyle(QTextCharFormat::WaveUnderline);
    QTextCursor cursor = codeEditor->textCursor();
    cursor.movePosition(QTextCursor::End);
    codeEditor->setCurrentCharFormat(deffmt);
    codeEditor->setType("vertex");

    highlighter = new Highlighter(codeEditor->document());

    CustomGLWidget* glWidget = ui->openGLWidget;
    glWidget->logger = ui->logWindow;
    glWidget->interpreter = new Interpreter(ui->logWindow);
}

MainWindow::~MainWindow()
{
    delete ui;
}
