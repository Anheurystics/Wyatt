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
    GLSLTranspiler(LogWindow *);
    string transpile(Shader_ptr);

    LogWindow *logger;
    Shader_ptr shader;
    map<string, ProgramLayout_ptr> *layouts;

    string eval_expr(Expr_ptr);
    string eval_invoke(Invoke_ptr);
    string eval_binary(Binary_ptr);
    string eval_unary(Unary_ptr);
    string eval_stmt(Stmt_ptr);
    string eval_vector(vector<Expr_ptr>);
    string resolve_ident(Ident_ptr);
    string resolve_vector(vector<Expr_ptr>);
    string resolve_binary(Binary_ptr);
    string resolve_unary(Unary_ptr);

    map<string, string> localtypes;
};

#endif // GLSLTRANSPILER_H
