#include "interpreter.h"

#define resolve_int(n) ((Int*)n)->value
#define resolve_float(n) ((Float*)n)->value
#define resolve_scalar(n) ((n->type == NODE_INT)? (float)(resolve_int(n)) : resolve_float(n))
#define resolve_vec2(v) resolve_scalar(v->x), resolve_scalar(v->y)
#define resolve_vec3(v) resolve_scalar(v->x), resolve_scalar(v->y), resolve_scalar(v->z)
#define resolve_vec4(v) resolve_scalar(v->x), resolve_scalar(v->y), resolve_scalar(v->z), resolve_scalar(v->w)

#define LOOP_TIMEOUT 5

Interpreter::Interpreter(LogWindow* logger) {
    this->logger = logger;

    globalScope = new Scope("global");

    string utilsrc = str_from_file("utils.txt");
    parse(utilsrc);
    if(status != 0) {
        logger->log("Error parsing utils.txt");
    }

    status = 0;

    for(map<string, FuncDef*>::iterator it = functions.begin(); it != functions.end(); ++it) {
        builtins[it->first] = it->second;
    }
}

void Interpreter::reset() {
    buffers.clear();
    programs.clear();
    shaders.clear();
    functions.clear();
    globalScope->clear();
    while(!functionScopeStack.empty()) functionScopeStack.pop();
    current_program_name = "";
    current_program = NULL;
    init = NULL;
    loop = NULL;
    gl = NULL;
}

