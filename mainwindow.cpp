#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent, std::string startupFile) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    startupCode = "func init(){\n\n}\n\nfunc loop(){\n\n}\n";

    tabs = ui->editors;
    tabs->setTabsClosable(true);
    currentEditor = (CodeEditor*)tabs->currentWidget()->findChild<QPlainTextEdit*>();
    currentEditor->setFont(currentEditor->monoFont);

    connect(ui->editors, SIGNAL(currentChanged(int)), this, SLOT(switchTab(int)));
    connect(ui->editors, SIGNAL(tabCloseRequested(int)), this, SLOT(closeTab(int)));

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
        ui->editors->setTabText(0, startupFileStr);
    } else {
        currentEditor->setPlainText(startupCode);
    }

    connect(currentEditor, SIGNAL(textChanged()), ui->openGLWidget, SLOT(updateCode()));

    highlighter = new Highlighter(currentEditor->document());

    glWidget = ui->openGLWidget;
    glWidget->logger = ui->logWindow;
    glWidget->reparseOnResize = ui->actionRestart_on_Resize;

    connect(ui->actionNew, &QAction::triggered, this, &MainWindow::newFile);
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::openFile);
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::saveFile);
    connect(ui->actionSave_As, &QAction::triggered, this, &MainWindow::saveAsFile);
    connect(ui->actionClose_Tab, &QAction::triggered, this, &MainWindow::closeFile);

    aspectRatioGroup = new QActionGroup(this);
    ui->action1_1->setChecked(true);
    aspectRatioGroup->addAction(ui->action1_1);
    aspectRatioGroup->addAction(ui->action3_2);
    aspectRatioGroup->addAction(ui->action4_3);
    aspectRatioGroup->addAction(ui->action16_9);
    connect(ui->action1_1, &QAction::triggered, this, &MainWindow::switchAspectRatio);
    connect(ui->action3_2, &QAction::triggered, this, &MainWindow::switchAspectRatio);
    connect(ui->action4_3, &QAction::triggered, this, &MainWindow::switchAspectRatio);
    connect(ui->action16_9, &QAction::triggered, this, &MainWindow::switchAspectRatio);
    aspectRatioGroup->setExclusive(true);

    glWidget->interpreter = new Prototype::Interpreter(ui->logWindow);

    txtFilter = tr("GFX files (*.gfx)");
}

int MainWindow::createNewTab(QString code) {
    QWidget* paren = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setMargin(0);

    CodeEditor* newEditor = new CodeEditor(0);
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
