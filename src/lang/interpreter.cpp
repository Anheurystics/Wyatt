#include "interpreter.h"
#include <sstream>
#include <algorithm>
#include <memory>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

int resolve_int(Expr_ptr expr)
{
    return static_pointer_cast<Int>(expr)->value;
}

float resolve_float(Expr_ptr expr)
{
    return static_pointer_cast<Float>(expr)->value;
}

float resolve_scalar(Expr_ptr expr)
{
    if (expr->type == NODE_INT)
    {
        return (float)resolve_int(expr);
    }
    else
    {
        return resolve_float(expr);
    }
}

#define resolve_vec2(v) resolve_scalar(v->x), resolve_scalar(v->y)
#define resolve_vec3(v) resolve_scalar(v->x), resolve_scalar(v->y), resolve_scalar(v->z)
#define resolve_vec4(v) resolve_scalar(v->x), resolve_scalar(v->y), resolve_scalar(v->z), resolve_scalar(v->w)
#define get_variable(dest, name)                    \
    if (!functionScopeStack.empty())                \
    {                                               \
        dest = functionScopeStack.top()->get(name); \
    }                                               \
    if (dest == nullptr)                            \
    {                                               \
        dest = globalScope->get(name);              \
    }

#define LOOP_TIMEOUT 5

Wyatt::Interpreter::Interpreter(LogWindow *logger) : scanner(&line, &column), parser(scanner, logger, &line, &column, &imports, &globals, &functions, &layouts, &shaders), logger(logger)
{
    globalScope = make_shared<Scope>("global", logger, &workingDir);
    transpiler = new GLSLTranspiler(logger);

    init_invoke = make_shared<Invoke>(make_shared<Ident>("init"), make_shared<ArgList>(nullptr));
    loop_invoke = make_shared<Invoke>(make_shared<Ident>("loop"), make_shared<ArgList>(nullptr));
}

void Wyatt::Interpreter::reset()
{
    shaders.clear();
    functions.clear();
    layouts.clear();

    globals.clear();
    imports.clear();

    globalScope->clear();
    while (!functionScopeStack.empty())
        functionScopeStack.pop();

    current_program_name = "";
    current_program = nullptr;
    init = nullptr;
    loop = nullptr;

    line = 1;
    column = 1;
}

string Wyatt::Interpreter::print_expr(Expr_ptr expr)
{
    if (expr == nullptr)
    {
        return "";
    }

    switch (expr->type)
    {
    case NODE_INT:
        return to_string(resolve_int(expr));
    case NODE_FLOAT:
    {
        float f = resolve_float(expr);
        if (f == floor(f))
        {
            return to_string(int(f));
        }
        return to_string(f);
    }
    case NODE_BOOL:
        return static_pointer_cast<Bool>(expr)->value ? "true" : "false";
    case NODE_STRING:
        return static_pointer_cast<String>(expr)->value;
    case NODE_VECTOR2:
    case NODE_VECTOR3:
    case NODE_VECTOR4:
    {
        string result = "[";
        Vector_ptr vec = static_pointer_cast<Vector>(expr);
        for (unsigned int i = 0; i < vec->size(); i++)
        {
            result += print_expr(vec->get(i));
            if (i < vec->size() - 1)
            {
                result += ", ";
            }
        }
        result += "]";
        return result;
    }
    case NODE_MATRIX2:
    {
        Matrix2_ptr mat2 = static_pointer_cast<Matrix2>(expr);
        return "|" + print_expr(mat2->v0->x) + ", " + print_expr(mat2->v0->y) + "|\n" +
               "|" + print_expr(mat2->v1->x) + ", " + print_expr(mat2->v1->y) + "|";
    }
    case NODE_MATRIX3:
    {
        Matrix3_ptr mat3 = static_pointer_cast<Matrix3>(expr);
        return "|" + print_expr(mat3->v0->x) + ", " + print_expr(mat3->v0->y) + ", " + print_expr(mat3->v0->z) + "|\n" +
               "|" + print_expr(mat3->v1->x) + ", " + print_expr(mat3->v1->y) + ", " + print_expr(mat3->v1->z) + "|\n" +
               "|" + print_expr(mat3->v2->x) + ", " + print_expr(mat3->v2->y) + ", " + print_expr(mat3->v2->z) + "|";
    }
    case NODE_MATRIX4:
    {
        Matrix4_ptr mat4 = static_pointer_cast<Matrix4>(expr);
        return "|" + print_expr(mat4->v0->x) + ", " + print_expr(mat4->v0->y) + ", " + print_expr(mat4->v0->z) + ", " + print_expr(mat4->v0->w) + "|\n" +
               "|" + print_expr(mat4->v1->x) + ", " + print_expr(mat4->v1->y) + ", " + print_expr(mat4->v1->z) + ", " + print_expr(mat4->v1->w) + "|\n" +
               "|" + print_expr(mat4->v2->x) + ", " + print_expr(mat4->v2->y) + ", " + print_expr(mat4->v2->z) + ", " + print_expr(mat4->v2->w) + "|\n" +
               "|" + print_expr(mat4->v3->x) + ", " + print_expr(mat4->v3->y) + ", " + print_expr(mat4->v3->z) + ", " + print_expr(mat4->v3->w) + "|";
    }
    case NODE_LIST:
    {
        List_ptr lst = static_pointer_cast<List>(expr);
        string output = "{";
        for (unsigned int i = 0; i < lst->list.size(); i++)
        {
            if (i != 0)
                output += ", ";
            output += print_expr(eval_expr(lst->list[i]));
        }
        output += "}";
        return output;
    }
    case NODE_BUFFER:
    {
        Buffer_ptr buffer = static_pointer_cast<Buffer>(expr);
        string output = "";
        for (auto it = buffer->layout->list.begin(); it != buffer->layout->list.end(); ++it)
        {
            string attrib = *it;
            output += attrib + " size: " + to_string(buffer->layout->attributes[attrib]) + " {";
            for (auto jt = buffer->data[attrib].begin(); jt != buffer->data[attrib].end(); ++jt)
            {
                if (jt != buffer->data[attrib].begin())
                {
                    output += ", ";
                }
                float f = *jt;
                output += to_string(f);
            }
        }
        output += "}";
        return output;
    }
    default:
        return "";
    }
}

Expr_ptr vector_binary(Vector_ptr a, OpType op, Vector_ptr b)
{
    unsigned int vector_size = a->size();
    float *aComponents = new float[vector_size];
    float *bComponents = new float[vector_size];

    for (unsigned int i = 0; i < vector_size; i++)
    {
        aComponents[i] = resolve_scalar(a->get(i));
        bComponents[i] = resolve_scalar(b->get(i));
    }

    Expr_ptr result = nullptr;

    if (op == OP_EXP)
    {
        float total = 0;
        for (unsigned int i = 0; i < vector_size; i++)
        {
            total += aComponents[i] * bComponents[i];
        }
        result = make_shared<Float>(total);
    }
    else if (op == OP_MOD)
    {
        if (a->size() == 2)
        {
            result = make_shared<Float>(aComponents[0] * bComponents[1] - aComponents[1] * bComponents[0]);
        }
        else if (a->size() == 3)
        {
            result = make_shared<Vector3>(
                make_shared<Float>(aComponents[1] * bComponents[2] - aComponents[2] * bComponents[1]),
                make_shared<Float>(aComponents[2] * bComponents[0] - aComponents[0] * bComponents[2]),
                make_shared<Float>(aComponents[0] * bComponents[1] - aComponents[1] * bComponents[0]));
        }
    }
    else
    {
        Vector_ptr c;
        if (a->size() == 2)
        {
            c = make_shared<Vector2>(nullptr, nullptr);
        }
        if (a->size() == 3)
        {
            c = make_shared<Vector3>(nullptr, nullptr, nullptr);
        }
        if (a->size() == 4)
        {
            c = make_shared<Vector4>(nullptr, nullptr, nullptr, nullptr);
        }

        function<float(float, float)> operation;
        switch (op)
        {
        case OP_PLUS:
            operation = plus<float>();
            break;
        case OP_MINUS:
            operation = minus<float>();
            break;
        case OP_MULT:
            operation = multiplies<float>();
            break;
        case OP_DIV:
            operation = divides<float>();
            break;
        default:
            return nullptr;
        }

        for (unsigned int i = 0; i < a->size(); i++)
        {
            c->set(i, make_shared<Float>(operation(aComponents[i], bComponents[i])));
        }

        result = c;
    }

    delete[] aComponents;
    delete[] bComponents;
    return result;
}

