#ifndef MYPARSER_H
#define MYPARSER_H

#include <cstdio>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <map>
#include <stack>
#include <vector>
#include <string>
#include <ctime>
#include <regex>
#include <memory>

#include "dummyglfunctions.h"
#include <QOpenGLFunctions>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>
#include <QMatrix2x2>
#include <QMatrix3x3>
#include <QMatrix4x4>

#include "scanner.h"
#include "parser.hpp"
#include "logwindow.h"
#include "helper.h"
#include "glsltranspiler.h"
#include "scope.h"
#include "scopelist.h"
#include "codeeditor.h"

// #define NO_GL
namespace Wyatt {

enum ValueType {
    VALUE_INT, VALUE_FLOAT, VALUE_BOOL, VALUE_VEC2, VALUE_VEC3, VALUE_VEC4, VALUE_MAT2, VALUE_MAT3, VALUE_MAT4, VALUE_BUFFER, VALUE_TEXTURE, VALUE_PROGRAM
};

class Value {
    public:
        ValueType type;

        Value(ValueType type): type(type) {}
};
/*
class Int: public Value {
    public:
        int value;
        Int(int value): Value(VALUE_INT), value(value) {}
};

class Float: public Value {
    public:
        float value;
        Float(float value): Value(VALUE_FLOAT) {}
};

class Bool: public Value {
    public:
        Bool(bool value): Value(VALUE_BOOL) {}
};
*/

class Vec2: public Value, public QVector2D {
    public:
        Vec2(float x, float y): Value(VALUE_VEC2), QVector2D(x, y) {}
};

class Vec3: public Value, public QVector3D {
    public:
        Vec3(float x, float y, float z): Value(VALUE_VEC2), QVector3D(x, y, z) {}
};

class Vec4: public Value, public QVector4D {
    public:
        Vec4(float x, float y, float z, float w): Value(VALUE_VEC4), QVector4D(x, y, z, w) {}
};

class Mat2: public Value, public QMatrix2x2 {
    public:
        Mat2(float a, float b, float c, float d): Value(VALUE_MAT2) {
            float comp[] = {a, b, c, d};
            copyDataTo(comp);
        }
};

class Mat3: public Value, public QMatrix3x3 {
    public:
        Mat3(float a, float b, float c, float d, float e, float f, float g, float h, float i): Value(VALUE_MAT3) {
            float comp[] = {a, b, c, d, e, f, g, h, i};
            copyDataTo(comp);
        }
};

class Mat4: public Value, public QMatrix4x4 {
    public:
        float a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p;
        Mat4(float a, float b, float c, float d, float e, float f, float g, float h, float i, float j, float k, float l, float m, float n, float o, float p): Value(VALUE_MAT4) {
            float comp[] = {a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p};
            copyDataTo(comp);
        }
};

class Interpreter {
    public:
        Interpreter(LogWindow*);

        int status = -1;
        string workingDir = "";

        unsigned int width, height;

        void parse(string, int*);
        Expr::ptr execute_stmts(Stmts::ptr);
        void prepare();
        void load_imports();
        void load_import(string);
        void execute_init();
        void execute_loop();
        void compile_program();
        void compile_shader(GLuint*, Shader::ptr);
        void setFunctions(QOpenGLFunctions* gl) {
            #ifndef NO_GL
            this->gl = gl;
            #endif
        }
        void reset();
        void resize(int, int);

    private:
        Wyatt::Scanner scanner;
        Wyatt::Parser parser;

        Scope::ptr globalScope;
        std::stack<ScopeList::ptr> functionScopeStack;
        map<string, shared_ptr<ShaderPair>> shaders;
        map<string, FuncDef::ptr> functions;
        map<string, ProgramLayout::ptr> layouts;

        vector<string> imports;
        vector<Decl::ptr> globals;

        int activeTextureSlot = 0;

        bool breakable = false;

        string current_program_name;
        Program::ptr current_program = nullptr;

        FuncDef::ptr init = nullptr;
        FuncDef::ptr loop = nullptr;
        Invoke::ptr init_invoke;
        Invoke::ptr loop_invoke;

        #ifdef NO_GL
        DummyGLFunctions* gl = new DummyGLFunctions();
        #else
        QOpenGLFunctions* gl = nullptr;
        #endif

        string print_expr(Expr::ptr);
        Expr::ptr eval_expr(Expr::ptr);
        Expr::ptr eval_binary(shared_ptr<Binary>);
        Expr::ptr invoke(shared_ptr<Invoke>);
        Expr::ptr eval_stmt(shared_ptr<Stmt>);
        Expr::ptr resolve_vector(vector<Expr::ptr>);

        LogWindow* logger;

        GLSLTranspiler* transpiler;

        unsigned int line = 1;
        unsigned int column = 1;
};
}

#endif //MYPARSER_H
