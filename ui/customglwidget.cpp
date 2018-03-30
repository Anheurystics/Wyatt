#include "customglwidget.h"

CustomGLWidget::CustomGLWidget(QWidget *parent = 0): QOpenGLWidget(parent)
{
    updateTimer = new QTimer(this);
    updateTimer ->setInterval(17);
    connect(updateTimer, SIGNAL(timeout()), this, SLOT(update()));
    updateTimer->start();

    codeChanged = false;
    hasResized = false;
    autoExecute = true;
}

void CustomGLWidget::toggleAutoExecute(bool autoExecute) {
    this->autoExecute = autoExecute;
    if(autoExecute) {
        updateTimer->start();
    } else {
        updateTimer->stop();
    }
}

void CustomGLWidget::toggleExecute() {
    if(!autoExecute) {
        QObject* source = QObject::sender();
        QPushButton* button = qobject_cast<QPushButton*>(source);
        if(button->text() == "Play") {
            codeChanged = true;
            updateTimer->start();
            button->setText("Stop");
        } else {
            updateTimer->stop();
            button->setText("Play");
        }
    }
}

void CustomGLWidget::updateCode()
{
    QObject *source = QObject::sender();
    CodeEditor *editor = qobject_cast<CodeEditor*>(source);

    code = editor->document()->toPlainText().toStdString() + '\n';
    if(autoExecute) {
        codeChanged = true;
        hasResized = false;
    }
}

void CustomGLWidget::initializeGL()
{
    initializeOpenGLFunctions();
}

void CustomGLWidget::paintGL() {
    // Don't render if paintGL is due to resizing
    if(hasResized) {
        hasResized = false;
        return;
    }

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

    hasResized = true;
}

CustomGLWidget::~CustomGLWidget()
{
    makeCurrent();
    doneCurrent();
}