Expr_ptr Wyatt::Interpreter::eval_binary(Binary_ptr bin)
{
    Expr_ptr lhs = eval_expr(bin->lhs);
    if (lhs == nullptr)
        return nullptr;

    OpType op = bin->op;

    Expr_ptr rhs = eval_expr(bin->rhs);
    if (rhs == nullptr)
        return nullptr;

    NodeType ltype = lhs->type;
    NodeType rtype = rhs->type;

    if (ltype == NODE_BOOL && rtype == NODE_BOOL)
    {
        bool a = static_pointer_cast<Bool>(lhs)->value;
        bool b = static_pointer_cast<Bool>(rhs)->value;

        switch (op)
        {
        case OP_AND:
            return make_shared<Bool>(a && b);
        case OP_OR:
            return make_shared<Bool>(a || b);
        default:
            break;
        }
    }

    if (ltype == NODE_INT && rtype == NODE_INT)
    {
        int a = resolve_int(lhs);
        int b = resolve_int(rhs);

        switch (op)
        {
        case OP_PLUS:
            return make_shared<Int>(a + b);
        case OP_MINUS:
            return make_shared<Int>(a - b);
        case OP_MULT:
            return make_shared<Int>(a * b);
        case OP_DIV:
            return make_shared<Float>(a / float(b));
        case OP_MOD:
            return make_shared<Int>(a % b);
        case OP_EQUAL:
            return make_shared<Bool>(a == b);
        case OP_LTHAN:
            return make_shared<Bool>(a < b);
        case OP_GTHAN:
            return make_shared<Bool>(a > b);
        case OP_NEQUAL:
            return make_shared<Bool>(a != b);
        case OP_LEQUAL:
            return make_shared<Bool>(a <= b);
        case OP_GEQUAL:
            return make_shared<Bool>(a >= b);
        default:
            break;
        }
    }

    if (ltype == NODE_FLOAT && rtype == NODE_FLOAT)
    {
        float a = resolve_float(lhs);
        float b = resolve_float(rhs);

        switch (op)
        {
        case OP_PLUS:
            return make_shared<Float>(a + b);
        case OP_MINUS:
            return make_shared<Float>(a - b);
        case OP_MULT:
            return make_shared<Float>(a * b);
        case OP_DIV:
            return make_shared<Float>(a / b);
        case OP_EQUAL:
            return make_shared<Bool>(a == b);
        case OP_LTHAN:
            return make_shared<Bool>(a < b);
        case OP_GTHAN:
            return make_shared<Bool>(a > b);
        case OP_NEQUAL:
            return make_shared<Bool>(a != b);
        case OP_LEQUAL:
            return make_shared<Bool>(a <= b);
        case OP_GEQUAL:
            return make_shared<Bool>(a >= b);
        default:
            break;
        }
    }

    if ((ltype == NODE_FLOAT && rtype == NODE_INT) || (ltype == NODE_INT && rtype == NODE_FLOAT))
    {
        float a = resolve_scalar(lhs);
        float b = resolve_scalar(rhs);

        switch (op)
        {
        case OP_PLUS:
            return make_shared<Float>(a + b);
        case OP_MINUS:
            return make_shared<Float>(a - b);
        case OP_MULT:
            return make_shared<Float>(a * b);
        case OP_DIV:
            return make_shared<Float>(a / b);
        case OP_EQUAL:
            return make_shared<Bool>(a == b);
        case OP_LTHAN:
            return make_shared<Bool>(a < b);
        case OP_GTHAN:
            return make_shared<Bool>(a > b);
        case OP_NEQUAL:
            return make_shared<Bool>(a != b);
        case OP_LEQUAL:
            return make_shared<Bool>(a <= b);
        case OP_GEQUAL:
            return make_shared<Bool>(a >= b);
        default:
            break;
        }
    }

    if (ltype == NODE_STRING || rtype == NODE_STRING)
    {
        bool left = (ltype == NODE_STRING);
        String_ptr str = (left ? static_pointer_cast<String>(lhs) : static_pointer_cast<String>(rhs));
        Expr_ptr other = eval_expr(left ? rhs : lhs);

        if (op == OP_PLUS)
        {
            return make_shared<String>(left ? (str->value + print_expr(other)) : (print_expr(other) + str->value));
        }
    }

    if (ltype == NODE_STRING && rtype == NODE_STRING)
    {
        string a = static_pointer_cast<String>(lhs)->value;
        string b = static_pointer_cast<String>(rhs)->value;
        int compare = a.compare(b);
        switch (op)
        {
        case OP_EQUAL:
            return make_shared<Bool>(compare == 0);
        case OP_LTHAN:
            return make_shared<Bool>(compare < 0);
        case OP_GTHAN:
            return make_shared<Bool>(compare > 0);
        case OP_LEQUAL:
            return make_shared<Bool>(compare <= 0);
        case OP_GEQUAL:
            return make_shared<Bool>(compare >= 0);
        default:
            break;
        }
    }

#define isvector(type) (type == NODE_VECTOR2 || type == NODE_VECTOR3 || type == NODE_VECTOR4)

    if (ltype == rtype && isvector(ltype))
    {
        Expr_ptr result = vector_binary(static_pointer_cast<Vector>(eval_expr(lhs)), op, static_pointer_cast<Vector>(eval_expr(rhs)));
        if (result != nullptr)
        {
            return result;
        }
    }

    if ((isvector(ltype) && (rtype == NODE_INT || rtype == NODE_FLOAT)) || (ltype == NODE_FLOAT || ltype == NODE_INT) && isvector(rtype))
    {
        bool lvector = isvector(ltype);

        Vector_ptr a = static_pointer_cast<Vector>(eval_expr(lvector ? lhs : rhs));
        float *components = new float[a->size()];
        float b = resolve_scalar(lvector ? rhs : lhs);

        for (unsigned int i = 0; i < a->size(); i++)
        {
            components[i] = resolve_scalar(a->get(i));
        }

        Vector_ptr v;
        if (a->size() == 2)
            v = make_shared<Vector2>(nullptr, nullptr);
        if (a->size() == 3)
            v = make_shared<Vector3>(nullptr, nullptr, nullptr);
        if (a->size() == 4)
            v = make_shared<Vector4>(nullptr, nullptr, nullptr, nullptr);

        bool no_op = false;

        function<float(float, float)> operation;
        if (op == OP_MULT)
        {
            operation = multiplies<float>();
        }
        else if (op == OP_DIV && lvector)
        {
            operation = divides<float>();
        }
        else
        {
            no_op = true;
        }

        if (no_op)
        {
            delete[] components;
        }
        else
        {
            for (unsigned int i = 0; i < a->size(); i++)
            {
                v->set(i, make_shared<Float>(operation(components[i], b)));
            }
            delete[] components;
            return v;
        }
    }

    if (ltype == NODE_MATRIX2 && (rtype == NODE_INT || rtype == NODE_FLOAT))
    {
        Matrix2_ptr a = static_pointer_cast<Matrix2>(eval_expr(lhs));

        if (op == OP_MULT || op == OP_DIV)
        {
            Vector2_ptr v0 = static_pointer_cast<Vector2>(eval_binary(make_shared<Binary>(a->v0, op, rhs)));
            Vector2_ptr v1 = static_pointer_cast<Vector2>(eval_binary(make_shared<Binary>(a->v1, op, rhs)));
            return make_shared<Matrix2>(v0, v1);
        }
    }

    if ((ltype == NODE_INT || ltype == NODE_FLOAT) && (rtype == NODE_MATRIX2))
    {
        Matrix2_ptr a = static_pointer_cast<Matrix2>(eval_expr(rhs));

        if (op == OP_MULT)
        {
            Vector2_ptr v0 = static_pointer_cast<Vector2>(eval_binary(make_shared<Binary>(a->v0, op, lhs)));
            Vector2_ptr v1 = static_pointer_cast<Vector2>(eval_binary(make_shared<Binary>(a->v1, op, lhs)));
            return make_shared<Matrix2>(v0, v1);
        }
    }

    if (ltype == NODE_MATRIX2 && rtype == NODE_MATRIX2)
    {
        Matrix2_ptr a = static_pointer_cast<Matrix2>(eval_expr(lhs));
        Matrix2_ptr b = static_pointer_cast<Matrix2>(eval_expr(rhs));

        if (op == OP_MULT)
        {
            Vector2_ptr r0 = make_shared<Vector2>(make_shared<Binary>(a->v0, OP_EXP, b->c0), make_shared<Binary>(a->v0, OP_EXP, b->c1));
            Vector2_ptr r1 = make_shared<Vector2>(make_shared<Binary>(a->v1, OP_EXP, b->c0), make_shared<Binary>(a->v1, OP_EXP, b->c1));
            return eval_expr(make_shared<Matrix2>(r0, r1));
        }
    }

    if (ltype == NODE_VECTOR2 && rtype == NODE_MATRIX2)
    {
        Vector2_ptr a = static_pointer_cast<Vector2>(eval_expr(lhs));
        Matrix2_ptr b = static_pointer_cast<Matrix2>(eval_expr(rhs));

        if (op == OP_MULT)
        {
            return eval_expr(make_shared<Vector2>(make_shared<Binary>(a, OP_EXP, b->c0), make_shared<Binary>(a, OP_EXP, b->c1)));
        }
    }

    if (ltype == NODE_MATRIX3 && (rtype == NODE_INT || rtype == NODE_FLOAT))
    {
        Matrix3_ptr a = static_pointer_cast<Matrix3>(eval_expr(lhs));

        if (op == OP_MULT || op == OP_DIV)
        {
            Vector3_ptr v0 = static_pointer_cast<Vector3>(eval_binary(make_shared<Binary>(a->v0, op, rhs)));
            Vector3_ptr v1 = static_pointer_cast<Vector3>(eval_binary(make_shared<Binary>(a->v1, op, rhs)));
            Vector3_ptr v2 = static_pointer_cast<Vector3>(eval_binary(make_shared<Binary>(a->v2, op, rhs)));
            return make_shared<Matrix3>(v0, v1, v2);
        }
    }

    if ((ltype == NODE_INT || ltype == NODE_FLOAT) && (rtype == NODE_MATRIX3))
    {
        Matrix3_ptr a = static_pointer_cast<Matrix3>(eval_expr(rhs));

        if (op == OP_MULT)
        {
            Vector3_ptr v0 = static_pointer_cast<Vector3>(eval_binary(make_shared<Binary>(a->v0, op, lhs)));
            Vector3_ptr v1 = static_pointer_cast<Vector3>(eval_binary(make_shared<Binary>(a->v1, op, lhs)));
            Vector3_ptr v2 = static_pointer_cast<Vector3>(eval_binary(make_shared<Binary>(a->v2, op, lhs)));
            return make_shared<Matrix3>(v0, v1, v2);
        }
    }

    if (ltype == NODE_MATRIX3 && rtype == NODE_MATRIX3)
    {
        Matrix3_ptr a = static_pointer_cast<Matrix3>(eval_expr(lhs));
        Matrix3_ptr b = static_pointer_cast<Matrix3>(eval_expr(rhs));

        if (op == OP_MULT)
        {
            Vector3_ptr r0 = make_shared<Vector3>(make_shared<Binary>(a->v0, OP_EXP, b->c0), make_shared<Binary>(a->v0, OP_EXP, b->c1), make_shared<Binary>(a->v0, OP_EXP, b->c2));
            Vector3_ptr r1 = make_shared<Vector3>(make_shared<Binary>(a->v1, OP_EXP, b->c0), make_shared<Binary>(a->v1, OP_EXP, b->c1), make_shared<Binary>(a->v1, OP_EXP, b->c2));
            Vector3_ptr r2 = make_shared<Vector3>(make_shared<Binary>(a->v2, OP_EXP, b->c0), make_shared<Binary>(a->v2, OP_EXP, b->c1), make_shared<Binary>(a->v2, OP_EXP, b->c2));
            return eval_expr(make_shared<Matrix3>(r0, r1, r2));
        }
    }

    if (ltype == NODE_VECTOR3 && rtype == NODE_MATRIX3)
    {
        Vector3_ptr a = static_pointer_cast<Vector3>(eval_expr(lhs));
        Matrix3_ptr b = static_pointer_cast<Matrix3>(eval_expr(rhs));

        if (op == OP_MULT)
        {
            return eval_expr(make_shared<Vector3>(make_shared<Binary>(a, OP_EXP, b->c0), make_shared<Binary>(a, OP_EXP, b->c1), make_shared<Binary>(a, OP_EXP, b->c2)));
        }
    }

    if (ltype == NODE_MATRIX4 && (rtype == NODE_INT || rtype == NODE_FLOAT))
    {
        Matrix4_ptr a = static_pointer_cast<Matrix4>(eval_expr(lhs));

        if (op == OP_MULT || op == OP_DIV)
        {
            Vector4_ptr v0 = static_pointer_cast<Vector4>(eval_binary(make_shared<Binary>(a->v0, op, rhs)));
            Vector4_ptr v1 = static_pointer_cast<Vector4>(eval_binary(make_shared<Binary>(a->v1, op, rhs)));
            Vector4_ptr v2 = static_pointer_cast<Vector4>(eval_binary(make_shared<Binary>(a->v2, op, rhs)));
            Vector4_ptr v3 = static_pointer_cast<Vector4>(eval_binary(make_shared<Binary>(a->v3, op, rhs)));
            return make_shared<Matrix4>(v0, v1, v2, v3);
        }
    }

    if ((ltype == NODE_INT || ltype == NODE_FLOAT) && (rtype == NODE_MATRIX4))
    {
        Matrix4_ptr a = static_pointer_cast<Matrix4>(eval_expr(rhs));

        if (op == OP_MULT)
        {
            Vector4_ptr v0 = static_pointer_cast<Vector4>(eval_binary(make_shared<Binary>(a->v0, op, lhs)));
            Vector4_ptr v1 = static_pointer_cast<Vector4>(eval_binary(make_shared<Binary>(a->v1, op, lhs)));
            Vector4_ptr v2 = static_pointer_cast<Vector4>(eval_binary(make_shared<Binary>(a->v2, op, lhs)));
            Vector4_ptr v3 = static_pointer_cast<Vector4>(eval_binary(make_shared<Binary>(a->v3, op, lhs)));
            return make_shared<Matrix4>(v0, v1, v2, v3);
        }
    }

    if (ltype == NODE_MATRIX4 && rtype == NODE_MATRIX4)
    {
        Matrix4_ptr a = static_pointer_cast<Matrix4>(eval_expr(lhs));
        Matrix4_ptr b = static_pointer_cast<Matrix4>(eval_expr(rhs));

        if (op == OP_MULT)
        {
            Vector4_ptr r0 = make_shared<Vector4>(make_shared<Binary>(a->v0, OP_EXP, b->c0), make_shared<Binary>(a->v0, OP_EXP, b->c1), make_shared<Binary>(a->v0, OP_EXP, b->c2), make_shared<Binary>(a->v0, OP_EXP, b->c3));
            Vector4_ptr r1 = make_shared<Vector4>(make_shared<Binary>(a->v1, OP_EXP, b->c0), make_shared<Binary>(a->v1, OP_EXP, b->c1), make_shared<Binary>(a->v1, OP_EXP, b->c2), make_shared<Binary>(a->v1, OP_EXP, b->c3));
            Vector4_ptr r2 = make_shared<Vector4>(make_shared<Binary>(a->v2, OP_EXP, b->c0), make_shared<Binary>(a->v2, OP_EXP, b->c1), make_shared<Binary>(a->v2, OP_EXP, b->c2), make_shared<Binary>(a->v2, OP_EXP, b->c3));
            Vector4_ptr r3 = make_shared<Vector4>(make_shared<Binary>(a->v3, OP_EXP, b->c0), make_shared<Binary>(a->v3, OP_EXP, b->c1), make_shared<Binary>(a->v3, OP_EXP, b->c2), make_shared<Binary>(a->v3, OP_EXP, b->c3));
            return eval_expr(make_shared<Matrix4>(r0, r1, r2, r3));
        }
    }

    if (ltype == NODE_VECTOR4 && rtype == NODE_MATRIX4)
    {
        Vector4_ptr a = static_pointer_cast<Vector4>(eval_expr(lhs));
        Matrix4_ptr b = static_pointer_cast<Matrix4>(eval_expr(rhs));

        if (op == OP_MULT)
        {
            return eval_expr(make_shared<Vector4>(make_shared<Binary>(a, OP_EXP, b->c0), make_shared<Binary>(a, OP_EXP, b->c1), make_shared<Binary>(a, OP_EXP, b->c2), make_shared<Binary>(a, OP_EXP, b->c3)));
        }
    }

    logger->log(bin, "ERROR", "Invalid operation between " + type_to_name(ltype) + " and " + type_to_name(rtype));
    return nullptr;
}

