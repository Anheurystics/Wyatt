#include "interpreter.h"
#include <sstream>
#include <memory>

int resolve_int(shared_ptr<Expr> expr) {
    return static_pointer_cast<Int>(expr)->value;
}

float resolve_float(shared_ptr<Expr> expr) {
    return static_pointer_cast<Float>(expr)->value;
}

float resolve_scalar(shared_ptr<Expr> expr) {
    if(expr->type == NODE_INT) {
        return (float)resolve_int(expr);
    } else {
        return resolve_float(expr);
    }
}

#define resolve_vec2(v) resolve_scalar(v->x), resolve_scalar(v->y)
#define resolve_vec3(v) resolve_scalar(v->x), resolve_scalar(v->y), resolve_scalar(v->z)
#define resolve_vec4(v) resolve_scalar(v->x), resolve_scalar(v->y), resolve_scalar(v->z), resolve_scalar(v->w)

#define LOOP_TIMEOUT 5

Prototype::Interpreter::Interpreter(LogWindow* logger): scanner(), parser(scanner, &functions, &shaders) {
    this->logger = logger;

    globalScope = make_shared<Scope>("global");

    string utilsrc = str_from_file("utils.txt");
    parse(utilsrc);
    if(status != 0) {
        logger->log("Error parsing utils.txt");
    }

    status = 0;

    for(map<string, shared_ptr<FuncDef>>::iterator it = functions.begin(); it != functions.end(); ++it) {
        builtins[it->first] = it->second;
    }
    functions.clear();
}

#define clear_map(type, name) \
    for(map<string, shared_ptr<type>>::iterator it = name.begin(); it != name.end(); ++it) { \
        name.erase(it); \
    } \

void Prototype::Interpreter::reset() {
    clear_map(Buffer, buffers);
    clear_map(Program, programs);
    clear_map(ShaderPair, shaders);
    clear_map(FuncDef, functions);

    globalScope->clear();
    while(!functionScopeStack.empty()) functionScopeStack.pop();

    current_program_name = "";
    current_program = NULL;
    init = NULL;
    loop = NULL;
    gl = NULL;
}

string tostring(shared_ptr<Expr> expr) {
    switch(expr->type) {
        case NODE_INT:
            return to_string(resolve_int(expr));
        case NODE_FLOAT:
            return to_string(resolve_float(expr));
        case NODE_BOOL:
            return static_pointer_cast<Bool>(expr)->value? "true" : "false";
        case NODE_STRING:
            return static_pointer_cast<String>(expr)->value;
        case NODE_VECTOR2:
            {
                shared_ptr<Vector2> vec2 = static_pointer_cast<Vector2>(expr);
                return "[" + to_string(resolve_scalar(vec2->x)) + ", " + to_string(resolve_scalar(vec2->y)) + "]";
            }
        case NODE_VECTOR3:
            {
                shared_ptr<Vector3> vec3 = static_pointer_cast<Vector3>(expr);
                return "[" + to_string(resolve_scalar(vec3->x)) + ", " + to_string(resolve_scalar(vec3->y)) + ", " + to_string(resolve_scalar(vec3->z)) + "]";
            }
        case NODE_VECTOR4:
            {
                shared_ptr<Vector4> vec4 = static_pointer_cast<Vector4>(expr);
                return "[" + to_string(resolve_scalar(vec4->x)) + ", " + to_string(resolve_scalar(vec4->y)) + ", " + to_string(resolve_scalar(vec4->z)) + ", " + to_string(resolve_scalar(vec4->w)) + "]";
            }
        case NODE_MATRIX2:
            {
                shared_ptr<Matrix2> mat2 = static_pointer_cast<Matrix2>(expr);
                return "|" + to_string(resolve_scalar(mat2->v0->x)) + ", " + to_string(resolve_scalar(mat2->v0->y)) + "|\n" +
                       "|" + to_string(resolve_scalar(mat2->v1->x)) + ", " + to_string(resolve_scalar(mat2->v1->y)) + "|";
            }
        case NODE_MATRIX3:
            {
                shared_ptr<Matrix3> mat3 = static_pointer_cast<Matrix3>(expr);
                return "|" + to_string(resolve_scalar(mat3->v0->x)) + ", " + to_string(resolve_scalar(mat3->v0->y)) + ", " + to_string(resolve_scalar(mat3->v0->z)) + "|\n" +
                       "|" + to_string(resolve_scalar(mat3->v1->x)) + ", " + to_string(resolve_scalar(mat3->v1->y)) + ", " + to_string(resolve_scalar(mat3->v1->z)) + "|\n" +
                       "|" + to_string(resolve_scalar(mat3->v2->x)) + ", " + to_string(resolve_scalar(mat3->v2->y)) + ", " + to_string(resolve_scalar(mat3->v2->z)) + "|";
            }
        case NODE_MATRIX4:
            {
                shared_ptr<Matrix4> mat4 = static_pointer_cast<Matrix4>(expr);
                return "|" + to_string(resolve_scalar(mat4->v0->x)) + ", " + to_string(resolve_scalar(mat4->v0->y)) + ", " + to_string(resolve_scalar(mat4->v0->z)) + ", " + to_string(resolve_scalar(mat4->v0->w)) + "|\n" +
                       "|" + to_string(resolve_scalar(mat4->v1->x)) + ", " + to_string(resolve_scalar(mat4->v1->y)) + ", " + to_string(resolve_scalar(mat4->v1->z)) + ", " + to_string(resolve_scalar(mat4->v1->w)) + "|\n" +
                       "|" + to_string(resolve_scalar(mat4->v2->x)) + ", " + to_string(resolve_scalar(mat4->v2->y)) + ", " + to_string(resolve_scalar(mat4->v2->z)) + ", " + to_string(resolve_scalar(mat4->v2->w)) + "|\n" +
                       "|" + to_string(resolve_scalar(mat4->v3->x)) + ", " + to_string(resolve_scalar(mat4->v3->y)) + ", " + to_string(resolve_scalar(mat4->v3->z)) + ", " + to_string(resolve_scalar(mat4->v3->w)) + "|";
            }
        default:
            return "";
    }
}