Expr* Interpreter::eval_binary(Binary* bin) {
    Expr* lhs = eval_expr(bin->lhs);
    if(!lhs) return NULL;

    OpType op = bin->op;

    Expr* rhs = eval_expr(bin->rhs);
    if(!rhs) return NULL;

    NodeType ltype = lhs->type;
    NodeType rtype = rhs->type;

    if(ltype == NODE_BOOL && rtype == NODE_BOOL) {
        bool a = ((Bool*)lhs)->value;
        bool b = ((Bool*)rhs)->value;

        switch(op) {
            case OP_AND: return new Bool(a && b);
            case OP_OR: return new Bool(a || b);
            default: return NULL;
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
            case OP_MOD: return new Int(a % b);
            case OP_EQUAL: return new Bool(a == b);
            case OP_LESSTHAN: return new Bool(a < b);
            case OP_GREATERTHAN: return new Bool(a > b);
            case OP_NEQUAL: return new Bool(a != b);
            case OP_LEQUAL: return new Bool(a <= b);
            case OP_GEQUAL: return new Bool(a >= b);
            default: return NULL;
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
            case OP_EQUAL: return new Bool(a == b);
            case OP_LESSTHAN: return new Bool(a < b);
            case OP_GREATERTHAN: return new Bool(a > b);
            case OP_NEQUAL: return new Bool(a != b);
            case OP_LEQUAL: return new Bool(a <= b);
            case OP_GEQUAL: return new Bool(a >= b);
            default: return NULL;
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
            case OP_EQUAL: return new Bool(a == b);
            case OP_LESSTHAN: return new Bool(a < b);
            case OP_GREATERTHAN: return new Bool(a > b);
            case OP_NEQUAL: return new Bool(a != b);
            case OP_LEQUAL: return new Bool(a <= b);
            case OP_GEQUAL: return new Bool(a >= b);
            default: return NULL;
        }
    }

    if(ltype == NODE_VECTOR2 && rtype == NODE_VECTOR2) {
        Vector2* a = (Vector2*)eval_expr(lhs);
        Vector2* b = (Vector2*)eval_expr(rhs);

        float ax = resolve_scalar(a->x);
        float ay = resolve_scalar(a->y);
        float bx = resolve_scalar(b->x);
        float by = resolve_scalar(b->y);

        switch(op) {
            case OP_PLUS: return new Vector2(new Float(ax+bx), new Float(ay+by));
            case OP_MINUS: return new Vector2(new Float(ax-bx), new Float(ay-by));
            case OP_MULT: return new Float(ax*bx + ay*by);
            case OP_MOD: return new Float(ax*by - ay*bx);
            default: return NULL;
        }
    }

    if(ltype == NODE_VECTOR2 && (rtype == NODE_INT || rtype == NODE_FLOAT)) {
        Vector2* a = (Vector2*)eval_expr(lhs);
        float ax = resolve_scalar(a->x), ay = resolve_scalar(a->y);
        float b = resolve_scalar(rhs);

        switch(op) {
            case OP_MULT: return new Vector2(new Float(ax*b), new Float(ay*b));
            case OP_DIV: return new Vector2(new Float(ax/b), new Float(ay/b));
            default: return NULL;
        }
    }

    if((ltype == NODE_INT || ltype == NODE_FLOAT) && rtype == NODE_VECTOR2) {
        float a = resolve_scalar(lhs);
        Vector2* b = (Vector2*)eval_expr(rhs);
        float bx = resolve_scalar(b->x), by = resolve_scalar(b->y);

        switch(op) {
            case OP_MULT: return new Vector2(new Float(bx*a), new Float(by*a));
            default: return NULL;
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
            default: return NULL;
        }
    }

    if(ltype == NODE_VECTOR3 && (rtype == NODE_INT || rtype == NODE_FLOAT)) {
        Vector3* a = (Vector3*)eval_expr(lhs);
        float ax = resolve_scalar(a->x), ay = resolve_scalar(a->y), az = resolve_scalar(a->z);
        float b = resolve_scalar(rhs);

        switch(op) {
            case OP_MULT: return new Vector3(new Float(ax*b), new Float(ay*b), new Float(az*b));
            case OP_DIV: return new Vector3(new Float(ax/b), new Float(ay/b), new Float(az/b));
            default: return NULL;
        }
    }

    if((ltype == NODE_INT || ltype == NODE_FLOAT) && rtype == NODE_VECTOR3) {
        float a = resolve_scalar(lhs);
        Vector3* b = (Vector3*)eval_expr(rhs);
        float bx = resolve_scalar(b->x), by = resolve_scalar(b->y), bz = resolve_scalar(b->z);

        switch(op) {
            case OP_MULT: return new Vector3(new Float(bx*a), new Float(by*a), new Float(bz*a));
            default: return NULL;
        }
    }

    if(ltype == NODE_VECTOR4 && rtype == NODE_VECTOR4) {
        Vector4* a = (Vector4*)eval_expr(lhs);
        Vector4* b = (Vector4*)eval_expr(rhs);

        float ax = resolve_scalar(a->x); 
        float ay = resolve_scalar(a->y);
        float az = resolve_scalar(a->z);
        float aw = resolve_scalar(a->w);
        float bx = resolve_scalar(b->x);
        float by = resolve_scalar(b->y);
        float bz = resolve_scalar(b->z);
        float bw = resolve_scalar(b->w);

        switch(op) {
            case OP_PLUS: return new Vector4(new Float(ax+bx), new Float(ay+by), new Float(az+bz), new Float(aw+bw));
            case OP_MINUS: return new Vector4(new Float(ax-bx), new Float(ay-by), new Float(az-bz), new Float(aw-bw));
            case OP_MULT: return new Float(ax*bx + ay*by + az*bz + aw*bw);
            default: return NULL;
        }
    }

    if(ltype == NODE_VECTOR4 && (rtype == NODE_INT || rtype == NODE_FLOAT)) {
        Vector4* a = (Vector4*)eval_expr(lhs);
        float ax = resolve_scalar(a->x), ay = resolve_scalar(a->y), az = resolve_scalar(a->z), aw = resolve_scalar(a->w);
        float b = resolve_scalar(rhs);

        switch(op) {
            case OP_MULT: return new Vector4(new Float(ax*b), new Float(ay*b), new Float(az*b), new Float(aw*b));
            case OP_DIV: return new Vector4(new Float(ax/b), new Float(ay/b), new Float(az/b), new Float(aw/b));
            default: return NULL;
        }
    }

    if((ltype == NODE_INT || ltype == NODE_FLOAT) && rtype == NODE_VECTOR4) {
        float a = resolve_scalar(lhs);
        Vector4* b = (Vector4*)eval_expr(rhs);
        float bx = resolve_scalar(b->x), by = resolve_scalar(b->y), bz = resolve_scalar(b->z), bw = resolve_scalar(b->w);

        switch(op) {
            case OP_MULT: return new Vector4(new Float(bx*a), new Float(by*a), new Float(bz*a), new Float(bw*a));
            default: return NULL;
        }
    }

    if(ltype == NODE_MATRIX2 && (rtype == NODE_INT || rtype == NODE_FLOAT)) {
        Matrix2* a = (Matrix2*)eval_expr(lhs);

        if(op != OP_MULT && op != OP_DIV) return NULL;

        Vector2* v0 = (Vector2*)eval_binary(new Binary(a->v0, op, rhs));
        Vector2* v1 = (Vector2*)eval_binary(new Binary(a->v1, op, rhs));

        return new Matrix2(v0, v1);
    }

    if((ltype == NODE_INT || ltype == NODE_FLOAT) && (rtype == NODE_MATRIX2)) {
        Matrix2* a = (Matrix2*)eval_expr(rhs);

        if(op != OP_MULT) return NULL;

        Vector2* v0 = (Vector2*)eval_binary(new Binary(a->v0, op, lhs));
        Vector2* v1 = (Vector2*)eval_binary(new Binary(a->v1, op, lhs));

        return new Matrix2(v0, v1);
    }

    if(ltype == NODE_MATRIX2 && rtype == NODE_MATRIX2) {
        Matrix2* a = (Matrix2*)eval_expr(lhs);
        Matrix2* b = (Matrix2*)eval_expr(rhs);

        if(op != OP_MULT) return NULL;

        Vector2* r0 = new Vector2(new Binary(a->v0, OP_MULT, b->c0), new Binary(a->v0, OP_MULT, b->c1));
        Vector2* r1 = new Vector2(new Binary(a->v1, OP_MULT, b->c0), new Binary(a->v1, OP_MULT, b->c1));

        return eval_expr(new Matrix2(r0, r1));
    }

    if(ltype == NODE_VECTOR2 && rtype == NODE_MATRIX2) {
        Vector2* a = (Vector2*)eval_expr(lhs);
        Matrix2* b = (Matrix2*)eval_expr(rhs);

        if(op != OP_MULT) return NULL;

        return eval_expr(new Vector2(new Binary(a, OP_MULT, b->c0), new Binary(a, OP_MULT, b->c1)));
    }

    if(ltype == NODE_MATRIX3 && (rtype == NODE_INT || rtype == NODE_FLOAT)) {
        Matrix3* a = (Matrix3*)eval_expr(lhs);

        if(op != OP_MULT && op != OP_DIV) return NULL;

        Vector3* v0 = (Vector3*)eval_binary(new Binary(a->v0, op, rhs));
        Vector3* v1 = (Vector3*)eval_binary(new Binary(a->v1, op, rhs));
        Vector3* v2 = (Vector3*)eval_binary(new Binary(a->v2, op, rhs));

        return new Matrix3(v0, v1, v2);
    }

    if((ltype == NODE_INT || ltype == NODE_FLOAT) && (rtype == NODE_MATRIX3)) {
        Matrix3* a = (Matrix3*)eval_expr(rhs);

        if(op != OP_MULT) return NULL;

        Vector3* v0 = (Vector3*)eval_binary(new Binary(a->v0, op, lhs));
        Vector3* v1 = (Vector3*)eval_binary(new Binary(a->v1, op, lhs));
        Vector3* v2 = (Vector3*)eval_binary(new Binary(a->v2, op, lhs));

        return new Matrix3(v0, v1, v2);
    }

    if(ltype == NODE_MATRIX3 && rtype == NODE_MATRIX3) {
        Matrix3* a = (Matrix3*)eval_expr(lhs);
        Matrix3* b = (Matrix3*)eval_expr(rhs);
        
        if(op != OP_MULT) return NULL;

        Vector3* r0 = new Vector3(new Binary(a->v0, OP_MULT, b->c0), new Binary(a->v0, OP_MULT, b->c1), new Binary(a->v0, OP_MULT, b->c2));
        Vector3* r1 = new Vector3(new Binary(a->v1, OP_MULT, b->c0), new Binary(a->v1, OP_MULT, b->c1), new Binary(a->v1, OP_MULT, b->c2));
        Vector3* r2 = new Vector3(new Binary(a->v2, OP_MULT, b->c0), new Binary(a->v2, OP_MULT, b->c1), new Binary(a->v2, OP_MULT, b->c2));

        return eval_expr(new Matrix3(r0, r1, r2));
    }

    if(ltype == NODE_VECTOR3 && rtype == NODE_MATRIX3) {
        Vector3* a = (Vector3*)eval_expr(lhs);
        Matrix3* b = (Matrix3*)eval_expr(rhs);

        if(op != OP_MULT) return NULL;

        return eval_expr(new Vector3(new Binary(a, OP_MULT, b->c0), new Binary(a, OP_MULT, b->c1), new Binary(a, OP_MULT, b->c2)));
    }

    if(ltype == NODE_MATRIX4 && (rtype == NODE_INT || rtype == NODE_FLOAT)) {
        Matrix4* a = (Matrix4*)eval_expr(lhs);

        if(op != OP_MULT && op != OP_DIV) return NULL;

        Vector4* v0 = (Vector4*)eval_binary(new Binary(a->v0, op, rhs));
        Vector4* v1 = (Vector4*)eval_binary(new Binary(a->v1, op, rhs));
        Vector4* v2 = (Vector4*)eval_binary(new Binary(a->v2, op, rhs));
        Vector4* v3 = (Vector4*)eval_binary(new Binary(a->v3, op, rhs));

        return new Matrix4(v0, v1, v2, v3);
    }

    if((ltype == NODE_INT || ltype == NODE_FLOAT) && (rtype == NODE_MATRIX4)) {
        Matrix4* a = (Matrix4*)eval_expr(rhs);

        if(op != OP_MULT) return NULL;

        Vector4* v0 = (Vector4*)eval_binary(new Binary(a->v0, op, lhs));
        Vector4* v1 = (Vector4*)eval_binary(new Binary(a->v1, op, lhs));
        Vector4* v2 = (Vector4*)eval_binary(new Binary(a->v2, op, lhs));
        Vector4* v3 = (Vector4*)eval_binary(new Binary(a->v3, op, lhs));

        return new Matrix4(v0, v1, v2, v3);
    }

    if(ltype == NODE_MATRIX4 && rtype == NODE_MATRIX4) {
        Matrix4* a = (Matrix4*)eval_expr(lhs);
        Matrix4* b = (Matrix4*)eval_expr(rhs);
        
        if(op != OP_MULT) return NULL;

        Vector4* r0 = new Vector4(new Binary(a->v0, OP_MULT, b->c0), new Binary(a->v0, OP_MULT, b->c1), new Binary(a->v0, OP_MULT, b->c2), new Binary(a->v0, OP_MULT, b->c3));
        Vector4* r1 = new Vector4(new Binary(a->v1, OP_MULT, b->c0), new Binary(a->v1, OP_MULT, b->c1), new Binary(a->v1, OP_MULT, b->c2), new Binary(a->v1, OP_MULT, b->c3));
        Vector4* r2 = new Vector4(new Binary(a->v2, OP_MULT, b->c0), new Binary(a->v2, OP_MULT, b->c1), new Binary(a->v2, OP_MULT, b->c2), new Binary(a->v2, OP_MULT, b->c3));
        Vector4* r3 = new Vector4(new Binary(a->v3, OP_MULT, b->c0), new Binary(a->v3, OP_MULT, b->c1), new Binary(a->v3, OP_MULT, b->c2), new Binary(a->v3, OP_MULT, b->c3));

        return eval_expr(new Matrix4(r0, r1, r2, r3));
    }

    if(ltype == NODE_VECTOR4 && rtype == NODE_MATRIX4) {
        Vector4* a = (Vector4*)eval_expr(lhs);
        Matrix4* b = (Matrix4*)eval_expr(rhs);

        if(op != OP_MULT) return NULL;

        return eval_expr(new Vector4(new Binary(a, OP_MULT, b->c0), new Binary(a, OP_MULT, b->c1), new Binary(a, OP_MULT, b->c2), new Binary(a, OP_MULT, b->c3)));
    }

    return NULL;
}

