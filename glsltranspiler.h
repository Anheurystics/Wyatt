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
        string transpile(Shader_ptr);
        
        Shader_ptr shader;

        string eval_expr(Expr_ptr);
        string eval_invoke(Invoke_ptr);
        string eval_binary(Binary_ptr);
        string eval_stmt(Stmt_ptr);
        string resolve_vector(vector<Expr_ptr>);
};

#endif // GLSLTRANSPILER_H
