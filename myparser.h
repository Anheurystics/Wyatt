#ifndef MYPARSER_H
#define MYPARSER_H

#include <cstdio>
#include <iostream>
#include <iomanip>
#include <map>
#include <vector>
#include <string>
#include <ctime>

#include <QOpenGLFunctions>

#include "parser.h"
#include "scanner.h"

class MyParser {
    public:
        struct Buffer {
            GLuint handle;
            std::vector<float> data;
        };

        struct Program {
            GLuint handle;
            GLuint vert, frag;
        };

        MyParser();
        std::map<std::string, Expr*> variables;
        std::map<std::string, Buffer*> buffers;

        Program* program = NULL;

        int status = -1;

        void parse(std::string);
        void execute_stmts(Stmts*);
        void execute_init();
        void execute_loop();
        void compile_program();
        void compile_shader(GLuint*, ShaderSource*);
        void setFunctions(QOpenGLFunctions* gl) {
            this->gl = gl;
        }
    private:
        Stmts* init = NULL;
        Stmts* loop = NULL;
        ShaderSource* vertSource = NULL;
        ShaderSource* fragSource = NULL;
        QOpenGLFunctions* gl;

        Expr* eval_expr(Expr*);
        Expr* eval_binary(Binary*);
        void eval_stmt(Stmt*);
};

#endif //MYPARSER_H
