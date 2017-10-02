#ifndef CUSTOMGLWIDGET_H
#define CUSTOMGLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>

#include "mainwindow.h"
#include "codeeditor.h"

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

        std::string code;

        public slots:
            void updateCode();

    private:
        MyParser parser;

        GLuint program;
        GLuint vertShader, fragShader;
        GLuint vbo;

        std::map<std::string, GLuint> buffers;

        std::string vertexSource;
        std::string fragmentSource;
};

#endif // CUSTOMGLWIDGET_H
