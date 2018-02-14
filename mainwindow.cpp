#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent, std::string startupFile) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    startupCode = "func init(){\n\n}\n\nfunc loop(){\n\n}\n";

    codeEditor = ui->codeEditor;

    if(startupFile != "") {
        setOpenedFile(QString::fromStdString(startupFile));

        ifstream file;
        file.open(openFileName.toStdString());
        string contents = "";
        string line = "";
        while(getline(file, line)) {
            contents += line + '\n';
        }
        file.close();
        codeEditor->setPlainText(QString::fromStdString(contents));
    } else {
        codeEditor->setPlainText(startupCode);
    }

    QObject::connect(codeEditor, SIGNAL(textChanged()), ui->openGLWidget, SLOT(updateCode()));

    highlighter = new Highlighter(codeEditor->document());

    CustomGLWidget* glWidget = ui->openGLWidget;
    glWidget->logger = ui->logWindow;

    openGLWidget = glWidget;

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
