#ifndef CUSTOMGLWIDGET_H
#define CUSTOMGLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QTimer>

#include "mainwindow.h"
#include "codeeditor.h"
#include "interpreter.h"

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

        LogWindow* logger;
        Prototype::Interpreter* interpreter;

        bool codeChanged;

    private:
        std::string code;

        public slots:
            void updateCode();
};

#endif // CUSTOMGLWIDGET_H
