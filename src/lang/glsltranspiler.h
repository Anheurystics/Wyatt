#ifndef GLSLTRANSPILER_H
#define GLSLTRANSPILER_H

#include <string>
#include <iostream>
#include "nodes.h"

#include "logwindow.h"

using namespace std;

class GLSLTranspiler
{
    public:
        GLSLTranspiler(LogWindow*);
        string transpile(Shader::ptr);
        
        LogWindow* logger;
        Shader::ptr shader;
        map<string, ProgramLayout::ptr>* layouts;

        string eval_expr(Expr::ptr);
        string eval_invoke(Invoke::ptr);
        string eval_binary(Binary::ptr);
        string eval_unary(Unary::ptr);
        string eval_stmt(Stmt::ptr);
        string eval_vector(vector<Expr::ptr>);
        string resolve_ident(Ident::ptr);
        string resolve_vector(vector<Expr::ptr>);
        string resolve_binary(Binary::ptr);
        string resolve_unary(Unary::ptr);

        map<string, string> localtypes;
};

#endif // GLSLTRANSPILER_H