shared_ptr<Expr> Prototype::Interpreter::eval_binary(shared_ptr<Binary> bin) {
    shared_ptr<Expr> lhs = eval_expr(bin->lhs);
    if(!lhs) return NULL;

    OpType op = bin->op;

    shared_ptr<Expr> rhs = eval_expr(bin->rhs);
    if(!rhs) return NULL;

    NodeType ltype = lhs->type;
    NodeType rtype = rhs->type;

    if(ltype == NODE_BOOL && rtype == NODE_BOOL) {
        bool a = static_pointer_cast<Bool>(lhs)->value;
        bool b = static_pointer_cast<Bool>(rhs)->value;

        switch(op) {
            case OP_AND: return make_shared<Bool>(a && b);
            case OP_OR: return make_shared<Bool>(a || b);
            default: break;
        }
    }

    if(ltype == NODE_INT && rtype == NODE_INT) {
        int a = resolve_int(lhs);
        int b = resolve_int(rhs);

        switch(op) {
            case OP_PLUS: return make_shared<Int>(a + b);
            case OP_MINUS: return make_shared<Int>(a - b);
            case OP_MULT: return make_shared<Int>(a * b);
            case OP_DIV: return make_shared<Float>(a / (float)b);
            case OP_MOD: return make_shared<Int>(a % b);
            case OP_EQUAL: return make_shared<Bool>(a == b);
            case OP_LESSTHAN: return make_shared<Bool>(a < b);
            case OP_GREATERTHAN: return make_shared<Bool>(a > b);
            case OP_NEQUAL: return make_shared<Bool>(a != b);
            case OP_LEQUAL: return make_shared<Bool>(a <= b);
            case OP_GEQUAL: return make_shared<Bool>(a >= b);
            default: break;
        }
    }

    if(ltype == NODE_FLOAT && rtype == NODE_FLOAT) {
        float a = resolve_float(lhs);
        float b = resolve_float(rhs);

        switch(op) {
            case OP_PLUS: return make_shared<Float>(a + b);
            case OP_MINUS: return make_shared<Float>(a - b);
            case OP_MULT: return make_shared<Float>(a * b);
            case OP_DIV: return make_shared<Float>(a / b);
            case OP_EQUAL: return make_shared<Bool>(a == b);
            case OP_LESSTHAN: return make_shared<Bool>(a < b);
            case OP_GREATERTHAN: return make_shared<Bool>(a > b);
            case OP_NEQUAL: return make_shared<Bool>(a != b);
            case OP_LEQUAL: return make_shared<Bool>(a <= b);
            case OP_GEQUAL: return make_shared<Bool>(a >= b);
            default: break;
        }
    }

    if((ltype == NODE_FLOAT && rtype == NODE_INT) || (ltype == NODE_INT && rtype == NODE_FLOAT)) {
        float a = resolve_scalar(lhs);
        float b = resolve_scalar(rhs);

        switch(op) {
            case OP_PLUS: return make_shared<Float>(a + b);
            case OP_MINUS: return make_shared<Float>(a - b);
            case OP_MULT: return make_shared<Float>(a * b);
            case OP_DIV: return make_shared<Float>(a / b);
            case OP_EQUAL: return make_shared<Bool>(a == b);
            case OP_LESSTHAN: return make_shared<Bool>(a < b);
            case OP_GREATERTHAN: return make_shared<Bool>(a > b);
            case OP_NEQUAL: return make_shared<Bool>(a != b);
            case OP_LEQUAL: return make_shared<Bool>(a <= b);
            case OP_GEQUAL: return make_shared<Bool>(a >= b);
            default: break;
        }
    }

    if(ltype == NODE_STRING || rtype == NODE_STRING) {
        bool left = (ltype == NODE_STRING);
        shared_ptr<String> str = shared_ptr<String>(left? static_pointer_cast<String>(lhs) : static_pointer_cast<String>(rhs));
        shared_ptr<Expr> other = eval_expr(left? lhs : rhs);

        return make_shared<String>(left? (str->value + tostring(other)) : (tostring(other) + str->value));
    }

    if(ltype == NODE_VECTOR2 && rtype == NODE_VECTOR2) {
        shared_ptr<Vector2> a = static_pointer_cast<Vector2>(eval_expr(lhs));
        shared_ptr<Vector2> b = static_pointer_cast<Vector2>(eval_expr(rhs));

        float ax = resolve_scalar(a->x);
        float ay = resolve_scalar(a->y);
        float bx = resolve_scalar(b->x);
        float by = resolve_scalar(b->y);

        switch(op) {
            case OP_PLUS: return make_shared<Vector2>(make_shared<Float>(ax+bx), make_shared<Float>(ay+by));
            case OP_MINUS: return make_shared<Vector2>(make_shared<Float>(ax-bx), make_shared<Float>(ay-by));
            case OP_MULT: return make_shared<Float>(ax*bx + ay*by);
            case OP_MOD: return make_shared<Float>(ax*by - ay*bx);
            default: break;
        }
    }

    if(ltype == NODE_VECTOR2 && (rtype == NODE_INT || rtype == NODE_FLOAT)) {
        shared_ptr<Vector2> a = static_pointer_cast<Vector2>(eval_expr(lhs));
        float ax = resolve_scalar(a->x), ay = resolve_scalar(a->y);
        float b = resolve_scalar(rhs);

        switch(op) {
            case OP_MULT: return make_shared<Vector2>(make_shared<Float>(ax*b), make_shared<Float>(ay*b));
            case OP_DIV: return make_shared<Vector2>(make_shared<Float>(ax/b), make_shared<Float>(ay/b));
            default: break;
        }
    }

    if((ltype == NODE_INT || ltype == NODE_FLOAT) && rtype == NODE_VECTOR2) {
        float a = resolve_scalar(lhs);
        shared_ptr<Vector2> b = static_pointer_cast<Vector2>(eval_expr(rhs));
        float bx = resolve_scalar(b->x), by = resolve_scalar(b->y);

        switch(op) {
            case OP_MULT: return make_shared<Vector2>(make_shared<Float>(bx*a), make_shared<Float>(by*a));
            default: break;
        }
    }

    if(ltype == NODE_VECTOR3 && rtype == NODE_VECTOR3) {
        shared_ptr<Vector3> a = static_pointer_cast<Vector3>(eval_expr(lhs));
        shared_ptr<Vector3> b = static_pointer_cast<Vector3>(eval_expr(rhs));

        float ax = resolve_scalar(a->x); 
        float ay = resolve_scalar(a->y);
        float az = resolve_scalar(a->z);
        float bx = resolve_scalar(b->x);
        float by = resolve_scalar(b->y);
        float bz = resolve_scalar(b->z);

        switch(op) {
            case OP_PLUS: return make_shared<Vector3>(make_shared<Float>(ax+bx), make_shared<Float>(ay+by), make_shared<Float>(az+bz));
            case OP_MINUS: return make_shared<Vector3>(make_shared<Float>(ax-bx), make_shared<Float>(ay-by), make_shared<Float>(az-bz));
            case OP_MULT: return make_shared<Float>(ax*bx + ay*by + az*bz);
            case OP_MOD: return make_shared<Vector3>(make_shared<Float>(ay*bz-az*by), make_shared<Float>(az*bx-ax*bz), make_shared<Float>(ax*by-ay*bx));
            default: break;
        }
    }

    if(ltype == NODE_VECTOR3 && (rtype == NODE_INT || rtype == NODE_FLOAT)) {
        shared_ptr<Vector3> a = static_pointer_cast<Vector3>(eval_expr(lhs));
        float ax = resolve_scalar(a->x), ay = resolve_scalar(a->y), az = resolve_scalar(a->z);
        float b = resolve_scalar(rhs);

        switch(op) {
            case OP_MULT: return make_shared<Vector3>(make_shared<Float>(ax*b), make_shared<Float>(ay*b), make_shared<Float>(az*b));
            case OP_DIV: return make_shared<Vector3>(make_shared<Float>(ax/b), make_shared<Float>(ay/b), make_shared<Float>(az/b));
            default: break;
        }
    }

    if((ltype == NODE_INT || ltype == NODE_FLOAT) && rtype == NODE_VECTOR3) {
        float a = resolve_scalar(lhs);
        shared_ptr<Vector3> b = static_pointer_cast<Vector3>(eval_expr(rhs));
        float bx = resolve_scalar(b->x), by = resolve_scalar(b->y), bz = resolve_scalar(b->z);

        switch(op) {
            case OP_MULT: return make_shared<Vector3>(make_shared<Float>(bx*a), make_shared<Float>(by*a), make_shared<Float>(bz*a));
            default: break;
        }
    }

    if(ltype == NODE_VECTOR4 && rtype == NODE_VECTOR4) {
        shared_ptr<Vector4> a = static_pointer_cast<Vector4>(eval_expr(lhs));
        shared_ptr<Vector4> b = static_pointer_cast<Vector4>(eval_expr(rhs));

        float ax = resolve_scalar(a->x); 
        float ay = resolve_scalar(a->y);
        float az = resolve_scalar(a->z);
        float aw = resolve_scalar(a->w);
        float bx = resolve_scalar(b->x);
        float by = resolve_scalar(b->y);
        float bz = resolve_scalar(b->z);
        float bw = resolve_scalar(b->w);

        switch(op) {
            case OP_PLUS: return make_shared<Vector4>(make_shared<Float>(ax+bx), make_shared<Float>(ay+by), make_shared<Float>(az+bz), make_shared<Float>(aw+bw));
            case OP_MINUS: return make_shared<Vector4>(make_shared<Float>(ax-bx), make_shared<Float>(ay-by), make_shared<Float>(az-bz), make_shared<Float>(aw-bw));
            case OP_MULT: return make_shared<Float>(ax*bx + ay*by + az*bz + aw*bw);
            default: break;
        }
    }

    if(ltype == NODE_VECTOR4 && (rtype == NODE_INT || rtype == NODE_FLOAT)) {
        shared_ptr<Vector4> a = static_pointer_cast<Vector4>(eval_expr(lhs));
        float ax = resolve_scalar(a->x), ay = resolve_scalar(a->y), az = resolve_scalar(a->z), aw = resolve_scalar(a->w);
        float b = resolve_scalar(rhs);

        switch(op) {
            case OP_MULT: return make_shared<Vector4>(make_shared<Float>(ax*b), make_shared<Float>(ay*b), make_shared<Float>(az*b), make_shared<Float>(aw*b));
            case OP_DIV: return make_shared<Vector4>(make_shared<Float>(ax/b), make_shared<Float>(ay/b), make_shared<Float>(az/b), make_shared<Float>(aw/b));
            default: break;
        }
    }

    if((ltype == NODE_INT || ltype == NODE_FLOAT) && rtype == NODE_VECTOR4) {
        float a = resolve_scalar(lhs);
        shared_ptr<Vector4> b = static_pointer_cast<Vector4>(eval_expr(rhs));
        float bx = resolve_scalar(b->x), by = resolve_scalar(b->y), bz = resolve_scalar(b->z), bw = resolve_scalar(b->w);

        switch(op) {
            case OP_MULT: return make_shared<Vector4>(make_shared<Float>(bx*a), make_shared<Float>(by*a), make_shared<Float>(bz*a), make_shared<Float>(bw*a));
            default: break;
        }
    }

    if(ltype == NODE_MATRIX2 && (rtype == NODE_INT || rtype == NODE_FLOAT)) {
        shared_ptr<Matrix2> a = static_pointer_cast<Matrix2>(eval_expr(lhs));

        if(op == OP_MULT || op == OP_DIV) {
            shared_ptr<Vector2> v0 = static_pointer_cast<Vector2>(eval_binary(make_shared<Binary>(a->v0, op, rhs)));
            shared_ptr<Vector2> v1 = static_pointer_cast<Vector2>(eval_binary(make_shared<Binary>(a->v1, op, rhs)));
            return make_shared<Matrix2>(v0, v1);
        }
    }

    if((ltype == NODE_INT || ltype == NODE_FLOAT) && (rtype == NODE_MATRIX2)) {
        shared_ptr<Matrix2> a = static_pointer_cast<Matrix2>(eval_expr(rhs));

        if(op == OP_MULT) {
            shared_ptr<Vector2> v0 = static_pointer_cast<Vector2>(eval_binary(make_shared<Binary>(a->v0, op, lhs)));
            shared_ptr<Vector2> v1 = static_pointer_cast<Vector2>(eval_binary(make_shared<Binary>(a->v1, op, lhs)));
            return make_shared<Matrix2>(v0, v1);
        }
    }

    if(ltype == NODE_MATRIX2 && rtype == NODE_MATRIX2) {
        shared_ptr<Matrix2> a = static_pointer_cast<Matrix2>(eval_expr(lhs));
        shared_ptr<Matrix2> b = static_pointer_cast<Matrix2>(eval_expr(rhs));

        if(op == OP_MULT) {
            shared_ptr<Vector2> r0 = make_shared<Vector2>(make_shared<Binary>(a->v0, OP_MULT, b->c0), make_shared<Binary>(a->v0, OP_MULT, b->c1));
            shared_ptr<Vector2> r1 = make_shared<Vector2>(make_shared<Binary>(a->v1, OP_MULT, b->c0), make_shared<Binary>(a->v1, OP_MULT, b->c1));
            return eval_expr(make_shared<Matrix2>(r0, r1));
        }
    }

    if(ltype == NODE_VECTOR2 && rtype == NODE_MATRIX2) {
        shared_ptr<Vector2> a = static_pointer_cast<Vector2>(eval_expr(lhs));
        shared_ptr<Matrix2> b = static_pointer_cast<Matrix2>(eval_expr(rhs));

        if(op == OP_MULT) {
            return eval_expr(make_shared<Vector2>(make_shared<Binary>(a, OP_MULT, b->c0), make_shared<Binary>(a, OP_MULT, b->c1)));
        }
    }

    if(ltype == NODE_MATRIX3 && (rtype == NODE_INT || rtype == NODE_FLOAT)) {
        shared_ptr<Matrix3> a = static_pointer_cast<Matrix3>(eval_expr(lhs));

        if(op == OP_MULT || op == OP_DIV) {
            shared_ptr<Vector3> v0 = static_pointer_cast<Vector3>(eval_binary(make_shared<Binary>(a->v0, op, rhs)));
            shared_ptr<Vector3> v1 = static_pointer_cast<Vector3>(eval_binary(make_shared<Binary>(a->v1, op, rhs)));
            shared_ptr<Vector3> v2 = static_pointer_cast<Vector3>(eval_binary(make_shared<Binary>(a->v2, op, rhs)));
            return make_shared<Matrix3>(v0, v1, v2);
        }
    }

    if((ltype == NODE_INT || ltype == NODE_FLOAT) && (rtype == NODE_MATRIX3)) {
        shared_ptr<Matrix3> a = static_pointer_cast<Matrix3>(eval_expr(rhs));

        if(op == OP_MULT) {
            shared_ptr<Vector3> v0 = static_pointer_cast<Vector3>(eval_binary(make_shared<Binary>(a->v0, op, lhs)));
            shared_ptr<Vector3> v1 = static_pointer_cast<Vector3>(eval_binary(make_shared<Binary>(a->v1, op, lhs)));
            shared_ptr<Vector3> v2 = static_pointer_cast<Vector3>(eval_binary(make_shared<Binary>(a->v2, op, lhs)));
            return make_shared<Matrix3>(v0, v1, v2);
        }
    }

    if(ltype == NODE_MATRIX3 && rtype == NODE_MATRIX3) {
        shared_ptr<Matrix3> a = static_pointer_cast<Matrix3>(eval_expr(lhs));
        shared_ptr<Matrix3> b = static_pointer_cast<Matrix3>(eval_expr(rhs));
        
        if(op == OP_MULT) {
            shared_ptr<Vector3> r0 = make_shared<Vector3>(make_shared<Binary>(a->v0, OP_MULT, b->c0), make_shared<Binary>(a->v0, OP_MULT, b->c1), make_shared<Binary>(a->v0, OP_MULT, b->c2));
            shared_ptr<Vector3> r1 = make_shared<Vector3>(make_shared<Binary>(a->v1, OP_MULT, b->c0), make_shared<Binary>(a->v1, OP_MULT, b->c1), make_shared<Binary>(a->v1, OP_MULT, b->c2));
            shared_ptr<Vector3> r2 = make_shared<Vector3>(make_shared<Binary>(a->v2, OP_MULT, b->c0), make_shared<Binary>(a->v2, OP_MULT, b->c1), make_shared<Binary>(a->v2, OP_MULT, b->c2));
            return eval_expr(make_shared<Matrix3>(r0, r1, r2));
        }
    }

    if(ltype == NODE_VECTOR3 && rtype == NODE_MATRIX3) {
        shared_ptr<Vector3> a = static_pointer_cast<Vector3>(eval_expr(lhs));
        shared_ptr<Matrix3> b = static_pointer_cast<Matrix3>(eval_expr(rhs));

        if(op == OP_MULT) {
            return eval_expr(make_shared<Vector3>(make_shared<Binary>(a, OP_MULT, b->c0), make_shared<Binary>(a, OP_MULT, b->c1), make_shared<Binary>(a, OP_MULT, b->c2)));
        }
    }

    if(ltype == NODE_MATRIX4 && (rtype == NODE_INT || rtype == NODE_FLOAT)) {
        shared_ptr<Matrix4> a = static_pointer_cast<Matrix4>(eval_expr(lhs));

        if(op == OP_MULT || op == OP_DIV) {
            shared_ptr<Vector4> v0 = static_pointer_cast<Vector4>(eval_binary(make_shared<Binary>(a->v0, op, rhs)));
            shared_ptr<Vector4> v1 = static_pointer_cast<Vector4>(eval_binary(make_shared<Binary>(a->v1, op, rhs)));
            shared_ptr<Vector4> v2 = static_pointer_cast<Vector4>(eval_binary(make_shared<Binary>(a->v2, op, rhs)));
            shared_ptr<Vector4> v3 = static_pointer_cast<Vector4>(eval_binary(make_shared<Binary>(a->v3, op, rhs)));
            return make_shared<Matrix4>(v0, v1, v2, v3);
        }
    }

    if((ltype == NODE_INT || ltype == NODE_FLOAT) && (rtype == NODE_MATRIX4)) {
        shared_ptr<Matrix4> a = static_pointer_cast<Matrix4>(eval_expr(rhs));

        if(op == OP_MULT) {
            shared_ptr<Vector4> v0 = static_pointer_cast<Vector4>(eval_binary(make_shared<Binary>(a->v0, op, lhs)));
            shared_ptr<Vector4> v1 = static_pointer_cast<Vector4>(eval_binary(make_shared<Binary>(a->v1, op, lhs)));
            shared_ptr<Vector4> v2 = static_pointer_cast<Vector4>(eval_binary(make_shared<Binary>(a->v2, op, lhs)));
            shared_ptr<Vector4> v3 = static_pointer_cast<Vector4>(eval_binary(make_shared<Binary>(a->v3, op, lhs)));
            return make_shared<Matrix4>(v0, v1, v2, v3);
        }
    }

    if(ltype == NODE_MATRIX4 && rtype == NODE_MATRIX4) {
        shared_ptr<Matrix4> a = static_pointer_cast<Matrix4>(eval_expr(lhs));
        shared_ptr<Matrix4> b = static_pointer_cast<Matrix4>(eval_expr(rhs));
        
        if(op == OP_MULT) {
            shared_ptr<Vector4> r0 = make_shared<Vector4>(make_shared<Binary>(a->v0, OP_MULT, b->c0), make_shared<Binary>(a->v0, OP_MULT, b->c1), make_shared<Binary>(a->v0, OP_MULT, b->c2), make_shared<Binary>(a->v0, OP_MULT, b->c3));
            shared_ptr<Vector4> r1 = make_shared<Vector4>(make_shared<Binary>(a->v1, OP_MULT, b->c0), make_shared<Binary>(a->v1, OP_MULT, b->c1), make_shared<Binary>(a->v1, OP_MULT, b->c2), make_shared<Binary>(a->v1, OP_MULT, b->c3));
            shared_ptr<Vector4> r2 = make_shared<Vector4>(make_shared<Binary>(a->v2, OP_MULT, b->c0), make_shared<Binary>(a->v2, OP_MULT, b->c1), make_shared<Binary>(a->v2, OP_MULT, b->c2), make_shared<Binary>(a->v2, OP_MULT, b->c3));
            shared_ptr<Vector4> r3 = make_shared<Vector4>(make_shared<Binary>(a->v3, OP_MULT, b->c0), make_shared<Binary>(a->v3, OP_MULT, b->c1), make_shared<Binary>(a->v3, OP_MULT, b->c2), make_shared<Binary>(a->v3, OP_MULT, b->c3));
            return eval_expr(make_shared<Matrix4>(r0, r1, r2, r3));
        }
    }

    if(ltype == NODE_VECTOR4 && rtype == NODE_MATRIX4) {
        shared_ptr<Vector4> a = static_pointer_cast<Vector4>(eval_expr(lhs));
        shared_ptr<Matrix4> b = static_pointer_cast<Matrix4>(eval_expr(rhs));

        if(op == OP_MULT) {
            return eval_expr(make_shared<Vector4>(make_shared<Binary>(a, OP_MULT, b->c0), make_shared<Binary>(a, OP_MULT, b->c1), make_shared<Binary>(a, OP_MULT, b->c2), make_shared<Binary>(a, OP_MULT, b->c3)));
        }
    }

    logger->log(bin, "ERROR", "Invalid operation between " + type_to_name(ltype) + " and " + type_to_name(rtype));
    return NULL;
}

