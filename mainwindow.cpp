#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent, std::string startfile) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    codeEditor = ui->codeEditor;
    codeEditor->setPlainText(QString::fromStdString(str_from_file(startfile)));
    QObject::connect(codeEditor, SIGNAL(textChanged()), ui->openGLWidget, SLOT(updateCode()));

    highlighter = new Highlighter(codeEditor->document());

    CustomGLWidget* glWidget = ui->openGLWidget;
    glWidget->logger = ui->logWindow;
    glWidget->interpreter = new Interpreter(ui->logWindow);

    connect(ui->actionNew, &QAction::triggered, this, &MainWindow::newFile);
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::openFile);
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::saveFile);
    connect(ui->actionSave_As, &QAction::triggered, this, &MainWindow::saveAsFile);
}

MainWindow::~MainWindow()
{
    delete ui;
}
