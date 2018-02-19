#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent, std::string startupFile) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    startupCode = "func init(){\n\n}\n\nfunc loop(){\n\n}\n";

    tabs = ui->tabWidget;
    tabs->setTabsClosable(true);
    currentEditor = (CodeEditor*)tabs->currentWidget()->findChild<QPlainTextEdit*>();
    currentEditor->setFont(QFont("Monospace"));

    connect(ui->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(switchTab(int)));
    connect(ui->tabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(closeTab(int)));

    if(startupFile != "") {
        QString startupFileStr = QString::fromStdString(startupFile);
        ifstream file;
        file.open(startupFile);
        string contents = "";
        string line = "";
        while(getline(file, line)) {
            contents += line + '\n';
        }
        file.close();
        currentEditor->fileInfo.setFile(startupFileStr);
        currentEditor->setPlainText(QString::fromStdString(contents));
        ui->tabWidget->setTabText(0, startupFileStr);
    } else {
        currentEditor->setPlainText(startupCode);
    }

    connect(currentEditor, SIGNAL(textChanged()), ui->openGLWidget, SLOT(updateCode()));

    highlighter = new Highlighter(currentEditor->document());

    glWidget = ui->openGLWidget;
    glWidget->logger = ui->logWindow;

    connect(ui->actionNew, &QAction::triggered, this, &MainWindow::newFile);
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::openFile);
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::saveFile);
    connect(ui->actionSave_As, &QAction::triggered, this, &MainWindow::saveAsFile);

    glWidget->interpreter = new Prototype::Interpreter(ui->logWindow);

    txtFilter = tr("Text files (*.txt)");
}

int MainWindow::createNewTab(QString code) {
    QWidget* paren = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setMargin(0);

    CodeEditor* newEditor = new CodeEditor(0);
    newEditor->setFont(QFont("Monospace"));
    newEditor->setPlainText(code);
    layout->addWidget(newEditor);
    paren->setLayout(layout);
    tabs->addTab(paren, "untitled");

    return tabs->count() - 1;
}

int MainWindow::createOpenTab(QString code, QString fileName) {
    int newTab = createNewTab(code);
    CodeEditor* editor = (CodeEditor*)tabs->widget(newTab)->findChild<QPlainTextEdit*>();
    editor->fileInfo.setFile(fileName);
    tabs->setTabText(newTab, editor->fileInfo.fileName());
    return newTab;
}

MainWindow::~MainWindow()
{
    delete ui;
}
