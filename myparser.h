#ifndef MYPARSER_H
#define MYPARSER_H

#include <cstdio>
#include <iostream>
#include <iomanip>
#include <map>
#include <vector>
#include <string>

#if !noopengl
#include <QOpenGLFunctions>
#endif

#include "parser.h"
#include "scanner.h"


void parse(std::string code);

class MyParser {
    public:
        MyParser();
        std::map<std::string, Expr*> variables;
        int status = -1;

        void parse(std::string);
        void execute_stmts(Stmts*);
#include <QOpenGLFunctions>
        void execute_init();
        void execute_loop();
    private:
        Stmts* init = NULL;
        Stmts* loop = NULL;

        Expr* eval_expr(Expr*);
        Expr* eval_binary(Binary*);
        void eval_stmt(Stmt*);
};

#endif //MYPARSER_H
