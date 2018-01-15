#include "interpreter.h"
#include <sstream>
#include <memory>

int resolve_int(Expr_ptr expr) {
    return static_pointer_cast<Int>(expr)->value;
}

float resolve_float(Expr_ptr expr) {
    return static_pointer_cast<Float>(expr)->value;
}

float resolve_scalar(Expr_ptr expr) {
    if(expr->type == NODE_INT) {
        return (float)resolve_int(expr);
    } else {
        return resolve_float(expr);
    }
}

#define resolve_vec2(v) resolve_scalar(v->x), resolve_scalar(v->y)
#define resolve_vec3(v) resolve_scalar(v->x), resolve_scalar(v->y), resolve_scalar(v->z)
#define resolve_vec4(v) resolve_scalar(v->x), resolve_scalar(v->y), resolve_scalar(v->z), resolve_scalar(v->w)
#define get_variable(dest, name) \
    if(!functionScopeStack.empty()) { \
        dest = functionScopeStack.top()->get(name); \
    } \
    if(dest == nullptr) { \
        dest = globalScope->get(name); \
    } \

#define LOOP_TIMEOUT 5

Prototype::Interpreter::Interpreter(LogWindow* logger): scanner(&line, &column), parser(scanner, logger, &line, &column, &imports, &globals, &functions, &shaders), logger(logger) {
    globalScope = make_shared<Scope>("global", logger);
    transpiler = new GLSLTranspiler();

    init_invoke = make_shared<Invoke>(make_shared<Ident>("init"), make_shared<ArgList>(nullptr));
    loop_invoke = make_shared<Invoke>(make_shared<Ident>("loop"), make_shared<ArgList>(nullptr));
}

#define clear_map(type, name) \
    for(map<string, type>::iterator it = name.begin(); it != name.end(); ++it) { \
        name.erase(it); \
    } \

void Prototype::Interpreter::reset() {
    clear_map(Program_ptr, programs);
    clear_map(ShaderPair_ptr, shaders);
    clear_map(FuncDef_ptr, functions);

    globals.clear();
    imports.clear();

    globalScope->clear();
    while(!functionScopeStack.empty()) functionScopeStack.pop();

    current_program_name = "";
    current_program = nullptr;
    init = nullptr;
    loop = nullptr;
    gl = nullptr;

    line = 1;
    column = 1;
}

string tostring(Expr_ptr expr) {
    switch(expr->type) {
        case NODE_INT:
            return to_string(resolve_int(expr));
        case NODE_FLOAT:
            {
                float f = resolve_float(expr);
                if(f == floor(f)) {
                    return to_string(int(f));
                }
                return to_string(resolve_float(expr));
            }
        case NODE_BOOL:
            return static_pointer_cast<Bool>(expr)->value? "true" : "false";
        case NODE_STRING:
            return static_pointer_cast<String>(expr)->value;
        case NODE_VECTOR2:
            {
                Vector2_ptr vec2 = static_pointer_cast<Vector2>(expr);
                return "[" + tostring(vec2->x) + ", " + tostring(vec2->y) + "]";
            }
        case NODE_VECTOR3:
            {
                Vector3_ptr vec3 = static_pointer_cast<Vector3>(expr);
                return "[" + tostring(vec3->x) + ", " + tostring(vec3->y) + ", " + tostring(vec3->z) + "]";
            }
        case NODE_VECTOR4:
            {
                Vector4_ptr vec4 = static_pointer_cast<Vector4>(expr);
                return "[" + tostring(vec4->x) + ", " + tostring(vec4->y) + ", " + tostring(vec4->z) + ", " + tostring(vec4->w) + "]";
            }
        case NODE_MATRIX2:
            {
                Matrix2_ptr mat2 = static_pointer_cast<Matrix2>(expr);
                return "|" + tostring(mat2->v0->x) + ", " + tostring(mat2->v0->y) + "|\n" +
                       "|" + tostring(mat2->v1->x) + ", " + tostring(mat2->v1->y) + "|";
            }
        case NODE_MATRIX3:
            {
                Matrix3_ptr mat3 = static_pointer_cast<Matrix3>(expr);
                return "|" + tostring(mat3->v0->x) + ", " + tostring(mat3->v0->y) + ", " + tostring(mat3->v0->z) + "|\n" +
                       "|" + tostring(mat3->v1->x) + ", " + tostring(mat3->v1->y) + ", " + tostring(mat3->v1->z) + "|\n" +
                       "|" + tostring(mat3->v2->x) + ", " + tostring(mat3->v2->y) + ", " + tostring(mat3->v2->z) + "|";
            }
        case NODE_MATRIX4:
            {
                Matrix4_ptr mat4 = static_pointer_cast<Matrix4>(expr);
                return "|" + tostring(mat4->v0->x) + ", " + tostring(mat4->v0->y) + ", " + tostring(mat4->v0->z) + ", " + tostring(mat4->v0->w) + "|\n" +
                       "|" + tostring(mat4->v1->x) + ", " + tostring(mat4->v1->y) + ", " + tostring(mat4->v1->z) + ", " + tostring(mat4->v1->w) + "|\n" +
                       "|" + tostring(mat4->v2->x) + ", " + tostring(mat4->v2->y) + ", " + tostring(mat4->v2->z) + ", " + tostring(mat4->v2->w) + "|\n" +
                       "|" + tostring(mat4->v3->x) + ", " + tostring(mat4->v3->y) + ", " + tostring(mat4->v3->z) + ", " + tostring(mat4->v3->w) + "|";
            }
        default:
            return "";
    }
}

