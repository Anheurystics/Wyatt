cmake_minimum_required(VERSION 3.10)

find_package(OpenGL)
find_package(BISON)
find_package(FLEX)
find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Widgets REQUIRED)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

bison_target(WyattParser src/lang/parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.cpp DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/parser.hpp)
flex_target(WyattLexer src/lang/scanner.l ${CMAKE_CURRENT_BINARY_DIR}/scanner.cpp)

add_flex_bison_dependency(WyattLexer WyattParser)

set(SOURCES src/main.cpp
    src/lang/glsltranspiler.cpp
    src/lang/helper.cpp
    src/lang/interpreter.cpp
    src/lang/scope.cpp
    src/lang/scopelist.cpp
    src/ui/codeeditor.cpp
    src/ui/customglwidget.cpp
    src/ui/highlighter.cpp
    src/ui/logwindow.cpp
    src/ui/mainwindow.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/scanner.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/parser.cpp
)

set(INCLUDE_DIRS ${INCLUDE_DIRS} ${OPENGL_INCLUDE_DIRS})
set(LIBS ${LIBS} ${OPENGL_LIBRARY})

include_directories(${INCLUDE_DIRS})
include_directories(${QT_INCLUDES})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(src/lang)
include_directories(src/ui)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
add_executable(wyatt ${SOURCES} ${BISON_WyattParser_OUPUTS} ${FLEX_WyattLexer_OUTPUTS} )
target_link_libraries(wyatt ${LIBS})

qt5_use_modules(wyatt Core Gui Widgets)
