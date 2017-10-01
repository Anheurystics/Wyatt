#include "customglwidget.h"

CustomGLWidget::CustomGLWidget(QWidget *parent = 0): QOpenGLWidget(parent), program(0)
{
    MainWindow* window = qobject_cast<MainWindow*>(this->parent());

    vertexSource = "attribute vec3 pos;\nvoid main()\n{\n\tgl_Position = vec4(pos, 1.0);\n}";
    fragmentSource = "void main()\n{\n\tgl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);\n}";

    dirtyShaders = false;
}

void CustomGLWidget::uploadShaders() {
   	GLuint newProgram = glCreateProgram();

	GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
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

    GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    const char* fSrc = fragmentSource.c_str();
    glShaderSource(fragShader, 1, &fSrc, NULL);
    glCompileShader(fragShader);

    glGetShaderInfoLog(fragShader, 256, 0, log);
    glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
    if(success != GL_TRUE) {
    	std::cout << "fragment shader error\n---------------------\n" << log << std::endl;
        return;
    }

    glAttachShader(newProgram, vertShader);
    glAttachShader(newProgram, fragShader);
    glLinkProgram(newProgram);

    glGetProgramiv(newProgram, GL_LINK_STATUS, &success);
    glGetProgramInfoLog(program, 256, 0, log);
    if(success != GL_TRUE) {
        std::cout << "program error\n-------------\n" << log << std::endl;
        return;
    }

    glDeleteShader(vertShader);
    glDeleteShader(fragShader);
    glDeleteProgram(program);
    program = newProgram;
}

void CustomGLWidget::updateShaderCode()
{
    QObject *source = QObject::sender();
    ShaderEditor *editor = qobject_cast<ShaderEditor*>(source);

    //if(editor->getType().compare("vertex") == 0)
    //	vertexSource = editor->document()->toPlainText().toStdString();

    //if(editor->getType().compare("fragment") == 0)
    //    fragmentSource = editor->document()->toPlainText().toStdString();

    parse(editor->document()->toPlainText().toStdString() + '\n');

    dirtyShaders = true;

    update();
}

void CustomGLWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);

    glGenBuffers(1, &vbo);

    GLfloat triangle[] = {
        -1.0f, -1.0f, 0.0f,
         0.0f,  1.0f, 0.0f,
        1.0f, -1.0f, 0.0f
    };

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, 36, triangle, GL_STATIC_DRAW);

    uploadShaders();

    buffers["vbo"] = vbo;
}

void CustomGLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);

    if(dirtyShaders) {
        dirtyShaders = false;
        uploadShaders();
    }

    glUseProgram(program);

    glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, (void*)0);
    glEnableVertexAttribArray(0);

    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void CustomGLWidget::resizeGL(int width, int height)
{

}

CustomGLWidget::~CustomGLWidget()
{
    makeCurrent();
    doneCurrent();
}