Expr* Interpreter::invoke(Invoke* invoke) {
    string name = invoke->ident->name;
    FuncDef* def = NULL;

    if(name == "cos") {
        if(invoke->args->list.size() == 1) {
            Expr* v = eval_expr(invoke->args->list[0]);
            if(v->type == NODE_FLOAT || v->type == NODE_INT) {
                return new Float(cosf(resolve_scalar(v)));
            }
        }
    }

    if(name == "sin") {
        if(invoke->args->list.size() == 1) {
            Expr* v = eval_expr(invoke->args->list[0]);
            if(v->type == NODE_FLOAT || v->type == NODE_INT) {
                return new Float(sinf(resolve_scalar(v)));
            }
        }
    }

    if(name == "tan") {
        if(invoke->args->list.size() == 1) {
            Expr* v = eval_expr(invoke->args->list[0]);
            if(v->type == NODE_FLOAT || v->type == NODE_INT) {
                return new Float(tanf(resolve_scalar(v)));
            }
        }
    }

    if(name == "pi") {
        if(invoke->args->list.size() == 0) {
            return new Float(3.14159f);
        }
    }

    if(builtins.find(name) != builtins.end()) {
        def = builtins[name];
    }

    if(functions.find(name) != functions.end()) {
        def = functions[name];
    }

    if(def != NULL) {
        Scope* localScope = new Scope(name);
        unsigned int nParams = def->params->list.size();
        unsigned int nArgs = invoke->args->list.size();

        if(nParams != nArgs) {
            logger->log(invoke, "ERROR", "Function " + name + " expects " + to_string(nParams) + " arguments, got " + to_string(nArgs));
            return NULL;
        }
        if(nParams > 0) {
            for(unsigned int i = 0; i < nParams; i++) {
                Expr* arg = eval_expr(invoke->args->list[i]);
                if(arg == NULL) {
                    logger->log("ERROR: Invalid argument passed on to " + name);
                    return NULL;
                }
                localScope->declare(def->params->list[i]->name, arg);
            }
        }

        functionScopeStack.push(localScope);
        Expr* retValue = execute_stmts(def->stmts);
        functionScopeStack.pop();
        return retValue;
    } else {
        logger->log("ERROR: Call to undefined function " + name);
    }
    return NULL;
}

