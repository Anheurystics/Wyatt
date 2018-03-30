#-------------------------------------------------
#
# Project created by QtCreator 2017-07-14T11:18:28
#
#-------------------------------------------------

QT       += core gui
CONFIG(debug, debug|release) {
    CONFIG   += console
}

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = prototype
TEMPLATE = app

include( ui/ui.pri )
include( lang/lang.pri )

win32 {
    LIBS += -lopengl32
}

flexsource.commands = flex ${QMAKE_FILE_IN}
flexsource.input = FLEXSOURCES
flexsource.output = scanner.cpp
flexsource.variable_out = SOURCES
flexsource.name = Flex Sources ${QMAKE_FILE_IN}
flexsource.CONFIG += target_predeps
QMAKE_EXTRA_COMPILERS += flexsource

bisonsource.commands = bison ${QMAKE_FILE_IN}
bisonsource.input = BISONSOURCES
bisonsource.output = parser.cpp
bisonsource.variable_out = SOURCES
bisonsource.name = Bison Sources ${QMAKE_FILE_IN}
bisonsource.CONFIG += target_predeps
QMAKE_EXTRA_COMPILERS += bisonsource

bisonheader.commands = echo
bisonheader.input = BISONSOURCES
bisonheader.output = parser.hpp stack.hh location.hh position.hh
bisonheader.variable_out = HEADERS
bisonheader.name = Bison Headers ${QMAKE_FILE_IN}
bisonheader.CONFIG += target_predeps no_link
QMAKE_EXTRA_COMPILERS += bisonheader

SOURCES += \
    main.cpp \