shared_ptr<Expr> Prototype::Interpreter::invoke(shared_ptr<Invoke> invoke) {
    string name = invoke->ident->name;
    shared_ptr<FuncDef> def = NULL;

    if(name == "cos") {
        if(invoke->args->list.size() == 1) {
            shared_ptr<Expr> v = eval_expr(invoke->args->list[0]);
            if(v->type == NODE_FLOAT || v->type == NODE_INT) {
                return make_shared<Float>(cosf(resolve_scalar(v)));
            }
        }
    }

    if(name == "sin") {
        if(invoke->args->list.size() == 1) {
            shared_ptr<Expr> v = eval_expr(invoke->args->list[0]);
            if(v->type == NODE_FLOAT || v->type == NODE_INT) {
                return make_shared<Float>(sinf(resolve_scalar(v)));
            }
        }
    }

    if(name == "tan") {
        if(invoke->args->list.size() == 1) {
            shared_ptr<Expr> v = eval_expr(invoke->args->list[0]);
            if(v->type == NODE_FLOAT || v->type == NODE_INT) {
                return make_shared<Float>(tanf(resolve_scalar(v)));
            }
        }
    }

    if(name == "pi") {
        if(invoke->args->list.size() == 0) {
            return make_shared<Float>(3.14159f);
        }
    }

    if(builtins.find(name) != builtins.end()) {
        def = builtins[name];
    }

    if(functions.find(name) != functions.end()) {
        def = functions[name];
    }

    if(def != NULL) {
        shared_ptr<Scope> localScope = make_shared<Scope>(name);
        unsigned int nParams = def->params->list.size();
        unsigned int nArgs = invoke->args->list.size();

        if(nParams != nArgs) {
            logger->log(invoke, "ERROR", "Function " + name + " expects " + to_string(nParams) + " arguments, got " + to_string(nArgs));
            return NULL;
        }
        if(nParams > 0) {
            for(unsigned int i = 0; i < nParams; i++) {
                shared_ptr<Expr> arg = eval_expr(invoke->args->list[i]);
                if(arg == NULL) {
                    logger->log("ERROR: Invalid argument passed on to " + name);
                    return NULL;
                }
                localScope->declare(def->params->list[i]->name, arg);
            }
        }

        functionScopeStack.push(localScope);
        shared_ptr<Expr> retValue = execute_stmts(def->stmts);
        functionScopeStack.pop();
        return retValue;
    } else {
        logger->log("ERROR: Call to undefined function " + name);
    }
    return NULL;
}