Expr* Interpreter::resolve_vector(vector<Expr*> list) {
    vector<float> data;
    int n = 0;

    for(unsigned int i = 0; i < list.size(); i++) {
        Expr* expr = list[i];
        if(expr->type == NODE_FLOAT || expr->type == NODE_INT) {
            data.push_back(resolve_scalar(expr));
            n += 1;
        } else
        if(expr->type == NODE_VECTOR2) {
            Vector2* vec2 = (Vector2*)eval_expr(expr);
            data.push_back(resolve_scalar(vec2->x));
            data.push_back(resolve_scalar(vec2->y));
            n += 2;
        } else
        if(expr->type == NODE_VECTOR3) {
            Vector3* vec3 = (Vector3*)eval_expr(expr);
            data.push_back(resolve_scalar(vec3->x));
            data.push_back(resolve_scalar(vec3->y));
            data.push_back(resolve_scalar(vec3->z));
            n += 3;
        } else
        if(expr->type == NODE_VECTOR4) {
            Vector4* vec4 = (Vector4*)eval_expr(expr);
            data.push_back(resolve_scalar(vec4->x));
            data.push_back(resolve_scalar(vec4->y));
            data.push_back(resolve_scalar(vec4->z));
            data.push_back(resolve_scalar(vec4->w));
            n += 4;
        } else {
            return NULL;
        }
    }

    if(data.size() == 3) {
        return new Vector3(new Float(data[0]), new Float(data[1]), new Float(data[2]));
    }
    if(data.size() == 4) {
        return new Vector4(new Float(data[0]), new Float(data[1]), new Float(data[2]), new Float(data[3]));
    }

    return NULL;
}