Expr_ptr Wyatt::Interpreter::invoke(Invoke_ptr invoke)
{
    string name = invoke->ident->name;
    FuncDef_ptr def = nullptr;

    if (name == "cos")
    {
        if (invoke->args->list.size() == 1)
        {
            Expr_ptr v = eval_expr(invoke->args->list[0]);
            if (v->type == NODE_FLOAT || v->type == NODE_INT)
            {
                return make_shared<Float>(cosf(resolve_scalar(v)));
            }
        }
    }

    if (name == "sin")
    {
        if (invoke->args->list.size() == 1)
        {
            Expr_ptr v = eval_expr(invoke->args->list[0]);
            if (v->type == NODE_FLOAT || v->type == NODE_INT)
            {
                return make_shared<Float>(sinf(resolve_scalar(v)));
            }
        }
    }

    if (name == "tan")
    {
        if (invoke->args->list.size() == 1)
        {
            Expr_ptr v = eval_expr(invoke->args->list[0]);
            if (v->type == NODE_FLOAT || v->type == NODE_INT)
            {
                return make_shared<Float>(tanf(resolve_scalar(v)));
            }
        }
    }

    if (name == "type")
    {
        if (invoke->args->list.size() == 1)
        {
            return make_shared<String>(type_to_name(eval_expr(invoke->args->list[0])->type));
        }
    }

    if (functions.find(name) != functions.end())
    {
        def = functions[name];
    }

    if (def != nullptr)
    {
        ScopeList_ptr localScope = make_shared<ScopeList>(name, logger, &workingDir);
        unsigned int nParams = def->params->list.size();
        unsigned int nArgs = invoke->args->list.size();

        if (nParams != nArgs)
        {
            logger->log(invoke, "ERROR", "Function " + name + " expects " + to_string(nParams) + " arguments, got " + to_string(nArgs));
            return nullptr;
        }
        if (nParams > 0)
        {
            for (unsigned int i = 0; i < nParams; i++)
            {
                Expr_ptr arg = eval_expr(invoke->args->list[i]);
                if (arg == nullptr)
                {
                    logger->log(invoke->args->list[i], "ERROR", "Invalid argument passed on to " + name);
                    return nullptr;
                }

                Decl_ptr param = def->params->list[i];
                localScope->current()->declare(param, param->ident, param->datatype->name, arg);
            }
        }

        functionScopeStack.push(localScope);
        Expr_ptr retValue = execute_stmts(def->stmts);
        functionScopeStack.pop();
        return retValue;
    }
    else
    {
        logger->log(invoke, "ERROR", "Function " + name + " does not exist");
    }
    return nullptr;
}

Expr_ptr Wyatt::Interpreter::resolve_vector(vector<Expr_ptr> list)
{
    vector<float> data;
    int n = 0;

    for (unsigned int i = 0; i < list.size(); i++)
    {
        Expr_ptr expr = list[i];
        if (expr == nullptr)
        {
            logger->log(expr, "ERROR", "Invalid vector/matrix element");
            return nullptr;
        }
        if (expr->type == NODE_FLOAT || expr->type == NODE_INT)
        {
            data.push_back(resolve_scalar(expr));
            n += 1;
        }
        else if (expr->type == NODE_VECTOR2 || expr->type == NODE_VECTOR3 || expr->type == NODE_VECTOR4)
        {
            Vector_ptr vec = static_pointer_cast<Vector>(eval_expr(expr));
            for (unsigned int i = 0; i < vec->size(); i++)
            {
                data.push_back(resolve_scalar(vec->get(i)));
            }
            n += vec->size();
        }
        else
        {
            logger->log(expr, "ERROR", "Invalid vector/matrix element of type " + type_to_name(expr->type));
            return nullptr;
        }
    }

    if (data.size() == 3)
    {
        return make_shared<Vector3>(make_shared<Float>(data[0]), make_shared<Float>(data[1]), make_shared<Float>(data[2]));
    }
    if (data.size() == 4)
    {
        return make_shared<Vector4>(make_shared<Float>(data[0]), make_shared<Float>(data[1]), make_shared<Float>(data[2]), make_shared<Float>(data[3]));
    }

    return nullptr;
}

