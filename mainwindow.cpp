#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent, std::string startfile) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    CodeEditor* codeEditor = ui->codeEditor;
    codeEditor->setPlainText(QString::fromStdString(str_from_file(startfile)));
    QObject::connect(codeEditor, SIGNAL(textChanged()), ui->openGLWidget, SLOT(updateCode()));

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
