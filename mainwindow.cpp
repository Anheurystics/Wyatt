#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent, std::string startupFile) : QMainWindow(parent)
{
    resize(800, 600);
    setAnimated(true);
    setTabShape(QTabWidget::Triangular);

    // Create actions
    actionNew = new QAction(this);
    actionOpen = new QAction(this);
    actionSave = new QAction(this);
    actionSave_As = new QAction(this);
    actionClose_Tab = new QAction(this);
    action1_1 = new QAction(this);
    action1_1->setCheckable(true);
    action3_2 = new QAction(this);
    action3_2->setCheckable(true);
    action4_3 = new QAction(this);
    action4_3->setCheckable(true);
    action16_9 = new QAction(this);
    action16_9->setCheckable(true);
    actionRestart_on_Resize = new QAction(this);
    actionRestart_on_Resize->setCheckable(true);

    connect(actionNew, &QAction::triggered, this, &MainWindow::newFile);
    connect(actionOpen, &QAction::triggered, this, &MainWindow::openFile);
    connect(actionSave, &QAction::triggered, this, &MainWindow::saveFile);
    connect(actionSave_As, &QAction::triggered, this, &MainWindow::saveAsFile);
    connect(actionClose_Tab, &QAction::triggered, this, &MainWindow::closeFile);

    aspectRatioGroup = new QActionGroup(this);
    action1_1->setChecked(true);
    aspectRatioGroup->addAction(action1_1);
    aspectRatioGroup->addAction(action3_2);
    aspectRatioGroup->addAction(action4_3);
    aspectRatioGroup->addAction(action16_9);
    connect(action1_1, &QAction::triggered, this, &MainWindow::switchAspectRatio);
    connect(action3_2, &QAction::triggered, this, &MainWindow::switchAspectRatio);
    connect(action4_3, &QAction::triggered, this, &MainWindow::switchAspectRatio);
    connect(action16_9, &QAction::triggered, this, &MainWindow::switchAspectRatio);
    aspectRatioGroup->setExclusive(true);

    centralWidget = new QWidget(this);
    centralLayout = new QGridLayout(centralWidget);
    centralLayout->setSpacing(6);
    centralLayout->setMargin(4);

    // Split between code/log window and output
    hSplit = new QSplitter(centralWidget);
    hSplit->setOrientation(Qt::Horizontal);

    // Split between code and log window
    vSplit = new QSplitter(hSplit);
    vSplit->setOrientation(Qt::Vertical);
    QSizePolicy vSplitSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    vSplitSizePolicy.setHorizontalStretch(3);
    vSplit->setSizePolicy(vSplitSizePolicy);

    editors = new QTabWidget(vSplit);
    tab = new QWidget();
    tabLayout = new QVBoxLayout(tab);
    tabLayout->setSpacing(6);
    tabLayout->setMargin(0);
    codeEditor = new CodeEditor(tab);
    editors->addTab(tab, QString());
    tabLayout->addWidget(codeEditor);

    logWindow = new LogWindow(vSplit);

    vSplit->addWidget(editors);
    vSplit->addWidget(logWindow);

    QSizePolicy editorSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    editorSizePolicy.setVerticalStretch(4);
    editors->setSizePolicy(editorSizePolicy);

    QSizePolicy logWindowSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    logWindowSizePolicy.setVerticalStretch(1);
    logWindow->setSizePolicy(logWindowSizePolicy);

    hSplitWidget = new QWidget(hSplit);
    hSplitLayout = new QVBoxLayout(hSplitWidget);
    hSplitLayout->setSpacing(6);
    hSplitLayout->setMargin(0);

    openGLWidget = new CustomGLWidget(hSplitWidget);

    hSplitLayout->addWidget(openGLWidget);

    hSplit->addWidget(vSplit);
    hSplit->addWidget(hSplitWidget);
    hSplit->setSizes(QList<int>() << 300 << 300);

    centralLayout->addWidget(hSplit, 0, 0, 1, 1);

    setCentralWidget(centralWidget);
    menuBar = new QMenuBar(this);
    menuBar->setGeometry(QRect(0, 0, 800, 20));
    menuFile = new QMenu(menuBar);
    menuOptions = new QMenu(menuBar);
    menuAspect_Ratio = new QMenu(menuOptions);
    setMenuBar(menuBar);

    menuBar->addAction(menuFile->menuAction());
    menuBar->addAction(menuOptions->menuAction());
    menuFile->addAction(actionNew);
    menuFile->addAction(actionOpen);
    menuFile->addAction(actionSave);
    menuFile->addAction(actionSave_As);
    menuFile->addAction(actionClose_Tab);
    menuOptions->addAction(menuAspect_Ratio->menuAction());
    menuOptions->addAction(actionRestart_on_Resize);
    menuAspect_Ratio->addAction(action1_1);
    menuAspect_Ratio->addAction(action3_2);
    menuAspect_Ratio->addAction(action4_3);
    menuAspect_Ratio->addAction(action16_9);

    setWindowTitle(QApplication::translate("MainWindow", "Prototype", Q_NULLPTR));

    actionNew->setText(QApplication::translate("MainWindow", "New", Q_NULLPTR));
    actionNew->setShortcut(QApplication::translate("MainWindow", "Ctrl+N", Q_NULLPTR));
    actionOpen->setText(QApplication::translate("MainWindow", "Open", Q_NULLPTR));
    actionOpen->setShortcut(QApplication::translate("MainWindow", "Ctrl+O", Q_NULLPTR));
    actionSave->setText(QApplication::translate("MainWindow", "Save", Q_NULLPTR));
    actionSave->setShortcut(QApplication::translate("MainWindow", "Ctrl+S", Q_NULLPTR));
    actionSave_As->setText(QApplication::translate("MainWindow", "Save As...", Q_NULLPTR));
    actionSave_As->setShortcut(QApplication::translate("MainWindow", "Ctrl+Shift+S", Q_NULLPTR));
    actionClose_Tab->setText(QApplication::translate("MainWindow", "Close Tab", Q_NULLPTR));
    actionClose_Tab->setShortcut(QApplication::translate("MainWindow", "Ctrl+W", Q_NULLPTR));
    action1_1->setText(QApplication::translate("MainWindow", "1:1", Q_NULLPTR));
    action3_2->setText(QApplication::translate("MainWindow", "3:2", Q_NULLPTR));
    action4_3->setText(QApplication::translate("MainWindow", "4:3", Q_NULLPTR));
    action16_9->setText(QApplication::translate("MainWindow", "16:9", Q_NULLPTR));
    actionRestart_on_Resize->setText(QApplication::translate("MainWindow", "Restart on Resize", Q_NULLPTR));
    editors->setTabText(editors->indexOf(tab), QApplication::translate("MainWindow", "untitled", Q_NULLPTR));
    menuFile->setTitle(QApplication::translate("MainWindow", "File", Q_NULLPTR));
    menuOptions->setTitle(QApplication::translate("MainWindow", "Options", Q_NULLPTR));
    menuAspect_Ratio->setTitle(QApplication::translate("MainWindow", "Aspect Ratio", Q_NULLPTR));

    editors->setCurrentIndex(0);
    editors->setTabsClosable(true);

    QMetaObject::connectSlotsByName(this);

    startupCode = "func init(){\n\n}\n\nfunc loop(){\n\n}\n";

    currentEditor = codeEditor;

    connect(editors, SIGNAL(currentChanged(int)), this, SLOT(switchTab(int)));
    connect(editors, SIGNAL(tabCloseRequested(int)), this, SLOT(closeTab(int)));

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
        editors->setTabText(0, startupFileStr);
    } else {
        currentEditor->setPlainText(startupCode);
    }

    connect(currentEditor, SIGNAL(textChanged()), openGLWidget, SLOT(updateCode()));

    highlighter = new Highlighter(currentEditor->document());

    openGLWidget->logger = logWindow;
    openGLWidget->reparseOnResize = actionRestart_on_Resize;

    openGLWidget->interpreter = new Prototype::Interpreter(logWindow);

    txtFilter = tr("GFX files (*.gfx)");
}

int MainWindow::createNewTab(QString code) {
    QWidget* paren = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setMargin(0);

    CodeEditor* newEditor = new CodeEditor(paren);
    newEditor->setPlainText(code);
    paren->setLayout(layout);
    layout->addWidget(newEditor);
    editors->addTab(paren, "untitled");

    return editors->count() - 1;
}

int MainWindow::createOpenTab(QString code, QString fileName) {
    int newTab = createNewTab(code);
    CodeEditor* editor = (CodeEditor*)editors->widget(newTab)->findChild<QPlainTextEdit*>();
    editor->fileInfo.setFile(fileName);
    editors->setTabText(newTab, editor->fileInfo.fileName());
    return newTab;
}

MainWindow::~MainWindow()
{
}
