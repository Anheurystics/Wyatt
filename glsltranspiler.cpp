#include "glsltranspiler.h"

GLSLTranspiler::GLSLTranspiler()
{

}

string GLSLTranspiler::transpile(Shader_ptr shader) {
    this->shader = shader;

    string source = "#version 130\n";

    for(vector<Decl_ptr>::iterator it = shader->inputs->list.begin(); it != shader->inputs->list.end(); ++it) {
        Decl_ptr decl = *it;
        source += "in " + decl->datatype->name + " " + decl->name->name + ";\n";
    }

    for(map<string, string>::iterator it = shader->uniforms->begin(); it != shader->uniforms->end(); ++it) {
        string type = it->second;
        if(type == "texture2D") {
            type = "sampler2D";
        }
        source += "uniform " + type + " " + it->first + ";\n";
    }

    for(vector<Decl_ptr>::iterator it = shader->outputs->list.begin(); it != shader->outputs->list.end(); ++it) {
        Decl_ptr decl = *it;
        if(decl->name->name == "FinalPosition") {
            continue;
        }
        source += "out " + decl->datatype->name + " " + decl->name->name + ";\n";
    }

    for(map<string, FuncDef_ptr>::iterator it = shader->functions->begin(); it != shader->functions->end(); ++it) {
        string type = "var";
        if(it->first == "main") {
            type = "void";
        }

        source += type + " " + it->first + "() {\n";

        FuncDef_ptr def = it->second;
        Stmts_ptr stmts = def->stmts;
        for(vector<Stmt_ptr>::iterator jt = stmts->list.begin(); jt != stmts->list.end(); ++jt) {
            source += eval_stmt(*jt) + "\n";
        }

        source += "}\n";
    }

    return source;
}

string GLSLTranspiler::resolve_ident(Ident_ptr ident) {
    string name = ident->name;
    if(localtypes.find(name) != localtypes.end()) {
        return localtypes[name];
    } else
    if(shader->uniforms->find(name) != shader->uniforms->end()) {
        return shader->uniforms->at(name);
    } else {
        for(vector<Decl_ptr>::iterator jt = shader->inputs->list.begin(); jt != shader->inputs->list.end(); ++jt) {
            Decl_ptr decl = *jt;
            if(decl->name->name == name) {
                return decl->datatype->name;
            }
        }
    }

    return "";
}

string GLSLTranspiler::resolve_vector(vector<Expr_ptr> list) {
    string output = "(";
    int n = 0;
    for(vector<Expr_ptr>::iterator it = list.begin(); it != list.end(); ++it) {
        Expr_ptr expr = *it;
        if(expr->type == NODE_FLOAT || expr->type == NODE_INT) {
            output += (n != 0? ",":"") + eval_expr(expr);
            n += 1;
        } else
        if(expr->type == NODE_VECTOR2) {
            output += (n != 0? ",":"") + eval_expr(expr);
            n += 2;
        } else 
        if(expr->type == NODE_VECTOR3) {
            output += (n != 0? ",":"") + eval_expr(expr);
            n += 3;
        } else
        if(expr->type == NODE_VECTOR4) {
            output += (n != 0? ",":"") + eval_expr(expr);
            n += 4;
        } else
        if(expr->type == NODE_IDENT) {
            Ident_ptr ident = static_pointer_cast<Ident>(expr);
            string type = resolve_ident(ident);
            if(type != "") {
                output += (n != 0? "," : "") + ident->name;
            }
            if(type == "float" || type == "int") {
                n += 1;
            } else
            if(type == "vec2") {
                n += 2;
            } else
            if(type == "vec3") {
                n += 3;
            } else
            if(type == "vec4") {
                n += 4;
            }
        } else
        if(expr->type == NODE_BINARY) {

        }
    }

    return "vec" + to_string(n) + output + ")";
}

string GLSLTranspiler::eval_invoke(Invoke_ptr invoke) {
    string output = invoke->ident->name + "(";
    for(unsigned int i = 0; i < invoke->args->list.size(); i++) {
        Expr_ptr e = invoke->args->list[i];
        if(i != 0) {
            output += ",";
        }
        output += eval_expr(e);
    }
    output += ")";
    return output;
}

string GLSLTranspiler::eval_expr(Expr_ptr expr) {
    switch(expr->type) {
        case NODE_INT:
            return to_string(static_pointer_cast<Int>(expr)->value);
        case NODE_FLOAT:
            return to_string(static_pointer_cast<Float>(expr)->value);
        case NODE_IDENT:
            {
                string name = static_pointer_cast<Ident>(expr)->name;
                if(name == "FinalPosition") {
                    return "gl_Position";
                }
                return name;
            }
        case NODE_DOT:
            {
                Dot_ptr dot = static_pointer_cast<Dot>(expr);
                return dot->shader->name + "." + dot->name;
            }
        case NODE_BINARY:
            {
                return eval_binary(static_pointer_cast<Binary>(expr));
            }
        case NODE_VECTOR2:
            {
                Vector2_ptr vec2 = static_pointer_cast<Vector2>(expr);
                Expr_ptr x = vec2->x;
                Expr_ptr y = vec2->y;
                return resolve_vector({x, y});
            }
        case NODE_VECTOR3:
            {
                Vector3_ptr vec3 = static_pointer_cast<Vector3>(expr);
                Expr_ptr x = vec3->x;
                Expr_ptr y = vec3->y;
                Expr_ptr z = vec3->z;
                return resolve_vector({x, y, z});
            }
        case NODE_VECTOR4:
            {
                Vector4_ptr vec4 = static_pointer_cast<Vector4>(expr);
                Expr_ptr x = vec4->x;
                Expr_ptr y = vec4->y;
                Expr_ptr z = vec4->z;
                Expr_ptr w = vec4->w;
                return resolve_vector({x, y, z, w});
            }
        case NODE_FUNCEXPR:
            {
                FuncExpr_ptr func = static_pointer_cast<FuncExpr>(expr);
                return eval_invoke(func->invoke);
            }
        default:
            return "";
    }
}

string GLSLTranspiler::eval_binary(Binary_ptr bin) {
    string op_str = "";
    switch(bin->op) {
        case OP_PLUS:
            op_str = " + ";
            break;
        case OP_MINUS:
            op_str = " - ";
            break;
        case OP_MULT:
            op_str = " * ";
            break;
        case OP_DIV:
            op_str = " / " ;
            break;
        default:
            op_str = " ? ";
    }
    return eval_expr(bin->lhs) + op_str + eval_expr(bin->rhs);
}

string GLSLTranspiler::eval_stmt(Stmt_ptr stmt) {
    switch(stmt->type) {
        case NODE_DECL:
            {
                Decl_ptr decl = static_pointer_cast<Decl>(stmt);
                string output = decl->datatype->name + " " + decl->name->name;
                if(decl->value != nullptr) {
                    output += " = " + eval_expr(decl->value);
                }
                localtypes[decl->name->name] = decl->datatype->name;
                output += ";";  
                return output;
            }
        case NODE_ASSIGN:
            {
                Assign_ptr a = static_pointer_cast<Assign>(stmt);
                return eval_expr(a->lhs) + " = " + eval_expr(a->value) + ";";
            }
        default:
            return "";
    }
}
