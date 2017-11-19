#include "customglwidget.h"

CustomGLWidget::CustomGLWidget(QWidget *parent = 0): QOpenGLWidget(parent)
{
    MainWindow* window = qobject_cast<MainWindow*>(this->parent());

    QTimer* timer = new QTimer(this); 
    timer->setInterval(17);
    connect(timer, SIGNAL(timeout()), this, SLOT(update()));
    timer->start();

    codeChanged = false;
}

void CustomGLWidget::updateCode()
{
    QObject *source = QObject::sender();
    CodeEditor *editor = qobject_cast<CodeEditor*>(source);

    code = editor->document()->toPlainText().toStdString() + '\n';
    codeChanged = true;

    update();
}

void CustomGLWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
}

void CustomGLWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    if(codeChanged) {
        codeChanged = false;

        logger->clear();

        interpreter->parse(code);
        interpreter->setFunctions(context()->functions());
        interpreter->compile_program();
        interpreter->prepare();
        interpreter->execute_init();
    }

    if(interpreter->status == 0) {
        interpreter->execute_loop();
    }
}

void CustomGLWidget::resizeGL(int width, int height)
{
    resize(width, width);
}

CustomGLWidget::~CustomGLWidget()
{
    makeCurrent();
    doneCurrent();
}
