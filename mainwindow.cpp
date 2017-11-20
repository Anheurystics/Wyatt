#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    startupCode = "func init(){\n\n}\n\nfunc loop(){\n\n}\n";

    codeEditor = ui->codeEditor;
    codeEditor->setPlainText(startupCode);

    QObject::connect(codeEditor, SIGNAL(textChanged()), ui->openGLWidget, SLOT(updateCode()));

    highlighter = new Highlighter(codeEditor->document());

    CustomGLWidget* glWidget = ui->openGLWidget;
    glWidget->logger = ui->logWindow;

    connect(ui->actionNew, &QAction::triggered, this, &MainWindow::newFile);
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::openFile);
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::saveFile);
    connect(ui->actionSave_As, &QAction::triggered, this, &MainWindow::saveAsFile);

    glWidget->interpreter = new Prototype::Interpreter(ui->logWindow);

    txtFilter = tr("Text files (*.txt)");
}

MainWindow::~MainWindow()
{
    delete ui;
}
