#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    CodeEditor* vEditor = ui->codeEditor;
    QObject::connect(vEditor, SIGNAL(textChanged()), ui->openGLWidget, SLOT(updateCode()));

    vEditor->setPlainText("vert basic `\n#version 130\n\nin vec3 pos;\n\nvoid main() {\n\tgl_Position = vec4(pos, 1.0);\n}\n`;\n\nfrag basic `#version 130\n\nout vec4 FragColor;\n\nvoid main() {\n\tgl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);\n}\n`;\n\ninit {\n\tallocate a;\n\ta[pos] <- [-1,-1,0], [1,-1,0], [0,1,0];\n}\n\nloop {\n\tuse basic;\n\tdraw a;\n}\n");
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