Expr_ptr Wyatt::Interpreter::eval_expr(Expr_ptr node)
{

    if (node == nullptr)
    {
        return nullptr;
    }

    switch (node->type)
    {
    case NODE_NULL:
    {
        return node;
    }
    case NODE_IDENT:
    {
        Ident_ptr ident = static_pointer_cast<Ident>(node);
        Expr_ptr value = nullptr;
        get_variable(value, ident->name);
        if (value == nullptr)
        {
            logger->log(ident, "ERROR", "Variable " + ident->name + " does not exist");
        }

        return value;
    }
    case NODE_DOT:
    {
        Dot_ptr dot = static_pointer_cast<Dot>(node);
        Expr_ptr owner;
        get_variable(owner, dot->owner->name);
        if (owner == nullptr || owner->type == NODE_NULL)
        {
            logger->log(dot, "ERROR", "Can't access member of variable " + dot->owner->name);
            return nullptr;
        }

        if (owner->type == NODE_PROGRAM)
        {
            Program_ptr program = static_pointer_cast<Program>(owner);

            bool reupload = false;
            if (current_program_name != dot->owner->name)
            {
                current_program_name = dot->owner->name;
                current_program = program;
                reupload = true;
            }

            if (current_program == nullptr)
            {
                logger->log(dot, "ERROR", "Cannot get uniform from nonexistent shader");
                return nullptr;
            }

            if (reupload)
            {
                gl->glUseProgram(current_program->handle);
            }

            Shader_ptr src = current_program->vertSource;
            string type = "";
            if (src->uniforms->find(dot->name) != src->uniforms->end())
            {
                type = src->uniforms->at(dot->name);
            }
            else
            {
                src = current_program->fragSource;
                if (src->uniforms->find(dot->name) != src->uniforms->end())
                {
                    type = src->uniforms->at(dot->name);
                }
                else
                {
                    logger->log(dot, "ERROR", "Uniform " + dot->name + " of shader " + current_program_name + " does not exist");
                    return nullptr;
                }
            }

            GLint loc = gl->glGetUniformLocation(current_program->handle, dot->name.c_str());
            if (type == "float")
            {
                Float_ptr f = make_shared<Float>(0);
                gl->glGetUniformfv(current_program->handle, loc, &(f->value));
                return f;
            }
            else if (type == "vec2")
            {
                float value[2];
                gl->glGetUniformfv(current_program->handle, loc, &value[0]);
                return make_shared<Vector2>(make_shared<Float>(value[0]), make_shared<Float>(value[1]));
            }
            else if (type == "vec3")
            {
                float value[3];
                gl->glGetUniformfv(current_program->handle, loc, &value[0]);
                return make_shared<Vector3>(make_shared<Float>(value[0]), make_shared<Float>(value[1]), make_shared<Float>(value[2]));
            }
            else if (type == "vec4")
            {
                float value[4];
                gl->glGetUniformfv(current_program->handle, loc, &value[0]);
                return make_shared<Vector4>(make_shared<Float>(value[0]), make_shared<Float>(value[1]), make_shared<Float>(value[2]), make_shared<Float>(value[3]));
            }
            else if (type == "mat2")
            {
                float value[4];
                gl->glGetUniformfv(current_program->handle, loc, &value[0]);
                return make_shared<Matrix2>(make_shared<Vector2>(make_shared<Float>(value[0]), make_shared<Float>(value[1])), make_shared<Vector2>(make_shared<Float>(value[2]), make_shared<Float>(value[3])));
            }
            else if (type == "mat3")
            {
                float value[9];
                gl->glGetUniformfv(current_program->handle, loc, &value[0]);
                return make_shared<Matrix3>(
                    make_shared<Vector3>(make_shared<Float>(value[0]), make_shared<Float>(value[1]), make_shared<Float>(value[2])),
                    make_shared<Vector3>(make_shared<Float>(value[3]), make_shared<Float>(value[4]), make_shared<Float>(value[5])),
                    make_shared<Vector3>(make_shared<Float>(value[6]), make_shared<Float>(value[7]), make_shared<Float>(value[8])));
            }
            else if (type == "mat4")
            {
                float value[16];
                gl->glGetUniformfv(current_program->handle, loc, &value[0]);
                return make_shared<Matrix4>(
                    make_shared<Vector4>(make_shared<Float>(value[0]), make_shared<Float>(value[1]), make_shared<Float>(value[2]), make_shared<Float>(value[3])),
                    make_shared<Vector4>(make_shared<Float>(value[4]), make_shared<Float>(value[5]), make_shared<Float>(value[6]), make_shared<Float>(value[7])),
                    make_shared<Vector4>(make_shared<Float>(value[8]), make_shared<Float>(value[9]), make_shared<Float>(value[10]), make_shared<Float>(value[11])),
                    make_shared<Vector4>(make_shared<Float>(value[12]), make_shared<Float>(value[13]), make_shared<Float>(value[14]), make_shared<Float>(value[15])));
            }
        }
        else if (owner->type == NODE_TEXTURE)
        {
            Texture_ptr texture = static_pointer_cast<Texture>(owner);
            if (dot->name == "width")
            {
                return make_shared<Int>(texture->width);
            }
            if (dot->name == "height")
            {
                return make_shared<Int>(texture->height);
            }
            if (dot->name == "channels")
            {
                return make_shared<Int>(texture->channels);
            }
            return nullptr;
        }
        else if (owner->type == NODE_BUFFER)
        {
            Buffer_ptr buffer = static_pointer_cast<Buffer>(owner);
            if (buffer->data.find(dot->name) != buffer->data.end())
            {
                vector<float> attrib_data = buffer->data[dot->name];
                List_ptr list = make_shared<List>(nullptr);
                int size = buffer->sizes[dot->name];
                for (unsigned int i = 0; i < attrib_data.size(); i += size)
                {
                    if (size == 1)
                    {
                        list->list.push_back(make_shared<Float>(attrib_data[i]));
                    }
                    if (size == 2)
                    {
                        list->list.push_back(make_shared<Vector2>(make_shared<Float>(attrib_data[i]), make_shared<Float>(attrib_data[i + 1])));
                    }
                    if (size == 3)
                    {
                        list->list.push_back(make_shared<Vector3>(make_shared<Float>(attrib_data[i]), make_shared<Float>(attrib_data[i + 1]), make_shared<Float>(attrib_data[i + 2])));
                    }
                    if (size == 4)
                    {
                        list->list.push_back(make_shared<Vector4>(make_shared<Float>(attrib_data[i]), make_shared<Float>(attrib_data[i + 1]), make_shared<Float>(attrib_data[i + 2]), make_shared<Float>(attrib_data[i + 3])));
                    }
                }
                return list;
            }
        }

        break;
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
    {
        List_ptr list = static_pointer_cast<List>(node);
        if (list->list.size() == 0 && list->literal)
        {
            List_ptr newlist = make_shared<List>(nullptr);
            newlist->literal = false;
            newlist->first_line = list->first_line;
            newlist->last_line = list->last_line;
            return newlist;
        }
        else if (list->literal)
        {
            //FIXME: This severly impacts performance
            for (unsigned int i = 0; i < list->list.size(); i++)
            {
                list->list[i] = eval_expr(list->list[i]);
            }
            list->literal = false;
            return list;
        }

        break;
    }

    case NODE_BUFFER:
        return node;

    case NODE_TEXTURE:
        return node;

    case NODE_UPLOADLIST:
        return node;

    case NODE_VECTOR2:
    {
        Vector2_ptr vec2 = static_pointer_cast<Vector2>(node);
        Expr_ptr x = eval_expr(vec2->x);
        Expr_ptr y = eval_expr(vec2->y);

        if (x == nullptr)
        {
            logger->log(vec2->x, "ERROR", "Invalid component of index 0");
            return nullptr;
        }
        if (y == nullptr)
        {
            logger->log(vec2->y, "ERROR", "Invalid component of index 1");
            return nullptr;
        }

        if ((x->type == NODE_INT || x->type == NODE_FLOAT) && (y->type == NODE_INT || y->type == NODE_FLOAT))
        {
            return make_shared<Vector2>(static_pointer_cast<Expr>(x), static_pointer_cast<Expr>(y));
        }

        if (x->type == NODE_VECTOR2 && y->type == NODE_VECTOR2)
        {
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

        if (x == nullptr)
        {
            logger->log(vec3->x, "ERROR", "Invalid component of index 0");
            return nullptr;
        }
        if (y == nullptr)
        {
            logger->log(vec3->y, "ERROR", "Invalid component of index 1");
            return nullptr;
        }
        if (z == nullptr)
        {
            logger->log(vec3->z, "ERROR", "Invalid component of index 2");
            return nullptr;
        }

        if ((x->type == NODE_INT || x->type == NODE_FLOAT) && (y->type == NODE_INT || y->type == NODE_FLOAT) && (z->type == NODE_INT || z->type == NODE_FLOAT))
        {
            return make_shared<Vector3>(x, y, z);
        }

        if (x->type == NODE_VECTOR3 && y->type == NODE_VECTOR3 && z->type == NODE_VECTOR3)
        {
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

        if (x == nullptr)
        {
            logger->log(vec4->x, "ERROR", "Invalid component of index 0");
            return nullptr;
        }
        if (y == nullptr)
        {
            logger->log(vec4->y, "ERROR", "Invalid component of index 1");
            return nullptr;
        }
        if (z == nullptr)
        {
            logger->log(vec4->z, "ERROR", "Invalid component of index 2");
            return nullptr;
        }
        if (w == nullptr)
        {
            logger->log(vec4->w, "ERROR", "Invalid component of index 3");
            return nullptr;
        }

        if ((x->type == NODE_INT || x->type == NODE_FLOAT) && (y->type == NODE_INT || y->type == NODE_FLOAT) && (z->type == NODE_INT || z->type == NODE_FLOAT) && (w->type == NODE_INT || w->type == NODE_FLOAT))
        {
            return make_shared<Vector4>(x, y, z, w);
        }

        if (x->type == NODE_VECTOR4 && y->type == NODE_VECTOR4 && z->type == NODE_VECTOR4 && w->type == NODE_VECTOR4)
        {
            return make_shared<Matrix4>(static_pointer_cast<Vector4>(x), static_pointer_cast<Vector4>(y), static_pointer_cast<Vector4>(z), static_pointer_cast<Vector4>(w));
        }

        return nullptr;
    }

    case NODE_MATRIX2:
    {
        Matrix2_ptr mat2 = static_pointer_cast<Matrix2>(node);
        Expr_ptr v0 = eval_expr(mat2->v0);
        Expr_ptr v1 = eval_expr(mat2->v1);

        if (v0 == nullptr)
        {
            logger->log(mat2->v0, "ERROR", "Invalid component of index 0");
            return nullptr;
        }
        if (v1 == nullptr)
        {
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

        if (v0 == nullptr)
        {
            logger->log(mat3->v0, "ERROR", "Invalid component of index 0");
            return nullptr;
        }
        if (v1 == nullptr)
        {
            logger->log(mat3->v1, "ERROR", "Invalid component of index 1");
            return nullptr;
        }
        if (v2 == nullptr)
        {
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

        if (v0 == nullptr)
        {
            logger->log(mat4->v0, "ERROR", "Invalid component of index 0");
            return nullptr;
        }
        if (v1 == nullptr)
        {
            logger->log(mat4->v1, "ERROR", "Invalid component of index 1");
            return nullptr;
        }
        if (v2 == nullptr)
        {
            logger->log(mat4->v2, "ERROR", "Invalid component of index 2");
            return nullptr;
        }
        if (v3 == nullptr)
        {
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

        if (!rhs)
            return nullptr;

        if (un->op == OP_MINUS)
        {
            if (rhs->type == NODE_INT)
            {
                Int_ptr i = static_pointer_cast<Int>(rhs);
                return make_shared<Int>(-(i->value));
            }
            if (rhs->type == NODE_FLOAT)
            {
                Float_ptr fl = static_pointer_cast<Float>(rhs);
                return make_shared<Float>(-(fl->value));
            }
            if (rhs->type == NODE_VECTOR2 || rhs->type == NODE_VECTOR3 || rhs->type == NODE_VECTOR4)
            {
                return eval_binary(make_shared<Binary>(rhs, OP_MULT, make_shared<Float>(-1)));
            }
        }
        if (un->op == OP_NOT)
        {
            if (rhs->type == NODE_BOOL)
            {
                Bool_ptr b = static_pointer_cast<Bool>(rhs);
                return make_shared<Bool>(!(b->value));
            }
        }
        if (un->op == OP_ABS)
        {
            if (rhs->type == NODE_INT)
            {
                return make_shared<Int>(abs(resolve_int(rhs)));
            }
            if (rhs->type == NODE_FLOAT)
            {
                return make_shared<Float>(fabs(resolve_float(rhs)));
            }
            if (rhs->type == NODE_VECTOR2 || rhs->type == NODE_VECTOR3 || rhs->type == NODE_VECTOR4)
            {
                Vector_ptr vec = static_pointer_cast<Vector>(rhs);
                float square = 0;
                for (unsigned int i = 0; i < vec->size(); i++)
                {
                    float c = resolve_scalar(vec->get(i));
                    square += c * c;
                }
                return make_shared<Float>(sqrtf(square));
            }
            if (rhs->type == NODE_MATRIX2)
            {
                Matrix2_ptr mat2 = static_pointer_cast<Matrix2>(rhs);
                return make_shared<Float>(resolve_scalar(mat2->v0->x) * resolve_scalar(mat2->v1->y) - resolve_scalar(mat2->v0->y) * resolve_scalar(mat2->v1->x));
            }
#define mat3_det(a, b, c, d, e, f, g, h, i) (a * e * i) + (b * f * g) + (c * d * h) - (a * f * h) - (b * d * i) - (c * e * g)
            if (rhs->type == NODE_MATRIX3)
            {
                Matrix3_ptr mat3 = static_pointer_cast<Matrix3>(rhs);
                float a = resolve_scalar(mat3->v0->x), b = resolve_scalar(mat3->v0->y), c = resolve_scalar(mat3->v0->z);
                float d = resolve_scalar(mat3->v1->x), e = resolve_scalar(mat3->v1->y), f = resolve_scalar(mat3->v1->z);
                float g = resolve_scalar(mat3->v2->x), h = resolve_scalar(mat3->v2->y), i = resolve_scalar(mat3->v2->z);
                return make_shared<Float>(mat3_det(a, b, c, d, e, f, g, h, i));
            }
            if (rhs->type == NODE_MATRIX4)
            {
                Matrix4_ptr mat4 = static_pointer_cast<Matrix4>(rhs);
                float a = resolve_scalar(mat4->v0->x), b = resolve_scalar(mat4->v0->y), c = resolve_scalar(mat4->v0->z), d = resolve_scalar(mat4->v0->w);
                float e = resolve_scalar(mat4->v1->x), f = resolve_scalar(mat4->v1->y), g = resolve_scalar(mat4->v1->z), h = resolve_scalar(mat4->v1->w);
                float i = resolve_scalar(mat4->v2->x), j = resolve_scalar(mat4->v2->y), k = resolve_scalar(mat4->v2->z), l = resolve_scalar(mat4->v2->w);
                float m = resolve_scalar(mat4->v3->x), n = resolve_scalar(mat4->v3->y), o = resolve_scalar(mat4->v3->z), p = resolve_scalar(mat4->v3->w);

                float d1 = mat3_det(f, g, h, j, k, l, n, o, p);
                float d2 = mat3_det(e, g, h, i, k, l, m, o, p);
                float d3 = mat3_det(e, f, h, i, j, l, m, n, p);
                float d4 = mat3_det(e, f, g, i, j, k, m, n, o);

                return make_shared<Float>((a * d1) - (b * d2) + (c * d3) - (d * d4));
            }
            if (rhs->type == NODE_LIST)
            {
                List_ptr list = static_pointer_cast<List>(rhs);
                return make_shared<Int>(list->list.size());
            }
            return nullptr;
        }

        break;
    }

    case NODE_BINARY:
        return eval_binary(static_pointer_cast<Binary>(node));

    case NODE_FUNCEXPR:
        return invoke(static_pointer_cast<FuncExpr>(node)->invoke);

    case NODE_INDEX:
    {
        Index_ptr in = static_pointer_cast<Index>(node);
        Expr_ptr source = eval_expr(in->source);
        Expr_ptr index = eval_expr(in->index);

        if (source == nullptr || index == nullptr)
        {
            logger->log(in, "ERROR", "Invalid index expression");
            return nullptr;
        }

        if (source->type == NODE_VECTOR2 || source->type == NODE_VECTOR3 || source->type == NODE_VECTOR4)
        {
            if (index->type == NODE_INT)
            {
                Vector_ptr vec = static_pointer_cast<Vector>(source);
                unsigned int i = resolve_int(index);
                if (i < vec->size())
                {
                    return eval_expr(vec->get(i));
                }
                else
                {
                    logger->log(index, "ERROR", "Index " + to_string(i) + " out of range for " + type_to_name(vec->type) + " access");
                    return nullptr;
                }
            }
        }
        if (source->type == NODE_MATRIX2 && index->type == NODE_INT)
        {
            Matrix2_ptr mat2 = static_pointer_cast<Matrix2>(source);
            int i = resolve_int(index);
            if (i == 0)
                return mat2->v0;
            if (i == 1)
                return mat2->v1;

            logger->log(index, "ERROR", "Index out of range for mat2 access");
            return nullptr;
        }
        if (source->type == NODE_MATRIX3 && index->type == NODE_INT)
        {
            Matrix3_ptr mat3 = static_pointer_cast<Matrix3>(source);
            int i = resolve_int(index);
            if (i == 0)
                return mat3->v0;
            if (i == 1)
                return mat3->v1;
            if (i == 2)
                return mat3->v2;

            logger->log(index, "ERROR", "Index out of range for mat3 access");
            return nullptr;
        }
        if (source->type == NODE_MATRIX4 && index->type == NODE_INT)
        {
            Matrix4_ptr mat4 = static_pointer_cast<Matrix4>(source);
            int i = resolve_int(index);
            if (i == 0)
                return mat4->v0;
            if (i == 1)
                return mat4->v1;
            if (i == 2)
                return mat4->v2;
            if (i == 3)
                return mat4->v3;

            logger->log(index, "ERROR", "Index out of range for mat4 access");
            return nullptr;
        }
        if (source->type == NODE_LIST && index->type == NODE_INT)
        {
            List_ptr list = static_pointer_cast<List>(source);
            int i = resolve_int(index);
            int size = list->list.size();
            if (i >= 0 && i < size)
            {
                return eval_expr(list->list[i]);
            }
            else
            {
                logger->log(index, "ERROR", "Index out of range for list of length " + list->list.size());
                return nullptr;
            }
        }

        logger->log(index, "ERROR", "Invalid use of [] operator");
        return nullptr;
    }

    default:
        logger->log(node, "ERROR", "Illegal expression");
        return nullptr;
    }

    return nullptr;
}

Expr_ptr Wyatt::Interpreter::eval_stmt(Stmt_ptr stmt)
{
    switch (stmt->type)
    {
    case NODE_FUNCSTMT:
        invoke(static_pointer_cast<FuncStmt>(stmt)->invoke);
        return nullptr;

    case NODE_DECL:
    {
        Decl_ptr decl = static_pointer_cast<Decl>(stmt);
        Scope_ptr scope = globalScope;
        if (!functionScopeStack.empty())
        {
            scope = functionScopeStack.top()->current();
        }

        if (decl->datatype->name == "buffer")
        {
            Buffer_ptr buf = make_shared<Buffer>();
            buf->layout = new Layout();

            gl->glGenBuffers(1, &(buf->handle));
            gl->glGenBuffers(1, &(buf->indexHandle));

            decl->value = buf;
        }
        if (decl->datatype->name == "texture2D")
        {
            if (decl->value == nullptr)
            {
                decl->value = make_shared<Texture>();
            }
        }

        scope->declare(decl, decl->ident, decl->datatype->name, eval_expr(decl->value));

        return nullptr;
    }
    case NODE_ASSIGN:
    {
        Assign_ptr assign = static_pointer_cast<Assign>(stmt);
        Expr_ptr rhs = eval_expr(assign->value);
        if (rhs != nullptr)
        {
            Expr_ptr lhs = assign->lhs;
            if (lhs->type == NODE_IDENT)
            {
                Ident_ptr ident = static_pointer_cast<Ident>(lhs);
                if (!functionScopeStack.empty() && functionScopeStack.top()->assign(assign, ident, rhs))
                {
                    return nullptr;
                }
                if (globalScope->assign(assign, ident, rhs))
                {
                    return nullptr;
                }

                logger->log(lhs, "ERROR", "Variable " + ident->name + " does not exist");
                return nullptr;
            }
            else if (lhs->type == NODE_DOT)
            {
                Dot_ptr dot = static_pointer_cast<Dot>(lhs);
                Expr_ptr owner;
                get_variable(owner, dot->owner->name);
                if (owner == nullptr || owner->type == NODE_NULL)
                {
                    logger->log(dot, "ERROR", "Can't assign to member of variable " + dot->owner->name);
                    return nullptr;
                }

                if (owner->type == NODE_PROGRAM)
                {
                    Program_ptr program = static_pointer_cast<Program>(owner);
                    bool reupload = false;
                    if (current_program_name != dot->owner->name)
                    {
                        current_program_name = dot->owner->name;
                        current_program = program;
                        reupload = true;
                    }

                    if (current_program == nullptr)
                    {
                        logger->log(dot, "ERROR", "Cannot upload to nonexistent shader");
                        return nullptr;
                    }

                    if (reupload)
                    {
                        gl->glUseProgram(current_program->handle);
                    }

                    Shader_ptr src = current_program->vertSource;
                    string type = "";
                    if (src->uniforms->find(dot->name) != src->uniforms->end())
                    {
                        type = src->uniforms->at(dot->name);
                    }
                    else
                    {
                        src = current_program->fragSource;
                        if (src->uniforms->find(dot->name) != src->uniforms->end())
                        {
                            type = src->uniforms->at(dot->name);
                        }
                        else
                        {
                            logger->log(dot, "ERROR", "Uniform " + dot->name + " of shader " + current_program_name + " does not exist");
                            return nullptr;
                        }
                    }

                    GLint loc = gl->glGetUniformLocation(current_program->handle, dot->name.c_str());
                    if (type == "float")
                    {
                        if (rhs->type == NODE_FLOAT)
                        {
                            Float_ptr f = static_pointer_cast<Float>(rhs);
                            gl->glUniform1f(loc, resolve_float(f));
                        }
                        else
                        {
                            logger->log(dot, "ERROR", "Uniform upload mismatch: float required for " + dot->name + " of shader " + current_program_name);
                            return nullptr;
                        }
                    }
                    else if (type == "vec2")
                    {
                        if (rhs->type == NODE_VECTOR2)
                        {
                            Vector2_ptr vec2 = static_pointer_cast<Vector2>(eval_expr(rhs));
                            gl->glUniform2f(loc, resolve_vec2(vec2));
                        }
                        else
                        {
                            logger->log(dot, "ERROR", "Uniform upload mismatch: vec2 required for " + dot->name + " of shader " + current_program_name);
                            return nullptr;
                        }
                    }
                    else if (type == "vec3")
                    {
                        if (rhs->type == NODE_VECTOR3)
                        {
                            Vector3_ptr vec3 = static_pointer_cast<Vector3>(eval_expr(rhs));
                            gl->glUniform3f(loc, resolve_vec3(vec3));
                        }
                        else
                        {
                            logger->log(dot, "ERROR", "Uniform upload mismatch: vec3 required for " + dot->name + " of shader " + current_program_name);
                            return nullptr;
                        }
                    }
                    else if (type == "vec4")
                    {
                        if (rhs->type == NODE_VECTOR4)
                        {
                            Vector4_ptr vec4 = static_pointer_cast<Vector4>(eval_expr(rhs));
                            gl->glUniform4f(loc, resolve_vec4(vec4));
                        }
                        else
                        {
                            logger->log(dot, "ERROR", "Uniform upload mismatch: vec4 required for " + dot->name + " of shader " + current_program_name);
                            return nullptr;
                        }
                    }
                    else if (type == "mat2")
                    {
                        if (rhs->type == NODE_MATRIX2)
                        {
                            Matrix2_ptr mat2 = static_pointer_cast<Matrix2>(eval_expr(rhs));
                            float data[4];
                            data[0] = resolve_scalar(mat2->v0->x);
                            data[1] = resolve_scalar(mat2->v0->y);
                            data[2] = resolve_scalar(mat2->v1->x);
                            data[3] = resolve_scalar(mat2->v1->y);
                            gl->glUniformMatrix2fv(loc, 1, false, data);
                        }
                        else
                        {
                            logger->log(dot, "ERROR", "Uniform upload mismatch: vec4 required for " + dot->name + " of shader " + current_program_name);
                            return nullptr;
                        }
                    }
                    else if (type == "mat3")
                    {
                        if (rhs->type == NODE_MATRIX3)
                        {
                            Matrix3_ptr mat3 = static_pointer_cast<Matrix3>(eval_expr(rhs));
                            float data[9];
                            data[0] = resolve_scalar(mat3->v0->x);
                            data[1] = resolve_scalar(mat3->v0->y);
                            data[2] = resolve_scalar(mat3->v0->z);
                            data[3] = resolve_scalar(mat3->v1->x);
                            data[4] = resolve_scalar(mat3->v1->y);
                            data[5] = resolve_scalar(mat3->v1->z);
                            data[6] = resolve_scalar(mat3->v2->x);
                            data[7] = resolve_scalar(mat3->v2->y);
                            data[8] = resolve_scalar(mat3->v2->z);
                            gl->glUniformMatrix3fv(loc, 1, false, data);
                        }
                        else
                        {
                            logger->log(dot, "ERROR", "Uniform upload mismatch: mat3 required for " + dot->name + " of shader " + current_program_name);
                            return nullptr;
                        }
                    }
                    else if (type == "mat4")
                    {
                        if (rhs->type == NODE_MATRIX4)
                        {
                            Matrix4_ptr mat4 = static_pointer_cast<Matrix4>(eval_expr(rhs));
                            float data[16];
                            data[0] = resolve_scalar(mat4->v0->x);
                            data[1] = resolve_scalar(mat4->v0->y);
                            data[2] = resolve_scalar(mat4->v0->z);
                            data[3] = resolve_scalar(mat4->v0->w);
                            data[4] = resolve_scalar(mat4->v1->x);
                            data[5] = resolve_scalar(mat4->v1->y);
                            data[6] = resolve_scalar(mat4->v1->z);
                            data[7] = resolve_scalar(mat4->v1->w);
                            data[8] = resolve_scalar(mat4->v2->x);
                            data[9] = resolve_scalar(mat4->v2->y);
                            data[10] = resolve_scalar(mat4->v2->z);
                            data[11] = resolve_scalar(mat4->v2->w);
                            data[12] = resolve_scalar(mat4->v3->x);
                            data[13] = resolve_scalar(mat4->v3->y);
                            data[14] = resolve_scalar(mat4->v3->z);
                            data[15] = resolve_scalar(mat4->v3->w);
                            gl->glUniformMatrix4fv(loc, 1, false, data);
                        }
                        else
                        {
                            logger->log(dot, "ERROR", "Uniform upload mismatch: mat3 required for " + dot->name + " of shader " + current_program_name);
                            return nullptr;
                        }
                    }
                    else if (type == "texture2D")
                    {
                        switch (rhs->type)
                        {
                        case NODE_TEXTURE:
                        {
                            Shader_ptr frag = current_program->fragSource;
                            auto texSlots = frag->textureSlots;
                            auto it = find(texSlots->begin(), texSlots->end(), dot->name);
                            activeTextureSlot = it - texSlots->begin();

                            Texture_ptr tex = static_pointer_cast<Texture>(rhs);
                            gl->glActiveTexture(GL_TEXTURE0 + activeTextureSlot);
                            if (tex->handle == 0)
                            {
                                gl->glGenTextures(1, &(tex->handle));
                                gl->glBindTexture(GL_TEXTURE_2D, tex->handle);
                                gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                                gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                                gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                                gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                                gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex->width, tex->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex->image);
                            }
                            else
                            {
                                gl->glBindTexture(GL_TEXTURE_2D, tex->handle);
                            }
                            gl->glUniform1i(loc, activeTextureSlot);

                            break;
                        }
                        case NODE_STRING:
                        {
                            string filename = static_pointer_cast<String>(rhs)->value;
                            if (filename == "")
                            {
                                gl->glBindTexture(GL_TEXTURE_2D, 0);
                                gl->glActiveTexture(0);
                            }
                            else
                            {
                                int width, height, n;
                                string realfilename = "";
                                if (file_exists(workingDir + "/" + filename))
                                {
                                    realfilename = workingDir + "/" + filename;
                                }
                                else
                                {
                                    realfilename = filename;
                                }
                                unsigned char *data = stbi_load(realfilename.c_str(), &width, &height, &n, 4);
                                GLuint handle = 0;
                                glGenTextures(1, &handle);
                                glBindTexture(GL_TEXTURE_2D, handle);
                                gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                                gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                                gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                                gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
                                if (handle == 0)
                                {
                                    string message = "Cannot load " + filename + ": ";
                                    message += stbi_failure_reason();
                                    logger->log(rhs, "ERROR", message);
                                    break;
                                }
                                gl->glBindTexture(GL_TEXTURE_2D, handle);
                                gl->glActiveTexture(GL_TEXTURE0);
                            }
                            break;
                        }
                        break;
                        default:
                            break;
                        }
                    }
                }
                else if (owner->type == NODE_TEXTURE)
                {
                    Texture_ptr texture = static_pointer_cast<Texture>(owner);
                    if (dot->name == "width" || dot->name == "height" || dot->name == "channels")
                    {
                        logger->log(dot->owner, "ERROR", "Field \"" + dot->name + "\" of texture is read-only");
                        return nullptr;
                    }
                }
            }
            else if (lhs->type == NODE_INDEX)
            {
                Index_ptr in = static_pointer_cast<Index>(lhs);
                Expr_ptr source = eval_expr(in->source);
                Expr_ptr index = eval_expr(in->index);
                if (source == nullptr || index == nullptr)
                {
                    logger->log(in, "ERROR", "Invalid index expression");
                    return nullptr;
                }

                if (source->type == NODE_LIST && index->type == NODE_INT)
                {
                    List_ptr list = static_pointer_cast<List>(source);
                    int i = resolve_int(index);
                    int size = list->list.size();
                    if (i >= 0 && i < size)
                    {
                        list->list[i] = rhs;
                    }
                    else
                    {
                        logger->log(assign, "ERROR", "Index out of range for list of length " + list->list.size());
                    }
                    return nullptr;
                }

                bool is_scalar = (rhs->type == NODE_FLOAT || rhs->type == NODE_INT);
                if (source->type == NODE_VECTOR2 || source->type == NODE_VECTOR3 || source->type == NODE_VECTOR4)
                {
                    if (index->type == NODE_INT)
                    {
                        if (!is_scalar)
                        {
                            logger->log(assign, "ERROR", type_to_name(rhs->type) + " component needs to be a float or an int");
                            return nullptr;
                        }
                        Vector_ptr vec = static_pointer_cast<Vector>(source);
                        unsigned int i = resolve_int(index);
                        if (i < vec->size())
                        {
                            vec->set(i, rhs);
                        }
                        else
                        {
                            logger->log(index, "ERROR", "Index out of range for " + type_to_name(rhs->type) + " access");
                        }
                        return nullptr;
                    }
                }
                if (source->type == NODE_MATRIX2 && index->type == NODE_INT)
                {
                    if (rhs->type != NODE_VECTOR2)
                    {
                        logger->log(assign, "ERROR", "mat2 component needs to be vec2");
                        return nullptr;
                    }
                    Matrix2_ptr mat2 = static_pointer_cast<Matrix2>(source);
                    int i = resolve_int(index);
                    if (i == 0)
                        mat2->v0 = static_pointer_cast<Vector2>(rhs);
                    else if (i == 1)
                        mat2->v1 = static_pointer_cast<Vector2>(rhs);
                    else
                    {
                        logger->log(assign, "ERROR", "Index out of range for mat2 access");
                        return nullptr;
                    }
                    mat2->generate_columns();
                    return nullptr;
                }
                if (source->type == NODE_MATRIX3 && index->type == NODE_INT)
                {
                    if (rhs->type != NODE_VECTOR3)
                    {
                        logger->log(assign, "ERROR", "mat3 component needs to be vec3");
                        return nullptr;
                    }
                    Matrix3_ptr mat3 = static_pointer_cast<Matrix3>(source);
                    int i = resolve_int(index);
                    if (i == 0)
                        mat3->v0 = static_pointer_cast<Vector3>(rhs);
                    else if (i == 1)
                        mat3->v1 = static_pointer_cast<Vector3>(rhs);
                    else if (i == 2)
                        mat3->v2 = static_pointer_cast<Vector3>(rhs);
                    else
                    {
                        logger->log(index, "ERROR", "Index out of range for mat3 access");
                        return nullptr;
                    }
                    mat3->generate_columns();
                    return nullptr;
                }
                if (source->type == NODE_MATRIX4 && index->type == NODE_INT)
                {
                    if (rhs->type != NODE_VECTOR4)
                    {
                        logger->log(assign, "ERROR", "mat4 component needs to be vec4");
                        return nullptr;
                    }
                    Matrix4_ptr mat4 = static_pointer_cast<Matrix4>(source);
                    int i = resolve_int(index);
                    if (i == 0)
                        mat4->v0 = static_pointer_cast<Vector4>(rhs);
                    else if (i == 1)
                        mat4->v1 = static_pointer_cast<Vector4>(rhs);
                    else if (i == 2)
                        mat4->v2 = static_pointer_cast<Vector4>(rhs);
                    else if (i == 3)
                        mat4->v3 = static_pointer_cast<Vector4>(rhs);
                    else
                    {
                        logger->log(index, "ERROR", "Index out of range for mat4 access");
                        return nullptr;
                    }
                    mat4->generate_columns();
                    return nullptr;
                }

                logger->log(index, "ERROR", "Invalid use of [] operator");
                return nullptr;
            }
            else
            {
                logger->log(assign, "ERROR", "Invalid left-hand side expression in assignment");
                return nullptr;
            }
        }
        else
        {
            logger->log(assign, "ERROR", "Invalid assignment");
        }

        return nullptr;
    }
    case NODE_ALLOC:
    {
        Alloc_ptr alloc = static_pointer_cast<Alloc>(stmt);

        Scope_ptr scope = globalScope;
        if (!functionScopeStack.empty())
        {
            scope = functionScopeStack.top()->current();
        }

        Buffer_ptr buf = make_shared<Buffer>();
        buf->layout = new Layout();

        gl->glGenBuffers(1, &(buf->handle));
        gl->glGenBuffers(1, &(buf->indexHandle));

        scope->declare(alloc, alloc->ident, "buffer", buf);

        return nullptr;
    }
    case NODE_UPLOAD:
    {
        Upload_ptr upload = static_pointer_cast<Upload>(stmt);

        Expr_ptr expr = nullptr;
        get_variable(expr, upload->ident->name);
        if (expr == nullptr || expr->type != NODE_BUFFER)
        {
            logger->log(upload, "ERROR", "Cannot upload to non-buffer object");
            return nullptr;
        }

        Buffer_ptr buffer = static_pointer_cast<Buffer>(expr);
        if (upload->attrib->name == "indices")
        {
            for (unsigned int i = 0; i < upload->list->list.size(); i++)
            {
                Expr_ptr e = eval_expr(upload->list->list[i]);
                if (e == nullptr || e->type != NODE_INT)
                {
                    logger->log(upload, "ERROR", "Cannot upload non-int value into index buffer");
                    return nullptr;
                }
                else
                {
                    buffer->indices.push_back(resolve_int(e));
                }
            }
            return nullptr;
        }

        Layout *layout = buffer->layout;

#define attrib_size(type) ((type == NODE_FLOAT ? 1 : (type == NODE_VECTOR2 ? 2 : (type == NODE_VECTOR3 ? 3 : (type == NODE_VECTOR4 ? 4 : 0)))))
        vector<float> *target = &(buffer->data[upload->attrib->name]);
        for (unsigned int i = 0; i < upload->list->list.size(); i++)
        {
            if (upload->list->list[i] == nullptr)
            {
                logger->log(upload, "ERROR", "Can't upload illegal value into buffer");
                return nullptr;
            }
            Expr_ptr expr = eval_expr(upload->list->list[i]);
            if (expr == nullptr)
            {
                logger->log(upload, "ERROR", "Can't upload illegal value into buffer");
                return nullptr;
            }

            unsigned int size = attrib_size(expr->type);
            if (expr->type == NODE_LIST)
            {
                List_ptr list = static_pointer_cast<List>(expr);
                size = attrib_size(eval_expr(list->list[i])->type);
            }
            if (size == 0)
            {
                logger->log(upload, "ERROR", "Attribute type must be float, vector, or list");
                return nullptr;
            }

            if (layout->attributes.find(upload->attrib->name) == layout->attributes.end())
            {
                layout->attributes[upload->attrib->name] = size;
                layout->list.push_back(upload->attrib->name);
            }
            else
            {
                if (size != layout->attributes[upload->attrib->name])
                {
                    logger->log(upload, "ERROR", "Attribute size must be consistent");
                    return nullptr;
                }
            }

            if (expr->type == NODE_FLOAT)
            {
                Float_ptr f = static_pointer_cast<Float>(expr);
                target->push_back(resolve_scalar(f));
            }

            if (expr->type == NODE_VECTOR2)
            {
                Vector2_ptr vec2 = static_pointer_cast<Vector2>(expr);
                target->push_back(resolve_scalar(vec2->x));
                target->push_back(resolve_scalar(vec2->y));
            }

            if (expr->type == NODE_VECTOR3)
            {
                Vector3_ptr vec3 = static_pointer_cast<Vector3>(expr);
                target->push_back(resolve_scalar(vec3->x));
                target->push_back(resolve_scalar(vec3->y));
                target->push_back(resolve_scalar(vec3->z));
            }

            if (expr->type == NODE_VECTOR4)
            {
                Vector4_ptr vec4 = static_pointer_cast<Vector4>(expr);
                target->push_back(resolve_scalar(vec4->x));
                target->push_back(resolve_scalar(vec4->y));
                target->push_back(resolve_scalar(vec4->z));
                target->push_back(resolve_scalar(vec4->w));
            }

            if (expr->type == NODE_LIST)
            {
                List_ptr list = static_pointer_cast<List>(expr);
                for (auto it = list->list.begin(); it != list->list.end(); ++it)
                {
                    Expr_ptr item = eval_expr(*it);
                    if (size != attrib_size(item->type))
                    {
                        logger->log(upload, "ERROR", "Attribute size must be consistent");
                        return nullptr;
                    }

                    if (item->type == NODE_FLOAT)
                    {
                        Float_ptr f = static_pointer_cast<Float>(item);
                        target->push_back(resolve_scalar(f));
                    }

                    if (item->type == NODE_VECTOR2)
                    {
                        Vector2_ptr vec2 = static_pointer_cast<Vector2>(item);
                        target->push_back(resolve_scalar(vec2->x));
                        target->push_back(resolve_scalar(vec2->y));
                    }

                    if (item->type == NODE_VECTOR3)
                    {
                        Vector3_ptr vec3 = static_pointer_cast<Vector3>(item);
                        target->push_back(resolve_scalar(vec3->x));
                        target->push_back(resolve_scalar(vec3->y));
                        target->push_back(resolve_scalar(vec3->z));
                    }

                    if (item->type == NODE_VECTOR4)
                    {
                        Vector4_ptr vec4 = static_pointer_cast<Vector4>(item);
                        target->push_back(resolve_scalar(vec4->x));
                        target->push_back(resolve_scalar(vec4->y));
                        target->push_back(resolve_scalar(vec4->z));
                        target->push_back(resolve_scalar(vec4->w));
                    }
                }
            }
        }

        buffer->sizes[upload->attrib->name] = target->size() / buffer->layout->attributes[upload->attrib->name];

        return nullptr;
    }
    case NODE_COMPBINARY:
    {
        CompBinary_ptr compbin = static_pointer_cast<CompBinary>(stmt);
        Expr_ptr lhs = eval_expr(compbin->lhs);
        Expr_ptr rhs = eval_expr(compbin->rhs);
        OpType op = compbin->op;

        if (lhs == nullptr)
        {
            logger->log(compbin->lhs, "ERROR", "Illegal expression at the left-hand side");
            return nullptr;
        }

        if (rhs == nullptr)
        {
            logger->log(compbin->rhs, "ERROR", "Illegal expression at the right-hand side");
            return nullptr;
        }

        if (op == OP_PLUS && lhs->type == NODE_LIST)
        {
            List_ptr list = static_pointer_cast<List>(lhs);

            if (rhs->type != NODE_UPLOADLIST)
            {
                list->list.push_back(rhs);
            }
            else
            {
                UploadList_ptr uploadList = static_pointer_cast<UploadList>(rhs);
                for (unsigned int i = 0; i < uploadList->list.size(); i++)
                {
                    Expr_ptr expr = eval_expr(uploadList->list[i]);
                    if (expr != nullptr)
                    {
                        list->list.push_back(expr);
                    }
                    else
                    {
                        logger->log(uploadList->list[i], "ERROR", "Can't append illegal value to list");
                        return nullptr;
                    }
                }
            }
        }
        else
        {
            rhs = compbin->rhs;
            if (op == OP_PLUS && rhs->type == NODE_UPLOADLIST)
            {
                UploadList_ptr uploadList = static_pointer_cast<UploadList>(compbin->rhs);
                rhs = uploadList->list[0];
            }
            Binary_ptr bin = make_shared<Binary>(compbin->lhs, op, rhs);
            Assign_ptr assign = make_shared<Assign>(compbin->lhs, bin);
            bin->first_line = assign->first_line = compbin->first_line;
            bin->last_line = assign->last_line = compbin->last_line;
            return eval_stmt(assign);
        }

        return nullptr;
    }
    case NODE_DRAW:
    {
        Draw_ptr draw = static_pointer_cast<Draw>(stmt);

        if (draw->program != nullptr)
        {
            if (current_program_name != draw->program->name)
            {
                current_program_name = draw->program->name;
                current_program = static_pointer_cast<Program>(globalScope->get(current_program_name));
            }
        }

        if (current_program == nullptr)
        {
            logger->log(draw, "ERROR", "Cannot bind program with name " + current_program_name);
            return nullptr;
        }
        else
        {
            gl->glUseProgram(current_program->handle);
        }

        Expr_ptr expr = nullptr;
        get_variable(expr, draw->ident->name);
        if (expr == nullptr || expr->type != NODE_BUFFER)
        {
            logger->log(draw, "ERROR", "Can't draw non-buffer object");
            return nullptr;
        }

        Texture_ptr target = static_pointer_cast<Texture>(eval_expr(draw->target));
        if (target != nullptr)
        {
            if (target->framebuffer == 0)
            {
                gl->glGenFramebuffers(1, &(target->framebuffer));
                gl->glBindFramebuffer(GL_FRAMEBUFFER, target->framebuffer);

                if (target->handle == 0)
                {
                    //TODO: Handle resizing of screen
                    target->width = width;
                    target->height = width;

                    gl->glGenTextures(1, &(target->handle));
                    gl->glBindTexture(GL_TEXTURE_2D, target->handle);
                    gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, target->width, target->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
                    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    gl->glBindTexture(GL_TEXTURE_2D, 0);
                }
                gl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target->handle, 0);
            }
            else
            {
                gl->glBindFramebuffer(GL_FRAMEBUFFER, target->framebuffer);
            }
        }
        else
        {
            gl->glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        Buffer_ptr buffer = static_pointer_cast<Buffer>(expr);
        if (buffer != nullptr)
        {
            Layout *layout = buffer->layout;
            vector<float> final_vector;

            map<string, unsigned int> attributes = layout->attributes;
            if (attributes.size() == 0)
            {
                logger->log(draw, "ERROR", "Cannot draw empty buffer");
                return nullptr;
            }

            gl->glBindBuffer(GL_ARRAY_BUFFER, buffer->handle);
            for (unsigned int i = 0; i < buffer->sizes[layout->list[0]]; i++)
            {
                for (unsigned int j = 0; j < layout->list.size(); j++)
                {
                    string attrib = layout->list[j];
                    for (unsigned int k = 0; k < layout->attributes[attrib]; k++)
                    {
                        final_vector.push_back(buffer->data[attrib][(i * layout->attributes[attrib]) + k]);
                    }
                }
            }

            gl->glBufferData(GL_ARRAY_BUFFER, final_vector.size() * sizeof(float), &final_vector[0], GL_STATIC_DRAW);

            int total_size = 0;
            for (auto it = layout->attributes.begin(); it != layout->attributes.end(); ++it)
            {
                total_size += it->second;
            }

            int cumulative_size = 0;
            for (unsigned int i = 0; i < layout->list.size(); i++)
            {
                //FIXME: Use VAOs, handle multiple shaders
                string attrib = layout->list[i];
                GLint location = gl->glGetAttribLocation(current_program->handle, attrib.c_str());
                gl->glVertexAttribPointer(location, layout->attributes[attrib], GL_FLOAT, false, total_size * sizeof(float), (void *)(cumulative_size * sizeof(float)));
                gl->glEnableVertexAttribArray(location);

                cumulative_size += layout->attributes[attrib];
            }

            if (buffer->indices.size() > 0)
            {
                gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer->indexHandle);
                gl->glBufferData(GL_ELEMENT_ARRAY_BUFFER, buffer->indices.size() * sizeof(unsigned int), &(buffer->indices)[0], GL_STATIC_DRAW);
                gl->glDrawElements(GL_TRIANGLES, buffer->indices.size(), GL_UNSIGNED_INT, 0);
                gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            }
            else
            {
                gl->glDrawArrays(GL_TRIANGLES, 0, final_vector.size() / total_size);
            }

            if (draw->target != nullptr)
            {
                gl->glBindFramebuffer(GL_FRAMEBUFFER, 0);
            }
        }
        else
        {
            logger->log(draw, "ERROR", "Can't draw non-existent buffer " + draw->ident->name);
        }

        return nullptr;
    }
    case NODE_CLEAR:
    {
        Clear_ptr clear = static_pointer_cast<Clear>(stmt);
        if (clear->color != nullptr)
        {
            Expr_ptr color = eval_expr(clear->color);
            if (color != nullptr && color->type == NODE_VECTOR3)
            {
                Vector3_ptr v = static_pointer_cast<Vector3>(color);
                gl->glClearColor(resolve_scalar(v->x), resolve_scalar(v->y), resolve_scalar(v->z), 1.0f);
            }
            else
            {
                logger->log(clear, "ERROR", "Invalid clear color");
            }
        }
        gl->glClear(GL_COLOR_BUFFER_BIT);
        return nullptr;
    }
    case NODE_VIEWPORT:
    {
        Viewport_ptr viewport = static_pointer_cast<Viewport>(stmt);
        if (viewport->bounds != nullptr)
        {
            Expr_ptr bounds = eval_expr(viewport->bounds);
            if (bounds != nullptr && bounds->type == NODE_VECTOR4)
            {
                Vector4_ptr v = static_pointer_cast<Vector4>(bounds);
                int bounds_int[4];

                for (unsigned int i = 0; i < 4; i++)
                {
                    if (v->get(i) == nullptr || (v->get(i)->type != NODE_INT && v->get(i)->type != NODE_FLOAT))
                    {
                        logger->log(v, "ERROR", "Viewport bounds elements needs to a scalar");
                        return nullptr;
                    }
                    bounds_int[i] = (int)resolve_scalar(v->get(i));
                }

                gl->glViewport(bounds_int[0], bounds_int[1], bounds_int[2], bounds_int[3]);
            }
            else
            {
                logger->log(viewport, "ERROR", "Viewport bounds needs to be of type vec4");
            }
        }
        else
        {
            logger->log(viewport, "ERROR", "Unspecified bounds for viewport statement");
        }
        return nullptr;
    }
    case NODE_IF:
    {
        If_ptr ifstmt = static_pointer_cast<If>(stmt);
        Expr_ptr condition = eval_expr(ifstmt->condition);
        if (!condition)
            return nullptr;
        if (condition->type == NODE_BOOL)
        {
            bool b = (static_pointer_cast<Bool>(condition)->value);
            Stmts_ptr execute = nullptr;
            if (!b)
            {
                if (ifstmt->elseIfBlocks != nullptr)
                {
                    for (auto it = ifstmt->elseIfBlocks->begin(); it != ifstmt->elseIfBlocks->end(); ++it)
                    {
                        If_ptr elseIf = *it;
                        bool eb = static_pointer_cast<Bool>(eval_expr(elseIf->condition))->value;
                        if (eb)
                        {
                            execute = elseIf->block;
                            break;
                        }
                    }
                }
                if (execute == nullptr)
                {
                    execute = ifstmt->elseBlock;
                }
            }
            else
            {
                execute = ifstmt->block;
            }
            if (execute != nullptr)
            {
                functionScopeStack.top()->attach("if");
                Expr_ptr returnValue = execute_stmts(execute);
                functionScopeStack.top()->detach();
                if (returnValue != nullptr)
                {
                    return returnValue;
                }
            }
        }
        else
        {
            logger->log(ifstmt, "ERROR", "Condition in if statement not a boolean");
        }
        return nullptr;
    }
    case NODE_WHILE:
    {
        While_ptr whilestmt = static_pointer_cast<While>(stmt);
        Expr_ptr condition = eval_expr(whilestmt->condition);
        if (!condition)
            return nullptr;
        if (condition->type == NODE_BOOL)
        {
            time_t start = time(nullptr);
            functionScopeStack.top()->attach("while");
            breakable = true;
            while (true)
            {
                condition = eval_expr(whilestmt->condition);
                bool b = (static_pointer_cast<Bool>(condition)->value);
                if (!b)
                    break;

                Expr_ptr returnValue = execute_stmts(whilestmt->block);
                if (returnValue != nullptr)
                {
                    breakable = false;
                    return returnValue;
                }

                time_t now = time(nullptr);

                int diff = difftime(now, start);
                if (diff > LOOP_TIMEOUT)
                {
                    break;
                }
            }
            breakable = false;
            functionScopeStack.top()->detach();
        }
        else
        {
            logger->log(whilestmt, "ERROR", "Condition in while statement not a boolean");
        }

        return nullptr;
    }
    case NODE_FOR:
    {
        For_ptr forstmt = static_pointer_cast<For>(stmt);
        Ident_ptr iterator = forstmt->iterator;
        Expr_ptr start = eval_expr(forstmt->start), end = eval_expr(forstmt->end), increment = eval_expr(forstmt->increment);
        Expr_ptr list = eval_expr(forstmt->list);
        if (list == nullptr)
        {
            if (start->type == NODE_INT || end->type == NODE_INT || increment->type == NODE_INT)
            {
                functionScopeStack.top()->attach("for");
                eval_stmt(make_shared<Decl>(make_shared<Ident>("int"), iterator, start));
                time_t start = time(nullptr);

                breakable = true;
                while (true)
                {
                    Bool_ptr terminate = static_pointer_cast<Bool>(eval_binary(make_shared<Binary>(iterator, OP_EQUAL, end)));
                    if (terminate == nullptr || terminate->value)
                    {
                        break;
                    }

                    Expr_ptr returnValue = execute_stmts(forstmt->block);
                    if (returnValue != nullptr)
                    {
                        breakable = false;
                        return returnValue;
                    }

                    eval_stmt(make_shared<Assign>(iterator, make_shared<Binary>(iterator, OP_PLUS, increment)));

                    time_t now = time(nullptr);
                    int diff = difftime(now, start);
                    if (diff > LOOP_TIMEOUT)
                    {
                        break;
                    }
                }
                breakable = false;
                functionScopeStack.top()->detach();
            }
        }
        else if (list->type == NODE_LIST)
        {
            List_ptr lst = static_pointer_cast<List>(list);
            functionScopeStack.top()->attach("for");
            eval_stmt(make_shared<Decl>(make_shared<Ident>("var"), iterator, nullptr));
            unsigned int i = 0;

            time_t start = time(nullptr);

            breakable = true;
            while (i < lst->list.size())
            {
                eval_stmt(make_shared<Assign>(iterator, lst->list[i]));

                Expr_ptr returnValue = execute_stmts(forstmt->block);
                if (returnValue != nullptr)
                {
                    return returnValue;
                }

                i++;

                time_t now = time(nullptr);
                int diff = difftime(now, start);
                if (diff > LOOP_TIMEOUT)
                {
                    break;
                }
            }
            breakable = false;
            functionScopeStack.top()->detach();
        }

        return nullptr;
    }
    case NODE_PRINT:
    {
        Print_ptr print = static_pointer_cast<Print>(stmt);
        Expr_ptr output = eval_expr(print->expr);
        if (output == nullptr)
            return nullptr;

        logger->log(print_expr(output));
        return nullptr;
    }

    default:
        return nullptr;
    }
}

Expr_ptr Wyatt::Interpreter::execute_stmts(Stmts_ptr stmts)
{
    Expr_ptr returnValue = nullptr;
    for (unsigned int it = 0; it < stmts->list.size(); it++)
    {
        Stmt_ptr stmt = stmts->list.at(it);
        if (stmt->type == NODE_BREAK && breakable)
        {
            //return null_expr to break
            returnValue = null_expr;
            break;
        }

        if (stmt->type == NODE_RETURN)
        {
            Return_ptr ret = static_pointer_cast<Return>(stmt);
            returnValue = ret->value;
            break;
        }

        Expr_ptr expr = eval_stmt(stmt);
        if (expr != nullptr)
        {
            returnValue = expr;
            break;
        }
    }

    if (returnValue != nullptr)
    {
        return eval_expr(returnValue);
    }
    else
    {
        return nullptr;
    }
}

void Wyatt::Interpreter::compile_shader(GLuint *handle, Shader_ptr shader)
{
    transpiler->layouts = &layouts;
    string code = transpiler->transpile(shader);
    cout << code << endl;
    const char *src = code.c_str();
    gl->glShaderSource(*handle, 1, &src, nullptr);
    gl->glCompileShader(*handle);

    GLint success;
    char log[256];

    gl->glGetShaderInfoLog(*handle, 256, 0, log);
    gl->glGetShaderiv(*handle, GL_COMPILE_STATUS, &success);
    if (success != GL_TRUE)
    {
        cout << shader->name << " shader error\n"
             << success << " " << log << endl;
    }
}

void Wyatt::Interpreter::compile_program()
{
    for (auto it = shaders.begin(); it != shaders.end(); ++it)
    {
        Program_ptr program = make_shared<Program>();
        program->handle = gl->glCreateProgram();
        program->vert = gl->glCreateShader(GL_VERTEX_SHADER);
        program->frag = gl->glCreateShader(GL_FRAGMENT_SHADER);

        gl->glAttachShader(program->handle, program->vert);
        gl->glAttachShader(program->handle, program->frag);

        program->vertSource = it->second->vertex;
        program->fragSource = it->second->fragment;

        if (program->vertSource == nullptr)
        {
            logger->log("ERROR: Missing vertex shader source for program " + it->first);
            continue;
        }

        if (program->fragSource == nullptr)
        {
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
        if (success != GL_TRUE)
        {
            logger->log("ERROR: Can't compile program-- see error log below for details");
            logger->log(string(log));
        }

        globalScope->declare(nullptr, make_shared<Ident>(program->vertSource->name), "program", program);
    }
}

void Wyatt::Interpreter::execute_init()
{
    if (!init || status)
        return;

    Decl_ptr piDecl = make_shared<Decl>(make_shared<Ident>("float"), make_shared<Ident>("PI"), make_shared<Float>(3.14159f));
    piDecl->constant = true;

    Decl_ptr widthDecl = make_shared<Decl>(make_shared<Ident>("int"), make_shared<Ident>("WIDTH"), make_shared<Int>(width));
    widthDecl->constant = true;

    Decl_ptr heightDecl = make_shared<Decl>(make_shared<Ident>("int"), make_shared<Ident>("HEIGHT"), make_shared<Int>(height));
    heightDecl->constant = true;

    globals.insert(globals.begin(), piDecl);
    globals.insert(globals.begin(), widthDecl);
    globals.insert(globals.begin(), heightDecl);

    for (auto it = globals.begin(); it != globals.end(); ++it)
    {
        Decl_ptr decl = *it;
        eval_stmt(decl);
    }

    invoke(init_invoke);
}

void Wyatt::Interpreter::execute_loop()
{
    if (!loop || status)
        return;

    invoke(loop_invoke);
}

void Wyatt::Interpreter::load_imports()
{
    for (unsigned int i = 0; i < imports.size(); i++)
    {
        string file = imports[i];
        load_import(file);
    }
}

void Wyatt::Interpreter::load_import(string file)
{
    string src = "";
    if (file_exists(workingDir + "/" + file))
    {
        src = str_from_file(workingDir + "/" + file);
    }
    else
    {
        src = str_from_file(file);
    }
    int import_status = 0;
    parse(src, &import_status);
    if (import_status != 0)
    {
        logger->log("Error importing " + file);
    }
}

void Wyatt::Interpreter::parse(string code, int *status)
{
    logger->clear();
    istringstream ss(code);
    *(scanner.line) = 1;
    *(scanner.column) = 1;
    scanner.switch_streams(&ss, nullptr);
    *status = parser.parse();
}

void Wyatt::Interpreter::prepare()
{
    globalScope->clear();

    if (status == 0)
    {
        CodeEditor::autocomplete_functions = functions;
    }

    init = functions["init"];
    if (init == nullptr)
    {
        logger->log("WARNING: No init function detected");
    }

    loop = functions["loop"];
    if (loop == nullptr)
    {
        logger->log("WARNING: No loop function detected");
    }
}

void Wyatt::Interpreter::resize(int width, int height)
{
    this->width = width;
    this->height = height;

    globalScope->fast_assign("WIDTH", make_shared<Int>(width));
    globalScope->fast_assign("HEIGHT", make_shared<Int>(height));
}
