#include "customglwidget.h"

CustomGLWidget::CustomGLWidget(QWidget *parent = 0): QOpenGLWidget(parent)
{
    //MainWindow* window = qobject_cast<MainWindow*>(this->parent());
    dirtyShaders = true;
}

void CustomGLWidget::updateCode()
{
    QObject *source = QObject::sender();
    CodeEditor *editor = qobject_cast<CodeEditor*>(source);

    code = editor->document()->toPlainText().toStdString() + '\n';
    dirtyShaders = true;
    update();
}

void CustomGLWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
}

void CustomGLWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT);

    if(dirtyShaders) {
        parser.parse(code);
        parser.setFunctions(context()->functions());
        parser.compile_program();
        parser.execute_init();

        dirtyShaders = false;
    }

    std::cout << "parser " << (!parser.status? "OK" : "ERROR") << std::endl;
    parser.execute_loop();
}

void CustomGLWidget::resizeGL(int width, int height)
{
    std::cout << "resize: " << width << " x " << height << std::endl;
}

CustomGLWidget::~CustomGLWidget()
{
    makeCurrent();
    doneCurrent();
}
