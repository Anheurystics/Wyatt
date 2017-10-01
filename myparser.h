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


void parse(std::string code);

class MyParser {
    public:
        MyParser();
        std::map<std::string, Expr*> variables;

        void parse(std::string);
    private:
        Expr* eval_expr(Expr*);
        Expr* eval_binary(Binary*);
        void eval_stmt(Stmt*);
};

#endif //MYPARSER_H
