#include "glsltranspiler.h"

GLSLTranspiler::GLSLTranspiler()
{

}

void GLSLTranspiler::transpile(shared_ptr<Stmts> vertex, shared_ptr<Stmts> fragment) {
    vector<shared_ptr<Stmt>> vertex_stmts = vertex->list;
    for(unsigned int i = 0; i < vertex_stmts.size(); i++) {
        shared_ptr<Stmt> stmt = vertex_stmts[i];
        vertSource += eval_stmt(stmt);
    }

    vector<shared_ptr<Stmt>> fragment_stmts = fragment->list;
    for(unsigned int i = 0; i < fragment_stmts.size(); i++) {
        shared_ptr<Stmt> stmt = fragment_stmts[i];
        fragSource += eval_stmt(stmt);
    }

    cout << vertSource << endl;
    cout << fragSource << endl;
}

string GLSLTranspiler::eval_expr(shared_ptr<Expr> expr) {
    switch(expr->type) {
        case NODE_INT:
            return to_string(static_pointer_cast<Int>(expr)->value);
        case NODE_FLOAT:
            return to_string(static_pointer_cast<Float>(expr)->value);
        case NODE_IDENT:
            return static_pointer_cast<Ident>(expr)->name; 
        case NODE_DOT:
            {
                shared_ptr<Dot> dot = static_pointer_cast<Dot>(expr);
                return dot->shader + "." + dot->name;
            }
        case NODE_BINARY:
            {
                return eval_binary(static_pointer_cast<Binary>(expr));
            }
        default:
            return "";
    }
}

string GLSLTranspiler::eval_binary(shared_ptr<Binary> bin) {
    string op_str = "";
    switch(bin->op) {
        case OP_MULT:
            op_str = " * ";
            break;
        default:
            op_str = " ? ";
    }
    return eval_expr(bin->lhs) + op_str + eval_expr(bin->rhs);
}

string GLSLTranspiler::eval_stmt(shared_ptr<Stmt> stmt) {
    switch(stmt->type) {
        case NODE_ASSIGN:
            {
                shared_ptr<Assign> a = static_pointer_cast<Assign>(stmt);
                return eval_expr(a->lhs) + " = " + eval_expr(a->value) + ";";
            }
        default:
            return "";
    }
}
