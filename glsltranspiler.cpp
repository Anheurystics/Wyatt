#include "glsltranspiler.h"

GLSLTranspiler::GLSLTranspiler(LogWindow* logger): logger(logger)
{

}

//TODO: Organize better
string GLSLTranspiler::transpile(Shader_ptr shader) {
    this->shader = shader;

    string source = "#version 130\n";

    for(auto it = shader->inputs->list.begin(); it != shader->inputs->list.end(); ++it) {
        Decl_ptr decl = *it;
        source += "in " + decl->datatype->name + " " + decl->ident->name + ";\n";
    }

    source += "\n";

    for(auto it = shader->uniforms->begin(); it != shader->uniforms->end(); ++it) {
        string type = it->second;
        if(type == "texture2D") {
            type = "sampler2D";
        }
        source += "uniform " + type + " " + it->first + ";\n";
    }

    source += "\n";

    for(auto it = shader->outputs->list.begin(); it != shader->outputs->list.end(); ++it) {
        Decl_ptr decl = *it;
        if(decl->ident->name == "FinalPosition") {
            continue;
        }
        source += "out " + decl->datatype->name + " " + decl->ident->name + ";\n";
    }

    source += "\n";

    for(auto it = shader->functions->begin(); it != shader->functions->end(); ++it) {
        string type = "var";
        if(it->first == "main") {
            type = "void";
        }

        source += type + " " + it->first + "() {\n";

        FuncDef_ptr def = it->second;
        Stmts_ptr stmts = def->stmts;
        for(auto jt = stmts->list.begin(); jt != stmts->list.end(); ++jt) {
            source += "\t" + eval_stmt(*jt) + "\n";
        }

        source += "}\n";
    }

    return source;
}

int type_to_size(string type) {
    if(type == "int" || type == "float") {
        return 1;
    }
    if(type == "vec2") {
        return 2;
    }
    if(type == "vec3") {
        return 3;
    }
    if(type == "vec4") {
        return 4;
    }
    return 0;
}

string GLSLTranspiler::resolve_ident(Ident_ptr ident) {
    string name = ident->name;
    if(localtypes.find(name) != localtypes.end()) {
        return localtypes[name];
    } else
    if(shader->uniforms->find(name) != shader->uniforms->end()) {
        return shader->uniforms->at(name);
    } else {
        for(auto jt = shader->inputs->list.begin(); jt != shader->inputs->list.end(); ++jt) {
            Decl_ptr decl = *jt;
            if(decl->ident->name == name) {
                return decl->datatype->name;
            }
        }
    }

    return "";
}

string GLSLTranspiler::resolve_binary(Binary_ptr bin) {
    OpType op = bin->op;
    Expr_ptr lhs = bin->lhs;
    Expr_ptr rhs = bin->rhs;

    string ltype = "";
    if(lhs->type == NODE_IDENT) {
        ltype = resolve_ident(static_pointer_cast<Ident>(lhs));
    } else {
        if(lhs->type == NODE_FLOAT) {
            ltype = "float";
        } else if(lhs->type == NODE_INT) {
            ltype = "int";
        } else {
            vector<Expr_ptr> list;
            Vector_ptr vec = static_pointer_cast<Vector>(lhs);
            for(unsigned int i = 0; i < vec->size(); i++) {
                list.push_back(vec->get(i));
            }
            ltype = resolve_vector(list);
        }
    }

    string rtype = "";
    if(rhs->type == NODE_IDENT) {
        rtype = resolve_ident(static_pointer_cast<Ident>(rhs));
    } else {
        if(rhs->type == NODE_FLOAT) {
            rtype = "float";
        } else if(rhs->type == NODE_INT) {
            rtype = "int";
        } else {
            vector<Expr_ptr> list;
            Vector_ptr vec = static_pointer_cast<Vector>(rhs);
            for(unsigned int i = 0; i < vec->size(); i++) {
                list.push_back(vec->get(i));
            }
            rtype = resolve_vector(list);
        }
    }

    int n = 0;
    if((ltype == "float"|| ltype == "int") && (rtype == "float"|| rtype == "int")) {
        n = 1;
    } else
    if(op == OP_PLUS || op == OP_MINUS) {
        if(ltype == rtype) {
           if(ltype == "vec2") n = 2;
           if(ltype == "vec3") n = 3;
           if(ltype == "vec4") n = 4;
        }
    } else
    if(op == OP_MULT) {
        if(ltype == rtype) {
           if(ltype == "vec2") n = 2;
           if(ltype == "vec3") n = 3;
           if(ltype == "vec4") n = 4;
        }
    }
    
    return "vec" + to_string(n);
}

