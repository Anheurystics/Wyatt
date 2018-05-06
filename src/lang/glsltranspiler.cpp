#include "glsltranspiler.h"

GLSLTranspiler::GLSLTranspiler(LogWindow* logger): logger(logger)
{

}

//TODO: Organize better
string GLSLTranspiler::transpile(Shader_ptr shader) {
    this->shader = shader;

    string source = "#version 130\n";

    vector<Decl_ptr> new_inputs;
    for(auto it = shader->inputs->list.begin(); it != shader->inputs->list.end(); ++it) {
        Decl_ptr decl = *it;
        if(decl->datatype->name == "input") {
            if(layouts->find(decl->ident->name) != layouts->end()) {
                ProgramLayout_ptr layout = layouts->at(decl->ident->name);
                for(auto jt = layout->attribs->begin(); jt != layout->attribs->end(); ++jt) {
                    Decl_ptr attrib = *jt;
                    source += "in " + attrib->datatype->name + " " + attrib->ident->name + ";\n";
                    new_inputs.push_back(attrib);
                }
            } else {
                logger->log(decl, "ERROR", "Layout of name " + decl->ident->name + " does not exist");
            }
        } else {
            source += "in " + decl->datatype->name + " " + decl->ident->name + ";\n";
        }
    }
    shader->inputs->list.insert(shader->inputs->list.end(), new_inputs.begin(), new_inputs.end());

    source += "\n";

    for(auto it = shader->uniforms->begin(); it != shader->uniforms->end(); ++it) {
        string type = it->second;
        if(type == "texture2D") {
            type = "sampler2D";
        }
        source += "uniform " + type + " " + it->first + ";\n";
    }

    source += "\n";

    vector<Decl_ptr> new_outputs;
    for(auto it = shader->outputs->list.begin(); it != shader->outputs->list.end(); ++it) {
        Decl_ptr decl = *it;
        if(decl->ident->name == "FinalPosition") {
            continue;
        }
        if(decl->datatype->name == "output") {
            if(layouts->find(decl->ident->name) != layouts->end()) {
                ProgramLayout_ptr layout = layouts->at(decl->ident->name);
                for(auto jt = layout->attribs->begin(); jt != layout->attribs->end(); ++jt) {
                    Decl_ptr attrib = *jt;
                    source += "out " + attrib->datatype->name + " " + attrib->ident->name + ";\n";
                    new_outputs.push_back(attrib);
                }
            } else {
                logger->log(decl, "ERROR", "Layout of name " + decl->ident->name + " does not exist");
            }
        } else {
            source += "out " + decl->datatype->name + " " + decl->ident->name + ";\n";
        }
    }
    shader->outputs->list.insert(shader->outputs->list.end(), new_outputs.begin(), new_outputs.end());

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

string GLSLTranspiler::resolve_unary(Unary_ptr un) {
    OpType op = un->op;
    Expr_ptr rhs = un->rhs;

    string rtype = "";
    if(rhs->type == NODE_IDENT) {
        rtype = resolve_ident(static_pointer_cast<Ident>(rhs));
    } else if(rhs->type == NODE_FLOAT) {
        rtype = "float";
    } else if(rhs->type == NODE_INT) {
        rtype = "int";
    } else if(rhs->type == NODE_LIST) {
        vector<Expr_ptr> list;
        Vector_ptr vec = static_pointer_cast<Vector>(rhs);
        for(unsigned int i = 0; i < vec->size(); i++) {
            list.push_back(vec->get(i));
        }
        rtype = resolve_vector(list);
    } else if (rhs->type == NODE_BINARY) {
        rtype = resolve_binary(static_pointer_cast<Binary>(rhs));
    }

    if(op == OP_ABS) {
        if(rtype == "vec2" || rtype == "vec3" || rtype == "vec4") {
            rtype = "float";
        }
    }

    return rtype;
}

string GLSLTranspiler::resolve_binary(Binary_ptr bin) {
    OpType op = bin->op;
    Expr_ptr lhs = bin->lhs;
    Expr_ptr rhs = bin->rhs;

    string ltype = "";
    if(lhs->type == NODE_IDENT) {
        ltype = resolve_ident(static_pointer_cast<Ident>(lhs));
    } else if(lhs->type == NODE_FLOAT) {
        ltype = "float";
    } else if(lhs->type == NODE_INT) {
        ltype = "int";
    } else if(lhs->type == NODE_LIST) {
        vector<Expr_ptr> list;
        Vector_ptr vec = static_pointer_cast<Vector>(lhs);
        for(unsigned int i = 0; i < vec->size(); i++) {
            list.push_back(vec->get(i));
        }
        ltype = resolve_vector(list);
    } else if(lhs->type == NODE_BINARY) {
        ltype = resolve_binary(static_pointer_cast<Binary>(lhs)); 
    }
    

    string rtype = "";
    if(rhs->type == NODE_IDENT) {
        rtype = resolve_ident(static_pointer_cast<Ident>(rhs));
    } else if(rhs->type == NODE_FLOAT) {
        rtype = "float";
    } else if(rhs->type == NODE_INT) {
        rtype = "int";
    } else if(rhs->type == NODE_LIST) {
        vector<Expr_ptr> list;
        Vector_ptr vec = static_pointer_cast<Vector>(rhs);
        for(unsigned int i = 0; i < vec->size(); i++) {
            list.push_back(vec->get(i));
        }
        rtype = resolve_vector(list);
    } else if (rhs->type == NODE_BINARY) {
        rtype = resolve_binary(static_pointer_cast<Binary>(rhs));
    }

    int n = 0;
    if((ltype == "float"|| ltype == "int") && (rtype == "float"|| rtype == "int")) {
        return "float";
        // n = 1;
    } else
    if(op == OP_PLUS || op == OP_MINUS || op == OP_MULT) {
        if(ltype == rtype) {
           if(ltype == "vec2") n = 2;
           if(ltype == "vec3") n = 3;
           if(ltype == "vec4") n = 4;
        }
    } else
    if(op == OP_EXP) {
        if(ltype == rtype) {
           if(ltype == "vec2" || ltype == "vec2" || ltype == "vec4") n = 1;
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
        } else
        if(expr->type == NODE_UNARY) {
            Unary_ptr un = static_pointer_cast<Unary>(expr);
            string type = resolve_unary(un);
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
        } else 
        if(expr->type == NODE_UNARY) {
            Unary_ptr un = static_pointer_cast<Unary>(expr);
            string type = resolve_unary(un);
            if(type != "") {
                output += (n != 0? "," : "") + eval_unary(un);
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
    string output = "";
    switch(expr->type) {
        case NODE_INT:
            output = to_string(static_pointer_cast<Int>(expr)->value);
            break;
        case NODE_FLOAT:
            output = to_string(static_pointer_cast<Float>(expr)->value);
            break;
        case NODE_IDENT:
            {
                string name = static_pointer_cast<Ident>(expr)->name;
                if(name == "FinalPosition") {
                    output = "gl_Position";
                } else {
                    output = name;
                }
                break;
            }
        case NODE_DOT:
            {
                Dot_ptr dot = static_pointer_cast<Dot>(expr);
                output = dot->owner->name + "." + dot->name;
                break;
            }
        case NODE_BINARY:
            {
                output = eval_binary(static_pointer_cast<Binary>(expr));
                break;
            }
        case NODE_UNARY:
            {
                output = eval_unary(static_pointer_cast<Unary>(expr));
                break;
            }
        case NODE_VECTOR2:
            {
                Vector2_ptr vec2 = static_pointer_cast<Vector2>(expr);
                Expr_ptr x = vec2->x;
                Expr_ptr y = vec2->y;
                output = eval_vector({x, y});
                break;
            }
        case NODE_VECTOR3:
            {
                Vector3_ptr vec3 = static_pointer_cast<Vector3>(expr);
                Expr_ptr x = vec3->x;
                Expr_ptr y = vec3->y;
                Expr_ptr z = vec3->z;
                output = eval_vector({x, y, z});
                break;
            }
        case NODE_VECTOR4:
            {
                Vector4_ptr vec4 = static_pointer_cast<Vector4>(expr);
                Expr_ptr x = vec4->x;
                Expr_ptr y = vec4->y;
                Expr_ptr z = vec4->z;
                Expr_ptr w = vec4->w;
                output = eval_vector({x, y, z, w});
                break;
            }
        case NODE_FUNCEXPR:
            {
                FuncExpr_ptr func = static_pointer_cast<FuncExpr>(expr);
                output = eval_invoke(func->invoke);
                break;
            }
        default:
            output = "";
            break;
    }
    if(expr->parenthesized) {
        output = "(" + output + ")";
    }
    return output;
}

string GLSLTranspiler::eval_unary(Unary_ptr un) {
    Expr_ptr rhs = un->rhs;
    string rtype = "";
    if(rhs->type == NODE_IDENT) {
        rtype = resolve_ident(static_pointer_cast<Ident>(rhs));
    } else if(rhs->type == NODE_FLOAT) {
        rtype = "float";
    } else if(rhs->type == NODE_INT) {
        rtype = "int";
    } else if(rhs->type == NODE_LIST) {
        vector<Expr_ptr> list;
        Vector_ptr vec = static_pointer_cast<Vector>(rhs);
        for(unsigned int i = 0; i < vec->size(); i++) {
            list.push_back(vec->get(i));
        }
        rtype = resolve_vector(list);
    } else if (rhs->type == NODE_BINARY) {
        rtype = resolve_binary(static_pointer_cast<Binary>(rhs));
    }
    switch(un->op) {
        case OP_MINUS:
            return "-" + eval_expr(rhs);
        case OP_ABS:
            if(rtype == "vec2" || rtype == "vec3" || rtype == "vec4") {
                return "length(" + eval_expr(rhs) + ")";
            } else {
                return "abs(" + eval_expr(rhs) + ")";
            }
        default:
            return "???";
    }
}

string GLSLTranspiler::eval_binary(Binary_ptr bin) {
    string op_str = "";
    string lhs = eval_expr(bin->lhs);
    string rhs = eval_expr(bin->rhs);
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
        case OP_EXP:
            return "dot(" + lhs + ", " + rhs + ")";
        case OP_MOD:
            return "cross(" + lhs + ", " + rhs + ")";
        case OP_DIV:
            op_str = " / ";
            break;
        case OP_LTHAN:
            op_str = " < ";
            break;
        case OP_GTHAN:
            op_str = " > " ;
            break;
        default:
            op_str = " ? ";
    }
    return lhs + op_str + rhs;
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
                if(ifStmt->elseIfBlocks) {
                    for(auto it = ifStmt->elseIfBlocks->begin(); it != ifStmt->elseIfBlocks->end(); ++it) {
                        output += " else " + eval_stmt(*it);
                    }
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