Expr* Interpreter::eval_expr(Expr* node) {
    switch(node->type) {
        case NODE_IDENT: 
            {
                Ident* ident = (Ident*)node;
                Expr* value = NULL;
                if(!functionScopeStack.empty()) {
                    value = functionScopeStack.top()->get(ident->name);
                    if(value != NULL) {
                        return value;
                    }
                }

                value = globalScope->get(ident->name);
                if(value == NULL) {
                    logger->log("ERROR: Undefined variable " + ident->name);
                }

                return value;
            }
       case NODE_UNIFORM:
            {
                Uniform* uniform = (Uniform*)node;
                if(current_program->vertSource->name == uniform->shader) {
                    ShaderSource* src = current_program->vertSource;
                    string type = "";
                    if(src->uniforms.find(uniform->name) != src->uniforms.end()) {
                        type = src->uniforms[uniform->name];
                    } else {
                        src = current_program->fragSource;
                        if(src->uniforms.find(uniform->name) != src->uniforms.end()) {
                            type = src->uniforms[uniform->name];
                        } else {
                            logger->log("ERROR: Uniform " + uniform->name + " of shader " + current_program_name + " does not exist!");
                            return NULL;
                        }
                    }

                    GLint loc = gl->glGetUniformLocation(current_program->handle, uniform->name.c_str());
                    if(type == "float") {
                        Float* f = new Float(0);
                        gl->glGetUniformfv(current_program->handle, loc, &(f->value));
                        return f;
                    } else
                    if(type == "vec2") {
                        float value[2];
                        gl->glGetUniformfv(current_program->handle, loc, &value[0]);
                        return new Vector2(new Float(value[0]), new Float(value[1]));
                    } else 
                    if(type == "vec3") {
                        float value[3];
                        gl->glGetUniformfv(current_program->handle, loc, &value[0]);
                        return new Vector3(new Float(value[0]), new Float(value[1]), new Float(value[2]));
                    } else
                    if(type == "vec4") {
                        float value[4];
                        gl->glGetUniformfv(current_program->handle, loc, &value[0]);
                        return new Vector4(new Float(value[0]), new Float(value[1]), new Float(value[2]), new Float(value[3]));
                    } else 
                    if(type == "mat2") {
                        float value[4];
                        gl->glGetUniformfv(current_program->handle, loc, &value[0]);
                        return new Matrix2(new Vector2(new Float(value[0]), new Float(value[1])), new Vector2(new Float(value[2]), new Float(value[3])));
                    } else
                    if(type == "mat3") {
                        float value[9];
                        gl->glGetUniformfv(current_program->handle, loc, &value[0]);
                        return new Matrix3(
                            new Vector3(new Float(value[0]), new Float(value[1]), new Float(value[2])),
                            new Vector3(new Float(value[3]), new Float(value[4]), new Float(value[5])),
                            new Vector3(new Float(value[6]), new Float(value[7]), new Float(value[8]))
                        );
                    } else
                    if(type == "mat4") {
                        float value[16];
                        gl->glGetUniformfv(current_program->handle, loc, &value[0]);
                        return new Matrix4(
                            new Vector4(new Float(value[0]), new Float(value[1]), new Float(value[2]), new Float(value[3])),
                            new Vector4(new Float(value[4]), new Float(value[5]), new Float(value[6]), new Float(value[7])),
                            new Vector4(new Float(value[8]), new Float(value[9]), new Float(value[10]), new Float(value[11])),
                            new Vector4(new Float(value[12]), new Float(value[13]), new Float(value[14]), new Float(value[15]))
                        );
                    }

                }
            }

        case NODE_BOOL:
            return node;

        case NODE_INT:
            return node;

        case NODE_FLOAT:
            return node;

        case NODE_VECTOR2:
            {
                Vector2* vec2 = (Vector2*)node;
                Expr* x = eval_expr(vec2->x);
                Expr* y = eval_expr(vec2->y);

                if((x->type == NODE_INT || x->type == NODE_FLOAT) && (y->type == NODE_INT || y->type == NODE_FLOAT)) {
                    return new Vector2(x, y);
                }

                if(x->type == NODE_VECTOR2 && y->type == NODE_VECTOR2) {
                    return new Matrix2((Vector2*)x, (Vector2*)y);
                }

                return resolve_vector({x, y});
            }

        case NODE_VECTOR3:
            {
                Vector3* vec3 = (Vector3*)(node);
                Expr* x = eval_expr(vec3->x);
                Expr* y = eval_expr(vec3->y);
                Expr* z = eval_expr(vec3->z);
                
                if((x->type == NODE_INT || x->type == NODE_FLOAT) && (y->type == NODE_INT || y->type == NODE_FLOAT) && (z->type == NODE_INT || z->type == NODE_FLOAT)) {
                    return new Vector3(x, y, z);
                }

                if(x->type == NODE_VECTOR3 && y->type == NODE_VECTOR3 && z->type == NODE_VECTOR3) {
                    return new Matrix3((Vector3*)x, (Vector3*)y, (Vector3*)z);
                }

                return resolve_vector({x, y, z});
            }

        case NODE_VECTOR4:
            {
                Vector4* vec4 = (Vector4*)(node);
                Expr* x = eval_expr(vec4->x);
                Expr* y = eval_expr(vec4->y);
                Expr* z = eval_expr(vec4->z);
                Expr* w = eval_expr(vec4->w);
                
                if((x->type == NODE_INT || x->type == NODE_FLOAT) && (y->type == NODE_INT || y->type == NODE_FLOAT) && (z->type == NODE_INT || z->type == NODE_FLOAT) && (w->type == NODE_INT || w->type == NODE_FLOAT)) {
                    return new Vector4(x, y, z, w);
                }

                if(x->type == NODE_VECTOR4 && y->type == NODE_VECTOR4 && z->type == NODE_VECTOR4 && w->type == NODE_VECTOR4) {
                    return new Matrix4((Vector4*)x, (Vector4*)y, (Vector4*)z, (Vector4*)w);
                }

                return NULL;
            }

        case NODE_MATRIX2:
            {
                Matrix2* mat2 = (Matrix2*)node;
                mat2->v0 = (Vector2*)eval_expr(mat2->v0);
                mat2->v1 = (Vector2*)eval_expr(mat2->v1);
                return mat2;
            }

        case NODE_MATRIX3:
            {
                Matrix3* mat3 = (Matrix3*)node;
                mat3->v0 = (Vector3*)eval_expr(mat3->v0);
                mat3->v1 = (Vector3*)eval_expr(mat3->v1);
                mat3->v2 = (Vector3*)eval_expr(mat3->v2);
                return mat3;
            }

        case NODE_MATRIX4:
            {
                Matrix4* mat4 = (Matrix4*)node;
                mat4->v0 = (Vector4*)eval_expr(mat4->v0);
                mat4->v1 = (Vector4*)eval_expr(mat4->v1);
                mat4->v2 = (Vector4*)eval_expr(mat4->v2);
                mat4->v3 = (Vector4*)eval_expr(mat4->v3);
                return mat4;
            }

        case NODE_UNARY:
            {
                Unary* un = (Unary*)node;
                Expr* rhs = eval_expr(un->rhs);

                if(!rhs) return NULL;

                if(un->op == OP_MINUS) {
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
                if(un->op == OP_NOT) {
                    if(rhs->type == NODE_BOOL) {
                        Bool* b = (Bool*)rhs;
                        return new Bool(!(b->value));
                    }
                }
                if(un->op == OP_ABS) {
                    if(rhs->type == NODE_INT) {
                        return new Int(abs(resolve_int(rhs)));
                    }
                    if(rhs->type == NODE_FLOAT) {
                        return new Float(fabs(resolve_float(rhs)));
                    }
                    if(rhs->type == NODE_VECTOR2) {
                        Vector2* vec2 = (Vector2*)rhs;
                        float x = resolve_scalar(vec2->x), y = resolve_scalar(vec2->y);
                        return new Float(sqrtf(x * x + y * y));
                    }
                    if(rhs->type == NODE_VECTOR3) {
                        Vector3* vec3 = (Vector3*)rhs;
                        float x = resolve_scalar(vec3->x), y = resolve_scalar(vec3->y), z = resolve_scalar(vec3->z);
                        return new Float(sqrtf(x * x + y * y + z * z));
                    }
                    if(rhs->type == NODE_VECTOR4) {
                        Vector4* vec4 = (Vector4*)rhs;
                        float x = resolve_scalar(vec4->x), y = resolve_scalar(vec4->y), z = resolve_scalar(vec4->z), w = resolve_scalar(vec4->w);
                        return new Float(sqrtf(x * x + y * y + z * z + w * w));
                    }
                    return NULL;
                }
            }

        case NODE_BINARY:
            {
                Binary* bin = (Binary*)(node);
                return eval_binary(bin);
            }

        case NODE_FUNCEXPR:
            {
                FuncExpr* func = (FuncExpr*)node;
                return invoke(func->invoke);
            }

        default: return NULL;
    }

    return NULL;
}

Expr* Interpreter::eval_stmt(Stmt* stmt) {
    switch(stmt->type) {
        case NODE_FUNCSTMT:
            {
                FuncStmt* func = (FuncStmt*)stmt;
                return invoke(func->invoke);
            }
        case NODE_ASSIGN:
            {
                Assign* assign = (Assign*)stmt;
                Expr* rhs = eval_expr(assign->value);
                if(rhs != NULL) {
                    Ident* ident = assign->ident;
                    if(ident->type == NODE_IDENT) {
                        Scope* scope = globalScope;
                        if(!functionScopeStack.empty()) {
                            scope = functionScopeStack.top();
                        }
                        scope->declare(assign->ident->name, rhs);
                    } else if(ident->type == NODE_UNIFORM) {
                        Uniform* uniform = (Uniform*)ident;
                        if(current_program->vertSource->name == uniform->shader) {
                            ShaderSource* src = current_program->vertSource;
                            string type = "";
                            if(src->uniforms.find(uniform->name) != src->uniforms.end()) {
                                type = src->uniforms[uniform->name];
                            } else {
                                src = current_program->fragSource;
                                if(src->uniforms.find(uniform->name) != src->uniforms.end()) {
                                    type = src->uniforms[uniform->name];
                                } else {
                                    logger->log("ERROR: Uniform " + uniform->name + " of shader " + current_program_name + " does not exist!");
                                    return NULL;
                                }
                            }

                            GLint loc = gl->glGetUniformLocation(current_program->handle, uniform->name.c_str());
                            if(type == "float") {
                                if(rhs->type == NODE_FLOAT) {
                                    Float* f = (Float*)rhs;
                                    gl->glUniform1f(loc, resolve_float(f));
                                } else {
                                    logger->log("ERROR: Uniform upload mismatch: float required for " + uniform->name + " of shader " + current_program_name);
                                    return NULL;
                                }
                            } else
                            if(type == "vec2") {
                                if(rhs->type == NODE_VECTOR2) {
                                    Vector2* vec2 = (Vector2*)eval_expr(rhs);
                                    gl->glUniform2f(loc, resolve_vec2(vec2));
                                } else {
                                    logger->log("ERROR: Uniform upload mismatch: vec2 required for " + uniform->name + " of shader " + current_program_name);
                                }
                            } else 
                            if(type == "vec3") {
                                if(rhs->type == NODE_VECTOR3) {
                                    Vector3* vec3 = (Vector3*)eval_expr(rhs);
                                    gl->glUniform3f(loc, resolve_vec3(vec3));
                                } else {
                                    logger->log("ERROR: Uniform upload mismatch: vec3 required for " + uniform->name + " of shader " + current_program_name);
                                    return NULL;
                                }
                            } else
                            if(type == "vec4") {
                                if(rhs->type == NODE_VECTOR4) {
                                    Vector4* vec4 = (Vector4*)eval_expr(rhs);
                                    gl->glUniform4f(loc, resolve_vec4(vec4));
                                } else {
                                    logger->log("ERROR: Uniform upload mismatch: vec4 required for " + uniform->name + " of shader " + current_program_name);
                                    return NULL;
                                }
                            } else
                            if(type == "mat2") {
                                if(rhs->type == NODE_MATRIX2) {
                                    Matrix2* mat2 = (Matrix2*)eval_expr(rhs);
                                    float data[4];
                                    data[0] = resolve_scalar(mat2->v0->x); data[1] = resolve_scalar(mat2->v0->y);
                                    data[2] = resolve_scalar(mat2->v1->x); data[3] = resolve_scalar(mat2->v1->y);
                                    gl->glUniform2fv(loc, 1, data);
                                } else {
                                    logger->log("ERROR: Uniform upload mismatch: vec4 required for " + uniform->name + " of shader " + current_program_name);
                                    return NULL;
                                }
                            } else
                            if(type == "mat3") {
                                if(rhs->type == NODE_MATRIX3) {
                                    Matrix3* mat3 = (Matrix3*)eval_expr(rhs);
                                    float data[9];
                                    data[0] = resolve_scalar(mat3->v0->x); data[1] = resolve_scalar(mat3->v0->y); data[2] = resolve_scalar(mat3->v0->z);
                                    data[3] = resolve_scalar(mat3->v1->x); data[4] = resolve_scalar(mat3->v1->y); data[5] = resolve_scalar(mat3->v1->z);
                                    data[6] = resolve_scalar(mat3->v2->x); data[7] = resolve_scalar(mat3->v2->y); data[8] = resolve_scalar(mat3->v2->z);
                                    gl->glUniformMatrix3fv(loc, 1, false, data);
                                } else {
                                    logger->log("ERROR: Uniform upload mismatch: mat3 required for " + uniform->name + " of shader " + current_program_name);
                                    return NULL;
                                }
                            } else
                            if(type == "mat4") {
                                if(rhs->type == NODE_MATRIX4) {
                                    Matrix4* mat4 = (Matrix4*)eval_expr(rhs);
                                    float data[16];
                                    data[0] = resolve_scalar(mat4->v0->x); data[1] = resolve_scalar(mat4->v0->y); data[2] = resolve_scalar(mat4->v0->z); data[3] = resolve_scalar(mat4->v0->w);
                                    data[4] = resolve_scalar(mat4->v1->x); data[5] = resolve_scalar(mat4->v1->y); data[6] = resolve_scalar(mat4->v1->z); data[7] = resolve_scalar(mat4->v1->w);
                                    data[8] = resolve_scalar(mat4->v2->x); data[9] = resolve_scalar(mat4->v2->y); data[10] = resolve_scalar(mat4->v2->z); data[11] = resolve_scalar(mat4->v2->w);
                                    data[12] = resolve_scalar(mat4->v3->x); data[13] = resolve_scalar(mat4->v3->y); data[14] = resolve_scalar(mat4->v3->z); data[15] = resolve_scalar(mat4->v3->w);
                                    gl->glUniformMatrix4fv(loc, 1, false, data);
                                } else {
                                    logger->log("ERROR: Uniform upload mismatch: mat3 required for " + uniform->name + " of shader " + current_program_name);
                                    return NULL;
                                }
                            }

                        }
                    }
                } else {
                    logger->log("ERROR: Invalid assignment");
                }

                return NULL;
            }
        case NODE_ALLOC:
            {
                Alloc* alloc = (Alloc*)stmt;

                if(!buffers[alloc->ident->name]) {
                    Buffer* buf = new Buffer;
                    buf->layout = new Layout;

                    gl->glGenBuffers(1, &(buf->handle));
                    buffers[alloc->ident->name] = buf;
                } else {
                    logger->log("ERROR: Can't allocate to " + alloc->ident->name + ": buffer already exists!");
                }

                return NULL;
            }
        case NODE_UPLOAD:
            {
                Upload* upload = (Upload*)stmt;

                Buffer* buffer = buffers[upload->ident->name];
                if(buffer == NULL) {
                    logger->log("ERROR: Can't upload to unallocated buffer");
                    return NULL;
                }

                Layout* layout = buffer->layout;

                if(layout->attributes.find(upload->attrib->name) == layout->attributes.end()) {
                    layout->attributes[upload->attrib->name] = 3;
                    layout->list.push_back(upload->attrib->name);
                }

                vector<float>* target = &(buffer->data[upload->attrib->name]);
                for(unsigned int i = 0; i < upload->list->list.size(); i++) {
                    Expr* expr = eval_expr(upload->list->list[i]);
                    if(!expr) {
                        logger->log("ERROR: Can't upload illegal value into buffer");
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

                buffer->sizes[upload->attrib->name] = target->size() / buffer->layout->attributes[upload->attrib->name];

                return NULL;
            }
        case NODE_DRAW:
            {
                Draw* draw = (Draw*)stmt;

                if(current_program == NULL) {
                    logger->log("ERROR: Cannot bind program with name " + current_program_name);
                    return NULL;
                }

                Buffer* buffer = buffers[draw->ident->name];
                if(buffer != NULL) {
                    Layout* layout = buffer->layout;
                    vector<float> final_vector;

                    map<string, unsigned int> attributes = layout->attributes;
                    if(attributes.size() == 0) {
                        logger->log("ERROR: Cannot draw empty buffer!");
                        return NULL;
                    }

                    gl->glBindBuffer(GL_ARRAY_BUFFER, buffer->handle);
                    for(unsigned int i = 0; i < buffer->sizes[layout->list[0]]; i++) {
                        for(unsigned int j = 0; j < layout->list.size(); j++) {
                            string attrib = layout->list[j];
                            for(unsigned int k = 0; k < layout->attributes[attrib]; k++) {
                                final_vector.insert(final_vector.end(), buffer->data[attrib][(i * layout->attributes[attrib]) + k]);
                            }
                        }
                    }

                    gl->glBufferData(GL_ARRAY_BUFFER, final_vector.size() * sizeof(float), &final_vector[0], GL_STATIC_DRAW);

                    int total_size = 0;
                    for(map<string, unsigned int>::iterator it = layout->attributes.begin(); it != layout->attributes.end(); ++it) {
                        total_size += it->second;
                    }

                    int cumulative_size = 0;
                    for(unsigned int i = 0; i < layout->list.size(); i++) {
                        string attrib = layout->list[i];
                        GLint location = gl->glGetAttribLocation(current_program->handle, attrib.c_str());
                        gl->glVertexAttribPointer(location, layout->attributes[attrib], GL_FLOAT, false, total_size * sizeof(float), (void*)(cumulative_size * sizeof(float)));
                        gl->glEnableVertexAttribArray(location);

                        cumulative_size += layout->attributes[attrib];
                    }

                    gl->glDrawArrays(GL_TRIANGLES, 0, final_vector.size() / total_size);
                } else {
                    logger->log("ERROR: Can't draw non-existent buffer " + draw->ident->name);
                }

                return NULL;
            }
        case NODE_USE:
            {
                Use* use = (Use*)stmt;
                current_program_name = use->ident->name;
                current_program = programs[current_program_name];

                if(current_program == NULL) {
                    logger->log("ERROR: No valid vertex/fragment pair for program name " + current_program_name);
                    return NULL;
                }

                gl->glUseProgram(current_program->handle);
                return NULL;
            }
        case NODE_IF:
            {
                If* ifstmt = (If*)stmt;
                Expr* condition = eval_expr(ifstmt->condition);
                if(!condition) return NULL;
                if(condition->type == NODE_BOOL) {
                    bool b = ((Bool*)condition)->value;
                    if(b) {
                        Expr* returnValue = execute_stmts(ifstmt->block);
                        if(returnValue != NULL) {
                            return returnValue;
                        }
                    }
                } else {
                    logger->log("ERROR: Condition in if statement not a boolean");
                }
                return NULL;
            }
        case NODE_WHILE:
            {
                While* whilestmt = (While*)stmt;
                Expr* condition = eval_expr(whilestmt->condition);
                if(!condition) return NULL;
                if(condition->type == NODE_BOOL) {
                    time_t start = time(nullptr);
                    while(true) {
                        condition = eval_expr(whilestmt->condition);
                        bool b = ((Bool*)condition)->value;
                        if(!b) break;

                        Expr* returnValue = execute_stmts(whilestmt->block);
                        if(returnValue != NULL) {
                            return returnValue;
                        }

                        time_t now = time(nullptr);
                        
                        int diff = difftime(now, start);
                        if(diff > LOOP_TIMEOUT) { break; }
                    }
                } else {
                    logger->log("ERROR: Condition in while statement not a boolean");
                }

                return NULL;
            }
        case NODE_PRINT:
            {
                Print* print = (Print*)stmt;
                Expr* output = eval_expr(print->expr);
                if(output == NULL)
                    return NULL;

                switch(output->type) {
                    case NODE_INT:
                        logger->log(to_string(resolve_int(output)));
                        break;
                    case NODE_FLOAT:
                        logger->log(to_string(resolve_float(output)));
                        break;
                    case NODE_BOOL:
                        logger->log(((Bool*)output)->value? "true" : "false");
                        break;
                    case NODE_VECTOR2:
                        {
                            Vector2* vec2 = (Vector2*)output;
                            logger->log("[" + to_string(resolve_scalar(vec2->x)) + ", " + to_string(resolve_scalar(vec2->y)) + "]");
                            break;
                        }
                    case NODE_VECTOR3:
                        {
                            Vector3* vec3 = (Vector3*)output;
                            logger->log("[" + to_string(resolve_scalar(vec3->x)) + ", " + to_string(resolve_scalar(vec3->y)) + ", " + to_string(resolve_scalar(vec3->z)) + "]");
                            break;
                        }
                    case NODE_VECTOR4:
                        {
                            Vector4* vec4 = (Vector4*)output;
                            logger->log("[" + to_string(resolve_scalar(vec4->x)) + ", " + to_string(resolve_scalar(vec4->y)) + ", " + to_string(resolve_scalar(vec4->z)) + ", " + to_string(resolve_scalar(vec4->w)) + "]");
                            break;
                        }
                    case NODE_MATRIX2:
                        {
                            Matrix2* mat2 = (Matrix2*)output;
                            logger->log("|" + to_string(resolve_scalar(mat2->v0->x)) + ", " + to_string(resolve_scalar(mat2->v0->y)) + "|");
                            logger->log("|" + to_string(resolve_scalar(mat2->v1->x)) + ", " + to_string(resolve_scalar(mat2->v1->y)) + "|");
                            break;
                        }
                    case NODE_MATRIX3:
                        {
                            Matrix3* mat3 = (Matrix3*)output;
                            logger->log("|" + to_string(resolve_scalar(mat3->v0->x)) + ", " + to_string(resolve_scalar(mat3->v0->y)) + ", " + to_string(resolve_scalar(mat3->v0->z)) + "|");
                            logger->log("|" + to_string(resolve_scalar(mat3->v1->x)) + ", " + to_string(resolve_scalar(mat3->v1->y)) + ", " + to_string(resolve_scalar(mat3->v1->z)) + "|");
                            logger->log("|" + to_string(resolve_scalar(mat3->v2->x)) + ", " + to_string(resolve_scalar(mat3->v2->y)) + ", " + to_string(resolve_scalar(mat3->v2->z)) + "|");
                            break;
                        }
                    case NODE_MATRIX4:
                        {
                            Matrix4* mat4 = (Matrix4*)output;
                            logger->log("|" + to_string(resolve_scalar(mat4->v0->x)) + ", " + to_string(resolve_scalar(mat4->v0->y)) + ", " + to_string(resolve_scalar(mat4->v0->z)) + ", " + to_string(resolve_scalar(mat4->v0->w)) + "|");
                            logger->log("|" + to_string(resolve_scalar(mat4->v1->x)) + ", " + to_string(resolve_scalar(mat4->v1->y)) + ", " + to_string(resolve_scalar(mat4->v1->z)) + ", " + to_string(resolve_scalar(mat4->v1->w)) + "|");
                            logger->log("|" + to_string(resolve_scalar(mat4->v2->x)) + ", " + to_string(resolve_scalar(mat4->v2->y)) + ", " + to_string(resolve_scalar(mat4->v2->z)) + ", " + to_string(resolve_scalar(mat4->v2->w)) + "|");
                            logger->log("|" + to_string(resolve_scalar(mat4->v3->x)) + ", " + to_string(resolve_scalar(mat4->v3->y)) + ", " + to_string(resolve_scalar(mat4->v3->z)) + ", " + to_string(resolve_scalar(mat4->v3->w)) + "|");
                            break;
                        }

                    default: break;
                }
                return NULL;
            }

        default: return NULL;
    }
}

Expr* Interpreter::execute_stmts(Stmts* stmts) {
    Expr* returnValue = NULL;
    for(unsigned int it = 0; it < stmts->list.size(); it++) { 
        Stmt* stmt = stmts->list.at(it);
        if(stmt->type == NODE_RETURN) {
            Return* ret = (Return*)stmt;
            returnValue = ret->value;
            break;
        }

        Expr* expr = eval_stmt(stmt);
        if(expr != NULL) {
            returnValue = expr;
            break;
        }
    }

    if(returnValue != NULL) {
        return eval_expr(returnValue);
    } else {
        return NULL;
    }
}

void Interpreter::compile_shader(GLuint* handle, ShaderSource* source) {
    const char* src = source->code.c_str();
    gl->glShaderSource(*handle, 1, &src, NULL);
    gl->glCompileShader(*handle);

    GLint success;
    char log[256];

    gl->glGetShaderInfoLog(*handle, 256, 0, log);
    gl->glGetShaderiv(*handle, GL_COMPILE_STATUS, &success);
    if(success != GL_TRUE) {
        cout << source->name << " shader error\n" << success << " " << log << endl;
    }
}

void Interpreter::compile_program() {
    programs.clear();
    for(map<string, ShaderPair*>::iterator it = shaders.begin(); it != shaders.end(); ++it) {
        Program* program = new Program;
        program->handle = gl->glCreateProgram();
        program->vert = gl->glCreateShader(GL_VERTEX_SHADER);
        program->frag = gl->glCreateShader(GL_FRAGMENT_SHADER);

        gl->glAttachShader(program->handle, program->vert);
        gl->glAttachShader(program->handle, program->frag);

        program->vertSource = it->second->vertex;
        program->fragSource = it->second->fragment;

        if(program->vertSource == NULL) {
            logger->log("ERROR: Missing vertex shader source for program " + it->first);
            continue;
        }

        if(program->fragSource == NULL) {
            logger->log("ERROR: Missing fragment shader source for program " + it->first);
            continue;
        }

        compile_shader(&(program->vert), program->vertSource);
        compile_shader(&(program->frag), program->fragSource);

        GLint success;
        char log[256];
        gl->glLinkProgram(program->handle);
        gl->glGetProgramiv(program->handle, GL_LINK_STATUS, &success);
        gl->glGetProgramInfoLog(program->handle, 256, 0, log);
        if(success != GL_TRUE) {
            logger->log("ERROR: Can't compile program-- see error log below for details");
            logger->log(string(log));
        }

        programs[it->first] = program;
    }
}

void Interpreter::execute_init() {
    if(!init || status) return;
    buffers.clear();
    globalScope->clear();

    execute_stmts(init->stmts);
}

void Interpreter::execute_loop() {
    if(!loop || status) return;

    execute_stmts(loop->stmts);
}

void Interpreter::parse(string code) {
    YY_BUFFER_STATE state = yy_scan_string(code.c_str());
    reset();
    yylineno = 1;
    status = yyparse(&shaders, &functions);
    yy_delete_buffer(state);
}

void Interpreter::prepare() {
    init = functions["init"];
    if(init == NULL) {
        logger->log("ERROR: init() function required!");
    }

    loop = functions["loop"];
    if(loop == NULL) {
        logger->log("ERROR: loop() function required!");
    }
}