string GLSLTranspiler::resolve_vector(vector<Expr_ptr> list) {
    int n = 0;
    for(auto it = list.begin(); it != list.end(); ++it) {
        Expr_ptr expr = *it;
        string type = type_to_name(expr->type);
        int type_len = type_to_size(type);
        if(type_len != 0) {
            n += type_len;
        } else
        if(expr->type == NODE_IDENT) {
            Ident_ptr ident = static_pointer_cast<Ident>(expr);
            string type = resolve_ident(ident);
            n += type_to_size(type);
        } else
        if(expr->type == NODE_BINARY) {
            Binary_ptr bin = static_pointer_cast<Binary>(expr);
            string type = resolve_binary(bin);
            n += type_to_size(type);
        }
    }

    return "vec" + to_string(n);
}

string GLSLTranspiler::eval_vector(vector<Expr_ptr> list) {
    string output = "(";
    int n = 0;
    for(auto it = list.begin(); it != list.end(); ++it) {
        Expr_ptr expr = *it;
        string type = type_to_name(expr->type);
        int type_len = type_to_size(type);
        if(type_len != 0) {
            output += (n != 0? ",":"") + eval_expr(expr);
            n += type_len;
        } else
        if(expr->type == NODE_IDENT) {
            Ident_ptr ident = static_pointer_cast<Ident>(expr);
            string type = resolve_ident(ident);
            if(type != "") {
                output += (n != 0? "," : "") + ident->name;
            }
            n += type_to_size(type);
        } else
        if(expr->type == NODE_BINARY) {
            Binary_ptr bin = static_pointer_cast<Binary>(expr);
            string type = resolve_binary(bin);
            if(type != "") {
                output += (n != 0? "," : "") + eval_binary(bin);
            }
            n += type_to_size(type);
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
                return dot->owner->name + "." + dot->name;
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
                return eval_vector({x, y});
            }
        case NODE_VECTOR3:
            {
                Vector3_ptr vec3 = static_pointer_cast<Vector3>(expr);
                Expr_ptr x = vec3->x;
                Expr_ptr y = vec3->y;
                Expr_ptr z = vec3->z;
                return eval_vector({x, y, z});
            }
        case NODE_VECTOR4:
            {
                Vector4_ptr vec4 = static_pointer_cast<Vector4>(expr);
                Expr_ptr x = vec4->x;
                Expr_ptr y = vec4->y;
                Expr_ptr z = vec4->z;
                Expr_ptr w = vec4->w;
                return eval_vector({x, y, z, w});
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
                string output = decl->datatype->name + " " + decl->ident->name;
                if(decl->value != nullptr) {
                    output += " = " + eval_expr(decl->value);
                }
                localtypes[decl->ident->name] = decl->datatype->name;
                output += ";";  
                return output;
            }
        case NODE_ASSIGN:
            {
                Assign_ptr a = static_pointer_cast<Assign>(stmt);
                return eval_expr(a->lhs) + " = " + eval_expr(a->value) + ";";
            }
        case NODE_IF:
            {
                If_ptr ifStmt = static_pointer_cast<If>(stmt);
                string output = "if(";
                output += eval_expr(ifStmt->condition) + ") {\n";
                for(auto it = ifStmt->block->list.begin(); it != ifStmt->block->list.end(); ++it) {
                    output += eval_stmt(*it);
                } 
                output += "}";
                for(auto it = ifStmt->elseIfBlocks->begin(); it != ifStmt->elseIfBlocks->end(); ++it) {
                    output += " else " + eval_stmt(*it);
                }
                if(ifStmt->elseBlock != nullptr) {
                    output += " else {\n";
                    for(auto it = ifStmt->elseBlock->list.begin(); it != ifStmt->elseBlock->list.end(); ++it) {
                        output += eval_stmt(*it);
                    }
                    output += "}\n";
                }
                return output;
            }
        case NODE_PRINT:
            {
                logger->log(stmt, "ERROR", "Printing not allowed from inside shaders");
                return "";
            }
        default:
            return "";
    }
}