shared_ptr<Expr> Prototype::Interpreter::resolve_vector(vector<shared_ptr<Expr>> list) {
    vector<float> data;
    int n = 0;

    for(unsigned int i = 0; i < list.size(); i++) {
        shared_ptr<Expr> expr = list[i];
        if(expr->type == NODE_FLOAT || expr->type == NODE_INT) {
            data.push_back(resolve_scalar(expr));
            n += 1;
        } else
        if(expr->type == NODE_VECTOR2) {
            shared_ptr<Vector2> vec2 = static_pointer_cast<Vector2>(eval_expr(expr));
            data.push_back(resolve_scalar(vec2->x));
            data.push_back(resolve_scalar(vec2->y));
            n += 2;
        } else
        if(expr->type == NODE_VECTOR3) {
            shared_ptr<Vector3> vec3 = static_pointer_cast<Vector3>(eval_expr(expr));
            data.push_back(resolve_scalar(vec3->x));
            data.push_back(resolve_scalar(vec3->y));
            data.push_back(resolve_scalar(vec3->z));
            n += 3;
        } else
        if(expr->type == NODE_VECTOR4) {
            shared_ptr<Vector4> vec4 = static_pointer_cast<Vector4>(eval_expr(expr));
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
        return make_shared<Vector3>(make_shared<Float>(data[0]), make_shared<Float>(data[1]), make_shared<Float>(data[2]));
    }
    if(data.size() == 4) {
        return make_shared<Vector4>(make_shared<Float>(data[0]), make_shared<Float>(data[1]), make_shared<Float>(data[2]), make_shared<Float>(data[3]));
    }

    return NULL;
}

shared_ptr<Expr> Prototype::Interpreter::eval_expr(shared_ptr<Expr> node) {
    switch(node->type) {
        case NODE_IDENT: 
            {
                shared_ptr<Ident> ident = static_pointer_cast<Ident>(node);
                shared_ptr<Expr> value = NULL;
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
       case NODE_DOT:
            {
                shared_ptr<Dot> uniform = static_pointer_cast<Dot>(node);
                if(current_program->vertSource->name == uniform->shader) {
                    shared_ptr<ShaderSource> src = current_program->vertSource;
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
                        shared_ptr<Float> f = make_shared<Float>(0);
                        gl->glGetUniformfv(current_program->handle, loc, &(f->value));
                        return f;
                    } else
                    if(type == "vec2") {
                        float value[2];
                        gl->glGetUniformfv(current_program->handle, loc, &value[0]);
                        return make_shared<Vector2>(make_shared<Float>(value[0]), make_shared<Float>(value[1]));
                    } else 
                    if(type == "vec3") {
                        float value[3];
                        gl->glGetUniformfv(current_program->handle, loc, &value[0]);
                        return make_shared<Vector3>(make_shared<Float>(value[0]), make_shared<Float>(value[1]), make_shared<Float>(value[2]));
                    } else
                    if(type == "vec4") {
                        float value[4];
                        gl->glGetUniformfv(current_program->handle, loc, &value[0]);
                        return make_shared<Vector4>(make_shared<Float>(value[0]), make_shared<Float>(value[1]), make_shared<Float>(value[2]), make_shared<Float>(value[3]));
                    } else 
                    if(type == "mat2") {
                        float value[4];
                        gl->glGetUniformfv(current_program->handle, loc, &value[0]);
                        return make_shared<Matrix2>(make_shared<Vector2>(make_shared<Float>(value[0]), make_shared<Float>(value[1])), make_shared<Vector2>(make_shared<Float>(value[2]), make_shared<Float>(value[3])));
                    } else
                    if(type == "mat3") {
                        float value[9];
                        gl->glGetUniformfv(current_program->handle, loc, &value[0]);
                        return make_shared<Matrix3>(
                            make_shared<Vector3>(make_shared<Float>(value[0]), make_shared<Float>(value[1]), make_shared<Float>(value[2])),
                            make_shared<Vector3>(make_shared<Float>(value[3]), make_shared<Float>(value[4]), make_shared<Float>(value[5])),
                            make_shared<Vector3>(make_shared<Float>(value[6]), make_shared<Float>(value[7]), make_shared<Float>(value[8]))
                        );
                    } else
                    if(type == "mat4") {
                        float value[16];
                        gl->glGetUniformfv(current_program->handle, loc, &value[0]);
                        return make_shared<Matrix4>(
                            make_shared<Vector4>(make_shared<Float>(value[0]), make_shared<Float>(value[1]), make_shared<Float>(value[2]), make_shared<Float>(value[3])),
                            make_shared<Vector4>(make_shared<Float>(value[4]), make_shared<Float>(value[5]), make_shared<Float>(value[6]), make_shared<Float>(value[7])),
                            make_shared<Vector4>(make_shared<Float>(value[8]), make_shared<Float>(value[9]), make_shared<Float>(value[10]), make_shared<Float>(value[11])),
                            make_shared<Vector4>(make_shared<Float>(value[12]), make_shared<Float>(value[13]), make_shared<Float>(value[14]), make_shared<Float>(value[15]))
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

        case NODE_STRING:
            return node;

        case NODE_VECTOR2:
            {
                shared_ptr<Vector2> vec2 = static_pointer_cast<Vector2>(node);
                shared_ptr<Expr> x = eval_expr(vec2->x);
                shared_ptr<Expr> y = eval_expr(vec2->y);

                if((x->type == NODE_INT || x->type == NODE_FLOAT) && (y->type == NODE_INT || y->type == NODE_FLOAT)) {
                    return make_shared<Vector2>(static_pointer_cast<Expr>(x), static_pointer_cast<Expr>(y));
                }

                if(x->type == NODE_VECTOR2 && y->type == NODE_VECTOR2) {
                    return make_shared<Matrix2>(static_pointer_cast<Vector2>(x), static_pointer_cast<Vector2>(y));
                }

                return resolve_vector({x, y});
            }

        case NODE_VECTOR3:
            {
                shared_ptr<Vector3> vec3 = static_pointer_cast<Vector3>((node));
                shared_ptr<Expr> x = eval_expr(vec3->x);
                shared_ptr<Expr> y = eval_expr(vec3->y);
                shared_ptr<Expr> z = eval_expr(vec3->z);
                
                if((x->type == NODE_INT || x->type == NODE_FLOAT) && (y->type == NODE_INT || y->type == NODE_FLOAT) && (z->type == NODE_INT || z->type == NODE_FLOAT)) {
                    return make_shared<Vector3>(x, y, z);
                }

                if(x->type == NODE_VECTOR3 && y->type == NODE_VECTOR3 && z->type == NODE_VECTOR3) {
                    return make_shared<Matrix3>(static_pointer_cast<Vector3>(x), static_pointer_cast<Vector3>(y), static_pointer_cast<Vector3>(z));
                }

                return resolve_vector({x, y, z});
            }

        case NODE_VECTOR4:
            {
                shared_ptr<Vector4> vec4 = static_pointer_cast<Vector4>((node));
                shared_ptr<Expr> x = eval_expr(vec4->x);
                shared_ptr<Expr> y = eval_expr(vec4->y);
                shared_ptr<Expr> z = eval_expr(vec4->z);
                shared_ptr<Expr> w = eval_expr(vec4->w);
                
                if((x->type == NODE_INT || x->type == NODE_FLOAT) && (y->type == NODE_INT || y->type == NODE_FLOAT) && (z->type == NODE_INT || z->type == NODE_FLOAT) && (w->type == NODE_INT || w->type == NODE_FLOAT)) {
                    return make_shared<Vector4>(x, y, z, w);
                }

                if(x->type == NODE_VECTOR4 && y->type == NODE_VECTOR4 && z->type == NODE_VECTOR4 && w->type == NODE_VECTOR4) {
                    return make_shared<Matrix4>(static_pointer_cast<Vector4>(x), static_pointer_cast<Vector4>(y), static_pointer_cast<Vector4>(z), static_pointer_cast<Vector4>(w));
                }

                return NULL;
            }

        case NODE_MATRIX2:
            {
                shared_ptr<Matrix2> mat2 = static_pointer_cast<Matrix2>(node);
                mat2->v0 = static_pointer_cast<Vector2>(eval_expr(mat2->v0));
                mat2->v1 = static_pointer_cast<Vector2>(eval_expr(mat2->v1));
                return mat2;
            }

        case NODE_MATRIX3:
            {
                shared_ptr<Matrix3> mat3 = static_pointer_cast<Matrix3>(node);
                mat3->v0 = static_pointer_cast<Vector3>(eval_expr(mat3->v0));
                mat3->v1 = static_pointer_cast<Vector3>(eval_expr(mat3->v1));
                mat3->v2 = static_pointer_cast<Vector3>(eval_expr(mat3->v2));
                return mat3;
            }

        case NODE_MATRIX4:
            {
                shared_ptr<Matrix4> mat4 = static_pointer_cast<Matrix4>(node);
                mat4->v0 = static_pointer_cast<Vector4>(eval_expr(mat4->v0));
                mat4->v1 = static_pointer_cast<Vector4>(eval_expr(mat4->v1));
                mat4->v2 = static_pointer_cast<Vector4>(eval_expr(mat4->v2));
                mat4->v3 = static_pointer_cast<Vector4>(eval_expr(mat4->v3));
                return mat4;
            }

        case NODE_UNARY:
            {
                shared_ptr<Unary> un = static_pointer_cast<Unary>(node);
                shared_ptr<Expr> rhs = eval_expr(un->rhs);

                if(!rhs) return NULL;

                if(un->op == OP_MINUS) {
                    if(rhs->type == NODE_INT) {
                        shared_ptr<Int> i = static_pointer_cast<Int>(rhs);
                        return make_shared<Int>(-(i->value));
                    }
                    if(rhs->type == NODE_FLOAT) {
                        shared_ptr<Float> fl = static_pointer_cast<Float>(rhs);
                        return make_shared<Float>(-(fl->value));
                    }
                    if(rhs->type == NODE_VECTOR3) {
                        shared_ptr<Vector3> vec3 = static_pointer_cast<Vector3>(rhs);

                        //SHAMEFUL HACK
                        return eval_binary(make_shared<Binary>(vec3, OP_MULT, make_shared<Float>(-1)));
                    }
                }
                if(un->op == OP_NOT) {
                    if(rhs->type == NODE_BOOL) {
                        shared_ptr<Bool> b = static_pointer_cast<Bool>(rhs);
                        return make_shared<Bool>(!(b->value));
                    }
                }
                if(un->op == OP_ABS) {
                    if(rhs->type == NODE_INT) {
                        return make_shared<Int>(abs(resolve_int(rhs)));
                    }
                    if(rhs->type == NODE_FLOAT) {
                        return make_shared<Float>(fabs(resolve_float(rhs)));
                    }
                    if(rhs->type == NODE_VECTOR2) {
                        shared_ptr<Vector2> vec2 = static_pointer_cast<Vector2>(rhs);
                        float x = resolve_scalar(vec2->x), y = resolve_scalar(vec2->y);
                        return make_shared<Float>(sqrtf(x * x + y * y));
                    }
                    if(rhs->type == NODE_VECTOR3) {
                        shared_ptr<Vector3> vec3 = static_pointer_cast<Vector3>(rhs);
                        float x = resolve_scalar(vec3->x), y = resolve_scalar(vec3->y), z = resolve_scalar(vec3->z);
                        return make_shared<Float>(sqrtf(x * x + y * y + z * z));
                    }
                    if(rhs->type == NODE_VECTOR4) {
                        shared_ptr<Vector4> vec4 = static_pointer_cast<Vector4>(rhs);
                        float x = resolve_scalar(vec4->x), y = resolve_scalar(vec4->y), z = resolve_scalar(vec4->z), w = resolve_scalar(vec4->w);
                        return make_shared<Float>(sqrtf(x * x + y * y + z * z + w * w));
                    }
                    if(rhs->type == NODE_LIST) {
                        shared_ptr<List> list = static_pointer_cast<List>(rhs);
                        return make_shared<Int>(list->list.size());
                    }
                    return NULL;
                }
            }

        case NODE_BINARY:
            {
                shared_ptr<Binary> bin = static_pointer_cast<Binary>((node));
                return eval_binary(bin);
            }

        case NODE_FUNCEXPR:
            {
                shared_ptr<FuncExpr> func = static_pointer_cast<FuncExpr>(node);
                return invoke(func->invoke);
            }
        case NODE_INDEX:
            {
                shared_ptr<Index> in= static_pointer_cast<Index>(node);
                shared_ptr<Expr> source = eval_expr(in->source);
                shared_ptr<Expr> index = eval_expr(in->index);
                
                if(source->type == NODE_VECTOR2 && index->type == NODE_INT) {
                    shared_ptr<Vector2> vec2 = static_pointer_cast<Vector2>(source);
                    int i = resolve_int(index);
                    if(i == 0) return eval_expr(vec2->x);
                    if(i == 1) return eval_expr(vec2->y);
                        
                    logger->log(index, "ERROR", "Index out of range for vec2 access");
                    return NULL;
                } else
                if(source->type == NODE_VECTOR3 && index->type == NODE_INT) {
                    shared_ptr<Vector3> vec3 = static_pointer_cast<Vector3>(source);
                    int i = resolve_int(index);
                    if(i == 0) return eval_expr(vec3->x);
                    if(i == 1) return eval_expr(vec3->y);
                    if(i == 2) return eval_expr(vec3->z);
                        
                    logger->log(index, "ERROR", "Index out of range for vec3 access");
                    return NULL;
                }
                if(source->type == NODE_VECTOR4 && index->type == NODE_INT) {
                    shared_ptr<Vector4> vec4 = static_pointer_cast<Vector4>(source);
                    int i = resolve_int(index);
                    if(i == 0) return eval_expr(vec4->x);
                    if(i == 1) return eval_expr(vec4->y);
                    if(i == 2) return eval_expr(vec4->z);
                    if(i == 4) return eval_expr(vec4->w);
                        
                    logger->log(index, "ERROR", "Index out of range for vec4 access");
                    return NULL;
                }
                if(source->type == NODE_MATRIX2 && index->type == NODE_INT) {
                    shared_ptr<Matrix2> mat2 = static_pointer_cast<Matrix2>(source);
                    int i = resolve_int(index);
                    if(i == 0) return eval_expr(mat2->v0);
                    if(i == 1) return eval_expr(mat2->v1);
                    
                    logger->log(index, "ERROR", "Index out of range for mat2 access");
                    return NULL;
                }
                if(source->type == NODE_MATRIX3 && index->type == NODE_INT) {
                    shared_ptr<Matrix3> mat3 = static_pointer_cast<Matrix3>(source);
                    int i = resolve_int(index);
                    if(i == 0) return eval_expr(mat3->v0);
                    if(i == 1) return eval_expr(mat3->v1);
                    if(i == 2) return eval_expr(mat3->v2);

                    logger->log(index, "ERROR", "Index out of range for mat3 access");
                    return NULL;
                }
                if(source->type == NODE_MATRIX4 && index->type == NODE_INT) {
                    shared_ptr<Matrix4> mat4 = static_pointer_cast<Matrix4>(source);
                    int i = resolve_int(index);
                    if(i == 0) return eval_expr(mat4->v0);
                    if(i == 1) return eval_expr(mat4->v1);
                    if(i == 2) return eval_expr(mat4->v2);
                    if(i == 3) return eval_expr(mat4->v3);

                    logger->log(index, "ERROR", "Index out of range for mat4 access");
                    return NULL;
                }
                if(source->type == NODE_LIST && index->type == NODE_INT) {
                    shared_ptr<List> list = static_pointer_cast<List>(source);
                    int i = resolve_int(index);
                    if(i >= 0 && i < list->list.size()) {
                        return eval_expr(list->list[i]);
                    } else {
                        logger->log(index, "ERROR", "Index out of range for list of length " + list->list.size());
                        return NULL;
                    }
                }

                logger->log(index,"ERROR", "Invalid use of [] operator");
                return NULL;
            }

        default: logger->log(node, "ERROR", "Illegal expression"); return NULL;
    }
}

shared_ptr<Expr> Prototype::Interpreter::eval_stmt(shared_ptr<Stmt> stmt) {
    switch(stmt->type) {
        case NODE_FUNCSTMT:
            {
                shared_ptr<FuncStmt> func = static_pointer_cast<FuncStmt>(stmt);
                invoke(func->invoke);
                return NULL;
            }
        case NODE_ASSIGN:
            {
                shared_ptr<Assign> assign = static_pointer_cast<Assign>(stmt);
                shared_ptr<Expr> rhs = eval_expr(assign->value);
                if(rhs != NULL) {
                    shared_ptr<Expr> lhs = assign->lhs;
                    if(lhs->type == NODE_IDENT) {
                        shared_ptr<Scope> scope = globalScope;
                        if(!functionScopeStack.empty()) {
                            scope = functionScopeStack.top();
                        }
                        scope->declare(static_pointer_cast<Ident>(lhs)->name, rhs);
                    } else if(lhs->type == NODE_DOT) {
                        shared_ptr<Dot> uniform = static_pointer_cast<Dot>(lhs);
                        if(current_program->vertSource->name == uniform->shader) {
                            shared_ptr<ShaderSource> src = current_program->vertSource;
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
                                    shared_ptr<Float> f = static_pointer_cast<Float>(rhs);
                                    gl->glUniform1f(loc, resolve_float(f));
                                } else {
                                    logger->log("ERROR: Uniform upload mismatch: float required for " + uniform->name + " of shader " + current_program_name);
                                    return NULL;
                                }
                            } else
                            if(type == "vec2") {
                                if(rhs->type == NODE_VECTOR2) {
                                    shared_ptr<Vector2> vec2 = static_pointer_cast<Vector2>(eval_expr(rhs));
                                    gl->glUniform2f(loc, resolve_vec2(vec2));
                                } else {
                                    logger->log("ERROR: Uniform upload mismatch: vec2 required for " + uniform->name + " of shader " + current_program_name);
                                }
                            } else 
                            if(type == "vec3") {
                                if(rhs->type == NODE_VECTOR3) {
                                    shared_ptr<Vector3> vec3 = static_pointer_cast<Vector3>(eval_expr(rhs));
                                    gl->glUniform3f(loc, resolve_vec3(vec3));
                                } else {
                                    logger->log("ERROR: Uniform upload mismatch: vec3 required for " + uniform->name + " of shader " + current_program_name);
                                    return NULL;
                                }
                            } else
                            if(type == "vec4") {
                                if(rhs->type == NODE_VECTOR4) {
                                    shared_ptr<Vector4> vec4 = static_pointer_cast<Vector4>(eval_expr(rhs));
                                    gl->glUniform4f(loc, resolve_vec4(vec4));
                                } else {
                                    logger->log("ERROR: Uniform upload mismatch: vec4 required for " + uniform->name + " of shader " + current_program_name);
                                    return NULL;
                                }
                            } else
                            if(type == "mat2") {
                                if(rhs->type == NODE_MATRIX2) {
                                    shared_ptr<Matrix2> mat2 = static_pointer_cast<Matrix2>(eval_expr(rhs));
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
                                    shared_ptr<Matrix3> mat3 = static_pointer_cast<Matrix3>(eval_expr(rhs));
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
                                    shared_ptr<Matrix4> mat4 = static_pointer_cast<Matrix4>(eval_expr(rhs));
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
                    } else if (lhs->type == NODE_INDEX) {
                        shared_ptr<Index> in = static_pointer_cast<Index>(lhs);
                        shared_ptr<Expr> source = eval_expr(in->source);
                        shared_ptr<Expr> index = eval_expr(in->index);

                        if(source->type == NODE_LIST && index->type == NODE_INT) {
                            shared_ptr<List> list = static_pointer_cast<List>(source);
                            int i = resolve_int(index);
                            if(i >= 0 && i < list->list.size()) {
                                list->list[i] = rhs;
                            } else {
                                logger->log(assign, "ERROR", "Index out of range for list of length " + list->list.size());
                            }
                            return NULL;
                        }

                        bool is_scalar = (rhs->type == NODE_FLOAT || rhs->type == NODE_INT);
                        if(source->type == NODE_VECTOR2 && index->type == NODE_INT) {
                            if(!is_scalar) {
                                logger->log(assign, "ERROR", "vec2 component needs to be a float or an int");
                                return NULL;
                            }
                            shared_ptr<Vector2> vec2 = static_pointer_cast<Vector2>(source);
                            int i = resolve_int(index);
                            if(i == 0) vec2->x = rhs;
                            else if(i == 1) vec2->y = rhs;
                            else logger->log(index, "ERROR", "Index out of range for vec2 access");
                            return NULL;
                        } else
                        if(source->type == NODE_VECTOR3 && index->type == NODE_INT) {
                            if(!is_scalar) {
                                logger->log(assign, "ERROR", "vec3 component needs to be a float or an int");
                                return NULL;
                            }
                            shared_ptr<Vector3> vec3 = static_pointer_cast<Vector3>(source);
                            int i = resolve_int(index);
                            if(i == 0) vec3->x = rhs;
                            else if(i == 1) vec3->y = rhs;
                            else if(i == 2) vec3->z = rhs;
                            else logger->log(assign, "ERROR", "Index out of range for vec3 access");
                            return NULL;
                        }
                        if(source->type == NODE_VECTOR4 && index->type == NODE_INT) {
                            if(!is_scalar) {
                                logger->log(assign, "ERROR", "vec4 component needs to be a float or an int");
                                return NULL;
                            }
                            shared_ptr<Vector4> vec4 = static_pointer_cast<Vector4>(source);
                            int i = resolve_int(index);
                            if(i == 0) vec4->x = rhs;
                            else if(i == 1) vec4->y = rhs;
                            else if(i == 2) vec4->z = rhs;
                            else if(i == 4) vec4->w = rhs;
                            else logger->log(assign, "ERROR", "Index out of range for vec4 access");
                            return NULL;
                        }

                        if(source->type == NODE_MATRIX2 && index->type == NODE_INT) {
                            if(rhs->type != NODE_VECTOR2) {
                                logger->log(assign, "ERROR", "mat2 component needs to be vec2");
                                return NULL;
                            }
                            shared_ptr<Matrix2> mat2 = static_pointer_cast<Matrix2>(source);
                            int i = resolve_int(index);
                            if(i == 0) mat2->v0 = static_pointer_cast<Vector2>(rhs);
                            else if(i == 1) mat2->v1 = static_pointer_cast<Vector2>(rhs);
                            else { logger->log(assign, "ERROR", "Index out of range for mat2 access"); return NULL; }
                            mat2->generate_columns();
                            return NULL;
                        }
                        if(source->type == NODE_MATRIX3 && index->type == NODE_INT) {
                            if(rhs->type != NODE_VECTOR3) {
                                logger->log(assign, "ERROR", "mat3 component needs to be vec3");
                                return NULL;
                            }
                            shared_ptr<Matrix3> mat3 = static_pointer_cast<Matrix3>(source);
                            int i = resolve_int(index);
                            if(i == 0) mat3->v0 = static_pointer_cast<Vector3>(rhs);
                            else if(i == 1) mat3->v1 = static_pointer_cast<Vector3>(rhs);
                            else if(i == 2) mat3->v2 = static_pointer_cast<Vector3>(rhs);
                            else { logger->log(index, "ERROR", "Index out of range for mat3 access"); return NULL; }
                            mat3->generate_columns();
                            return NULL;
                        }
                        if(source->type == NODE_MATRIX4 && index->type == NODE_INT) {
                            if(rhs->type != NODE_VECTOR4) {
                                logger->log(assign, "ERROR", "mat4 component needs to be vec4");
                                return NULL;
                            }
                            shared_ptr<Matrix4> mat4 = static_pointer_cast<Matrix4>(source);
                            int i = resolve_int(index);
                            if(i == 0) mat4->v0 =static_pointer_cast<Vector4>( rhs);
                            else if(i == 1) mat4->v1 = static_pointer_cast<Vector4>(rhs);
                            else if(i == 2) mat4->v2 = static_pointer_cast<Vector4>(rhs);
                            else if(i == 3) mat4->v3 = static_pointer_cast<Vector4>(rhs);
                            else { logger->log(index, "ERROR", "Index out of range for mat4 access"); return NULL; }
                            mat4->generate_columns();
                            return NULL;
                        }

                        logger->log(index,"ERROR", "Invalid use of [] operator");
                        return NULL;

                    } else {
                        logger->log(assign, "ERROR", "Invalid left-hand side expression in assignment");
                        return NULL;
                    }
                } else {
                    logger->log("ERROR: Invalid assignment");
                }

                return NULL;
            }
        case NODE_ALLOC:
            {
                shared_ptr<Alloc> alloc = static_pointer_cast<Alloc>(stmt);

                if(!buffers[alloc->ident->name]) {
                    shared_ptr<Buffer> buf = make_shared<Buffer>();
                    buf->layout = make_shared<Layout>();

                    gl->glGenBuffers(1, &(buf->handle));
                    buffers[alloc->ident->name] = buf;
                } else {
                    logger->log("ERROR: Can't allocate to " + alloc->ident->name + ": buffer already exists!");
                }

                return NULL;
            }
        case NODE_UPLOAD:
            {
                shared_ptr<Upload> upload = static_pointer_cast<Upload>(stmt);

                shared_ptr<Buffer> buffer = buffers[upload->ident->name];
                if(buffer == NULL) {
                    logger->log("ERROR: Can't upload to unallocated buffer");
                    return NULL;
                }

                shared_ptr<Layout> layout = buffer->layout;

                if(layout->attributes.find(upload->attrib->name) == layout->attributes.end()) {
                    layout->attributes[upload->attrib->name] = 3;
                    layout->list.push_back(upload->attrib->name);
                }

                vector<float>* target = &(buffer->data[upload->attrib->name]);
                for(unsigned int i = 0; i < upload->list->list.size(); i++) {
                    shared_ptr<Expr> expr = eval_expr(upload->list->list[i]);
                    if(!expr) {
                        logger->log("ERROR: Can't upload illegal value into buffer");
                        break;
                    }

                    if(expr->type == NODE_VECTOR3) {
                        shared_ptr<Vector3> vec3 = static_pointer_cast<Vector3>(expr);
                        target->insert(target->end(), resolve_scalar(vec3->x));
                        target->insert(target->end(), resolve_scalar(vec3->y));
                        target->insert(target->end(), resolve_scalar(vec3->z));
                    }

                    if(expr->type == NODE_FLOAT) {
                        shared_ptr<Float> f = static_pointer_cast<Float>(expr);
                        target->insert(target->begin(), resolve_scalar(f));
                    }
                }

                buffer->sizes[upload->attrib->name] = target->size() / buffer->layout->attributes[upload->attrib->name];

                return NULL;
            }
        case NODE_DRAW:
            {
                shared_ptr<Draw> draw = static_pointer_cast<Draw>(stmt);

                if(current_program == NULL) {
                    logger->log("ERROR: Cannot bind program with name " + current_program_name);
                    return NULL;
                }

                shared_ptr<Buffer> buffer = buffers[draw->ident->name];
                if(buffer != NULL) {
                    shared_ptr<Layout> layout = buffer->layout;
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
                shared_ptr<Use> use = static_pointer_cast<Use>(stmt);
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
                shared_ptr<If> ifstmt = static_pointer_cast<If>(stmt);
                shared_ptr<Expr> condition = eval_expr(ifstmt->condition);
                if(!condition) return NULL;
                if(condition->type == NODE_BOOL) {
                    bool b = (static_pointer_cast<Bool>(condition)->value);
                    if(b) {
                        shared_ptr<Expr> returnValue = execute_stmts(ifstmt->block);
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
                shared_ptr<While> whilestmt = static_pointer_cast<While>(stmt);
                shared_ptr<Expr> condition = eval_expr(whilestmt->condition);
                if(!condition) return NULL;
                if(condition->type == NODE_BOOL) {
                    time_t start = time(nullptr);
                    while(true) {
                        condition = eval_expr(whilestmt->condition);
                        bool b = (static_pointer_cast<Bool>(condition)->value);
                        if(!b) break;

                        shared_ptr<Expr> returnValue = execute_stmts(whilestmt->block);
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
        case NODE_FOR:
            {
                shared_ptr<For> forstmt = static_pointer_cast<For>(stmt);
                shared_ptr<Ident> iterator = forstmt->iterator;
                shared_ptr<Expr> start = eval_expr(forstmt->start), end = eval_expr(forstmt->end), increment = eval_expr(forstmt->increment);
                if(start->type == NODE_INT || end->type == NODE_INT || increment->type == NODE_INT) {
                    eval_stmt(make_shared<Assign>(iterator, start));

                    time_t start = time(nullptr);
                    while(true) {
                        shared_ptr<Bool> terminate = static_pointer_cast<Bool>(eval_binary(make_shared<Binary>(iterator, OP_EQUAL, end)));
                        if(terminate->value) {
                            break;
                        }

                        shared_ptr<Expr> returnValue = execute_stmts(forstmt->block);
                        if(returnValue != NULL) {
                            return returnValue;
                        }

                        eval_stmt(make_shared<Assign>(iterator, make_shared<Binary>(iterator, OP_PLUS, increment)));

                        time_t now = time(nullptr);
                        int diff = difftime(now, start);
                        if(diff > LOOP_TIMEOUT) { break;}
                    }
                }

                return NULL;
            }
        case NODE_PRINT:
            {
                shared_ptr<Print> print = static_pointer_cast<Print>(stmt);
                shared_ptr<Expr> output = eval_expr(print->expr);
                if(output == NULL)
                    return NULL;


                logger->log(tostring(output));
                return NULL;
            }

        default: return NULL;
    }
}

shared_ptr<Expr> Prototype::Interpreter::execute_stmts(shared_ptr<Stmts> stmts) {
    shared_ptr<Expr> returnValue = NULL;
    for(unsigned int it = 0; it < stmts->list.size(); it++) { 
        shared_ptr<Stmt> stmt = stmts->list.at(it);
        if(stmt->type == NODE_RETURN) {
            shared_ptr<Return> ret = static_pointer_cast<Return>(stmt);
            returnValue = ret->value;
            break;
        }

        shared_ptr<Expr> expr = eval_stmt(stmt);
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

void Prototype::Interpreter::compile_shader(GLuint* handle, shared_ptr<ShaderSource> source) {
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

void Prototype::Interpreter::compile_program() {
    programs.clear();
    for(map<string, shared_ptr<ShaderPair>>::iterator it = shaders.begin(); it != shaders.end(); ++it) {
        shared_ptr<Program> program = make_shared<Program>();
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

void Prototype::Interpreter::execute_init() {
    if(!init || status) return;
    buffers.clear();
    globalScope->clear();

    execute_stmts(init->stmts);
}

void Prototype::Interpreter::execute_loop() {
    if(!loop || status) return;

    execute_stmts(loop->stmts);
}

void Prototype::Interpreter::parse(string code) {
    reset();

    istringstream ss(code);
    scanner.switch_streams(&ss, NULL);
    status = parser.parse();
}

void Prototype::Interpreter::prepare() {
    init = functions["init"];
    if(init == NULL) {
        logger->log("ERROR: init() function required!");
    }

    loop = functions["loop"];
    if(loop == NULL) {
        logger->log("ERROR: loop() function required!");
    }
}

