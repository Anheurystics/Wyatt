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
        void transpile(Stmts*, Stmts*);

        string eval_expr(Expr*);
        string eval_binary(Binary*);
        string eval_stmt(Stmt*);

        string vertSource = "";
        string fragSource = "";
};

#endif // GLSLTRANSPILER_H
