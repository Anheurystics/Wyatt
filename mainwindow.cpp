#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QObject::connect(ui->vertexSourceEditor, SIGNAL(textChanged()), ui->openGLWidget, SLOT(updateShaderCode()));
    QObject::connect(ui->fragmentSourceEditor, SIGNAL(textChanged()), ui->openGLWidget, SLOT(updateShaderCode()));

    ShaderEditor *vEditor = ui->vertexSourceEditor;
    ShaderEditor *fEditor = ui->fragmentSourceEditor;

    vEditor->setPlainText("attribute vec3 pos;\nvoid main()\n{\n\tgl_Position = vec4(pos, 1.0);\n}");
    QTextCharFormat deffmt = vEditor->currentCharFormat();
    deffmt.setUnderlineColor(QColor(Qt::red));
    deffmt.setUnderlineStyle(QTextCharFormat::WaveUnderline);
    QTextCursor cursor = vEditor->textCursor();
    cursor.movePosition(QTextCursor::End);
    vEditor->setCurrentCharFormat(deffmt);
    vEditor->setType("vertex");

    fEditor->setPlainText("void main()\n{\n\tgl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);\n}");
    fEditor->setType("fragment");
}

MainWindow::~MainWindow()
{
    delete ui;
}
