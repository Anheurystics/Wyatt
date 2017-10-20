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
        bool dirtyShaders;

        std::string code;

        public slots:
            void updateCode();

    private:
        MyParser parser;
};

#endif // CUSTOMGLWIDGET_H
