#ifndef CUSTOMGLWIDGET_H
#define CUSTOMGLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>

#include "mainwindow.h"
#include "shadereditor.h"

#include "myparser.h"

#include <iostream>
#include <cstring>
#include <map>

class CustomGLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit CustomGLWidget(QWidget *parent);
    ~CustomGLWidget();

    void initializeGL();
    void paintGL();
    void resizeGL(int, int);

private:
    void uploadShaders();
    bool dirtyShaders;

public slots:
    void updateShaderCode();

private:
    GLuint program;
    GLuint vbo;

    std::map<std::string, GLuint> buffers;

    std::string vertexSource;
    std::string fragmentSource;
};

#endif // CUSTOMGLWIDGET_H
