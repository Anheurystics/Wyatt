#ifndef GLSLTRANSPILER_H
#define GLSLTRANSPILER_H

#include <string>
#include <iostream>
#include "nodes.h"

using namespace std;

class GLSLTranspiler
{
    public:
        GLSLTranspiler();
        void transpile(shared_ptr<Stmts>, shared_ptr<Stmts>);

        string eval_expr(shared_ptr<Expr>);
        string eval_binary(shared_ptr<Binary>);
        string eval_stmt(shared_ptr<Stmt>);

        string vertSource = "";
        string fragSource = "";
};

#endif // GLSLTRANSPILER_H
