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
        string transpile(shared_ptr<Shader>);
        
        Shader_ptr shader;

        string eval_expr(shared_ptr<Expr>);
        string eval_binary(shared_ptr<Binary>);
        string eval_stmt(shared_ptr<Stmt>);
        string resolve_vector(vector<Expr_ptr>);
};

#endif // GLSLTRANSPILER_H