Expr_ptr Prototype::Interpreter::eval_binary(Binary_ptr bin) {
    Expr_ptr lhs = eval_expr(bin->lhs);
    if(!lhs) return nullptr;

    OpType op = bin->op;

    Expr_ptr rhs = eval_expr(bin->rhs);
    if(!rhs) return nullptr;

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
        String_ptr str = String_ptr(left? static_pointer_cast<String>(lhs) : static_pointer_cast<String>(rhs));
        Expr_ptr other = eval_expr(left? rhs : lhs);

        return make_shared<String>(left? (str->value + tostring(other)) : (tostring(other) + str->value));
    }

    if(ltype == NODE_VECTOR2 && rtype == NODE_VECTOR2) {
        Vector2_ptr a = static_pointer_cast<Vector2>(eval_expr(lhs));
        Vector2_ptr b = static_pointer_cast<Vector2>(eval_expr(rhs));

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
        Vector2_ptr a = static_pointer_cast<Vector2>(eval_expr(lhs));
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
        Vector2_ptr b = static_pointer_cast<Vector2>(eval_expr(rhs));
        float bx = resolve_scalar(b->x), by = resolve_scalar(b->y);

        switch(op) {
            case OP_MULT: return make_shared<Vector2>(make_shared<Float>(bx*a), make_shared<Float>(by*a));
            default: break;
        }
    }

    if(ltype == NODE_VECTOR3 && rtype == NODE_VECTOR3) {
        Vector3_ptr a = static_pointer_cast<Vector3>(eval_expr(lhs));
        Vector3_ptr b = static_pointer_cast<Vector3>(eval_expr(rhs));

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
        Vector3_ptr a = static_pointer_cast<Vector3>(eval_expr(lhs));
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
        Vector3_ptr b = static_pointer_cast<Vector3>(eval_expr(rhs));
        float bx = resolve_scalar(b->x), by = resolve_scalar(b->y), bz = resolve_scalar(b->z);

        switch(op) {
            case OP_MULT: return make_shared<Vector3>(make_shared<Float>(bx*a), make_shared<Float>(by*a), make_shared<Float>(bz*a));
            default: break;
        }
    }

    if(ltype == NODE_VECTOR4 && rtype == NODE_VECTOR4) {
        Vector4_ptr a = static_pointer_cast<Vector4>(eval_expr(lhs));
        Vector4_ptr b = static_pointer_cast<Vector4>(eval_expr(rhs));

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
        Vector4_ptr a = static_pointer_cast<Vector4>(eval_expr(lhs));
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
        Vector4_ptr b = static_pointer_cast<Vector4>(eval_expr(rhs));
        float bx = resolve_scalar(b->x), by = resolve_scalar(b->y), bz = resolve_scalar(b->z), bw = resolve_scalar(b->w);

        switch(op) {
            case OP_MULT: return make_shared<Vector4>(make_shared<Float>(bx*a), make_shared<Float>(by*a), make_shared<Float>(bz*a), make_shared<Float>(bw*a));
            default: break;
        }
    }

    if(ltype == NODE_MATRIX2 && (rtype == NODE_INT || rtype == NODE_FLOAT)) {
        Matrix2_ptr a = static_pointer_cast<Matrix2>(eval_expr(lhs));

        if(op == OP_MULT || op == OP_DIV) {
            Vector2_ptr v0 = static_pointer_cast<Vector2>(eval_binary(make_shared<Binary>(a->v0, op, rhs)));
            Vector2_ptr v1 = static_pointer_cast<Vector2>(eval_binary(make_shared<Binary>(a->v1, op, rhs)));
            return make_shared<Matrix2>(v0, v1);
        }
    }

    if((ltype == NODE_INT || ltype == NODE_FLOAT) && (rtype == NODE_MATRIX2)) {
        Matrix2_ptr a = static_pointer_cast<Matrix2>(eval_expr(rhs));

        if(op == OP_MULT) {
            Vector2_ptr v0 = static_pointer_cast<Vector2>(eval_binary(make_shared<Binary>(a->v0, op, lhs)));
            Vector2_ptr v1 = static_pointer_cast<Vector2>(eval_binary(make_shared<Binary>(a->v1, op, lhs)));
            return make_shared<Matrix2>(v0, v1);
        }
    }

    if(ltype == NODE_MATRIX2 && rtype == NODE_MATRIX2) {
        Matrix2_ptr a = static_pointer_cast<Matrix2>(eval_expr(lhs));
        Matrix2_ptr b = static_pointer_cast<Matrix2>(eval_expr(rhs));

        if(op == OP_MULT) {
            Vector2_ptr r0 = make_shared<Vector2>(make_shared<Binary>(a->v0, OP_MULT, b->c0), make_shared<Binary>(a->v0, OP_MULT, b->c1));
            Vector2_ptr r1 = make_shared<Vector2>(make_shared<Binary>(a->v1, OP_MULT, b->c0), make_shared<Binary>(a->v1, OP_MULT, b->c1));
            return eval_expr(make_shared<Matrix2>(r0, r1));
        }
    }

    if(ltype == NODE_VECTOR2 && rtype == NODE_MATRIX2) {
        Vector2_ptr a = static_pointer_cast<Vector2>(eval_expr(lhs));
        Matrix2_ptr b = static_pointer_cast<Matrix2>(eval_expr(rhs));

        if(op == OP_MULT) {
            return eval_expr(make_shared<Vector2>(make_shared<Binary>(a, OP_MULT, b->c0), make_shared<Binary>(a, OP_MULT, b->c1)));
        }
    }

    if(ltype == NODE_MATRIX3 && (rtype == NODE_INT || rtype == NODE_FLOAT)) {
        Matrix3_ptr a = static_pointer_cast<Matrix3>(eval_expr(lhs));

        if(op == OP_MULT || op == OP_DIV) {
            Vector3_ptr v0 = static_pointer_cast<Vector3>(eval_binary(make_shared<Binary>(a->v0, op, rhs)));
            Vector3_ptr v1 = static_pointer_cast<Vector3>(eval_binary(make_shared<Binary>(a->v1, op, rhs)));
            Vector3_ptr v2 = static_pointer_cast<Vector3>(eval_binary(make_shared<Binary>(a->v2, op, rhs)));
            return make_shared<Matrix3>(v0, v1, v2);
        }
    }

    if((ltype == NODE_INT || ltype == NODE_FLOAT) && (rtype == NODE_MATRIX3)) {
        Matrix3_ptr a = static_pointer_cast<Matrix3>(eval_expr(rhs));

        if(op == OP_MULT) {
            Vector3_ptr v0 = static_pointer_cast<Vector3>(eval_binary(make_shared<Binary>(a->v0, op, lhs)));
            Vector3_ptr v1 = static_pointer_cast<Vector3>(eval_binary(make_shared<Binary>(a->v1, op, lhs)));
            Vector3_ptr v2 = static_pointer_cast<Vector3>(eval_binary(make_shared<Binary>(a->v2, op, lhs)));
            return make_shared<Matrix3>(v0, v1, v2);
        }
    }

    if(ltype == NODE_MATRIX3 && rtype == NODE_MATRIX3) {
        Matrix3_ptr a = static_pointer_cast<Matrix3>(eval_expr(lhs));
        Matrix3_ptr b = static_pointer_cast<Matrix3>(eval_expr(rhs));
        
        if(op == OP_MULT) {
            Vector3_ptr r0 = make_shared<Vector3>(make_shared<Binary>(a->v0, OP_MULT, b->c0), make_shared<Binary>(a->v0, OP_MULT, b->c1), make_shared<Binary>(a->v0, OP_MULT, b->c2));
            Vector3_ptr r1 = make_shared<Vector3>(make_shared<Binary>(a->v1, OP_MULT, b->c0), make_shared<Binary>(a->v1, OP_MULT, b->c1), make_shared<Binary>(a->v1, OP_MULT, b->c2));
            Vector3_ptr r2 = make_shared<Vector3>(make_shared<Binary>(a->v2, OP_MULT, b->c0), make_shared<Binary>(a->v2, OP_MULT, b->c1), make_shared<Binary>(a->v2, OP_MULT, b->c2));
            return eval_expr(make_shared<Matrix3>(r0, r1, r2));
        }
    }

    if(ltype == NODE_VECTOR3 && rtype == NODE_MATRIX3) {
        Vector3_ptr a = static_pointer_cast<Vector3>(eval_expr(lhs));
        Matrix3_ptr b = static_pointer_cast<Matrix3>(eval_expr(rhs));

        if(op == OP_MULT) {
            return eval_expr(make_shared<Vector3>(make_shared<Binary>(a, OP_MULT, b->c0), make_shared<Binary>(a, OP_MULT, b->c1), make_shared<Binary>(a, OP_MULT, b->c2)));
        }
    }

    if(ltype == NODE_MATRIX4 && (rtype == NODE_INT || rtype == NODE_FLOAT)) {
        Matrix4_ptr a = static_pointer_cast<Matrix4>(eval_expr(lhs));

        if(op == OP_MULT || op == OP_DIV) {
            Vector4_ptr v0 = static_pointer_cast<Vector4>(eval_binary(make_shared<Binary>(a->v0, op, rhs)));
            Vector4_ptr v1 = static_pointer_cast<Vector4>(eval_binary(make_shared<Binary>(a->v1, op, rhs)));
            Vector4_ptr v2 = static_pointer_cast<Vector4>(eval_binary(make_shared<Binary>(a->v2, op, rhs)));
            Vector4_ptr v3 = static_pointer_cast<Vector4>(eval_binary(make_shared<Binary>(a->v3, op, rhs)));
            return make_shared<Matrix4>(v0, v1, v2, v3);
        }
    }

    if((ltype == NODE_INT || ltype == NODE_FLOAT) && (rtype == NODE_MATRIX4)) {
        Matrix4_ptr a = static_pointer_cast<Matrix4>(eval_expr(rhs));

        if(op == OP_MULT) {
            Vector4_ptr v0 = static_pointer_cast<Vector4>(eval_binary(make_shared<Binary>(a->v0, op, lhs)));
            Vector4_ptr v1 = static_pointer_cast<Vector4>(eval_binary(make_shared<Binary>(a->v1, op, lhs)));
            Vector4_ptr v2 = static_pointer_cast<Vector4>(eval_binary(make_shared<Binary>(a->v2, op, lhs)));
            Vector4_ptr v3 = static_pointer_cast<Vector4>(eval_binary(make_shared<Binary>(a->v3, op, lhs)));
            return make_shared<Matrix4>(v0, v1, v2, v3);
        }
    }

    if(ltype == NODE_MATRIX4 && rtype == NODE_MATRIX4) {
        Matrix4_ptr a = static_pointer_cast<Matrix4>(eval_expr(lhs));
        Matrix4_ptr b = static_pointer_cast<Matrix4>(eval_expr(rhs));
        
        if(op == OP_MULT) {
            Vector4_ptr r0 = make_shared<Vector4>(make_shared<Binary>(a->v0, OP_MULT, b->c0), make_shared<Binary>(a->v0, OP_MULT, b->c1), make_shared<Binary>(a->v0, OP_MULT, b->c2), make_shared<Binary>(a->v0, OP_MULT, b->c3));
            Vector4_ptr r1 = make_shared<Vector4>(make_shared<Binary>(a->v1, OP_MULT, b->c0), make_shared<Binary>(a->v1, OP_MULT, b->c1), make_shared<Binary>(a->v1, OP_MULT, b->c2), make_shared<Binary>(a->v1, OP_MULT, b->c3));
            Vector4_ptr r2 = make_shared<Vector4>(make_shared<Binary>(a->v2, OP_MULT, b->c0), make_shared<Binary>(a->v2, OP_MULT, b->c1), make_shared<Binary>(a->v2, OP_MULT, b->c2), make_shared<Binary>(a->v2, OP_MULT, b->c3));
            Vector4_ptr r3 = make_shared<Vector4>(make_shared<Binary>(a->v3, OP_MULT, b->c0), make_shared<Binary>(a->v3, OP_MULT, b->c1), make_shared<Binary>(a->v3, OP_MULT, b->c2), make_shared<Binary>(a->v3, OP_MULT, b->c3));
            return eval_expr(make_shared<Matrix4>(r0, r1, r2, r3));
        }
    }

    if(ltype == NODE_VECTOR4 && rtype == NODE_MATRIX4) {
        Vector4_ptr a = static_pointer_cast<Vector4>(eval_expr(lhs));
        Matrix4_ptr b = static_pointer_cast<Matrix4>(eval_expr(rhs));

        if(op == OP_MULT) {
            return eval_expr(make_shared<Vector4>(make_shared<Binary>(a, OP_MULT, b->c0), make_shared<Binary>(a, OP_MULT, b->c1), make_shared<Binary>(a, OP_MULT, b->c2), make_shared<Binary>(a, OP_MULT, b->c3)));
        }
    }

    logger->log(bin, "ERROR", "Invalid operation between " + type_to_name(ltype) + " and " + type_to_name(rtype));
    return nullptr;
}

