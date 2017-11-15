#include "glsltranspiler.h"

GLSLTranspiler::GLSLTranspiler()
{

}

void GLSLTranspiler::transpile(Stmts* vertex, Stmts* fragment) {
    vector<Stmt*> vertex_stmts = vertex->list;
    for(unsigned int i = 0; i < vertex_stmts.size(); i++) {
        Stmt* stmt = vertex_stmts[i];
        vertSource += eval_stmt(stmt);
    }

    vector<Stmt*> fragment_stmts = fragment->list;
    for(unsigned int i = 0; i < fragment_stmts.size(); i++) {
        Stmt* stmt = fragment_stmts[i];
        fragSource += eval_stmt(stmt);
    }

    cout << vertSource << endl;
    cout << fragSource << endl;
}

string GLSLTranspiler::eval_expr(Expr* expr) {
    switch(expr->type) {
        case NODE_INT:
            return to_string(((Int*)expr)->value);
        case NODE_FLOAT:
            return to_string(((Float*)expr)->value);
        case NODE_IDENT:
            return ((Ident*)expr)->name;
        case NODE_DOT:
            {
                Dot* dot = (Dot*)expr;
                return dot->shader + "." + dot->name;
            }
        case NODE_BINARY:
            {
                return eval_binary((Binary*)expr);
            }
        default:
            return "";
    }
}

string GLSLTranspiler::eval_binary(Binary* bin) {
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

string GLSLTranspiler::eval_stmt(Stmt* stmt) {
    switch(stmt->type) {
        case NODE_ASSIGN:
            {
                Assign* a = (Assign*)stmt;
                return eval_expr(a->lhs) + " = " + eval_expr(a->value) + ";";
            }
        default:
            return "";
    }
}
