#include "myparser.h"

#define resolve_int(n) ((Int*)n)->value
#define resolve_float(n) ((Float*)n)->value
#define resolve_scalar(n) ((n->type == NODE_INT)? (float)(resolve_int(n)) : resolve_float(n))

MyParser::MyParser() {

}

Expr* MyParser::eval_binary(Binary* bin) {
    Expr* lhs = eval_expr(bin->lhs);
    if(!lhs) return 0;

    OpType op = bin->op;

    Expr* rhs = eval_expr(bin->rhs);
    if(!rhs) return 0;

    NodeType ltype = lhs->type;
    NodeType rtype = rhs->type;

    if(ltype == NODE_BOOL && rtype == NODE_BOOL) {
       bool a = ((Bool*)lhs)->value;
       bool b = ((Bool*)rhs)->value;

       switch(op) {
           case OP_AND: return new Bool(a && b);
           case OP_OR: return new Bool(a || b);
           default: return 0;
       }
    }

    if(ltype == NODE_INT && rtype == NODE_INT) {
        int a = resolve_int(lhs);
        int b = resolve_int(rhs);

        switch(op) {
            case OP_PLUS: return new Int(a + b);
            case OP_MINUS: return new Int(a - b);
            case OP_MULT: return new Int(a * b);
            case OP_DIV: return new Float(a / (float)b);
            default: return 0;
        }
    }

    if(ltype == NODE_FLOAT && rtype == NODE_FLOAT) {
        float a = resolve_float(lhs);
        float b = resolve_float(rhs);

        switch(op) {
            case OP_PLUS: return new Float(a + b);
            case OP_MINUS: return new Float(a - b);
            case OP_MULT: return new Float(a * b);
            case OP_DIV: return new Float(a / b);
            default: return 0;
        }
    }

    if((ltype == NODE_FLOAT && rtype == NODE_INT) || (ltype == NODE_INT && rtype == NODE_FLOAT)) {
        float a = resolve_scalar(lhs);
        float b = resolve_scalar(rhs);

        switch(op) {
            case OP_PLUS: return new Float(a + b);
            case OP_MINUS: return new Float(a - b);
            case OP_MULT: return new Float(a * b);
            case OP_DIV: return new Float(a / b);
            default: return 0;
        }
    }

    if(ltype == NODE_VECTOR3 && rtype == NODE_VECTOR3) {
        Vector3* a = (Vector3*)eval_expr(lhs);
        Vector3* b = (Vector3*)eval_expr(rhs);

        float ax = resolve_scalar(a->x); 
        float ay = resolve_scalar(a->y);
        float az = resolve_scalar(a->z);
        float bx = resolve_scalar(b->x);
        float by = resolve_scalar(b->y);
        float bz = resolve_scalar(b->z);

        switch(op) {
            case OP_PLUS: return new Vector3(new Float(ax+bx), new Float(ay+by), new Float(az+bz));
            case OP_MINUS: return new Vector3(new Float(ax-bx), new Float(ay-by), new Float(az-bz));
            case OP_MULT: return new Float(ax*bx + ay*by + az*bz);
            case OP_MOD: return new Vector3(new Float(ay*bz-az*by), new Float(az*bx-ax*bz), new Float(ax*by-ay*bx));
            default: return 0;
        }
    }

    if(ltype == NODE_VECTOR3 && (rtype == NODE_INT || rtype == NODE_FLOAT)) {
        Vector3* a = (Vector3*)eval_expr(lhs);
        float ax = resolve_scalar(a->x), ay = resolve_scalar(a->y), az = resolve_scalar(a->z);
        float b = resolve_scalar(rhs);

        switch(op) {
            case OP_MULT: return new Vector3(new Float(ax*b), new Float(ay*b), new Float(az*b));
            case OP_DIV: return new Vector3(new Float(ax/b), new Float(ay/b), new Float(az/b));
            default: return 0;
        }
    }

    if((ltype == NODE_INT || ltype == NODE_FLOAT) && rtype == NODE_VECTOR3) {
        float a = resolve_scalar(lhs);
        Vector3* b = (Vector3*)eval_expr(rhs);
        float bx = resolve_scalar(b->x), by = resolve_scalar(b->y), bz = resolve_scalar(b->z);

        switch(op) {
            case OP_MULT: return new Vector3(new Float(bx*a), new Float(by*a), new Float(bz*a));
            case OP_DIV: return new Vector3(new Float(bx/a), new Float(by/a), new Float(bz/a));
            default: return 0;
        }
    }

    return 0;
}