Expr_ptr Prototype::Interpreter::invoke(Invoke_ptr invoke) {
    string name = invoke->ident->name;
    FuncDef_ptr def = nullptr;

    if(name == "cos") {
        if(invoke->args->list.size() == 1) {
            Expr_ptr v = eval_expr(invoke->args->list[0]);
            if(v->type == NODE_FLOAT || v->type == NODE_INT) {
                return make_shared<Float>(cosf(resolve_scalar(v)));
            }
        }
    }

    if(name == "sin") {
        if(invoke->args->list.size() == 1) {
            Expr_ptr v = eval_expr(invoke->args->list[0]);
            if(v->type == NODE_FLOAT || v->type == NODE_INT) {
                return make_shared<Float>(sinf(resolve_scalar(v)));
            }
        }
    }

    if(name == "tan") {
        if(invoke->args->list.size() == 1) {
            Expr_ptr v = eval_expr(invoke->args->list[0]);
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

    if(functions.find(name) != functions.end()) {
        def = functions[name];
    }

    if(def != nullptr) {
        ScopeList_ptr localScope = make_shared<ScopeList>(name, logger);
        unsigned int nParams = def->params->list.size();
        unsigned int nArgs = invoke->args->list.size();

        if(nParams != nArgs) {
            logger->log(invoke, "ERROR", "Function " + name + " expects " + to_string(nParams) + " arguments, got " + to_string(nArgs));
            return nullptr;
        }
        if(nParams > 0) {
            for(unsigned int i = 0; i < nParams; i++) {
                Expr_ptr arg = eval_expr(invoke->args->list[i]);
                if(arg == nullptr) {
                    logger->log(invoke->args->list[i], "ERROR", "Invalid argument passed on to " + name);
                    return nullptr;
                }

                Decl_ptr param = def->params->list[i];
                localScope->current()->declare(param->name->name, param->datatype->name, arg);
            }
        }

        functionScopeStack.push(localScope);
        Expr_ptr retValue = execute_stmts(def->stmts);
        functionScopeStack.pop();
        return retValue;
    } else {
        logger->log(invoke, "ERROR", "Call to undefined function " + name);
    }
    return nullptr;
}

Expr_ptr Prototype::Interpreter::resolve_vector(vector<Expr_ptr> list) {
    vector<float> data;
    int n = 0;

    for(unsigned int i = 0; i < list.size(); i++) {
        Expr_ptr expr = list[i];
        if(expr->type == NODE_FLOAT || expr->type == NODE_INT) {
            data.push_back(resolve_scalar(expr));
            n += 1;
        } else
        if(expr->type == NODE_VECTOR2) {
            Vector2_ptr vec2 = static_pointer_cast<Vector2>(eval_expr(expr));
            data.push_back(resolve_scalar(vec2->x));
            data.push_back(resolve_scalar(vec2->y));
            n += 2;
        } else
        if(expr->type == NODE_VECTOR3) {
            Vector3_ptr vec3 = static_pointer_cast<Vector3>(eval_expr(expr));
            data.push_back(resolve_scalar(vec3->x));
            data.push_back(resolve_scalar(vec3->y));
            data.push_back(resolve_scalar(vec3->z));
            n += 3;
        } else
        if(expr->type == NODE_VECTOR4) {
            Vector4_ptr vec4 = static_pointer_cast<Vector4>(eval_expr(expr));
            data.push_back(resolve_scalar(vec4->x));
            data.push_back(resolve_scalar(vec4->y));
            data.push_back(resolve_scalar(vec4->z));
            data.push_back(resolve_scalar(vec4->w));
            n += 4;
        } else {
            return nullptr;
        }
    }

    if(data.size() == 3) {
        return make_shared<Vector3>(make_shared<Float>(data[0]), make_shared<Float>(data[1]), make_shared<Float>(data[2]));
    }
    if(data.size() == 4) {
        return make_shared<Vector4>(make_shared<Float>(data[0]), make_shared<Float>(data[1]), make_shared<Float>(data[2]), make_shared<Float>(data[3]));
    }

    return nullptr;
}

Expr_ptr Prototype::Interpreter::eval_expr(Expr_ptr node) {
    switch(node->type) {
        case NODE_IDENT: 
            {
                Ident_ptr ident = static_pointer_cast<Ident>(node);
                Expr_ptr value = nullptr;
                get_variable(value, ident->name);
                if(value == nullptr) {
                    logger->log(ident, "ERROR", "Undefined variable " + ident->name);
                }

                return value;
            }
       case NODE_DOT:
            {
                Dot_ptr uniform = static_pointer_cast<Dot>(node);
                bool reupload = false;
                if(current_program_name != uniform->shader) {
                    current_program_name = uniform->shader;
                    current_program = programs[current_program_name];
                    reupload = true;
                }

                if(current_program == nullptr) {
                    logger->log(uniform, "ERROR", "Cannot get uniform from nonexistent shader");
                    return nullptr;
                }

                if(reupload) {
                    gl->glUseProgram(current_program->handle);
                }

                Shader_ptr src = current_program->vertSource;
                string type = "";
                if(src->uniforms->find(uniform->name) != src->uniforms->end()) {
                    type = src->uniforms->at(uniform->name);
                } else {
                    src = current_program->fragSource;
                    if(src->uniforms->find(uniform->name) != src->uniforms->end()) {
                        type = src->uniforms->at(uniform->name);
                    } else {
                        logger->log(uniform, "ERROR", "Uniform " + uniform->name + " of shader " + current_program_name + " does not exist!");
                        return nullptr;
                    }
                }

                GLint loc = gl->glGetUniformLocation(current_program->handle, uniform->name.c_str());
                if(type == "float") {
                    Float_ptr f = make_shared<Float>(0);
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

        case NODE_BOOL:
            return node;

        case NODE_INT:
            return node;

        case NODE_FLOAT:
            return node;

        case NODE_STRING:
            return node;

        case NODE_LIST:
            return node;

        case NODE_BUFFER:
            return node;

        case NODE_TEXTURE:
            return node;

        case NODE_VECTOR2:
            {
                Vector2_ptr vec2 = static_pointer_cast<Vector2>(node);
                Expr_ptr x = eval_expr(vec2->x);
                Expr_ptr y = eval_expr(vec2->y);

                if(x == nullptr) {
                    logger->log(vec2->x, "ERROR", "Invalid component of index 0");
                    return nullptr;
                }
                if(y == nullptr) {
                    logger->log(vec2->y, "ERROR", "Invalid component of index 1");
                    return nullptr;
                }

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
                Vector3_ptr vec3 = static_pointer_cast<Vector3>((node));
                Expr_ptr x = eval_expr(vec3->x);
                Expr_ptr y = eval_expr(vec3->y);
                Expr_ptr z = eval_expr(vec3->z);

                if(x == nullptr) {
                    logger->log(vec3->x, "ERROR", "Invalid component of index 0");
                    return nullptr;
                }
                if(y == nullptr) {
                    logger->log(vec3->y, "ERROR", "Invalid component of index 1");
                    return nullptr;
                }
                if(z == nullptr) {
                    logger->log(vec3->z, "ERROR", "Invalid component of index 2");
                    return nullptr;
                }
                
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
                Vector4_ptr vec4 = static_pointer_cast<Vector4>((node));
                Expr_ptr x = eval_expr(vec4->x);
                Expr_ptr y = eval_expr(vec4->y);
                Expr_ptr z = eval_expr(vec4->z);
                Expr_ptr w = eval_expr(vec4->w);
                
                if(x == nullptr) {
                    logger->log(vec4->x, "ERROR", "Invalid component of index 0");
                    return nullptr;
                }
                if(y == nullptr) {
                    logger->log(vec4->y, "ERROR", "Invalid component of index 1");
                    return nullptr;
                }
                if(z == nullptr) {
                    logger->log(vec4->z, "ERROR", "Invalid component of index 2");
                    return nullptr;
                }
                if(w == nullptr) {
                    logger->log(vec4->w, "ERROR", "Invalid component of index 3");
                    return nullptr;
                }
 
                if((x->type == NODE_INT || x->type == NODE_FLOAT) && (y->type == NODE_INT || y->type == NODE_FLOAT) && (z->type == NODE_INT || z->type == NODE_FLOAT) && (w->type == NODE_INT || w->type == NODE_FLOAT)) {
                    return make_shared<Vector4>(x, y, z, w);
                }

                if(x->type == NODE_VECTOR4 && y->type == NODE_VECTOR4 && z->type == NODE_VECTOR4 && w->type == NODE_VECTOR4) {
                    return make_shared<Matrix4>(static_pointer_cast<Vector4>(x), static_pointer_cast<Vector4>(y), static_pointer_cast<Vector4>(z), static_pointer_cast<Vector4>(w));
                }

                return nullptr;
            }

        case NODE_MATRIX2:
            {
                Matrix2_ptr mat2 = static_pointer_cast<Matrix2>(node);
                Expr_ptr v0 = eval_expr(mat2->v0);
                Expr_ptr v1 = eval_expr(mat2->v1);

                if(v0 == nullptr) {
                    logger->log(mat2->v0, "ERROR", "Invalid component of index 0");
                    return nullptr;
                }
                if(v1 == nullptr) {
                    logger->log(mat2->v1, "ERROR", "Invalid component of index 1");
                    return nullptr;
                }

                mat2->v0 = static_pointer_cast<Vector2>(v0);
                mat2->v1 = static_pointer_cast<Vector2>(v1);
                return mat2;
            }

        case NODE_MATRIX3:
            {
                Matrix3_ptr mat3 = static_pointer_cast<Matrix3>(node);
                Expr_ptr v0 = eval_expr(mat3->v0);
                Expr_ptr v1 = eval_expr(mat3->v1);
                Expr_ptr v2 = eval_expr(mat3->v2);

                if(v0 == nullptr) {
                    logger->log(mat3->v0, "ERROR", "Invalid component of index 0");
                    return nullptr;
                }
                if(v1 == nullptr) {
                    logger->log(mat3->v1, "ERROR", "Invalid component of index 1");
                    return nullptr;
                }
                if(v2 == nullptr) {
                    logger->log(mat3->v2, "ERROR", "Invalid component of index 2");
                    return nullptr;
                }

                mat3->v0 = static_pointer_cast<Vector3>(v0);
                mat3->v1 = static_pointer_cast<Vector3>(v1);
                mat3->v2 = static_pointer_cast<Vector3>(v2);
                return mat3;
            }

        case NODE_MATRIX4:
            {
                Matrix4_ptr mat4 = static_pointer_cast<Matrix4>(node);
                Expr_ptr v0 = eval_expr(mat4->v0);
                Expr_ptr v1 = eval_expr(mat4->v1);
                Expr_ptr v2 = eval_expr(mat4->v2);
                Expr_ptr v3 = eval_expr(mat4->v3);

                if(v0 == nullptr) {
                    logger->log(mat4->v0, "ERROR", "Invalid component of index 0");
                    return nullptr;
                }
                if(v1 == nullptr) {
                    logger->log(mat4->v1, "ERROR", "Invalid component of index 1");
                    return nullptr;
                }
                if(v2 == nullptr) {
                    logger->log(mat4->v2, "ERROR", "Invalid component of index 2");
                    return nullptr;
                }
                if(v3 == nullptr) {
                    logger->log(mat4->v3, "ERROR", "Invalid component of index 2");
                    return nullptr;
                }

                mat4->v0 = static_pointer_cast<Vector4>(v0);
                mat4->v1 = static_pointer_cast<Vector4>(v1);
                mat4->v2 = static_pointer_cast<Vector4>(v2);
                mat4->v3 = static_pointer_cast<Vector4>(v3);
                return mat4;
            }

        case NODE_UNARY:
            {
                Unary_ptr un = static_pointer_cast<Unary>(node);
                Expr_ptr rhs = eval_expr(un->rhs);

                if(!rhs) return nullptr;

                if(un->op == OP_MINUS) {
                    if(rhs->type == NODE_INT) {
                        Int_ptr i = static_pointer_cast<Int>(rhs);
                        return make_shared<Int>(-(i->value));
                    }
                    if(rhs->type == NODE_FLOAT) {
                        Float_ptr fl = static_pointer_cast<Float>(rhs);
                        return make_shared<Float>(-(fl->value));
                    }
                    if(rhs->type == NODE_VECTOR3) {
                        Vector3_ptr vec3 = static_pointer_cast<Vector3>(rhs);

                        //SHAMEFUL HACK
                        return eval_binary(make_shared<Binary>(vec3, OP_MULT, make_shared<Float>(-1)));
                    }
                }
                if(un->op == OP_NOT) {
                    if(rhs->type == NODE_BOOL) {
                        Bool_ptr b = static_pointer_cast<Bool>(rhs);
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
                        Vector2_ptr vec2 = static_pointer_cast<Vector2>(rhs);
                        float x = resolve_scalar(vec2->x), y = resolve_scalar(vec2->y);
                        return make_shared<Float>(sqrtf(x * x + y * y));
                    }
                    if(rhs->type == NODE_VECTOR3) {
                        Vector3_ptr vec3 = static_pointer_cast<Vector3>(rhs);
                        float x = resolve_scalar(vec3->x), y = resolve_scalar(vec3->y), z = resolve_scalar(vec3->z);
                        return make_shared<Float>(sqrtf(x * x + y * y + z * z));
                    }
                    if(rhs->type == NODE_VECTOR4) {
                        Vector4_ptr vec4 = static_pointer_cast<Vector4>(rhs);
                        float x = resolve_scalar(vec4->x), y = resolve_scalar(vec4->y), z = resolve_scalar(vec4->z), w = resolve_scalar(vec4->w);
                        return make_shared<Float>(sqrtf(x * x + y * y + z * z + w * w));
                    }
                    if(rhs->type == NODE_MATRIX2) {
                        Matrix2_ptr mat2 = static_pointer_cast<Matrix2>(rhs);
                        return make_shared<Float>(resolve_scalar(mat2->v0->x) * resolve_scalar(mat2->v1->y) - resolve_scalar(mat2->v0->y) * resolve_scalar(mat2->v1->x));
                    }
                    #define mat3_det(a,b,c,d,e,f,g,h,i) (a * (e * i - f * h)) - (b * (d * i - f * g)) + (c * (d * h - e * g))
                    if(rhs->type == NODE_MATRIX3) {
                        Matrix3_ptr mat3 = static_pointer_cast<Matrix3>(rhs);
                        float a = resolve_scalar(mat3->v0->x), b = resolve_scalar(mat3->v0->y), c = resolve_scalar(mat3->v0->z);
                        float d = resolve_scalar(mat3->v1->x), e = resolve_scalar(mat3->v1->y), f = resolve_scalar(mat3->v1->z);
                        float g = resolve_scalar(mat3->v2->x), h = resolve_scalar(mat3->v2->y), i = resolve_scalar(mat3->v2->z);
                        return make_shared<Float>(mat3_det(a, b, c, d, e, f, g, h, i));
                    }
                    if(rhs->type == NODE_MATRIX4) {
                        Matrix4_ptr mat4 = static_pointer_cast<Matrix4>(rhs);
                        float a = resolve_scalar(mat4->v0->x), b = resolve_scalar(mat4->v0->y), c = resolve_scalar(mat4->v0->z), d = resolve_scalar(mat4->v0->w);
                        float e = resolve_scalar(mat4->v1->x), f = resolve_scalar(mat4->v1->y), g = resolve_scalar(mat4->v1->z), h = resolve_scalar(mat4->v1->w);
                        float i = resolve_scalar(mat4->v2->x), j = resolve_scalar(mat4->v2->y), k = resolve_scalar(mat4->v2->z), l = resolve_scalar(mat4->v2->w);
                        float m = resolve_scalar(mat4->v3->x), n = resolve_scalar(mat4->v3->y), o = resolve_scalar(mat4->v3->z), p = resolve_scalar(mat4->v3->w);

                        return make_shared<Float>((a * mat3_det(f, g, h, j, k, l, n, o, p))
                                                - (b * mat3_det(e, g, h, i, k, l, m, o, p))
                                                + (c * mat3_det(e, f, h, i, j, l, m, n, p))
                                                - (d * mat3_det(e, f, g, i, j, k, m, n, o)));
                    }
                    if(rhs->type == NODE_LIST) {
                        List_ptr list = static_pointer_cast<List>(rhs);
                        return make_shared<Int>(list->list.size());
                    }
                    return nullptr;
                }
            }

        case NODE_BINARY:
            {
                Binary_ptr bin = static_pointer_cast<Binary>((node));
                return eval_binary(bin);
            }

        case NODE_FUNCEXPR:
            {
                FuncExpr_ptr func = static_pointer_cast<FuncExpr>(node);
                return invoke(func->invoke);
            }
        case NODE_INDEX:
            {
                Index_ptr in= static_pointer_cast<Index>(node);
                Expr_ptr source = eval_expr(in->source);
                Expr_ptr index = eval_expr(in->index);
                
                if(source->type == NODE_VECTOR2 && index->type == NODE_INT) {
                    Vector2_ptr vec2 = static_pointer_cast<Vector2>(source);
                    int i = resolve_int(index);
                    if(i == 0) return eval_expr(vec2->x);
                    if(i == 1) return eval_expr(vec2->y);
                        
                    logger->log(index, "ERROR", "Index out of range for vec2 access");
                    return nullptr;
                } else
                if(source->type == NODE_VECTOR3 && index->type == NODE_INT) {
                    Vector3_ptr vec3 = static_pointer_cast<Vector3>(source);
                    int i = resolve_int(index);
                    if(i == 0) return eval_expr(vec3->x);
                    if(i == 1) return eval_expr(vec3->y);
                    if(i == 2) return eval_expr(vec3->z);
                        
                    logger->log(index, "ERROR", "Index out of range for vec3 access");
                    return nullptr;
                }
                if(source->type == NODE_VECTOR4 && index->type == NODE_INT) {
                    Vector4_ptr vec4 = static_pointer_cast<Vector4>(source);
                    int i = resolve_int(index);
                    if(i == 0) return eval_expr(vec4->x);
                    if(i == 1) return eval_expr(vec4->y);
                    if(i == 2) return eval_expr(vec4->z);
                    if(i == 3) return eval_expr(vec4->w);
                        
                    logger->log(index, "ERROR", "Index out of range for vec4 access");
                    return nullptr;
                }
                if(source->type == NODE_MATRIX2 && index->type == NODE_INT) {
                    Matrix2_ptr mat2 = static_pointer_cast<Matrix2>(source);
                    int i = resolve_int(index);
                    if(i == 0) return mat2->v0;
                    if(i == 1) return mat2->v1;
                    
                    logger->log(index, "ERROR", "Index out of range for mat2 access");
                    return nullptr;
                }
                if(source->type == NODE_MATRIX3 && index->type == NODE_INT) {
                    Matrix3_ptr mat3 = static_pointer_cast<Matrix3>(source);
                    int i = resolve_int(index);
                    if(i == 0) return mat3->v0;
                    if(i == 1) return mat3->v1;
                    if(i == 2) return mat3->v2;

                    logger->log(index, "ERROR", "Index out of range for mat3 access");
                    return nullptr;
                }
                if(source->type == NODE_MATRIX4 && index->type == NODE_INT) {
                    Matrix4_ptr mat4 = static_pointer_cast<Matrix4>(source);
                    int i = resolve_int(index);
                    if(i == 0) return mat4->v0;
                    if(i == 1) return mat4->v1;
                    if(i == 2) return mat4->v2;
                    if(i == 3) return mat4->v3;

                    logger->log(index, "ERROR", "Index out of range for mat4 access");
                    return nullptr;
                }
                if(source->type == NODE_LIST && index->type == NODE_INT) {
                    List_ptr list = static_pointer_cast<List>(source);
                    int i = resolve_int(index);
                    int size = list->list.size();
                    if(i >= 0 && i < size ) {
                        return eval_expr(list->list[i]);
                    } else {
                        logger->log(index, "ERROR", "Index out of range for list of length " + list->list.size());
                        return nullptr;
                    }
                }

                logger->log(index,"ERROR", "Invalid use of [] operator");
                return nullptr;
            }

        default: logger->log(node, "ERROR", "Illegal expression"); return nullptr;
    }
}

Expr_ptr Prototype::Interpreter::eval_stmt(Stmt_ptr stmt) {
    switch(stmt->type) {
        case NODE_FUNCSTMT:
            {
                FuncStmt_ptr func = static_pointer_cast<FuncStmt>(stmt);
                invoke(func->invoke);
                return nullptr;
            }
        case NODE_DECL:
            {
                Decl_ptr decl = static_pointer_cast<Decl>(stmt);
                Scope_ptr scope = globalScope;
                if(!functionScopeStack.empty()) {
                    scope = functionScopeStack.top()->current();
                }

                if(decl->datatype->name == "buffer") {
                    Buffer_ptr buf = make_shared<Buffer>();
                    buf->layout = make_shared<Layout>();

                    gl->glGenBuffers(1, &(buf->handle));
                    gl->glGenBuffers(1, &(buf->indexHandle));

                    decl->value = buf;
                }

                scope->declare(decl->name->name, decl->datatype->name, decl->value == nullptr? null_expr : eval_expr(decl->value));
                return nullptr;
            }
        case NODE_ASSIGN:
            {
                Assign_ptr assign = static_pointer_cast<Assign>(stmt);
                Expr_ptr rhs = eval_expr(assign->value);
                if(rhs != nullptr) {
                    Expr_ptr lhs = assign->lhs;
                    if(lhs->type == NODE_IDENT) {
                        string name = static_pointer_cast<Ident>(lhs)->name;
                        if(!functionScopeStack.empty() && functionScopeStack.top()->assign(name, rhs)) {
                            return nullptr;
                        }
                        if(globalScope->assign(static_pointer_cast<Ident>(lhs)->name, rhs)) {
                            return nullptr;
                        }
                        logger->log(lhs, "ERROR", "Variable " + name + " does not exist!");

                        return nullptr;
                    } else if(lhs->type == NODE_DOT) {
                        Dot_ptr uniform = static_pointer_cast<Dot>(lhs);
                        bool reupload = false;
                        if(current_program_name != uniform->shader) {
                            current_program_name = uniform->shader;
                            current_program = programs[current_program_name];
                            reupload = true;
                        }

                        if(current_program == nullptr) {
                            logger->log(uniform, "ERROR", "Cannot upload to nonexistent shader");
                            return nullptr;
                        }

                        if(reupload) {
                            gl->glUseProgram(current_program->handle);
                        }

                        Shader_ptr src = current_program->vertSource;
                        string type = "";
                        if(src->uniforms->find(uniform->name) != src->uniforms->end()) {
                            type = src->uniforms->at(uniform->name);
                        } else {
                            src = current_program->fragSource;
                            if(src->uniforms->find(uniform->name) != src->uniforms->end()) {
                                type = src->uniforms->at(uniform->name);
                            } else {
                                logger->log(uniform, "ERROR", "Uniform " + uniform->name + " of shader " + current_program_name + " does not exist!");
                                return nullptr;
                            }
                        }

                        GLint loc = gl->glGetUniformLocation(current_program->handle, uniform->name.c_str());
                        if(type == "float") {
                            if(rhs->type == NODE_FLOAT) {
                                Float_ptr f = static_pointer_cast<Float>(rhs);
                                gl->glUniform1f(loc, resolve_float(f));
                            } else {
                                logger->log(uniform, "ERROR", "Uniform upload mismatch: float required for " + uniform->name + " of shader " + current_program_name);
                                return nullptr;
                            }
                        } else
                        if(type == "vec2") {
                            if(rhs->type == NODE_VECTOR2) {
                                Vector2_ptr vec2 = static_pointer_cast<Vector2>(eval_expr(rhs));
                                gl->glUniform2f(loc, resolve_vec2(vec2));
                            } else {
                                logger->log(uniform, "ERROR", "Uniform upload mismatch: vec2 required for " + uniform->name + " of shader " + current_program_name);
                                return nullptr;
                            }
                        } else 
                        if(type == "vec3") {
                            if(rhs->type == NODE_VECTOR3) {
                                Vector3_ptr vec3 = static_pointer_cast<Vector3>(eval_expr(rhs));
                                gl->glUniform3f(loc, resolve_vec3(vec3));
                            } else {
                                logger->log(uniform, "ERROR", "Uniform upload mismatch: vec3 required for " + uniform->name + " of shader " + current_program_name);
                                return nullptr;
                            }
                        } else
                        if(type == "vec4") {
                            if(rhs->type == NODE_VECTOR4) {
                                Vector4_ptr vec4 = static_pointer_cast<Vector4>(eval_expr(rhs));
                                gl->glUniform4f(loc, resolve_vec4(vec4));
                            } else {
                                logger->log(uniform, "ERROR", "Uniform upload mismatch: vec4 required for " + uniform->name + " of shader " + current_program_name);
                                return nullptr;
                            }
                        } else
                        if(type == "mat2") {
                            if(rhs->type == NODE_MATRIX2) {
                                Matrix2_ptr mat2 = static_pointer_cast<Matrix2>(eval_expr(rhs));
                                float data[4];
                                data[0] = resolve_scalar(mat2->v0->x); data[1] = resolve_scalar(mat2->v0->y);
                                data[2] = resolve_scalar(mat2->v1->x); data[3] = resolve_scalar(mat2->v1->y);
                                gl->glUniform2fv(loc, 1, data);
                            } else {
                                logger->log(uniform, "ERROR", "Uniform upload mismatch: vec4 required for " + uniform->name + " of shader " + current_program_name);
                                return nullptr;
                            }
                        } else
                        if(type == "mat3") {
                            if(rhs->type == NODE_MATRIX3) {
                                Matrix3_ptr mat3 = static_pointer_cast<Matrix3>(eval_expr(rhs));
                                float data[9];
                                data[0] = resolve_scalar(mat3->v0->x); data[1] = resolve_scalar(mat3->v0->y); data[2] = resolve_scalar(mat3->v0->z);
                                data[3] = resolve_scalar(mat3->v1->x); data[4] = resolve_scalar(mat3->v1->y); data[5] = resolve_scalar(mat3->v1->z);
                                data[6] = resolve_scalar(mat3->v2->x); data[7] = resolve_scalar(mat3->v2->y); data[8] = resolve_scalar(mat3->v2->z);
                                gl->glUniformMatrix3fv(loc, 1, false, data);
                            } else {
                                logger->log(uniform, "ERROR", "Uniform upload mismatch: mat3 required for " + uniform->name + " of shader " + current_program_name);
                                return nullptr;
                            }
                        } else
                        if(type == "mat4") {
                            if(rhs->type == NODE_MATRIX4) {
                                Matrix4_ptr mat4 = static_pointer_cast<Matrix4>(eval_expr(rhs));
                                float data[16];
                                data[0] = resolve_scalar(mat4->v0->x); data[1] = resolve_scalar(mat4->v0->y); data[2] = resolve_scalar(mat4->v0->z); data[3] = resolve_scalar(mat4->v0->w);
                                data[4] = resolve_scalar(mat4->v1->x); data[5] = resolve_scalar(mat4->v1->y); data[6] = resolve_scalar(mat4->v1->z); data[7] = resolve_scalar(mat4->v1->w);
                                data[8] = resolve_scalar(mat4->v2->x); data[9] = resolve_scalar(mat4->v2->y); data[10] = resolve_scalar(mat4->v2->z); data[11] = resolve_scalar(mat4->v2->w);
                                data[12] = resolve_scalar(mat4->v3->x); data[13] = resolve_scalar(mat4->v3->y); data[14] = resolve_scalar(mat4->v3->z); data[15] = resolve_scalar(mat4->v3->w);
                                gl->glUniformMatrix4fv(loc, 1, false, data);
                            } else {
                                logger->log(uniform, "ERROR", "Uniform upload mismatch: mat3 required for " + uniform->name + " of shader " + current_program_name);
                                return nullptr;
                            }
                        } else
                        if(type == "texture2D") {
                            switch(rhs->type) {
                                case NODE_TEXTURE:
                                    {
                                        Texture_ptr tex = static_pointer_cast<Texture>(rhs);
                                        if(tex->handle == 0) {
                                            gl->glGenTextures(1, &(tex->handle));
                                        }
                                        gl->glBindTexture(GL_TEXTURE_2D, tex->handle);
                                        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                                        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                                        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                                        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                                        gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex->width, tex->height, 0, GL_RGB, GL_UNSIGNED_BYTE, tex->image);

                                        gl->glActiveTexture(GL_TEXTURE0);
                                        break;
                                    }
                                case NODE_STRING:
                                    {
                                        cout << "string to tex\n";
                                        // Load texture from memory (SOIL? lodePNG?), bind to texture
                                        String_ptr filename = static_pointer_cast<String>(rhs);
                                    }
                                    break;
                                default:
                                    break;
                        }
                    }
                    } else if (lhs->type == NODE_INDEX) {
                        Index_ptr in = static_pointer_cast<Index>(lhs);
                        Expr_ptr source = eval_expr(in->source);
                        Expr_ptr index = eval_expr(in->index);

                        if(source->type == NODE_LIST && index->type == NODE_INT) {
                            List_ptr list = static_pointer_cast<List>(source);
                            int i = resolve_int(index);
                            int size = list->list.size();
                            if(i >= 0 && i < size) {
                                list->list[i] = rhs;
                            } else {
                                logger->log(assign, "ERROR", "Index out of range for list of length " + list->list.size());
                            }
                            return nullptr;
                        }

                        bool is_scalar = (rhs->type == NODE_FLOAT || rhs->type == NODE_INT);
                        if(source->type == NODE_VECTOR2 && index->type == NODE_INT) {
                            if(!is_scalar) {
                                logger->log(assign, "ERROR", "vec2 component needs to be a float or an int");
                                return nullptr;
                            }
                            Vector2_ptr vec2 = static_pointer_cast<Vector2>(source);
                            int i = resolve_int(index);
                            if(i == 0) vec2->x = rhs;
                            else if(i == 1) vec2->y = rhs;
                            else logger->log(index, "ERROR", "Index out of range for vec2 access");
                            return nullptr;
                        } else
                        if(source->type == NODE_VECTOR3 && index->type == NODE_INT) {
                            if(!is_scalar) {
                                logger->log(assign, "ERROR", "vec3 component needs to be a float or an int");
                                return nullptr;
                            }
                            Vector3_ptr vec3 = static_pointer_cast<Vector3>(source);
                            int i = resolve_int(index);
                            if(i == 0) vec3->x = rhs;
                            else if(i == 1) vec3->y = rhs;
                            else if(i == 2) vec3->z = rhs;
                            else logger->log(assign, "ERROR", "Index out of range for vec3 access");
                            return nullptr;
                        }
                        if(source->type == NODE_VECTOR4 && index->type == NODE_INT) {
                            if(!is_scalar) {
                                logger->log(assign, "ERROR", "vec4 component needs to be a float or an int");
                                return nullptr;
                            }
                            Vector4_ptr vec4 = static_pointer_cast<Vector4>(source);
                            int i = resolve_int(index);
                            if(i == 0) vec4->x = rhs;
                            else if(i == 1) vec4->y = rhs;
                            else if(i == 2) vec4->z = rhs;
                            else if(i == 3) vec4->w = rhs;
                            else logger->log(assign, "ERROR", "Index out of range for vec4 access");
                            return nullptr;
                        }

                        if(source->type == NODE_MATRIX2 && index->type == NODE_INT) {
                            if(rhs->type != NODE_VECTOR2) {
                                logger->log(assign, "ERROR", "mat2 component needs to be vec2");
                                return nullptr;
                            }
                            Matrix2_ptr mat2 = static_pointer_cast<Matrix2>(source);
                            int i = resolve_int(index);
                            if(i == 0) mat2->v0 = static_pointer_cast<Vector2>(rhs);
                            else if(i == 1) mat2->v1 = static_pointer_cast<Vector2>(rhs);
                            else { logger->log(assign, "ERROR", "Index out of range for mat2 access"); return nullptr; }
                            mat2->generate_columns();
                            return nullptr;
                        }
                        if(source->type == NODE_MATRIX3 && index->type == NODE_INT) {
                            if(rhs->type != NODE_VECTOR3) {
                                logger->log(assign, "ERROR", "mat3 component needs to be vec3");
                                return nullptr;
                            }
                            Matrix3_ptr mat3 = static_pointer_cast<Matrix3>(source);
                            int i = resolve_int(index);
                            if(i == 0) mat3->v0 = static_pointer_cast<Vector3>(rhs);
                            else if(i == 1) mat3->v1 = static_pointer_cast<Vector3>(rhs);
                            else if(i == 2) mat3->v2 = static_pointer_cast<Vector3>(rhs);
                            else { logger->log(index, "ERROR", "Index out of range for mat3 access"); return nullptr; }
                            mat3->generate_columns();
                            return nullptr;
                        }
                        if(source->type == NODE_MATRIX4 && index->type == NODE_INT) {
                            if(rhs->type != NODE_VECTOR4) {
                                logger->log(assign, "ERROR", "mat4 component needs to be vec4");
                                return nullptr;
                            }
                            Matrix4_ptr mat4 = static_pointer_cast<Matrix4>(source);
                            int i = resolve_int(index);
                            if(i == 0) mat4->v0 =static_pointer_cast<Vector4>( rhs);
                            else if(i == 1) mat4->v1 = static_pointer_cast<Vector4>(rhs);
                            else if(i == 2) mat4->v2 = static_pointer_cast<Vector4>(rhs);
                            else if(i == 3) mat4->v3 = static_pointer_cast<Vector4>(rhs);
                            else { logger->log(index, "ERROR", "Index out of range for mat4 access"); return nullptr; }
                            mat4->generate_columns();
                            return nullptr;
                        }

                        logger->log(index,"ERROR", "Invalid use of [] operator");
                        return nullptr;
                    }  else {
                        logger->log(assign, "ERROR", "Invalid left-hand side expression in assignment");
                        return nullptr;
                    }
                } else {
                    logger->log(assign, "ERROR", "Invalid assignment");
                }

                return nullptr;
            }
        case NODE_ALLOC:
            {
                Alloc_ptr alloc = static_pointer_cast<Alloc>(stmt);

                Scope_ptr scope = globalScope;
                if(!functionScopeStack.empty()) {
                    scope = functionScopeStack.top()->current();
                }

                Buffer_ptr buf = make_shared<Buffer>();
                buf->layout = make_shared<Layout>();

                gl->glGenBuffers(1, &(buf->handle));
                gl->glGenBuffers(1, &(buf->indexHandle));

                scope->declare(alloc->ident->name, "buffer", buf);

                return nullptr;
            }
        case NODE_UPLOAD:
            {
                Upload_ptr upload = static_pointer_cast<Upload>(stmt);

                Expr_ptr expr = nullptr;
                get_variable(expr, upload->ident->name);
                if(expr == nullptr || expr->type != NODE_BUFFER) {
                    logger->log(upload, "ERROR", "Can't upload to non-buffer object");
                }

                Buffer_ptr buffer = static_pointer_cast<Buffer>(expr);
                if(upload->attrib->name == "indices") {
                    for(unsigned int i = 0 ; i < upload->list->list.size(); i++) {
                        Expr_ptr e = eval_expr(upload->list->list[i]);
                        if(e == nullptr || e->type != NODE_INT) {
                            logger->log(upload, "ERROR", "Cannot upload non-int value into inded buffer!");
                            return nullptr;
                        } else {
                            buffer->indices.push_back(resolve_int(e));
                        }
                    }
                    return nullptr;
                }

                Layout_ptr layout = buffer->layout;

                if(layout->attributes.find(upload->attrib->name) == layout->attributes.end()) {
                    layout->attributes[upload->attrib->name] = 3;
                    layout->list.push_back(upload->attrib->name);
                }

                vector<float>* target = &(buffer->data[upload->attrib->name]);
                for(unsigned int i = 0; i < upload->list->list.size(); i++) {
                    Expr_ptr expr = eval_expr(upload->list->list[i]);
                    if(!expr) {
                        logger->log(upload, "ERROR", "Can't upload illegal value into buffer");
                        break;
                    }

                    if(expr->type == NODE_VECTOR3) {
                        Vector3_ptr vec3 = static_pointer_cast<Vector3>(expr);
                        target->insert(target->end(), resolve_scalar(vec3->x));
                        target->insert(target->end(), resolve_scalar(vec3->y));
                        target->insert(target->end(), resolve_scalar(vec3->z));
                    }

                    if(expr->type == NODE_FLOAT) {
                        Float_ptr f = static_pointer_cast<Float>(expr);
                        target->insert(target->begin(), resolve_scalar(f));
                    }
                }

                buffer->sizes[upload->attrib->name] = target->size() / buffer->layout->attributes[upload->attrib->name];

                return nullptr;
            }
        case NODE_DRAW:
            {
                Draw_ptr draw = static_pointer_cast<Draw>(stmt);

                if(current_program == nullptr) {
                    logger->log(draw, "ERROR", "Cannot bind program with name " + current_program_name);
                    return nullptr;
                }

                Expr_ptr expr = nullptr;
                get_variable(expr, draw->ident->name);
                if(expr  == nullptr || expr->type != NODE_BUFFER) {
                    logger->log(draw, "ERROR", "Can't draw non-buffer object");
                    return nullptr;
                }

                Buffer_ptr buffer = static_pointer_cast<Buffer>(expr);
                if(buffer != nullptr) {
                    Layout_ptr layout = buffer->layout;
                    vector<float> final_vector;

                    map<string, unsigned int> attributes = layout->attributes;
                    if(attributes.size() == 0) {
                        logger->log(draw, "ERROR", "Cannot draw empty buffer!");
                        return nullptr;
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

                    if(buffer->indices.size() > 0) {
                        gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer->indexHandle);
                        gl->glBufferData(GL_ELEMENT_ARRAY_BUFFER, buffer->indices.size() * sizeof(unsigned int), &(buffer->indices)[0], GL_STATIC_DRAW);
                        gl->glDrawElements(GL_TRIANGLES, buffer->indices.size(), GL_UNSIGNED_INT, 0);
                    } else {
                        gl->glDrawArrays(GL_TRIANGLES, 0, final_vector.size() / total_size);
                    }

                } else {
                    logger->log(draw, "ERROR", "Can't draw non-existent buffer " + draw->ident->name);
                }

                return nullptr;
            }
        case NODE_IF:
            {
                If_ptr ifstmt = static_pointer_cast<If>(stmt);
                Expr_ptr condition = eval_expr(ifstmt->condition);
                if(!condition) return nullptr;
                if(condition->type == NODE_BOOL) {
                    bool b = (static_pointer_cast<Bool>(condition)->value);
                    if(b) {
                        functionScopeStack.top()->attach("if");
                        Expr_ptr returnValue = execute_stmts(ifstmt->block);
                        functionScopeStack.top()->detach();
                        if(returnValue != nullptr) {
                            return returnValue;
                        }
                    }
                } else {
                    logger->log(ifstmt, "ERROR", "Condition in if statement not a boolean");
                }
                return nullptr;
            }
        case NODE_WHILE:
            {
                While_ptr whilestmt = static_pointer_cast<While>(stmt);
                Expr_ptr condition = eval_expr(whilestmt->condition);
                if(!condition) return nullptr;
                if(condition->type == NODE_BOOL) {
                    time_t start = time(nullptr);
                    functionScopeStack.top()->attach("while");
                    while(true) {
                        condition = eval_expr(whilestmt->condition);
                        bool b = (static_pointer_cast<Bool>(condition)->value);
                        if(!b) break;

                        Expr_ptr returnValue = execute_stmts(whilestmt->block);
                        if(returnValue != nullptr) {
                            return returnValue;
                        }

                        time_t now = time(nullptr);
                        
                        int diff = difftime(now, start);
                        if(diff > LOOP_TIMEOUT) { break; }
                    }
                    functionScopeStack.top()->detach();
                } else {
                    logger->log(whilestmt, "ERROR", "Condition in while statement not a boolean");
                }

                return nullptr;
            }
        case NODE_FOR:
            {
                For_ptr forstmt = static_pointer_cast<For>(stmt);
                Ident_ptr iterator = forstmt->iterator;
                Expr_ptr start = eval_expr(forstmt->start), end = eval_expr(forstmt->end), increment = eval_expr(forstmt->increment);
                if(start->type == NODE_INT || end->type == NODE_INT || increment->type == NODE_INT) {
                    functionScopeStack.top()->attach("for");
                    eval_stmt(make_shared<Decl>(make_shared<Ident>("int"), iterator, start));

                    time_t start = time(nullptr);
                    while(true) {
                        Bool_ptr terminate = static_pointer_cast<Bool>(eval_binary(make_shared<Binary>(iterator, OP_EQUAL, end)));
                        if(terminate->value) {
                            break;
                        }

                        Expr_ptr returnValue = execute_stmts(forstmt->block);
                        if(returnValue != nullptr) {
                            return returnValue;
                        }

                        eval_stmt(make_shared<Assign>(iterator, make_shared<Binary>(iterator, OP_PLUS, increment)));

                        time_t now = time(nullptr);
                        int diff = difftime(now, start);
                        if(diff > LOOP_TIMEOUT) { break;}
                    }
                    functionScopeStack.top()->detach();
                }

                return nullptr;
            }
        case NODE_PRINT:
            {
                Print_ptr print = static_pointer_cast<Print>(stmt);
                Expr_ptr output = eval_expr(print->expr);
                if(output == nullptr)
                    return nullptr;


                logger->log(tostring(output));
                return nullptr;
            }

        default: return nullptr;
    }
}

Expr_ptr Prototype::Interpreter::execute_stmts(Stmts_ptr stmts) {
    Expr_ptr returnValue = nullptr;
    for(unsigned int it = 0; it < stmts->list.size(); it++) { 
        Stmt_ptr stmt = stmts->list.at(it);
        if(stmt->type == NODE_RETURN) {
            Return_ptr ret = static_pointer_cast<Return>(stmt);
            returnValue = ret->value;
            break;
        }

        Expr_ptr expr = eval_stmt(stmt);
        if(expr != nullptr) {
            returnValue = expr;
            break;
        }
    }

    if(returnValue != nullptr) {
        return eval_expr(returnValue);
    } else {
        return nullptr;
    }
}

void Prototype::Interpreter::compile_shader(GLuint* handle, Shader_ptr shader) {
    string code = transpiler->transpile(shader);
    cout << code << endl;
    const char* src = code.c_str();
    gl->glShaderSource(*handle, 1, &src, nullptr);
    gl->glCompileShader(*handle);

    GLint success;
    char log[256];

    gl->glGetShaderInfoLog(*handle, 256, 0, log);
    gl->glGetShaderiv(*handle, GL_COMPILE_STATUS, &success);
    if(success != GL_TRUE) {
        cout << shader->name << " shader error\n" << success << " " << log << endl;
    }
}

void Prototype::Interpreter::compile_program() {
    programs.clear();
    for(map<string, ShaderPair_ptr>::iterator it = shaders.begin(); it != shaders.end(); ++it) {
        Program_ptr program = make_shared<Program>();
        program->handle = gl->glCreateProgram();
        program->vert = gl->glCreateShader(GL_VERTEX_SHADER);
        program->frag = gl->glCreateShader(GL_FRAGMENT_SHADER);

        gl->glAttachShader(program->handle, program->vert);
        gl->glAttachShader(program->handle, program->frag);

        program->vertSource = it->second->vertex;
        program->fragSource = it->second->fragment;

        if(program->vertSource == nullptr) {
            logger->log("ERROR: Missing vertex shader source for program " + it->first);
            continue;
        }

        if(program->fragSource == nullptr) {
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
    globalScope->clear();

    for(vector<Decl_ptr>::iterator it = globals.begin(); it != globals.end(); ++it) {
        Decl_ptr decl = *it;
        eval_stmt(decl);
    }

    invoke(init_invoke);
}

void Prototype::Interpreter::execute_loop() {
    if(!loop || status) return;

    invoke(loop_invoke);
}

void Prototype::Interpreter::load_imports() {
    for(unsigned int i = 0; i < imports.size(); i++) {
        string file = imports[i];
        load_import(file);
    }
}

void Prototype::Interpreter::load_import(string file) {
    string src = str_from_file(file);
    parse(src);
    if(status != 0) {
        logger->log("Error importing " + file);
    }

    status = 0;
}

void Prototype::Interpreter::parse(string code) {
    istringstream ss(code);
    scanner.switch_streams(&ss, nullptr);
    status = parser.parse();
}

void Prototype::Interpreter::prepare() {
    logger->clear();

    init = functions["init"];
    if(init == nullptr) {
        logger->log("ERROR: init() function required!");
    }

    loop = functions["loop"];
    if(loop == nullptr) {
        logger->log("ERROR: loop() function required!");
    }
}
