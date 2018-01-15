#-------------------------------------------------
#
# Project created by QtCreator 2017-07-14T11:18:28
#
#-------------------------------------------------

QT       += core gui
CONFIG   += console

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = prototype
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

LIBS += -lSOIL

win32 {
    LIBS += -lopengl32
}

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    customglwidget.cpp \
    interpreter.cpp \
    scanner.cpp \
    parser.cpp \
    codeeditor.cpp \
    highlighter.cpp \
    logwindow.cpp \
    helper.cpp \
    glsltranspiler.cpp \
    scope.cpp \
    scopelist.cpp

HEADERS += \
    mainwindow.h \
    customglwidget.h \
	interpreter.h \
    scanner.h \
    parser.hpp \
    location.hh \
    stack.hh \
    position.hh \
    nodes.h \
    codeeditor.h \
    highlighter.h \
    logwindow.h \
    helper.h \
    glsltranspiler.h \
    scope.h \
    scopelist.h

FORMS += \
    mainwindow.ui

DISTFILES += \
    parser.y \
    scanner.l \
    main.txt \
    utils.txt \
    todo