Expr* MyParser::eval_expr(Expr* node) {
    switch(node->type) {
        case NODE_IDENT: 
        {
            Ident* ident = (Ident*)node;
            if(variables.find(ident->name) == variables.end()) {
                std::cout << "ERROR: Variable " << ident->name << " does not exist!\n";
                return 0;
            } else {
                return variables[ident->name];
            }
        }

        case NODE_BOOL:
            return node;

        case NODE_INT:
            return node;

        case NODE_FLOAT:
            return node;

        case NODE_VECTOR3:
        {
            Vector3* vec3 = (Vector3*)(node);

            vec3->x = eval_expr(vec3->x);
            vec3->y = eval_expr(vec3->y);
            vec3->z = eval_expr(vec3->z);

            return vec3;
        }

        case NODE_UNARY:
        {
            Unary* un = (Unary*)node;
            Expr* rhs = eval_expr(un->rhs);

            if(rhs->type == NODE_INT) {
                Int* i = (Int*)rhs;
                return new Int(-(i->value));
            }

            if(rhs->type == NODE_FLOAT) {
                Float* fl = (Float*)rhs;
                return new Float(-(fl->value));
            }
            if(rhs->type == NODE_VECTOR3) {
                Vector3* vec3 = (Vector3*)rhs;

                //SHAMEFUL HACK
                return eval_binary(new Binary(vec3, OP_MULT, new Float(-1))); 
            }
        }

        case NODE_BINARY:
        {
            Binary* bin = (Binary*)(node);
            return eval_binary(bin);
        }

        default: return 0;
    }

    return 0;
}

void MyParser::eval_stmt(Stmt* stmt) {
    switch(stmt->type) {
        case NODE_ASSIGN:
            {
                Assign* assign = (Assign*)stmt;
                Expr* rhs = eval_expr(assign->value);
                if(rhs) {
                    variables[assign->ident->name] = rhs;
                } else {
                    std::cout << "ERROR: Invalid assignment\n";
                }

                return;
            }
        case NODE_ALLOC:
            {
                Alloc* alloc = (Alloc*)stmt;

                if(!buffers[alloc->ident->name]) {
                    Buffer* buf = new Buffer;
                    gl->glGenBuffers(1, &(buf->handle));
                    buffers[alloc->ident->name] = buf;
                } else {
                    std::cout << "ERROR: Can't allocate to " << alloc->ident->name << ": buffer already exists!\n";
                }

                return;
            }
        case NODE_UPLOAD:
            {
                Upload* upload = (Upload*)stmt;

                std::vector<float>* target = &(buffers[upload->ident->name]->data);
                for(unsigned int i = 0; i < upload->list->list.size(); i++) {
                    Expr* expr = eval_expr(upload->list->list[i]);
                    if(!expr) {
                        std::cout << "ERROR: Can't upload illegal value into buffer\n";
                        break;
                    }

                    if(expr->type == NODE_VECTOR3) {
                        Vector3* vec3 = (Vector3*)expr;
                        target->insert(target->end(), resolve_scalar(vec3->x));
                        target->insert(target->end(), resolve_scalar(vec3->y));
                        target->insert(target->end(), resolve_scalar(vec3->z));
                    }

                    if(expr->type == NODE_FLOAT) {
                        Float* f = (Float*)expr;
                        target->insert(target->begin(), resolve_scalar(f));
                    }
                }

                return;
            }
        case NODE_DRAW:
            {
                Draw* draw = (Draw*)stmt;

                Buffer* buffer = buffers[draw->ident->name];

                if(buffer) {
                    gl->glBindBuffer(GL_ARRAY_BUFFER, buffer->handle);
                    gl->glBufferData(GL_ARRAY_BUFFER, buffer->data.size() * sizeof(float), &(buffer->data)[0], GL_STATIC_DRAW);
                    gl->glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, (void*)0);
                    gl->glEnableVertexAttribArray(0);

                    gl->glDrawArrays(GL_TRIANGLES, 0, buffer->data.size() / 3);
                } else {
                    std::cout << "ERROR: Can't draw non-existent buffer " << draw->ident->name << std::endl;
                }

                return;
            }
        default: return;
    }
}

void MyParser::execute_stmts(Stmts* stmts) {
    for(unsigned int it = 0; it < stmts->list.size(); it++) { 
        eval_stmt(stmts->list.at(it));
    }
}

void MyParser::execute_init() {
    if(!init || status) return;
    buffers.clear();
    variables.clear();

    execute_stmts(init);
}

void MyParser::execute_loop() {
    if(!loop || status) return;
    execute_stmts(loop);
}

void MyParser::parse(std::string code) {
    YY_BUFFER_STATE state = yy_scan_string(code.c_str());

    status = yyparse(&init, &loop);

    yy_delete_buffer(state);

//    if(status == 0) {
//        for(unsigned int it = 0; it < nodes.size(); it++) {
//            Node* root = nodes[it];
//
//            if(root->type >= NODE_EXPR && root->type < NODE_STMT) {
//                Expr* result = eval_expr((Expr*)root);
//                if(result == 0) continue;
//                switch(result->type) {
//                    case NODE_INT:
//                    {
//                        Int* i = (Int*)result;
//                        std::cout << "= " << i->value << std::endl;
//                        break;
//                    }
//
//                    case NODE_FLOAT: 
//                    {
//                        Float* f = (Float*)result;
//                        std::cout << std::fixed << std::setprecision(4) << "= " << f->value << std::endl;
//                        break;
//                    }
//
//                    case NODE_VECTOR3:
//                    {
//                        Vector3* v = (Vector3*)result;
//                        std::cout << std::fixed << std::setprecision(4) << "= <" << resolve_scalar(v->x) << ", " << resolve_scalar(v->y) << ", " << resolve_scalar(v->z) << ">\n";
//                    }
//                    default: return;
//                }
//            }
//
//            if(root->type >= NODE_STMT) {
//                eval_stmt((Stmt*)root);
//            }
//        }
//    }
}

