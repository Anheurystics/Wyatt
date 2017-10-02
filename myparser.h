#ifndef MYPARSER_H
#define MYPARSER_H

#include <cstdio>
#include <iostream>
#include <iomanip>
#include <map>
#include <vector>
#include <string>

#include <QOpenGLFunctions>

#include "parser.h"
#include "scanner.h"

class MyParser {
    public:
        struct Buffer {
            GLuint handle;
            std::vector<float> data;
        };

        MyParser();
        std::map<std::string, Expr*> variables;
        std::map<std::string, Buffer*> buffers;
        int status = -1;

        void parse(std::string);
        void execute_stmts(Stmts*);
        void execute_init();
        void execute_loop();
        void setFunctions(QOpenGLFunctions* gl) {
            this->gl = gl;
        }
    private:
        Stmts* init = NULL;
        Stmts* loop = NULL;
        QOpenGLFunctions* gl;

        Expr* eval_expr(Expr*);
        Expr* eval_binary(Binary*);
        void eval_stmt(Stmt*);
};

#endif //MYPARSER_H
