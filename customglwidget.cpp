#include "customglwidget.h"

CustomGLWidget::CustomGLWidget(QWidget *parent = 0): QOpenGLWidget(parent), program(0)
{
    MainWindow* window = qobject_cast<MainWindow*>(this->parent());

    vertexSource = "attribute vec3 pos;\nvoid main()\n{\n\tgl_Position = vec4(pos, 1.0);\n}";
    fragmentSource = "void main()\n{\n\tgl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);\n}";

    dirtyShaders = false;
}

void CustomGLWidget::uploadShaders() {
    if(!program) {
        program = glCreateProgram();
        vertShader = glCreateShader(GL_VERTEX_SHADER);
        fragShader = glCreateShader(GL_FRAGMENT_SHADER);

        glAttachShader(program, vertShader);
        glAttachShader(program, fragShader);
    }

    const char* vSrc = vertexSource.c_str();
    glShaderSource(vertShader, 1, &vSrc, NULL);
    glCompileShader(vertShader);

    GLint success;
    char log[256];

    glGetShaderInfoLog(vertShader, 256, 0, log);
    glGetShaderiv(vertShader, GL_COMPILE_STATUS, &success);
    if(success != GL_TRUE) {
        std::cout << "vertex shader error\n-------------------\n" << success << " " << log << std::endl;
        return;
    }

    const char* fSrc = fragmentSource.c_str();
    glShaderSource(fragShader, 1, &fSrc, NULL);
    glCompileShader(fragShader);

    glGetShaderInfoLog(fragShader, 256, 0, log);
    glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
    if(success != GL_TRUE) {
        std::cout << "fragment shader error\n---------------------\n" << log << std::endl;
        return;
    }

    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    glGetProgramInfoLog(program, 256, 0, log);
    if(success != GL_TRUE) {
        std::cout << "program error\n-------------\n" << log << std::endl;
        return;
    }
}

void CustomGLWidget::updateCode()
{
    QObject *source = QObject::sender();
    CodeEditor *editor = qobject_cast<CodeEditor*>(source);

    //if(editor->getType().compare("vertex") == 0)
    //	vertexSource = editor->document()->toPlainText().toStdString();

    //if(editor->getType().compare("fragment") == 0)
    //    fragmentSource = editor->document()->toPlainText().toStdString();

    code = editor->document()->toPlainText().toStdString() + '\n';
    dirtyShaders = true;
    update();
}

void CustomGLWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);

    uploadShaders();
}

void CustomGLWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT);

    if(dirtyShaders) {
        parser.parse(code);
        parser.setFunctions(context()->functions());
        parser.execute_init();

        dirtyShaders = false;
    }

    glUseProgram(program);

    std::cout << "parser " << (!parser.status? "OK" : "ERROR") << std::endl;
    parser.execute_loop();
}

void CustomGLWidget::resizeGL(int width, int height)
{

}

CustomGLWidget::~CustomGLWidget()
{
    makeCurrent();
    doneCurrent();
}
