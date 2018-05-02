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

TARGET = wyatt 
TEMPLATE = app

include( src/ui/ui.pri )
include( src/lang/lang.pri )

win32 {
    LIBS += -lopengl32
}

linux-g++ {
    CONFIG += debug_and_release debug_and_release_target
}

flexsource.commands = flex --outfile=build/scanner.cpp ${QMAKE_FILE_IN}
flexsource.input = FLEXSOURCES
flexsource.output = build/scanner.cpp
flexsource.variable_out = SOURCES
flexsource.name = Flex Sources ${QMAKE_FILE_IN}
flexsource.CONFIG += target_predeps
QMAKE_EXTRA_COMPILERS += flexsource

bisonsource.commands = bison --defines=build/parser.hpp --output=build/parser.cpp ${QMAKE_FILE_IN}
bisonsource.input = BISONSOURCES
bisonsource.output = build/parser.cpp
bisonsource.variable_out = SOURCES
bisonsource.name = Bison Sources ${QMAKE_FILE_IN}
bisonsource.CONFIG += target_predeps
QMAKE_EXTRA_COMPILERS += bisonsource

bisonheader.commands = echo
bisonheader.input = BISONSOURCES
bisonheader.output = build/parser.hpp build/stack.hh build/location.hh build/position.hh
bisonheader.variable_out = HEADERS
bisonheader.name = Bison Headers ${QMAKE_FILE_IN}
bisonheader.CONFIG += target_predeps no_link
QMAKE_EXTRA_COMPILERS += bisonheader

INCLUDEPATH += build

SOURCES += \
    src/main.cpp \
