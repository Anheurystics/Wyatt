#include "customglwidget.h"

CustomGLWidget::CustomGLWidget(QWidget *parent = 0): QOpenGLWidget(parent)
{
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
}

void CustomGLWidget::paintGL() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    if(codeChanged) {
        codeChanged = false;

        glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        logger->clear();

        interpreter->reset();
        interpreter->parse(code, &(interpreter->status));
        interpreter->load_imports();
        interpreter->setFunctions(this);
        interpreter->prepare();
        interpreter->compile_program();
        interpreter->execute_init();
    }

    if(interpreter->status == 0) {
        interpreter->execute_loop();
    }
}

void CustomGLWidget::resizeGL(int width, int height)
{
    height = width / aspectRatio;
    resize(width, height);
    interpreter->width = width;
    interpreter->height = height;
    if(reparseOnResize->isChecked()) {
        codeChanged = true;
    }
}

CustomGLWidget::~CustomGLWidget()
{
    makeCurrent();
    doneCurrent();
}
