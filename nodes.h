#ifndef NODES_H
#define NODES_H

#include <string>
#include <vector>
#include <map>
#include <regex>
#include <iostream>

using namespace std;

enum NodeType {
    NODE_INVOKE,
    NODE_EXPR, NODE_BINARY, NODE_UNARY, NODE_BOOL, NODE_INT, NODE_FLOAT, NODE_STRING, NODE_VECTOR2, NODE_VECTOR3, NODE_VECTOR4, NODE_MATRIX2, NODE_MATRIX3, NODE_MATRIX4, NODE_IDENT, NODE_DOT,
    NODE_UPLOADLIST, NODE_FUNCEXPR, NODE_LIST, NODE_ARGLIST, NODE_PARAMLIST, NODE_INDEX,
    NODE_STMT, NODE_ASSIGN, NODE_DECL, NODE_ALLOC, NODE_UPLOAD, NODE_DRAW, NODE_USE, NODE_FUNCSTMT, NODE_STMTS, NODE_IF, NODE_WHILE, NODE_FOR, NODE_SSOURCE, NODE_PRINT, NODE_FUNCDEF, NODE_RETURN
};

inline string type_to_name(NodeType type) {
    switch(type) {
        case NODE_BOOL: return "bool";
        case NODE_INT: return "int";
        case NODE_FLOAT: return "float";
        case NODE_VECTOR2: return "vec2";
        case NODE_VECTOR3: return "vec3";
        case NODE_VECTOR4: return "vec4";
        case NODE_MATRIX2: return "mat2";
        case NODE_MATRIX3: return "mat3";
        case NODE_MATRIX4: return "mat4";
        default: return "";
    }
}

enum OpType {
    OP_PLUS, OP_MINUS, OP_MULT, OP_DIV, OP_MOD, OP_ABS, OP_AND, OP_OR, OP_NOT, OP_EQUAL, OP_LESSTHAN, OP_GREATERTHAN, OP_NEQUAL, OP_LEQUAL, OP_GEQUAL
};

class Node;
class Expr;
class Ident;
class Uniform;
class Binary;
class Unary;
class Bool;
class Int;
class Float;
class String;
class Vector2;
class Vector3;
class Vector4;
class Matrix2;
class Matrix3;
class Matrix4;

typedef shared_ptr<Node> node_ptr;
typedef shared_ptr<Expr> expr_ptr;
typedef shared_ptr<Ident> ident_ptr;
typedef shared_ptr<Uniform> uniform_ptr;
typedef shared_ptr<Binary> binary_ptr;
typedef shared_ptr<Unary> unary_ptr;
typedef shared_ptr<Bool> bool_ptr;
typedef shared_ptr<Int> int_ptr;
typedef shared_ptr<Float> float_ptr;
typedef shared_ptr<String> string_ptr;
typedef shared_ptr<Vector2> vector2_ptr;
typedef shared_ptr<Vector3> vector3_ptr;
typedef shared_ptr<Vector4> vector4_ptr;
typedef shared_ptr<Matrix2> matrix2_ptr;
typedef shared_ptr<Matrix3> matrix3_ptr;
typedef shared_ptr<Matrix4> matrix4_ptr;

class Node {
    public:
        unsigned int first_line, last_line;
        unsigned int first_column, last_column;
        NodeType type;
};

class Expr: public Node {
    public:
        Expr() {
            type = NODE_EXPR;
        }
};

class Ident: public Expr {
    public:
        string name;

        Ident(string name) {
            this->name = name;
            type = NODE_IDENT;
        }
};

class Dot: public Ident {
    public:
        string shader;

        Dot(shared_ptr<Ident> shader, shared_ptr<Ident> name): Ident(name->name) {
            this->shader = shader->name;
            type = NODE_DOT;
        }
};

class Binary: public Expr {
    public:
        OpType op;
        shared_ptr<Expr> lhs, rhs;

        Binary(shared_ptr<Expr> lhs, OpType op, shared_ptr<Expr> rhs) {
            this->op = op;
            this->lhs = lhs;
            this->rhs = rhs;
            type = NODE_BINARY; 
        }
};

class Unary: public Expr {
    public:
        OpType op;
        shared_ptr<Expr> rhs;

        Unary(OpType op, shared_ptr<Expr> rhs) {
            this->op = op;
            this->rhs = rhs;
            type = NODE_UNARY;
        }
};

class Bool: public Expr {
    public:
        bool value;

        Bool(bool value) {
            this->value = value;
            type = NODE_BOOL;
        }
};

class Int: public Expr {
    public:
        int value;

        Int(int value) {
            this->value = value;
            type = NODE_INT; 
        }
};

class Float: public Expr {
    public:
        float value;

        Float(float value) {
            this->value = value;
            type = NODE_FLOAT; 
        }
};

class String: public Expr {
    public:
        string value;

        String(string value) {
            this->value = value;
            type = NODE_STRING;
        }
};

class Vector2: public Expr {
    public:
        shared_ptr<Expr> x, y;

        Vector2(shared_ptr<Expr> x, shared_ptr<Expr> y) {
            this->x = x;
            this->y = y;
            type = NODE_VECTOR2;
        }
};

class Vector3: public Expr {
    public:
        shared_ptr<Expr> x, y, z;

        Vector3(shared_ptr<Expr> x, shared_ptr<Expr> y, shared_ptr<Expr> z) {
            this->x = x;
            this->y = y;
            this->z = z;
            type = NODE_VECTOR3; 
        }
};

class Vector4: public Expr {
    public:
        shared_ptr<Expr> x, y, z, w;

        Vector4(shared_ptr<Expr> x, shared_ptr<Expr> y, shared_ptr<Expr> z, shared_ptr<Expr> w) {
            this->x = x;
            this->y = y;
            this->z = z;
            this->w = w;
            type = NODE_VECTOR4;
        }
};

class Matrix2: public Expr {
    public:
        shared_ptr<Vector2> v0, v1;
        shared_ptr<Vector2> c0, c1;

        Matrix2(shared_ptr<Vector2> v0, shared_ptr<Vector2> v1): v0(v0), v1(v1) {
            type = NODE_MATRIX2;
            generate_columns();
        }

        void generate_columns() {
            c0 = make_shared<Vector2>(shared_ptr<Expr>(v0->x), shared_ptr<Expr>(v1->x));
            c1 = make_shared<Vector2>(shared_ptr<Expr>(v0->y), shared_ptr<Expr>(v1->y));
        }
};

class Matrix3: public Expr {
    public:
        shared_ptr<Vector3> v0, v1, v2;
        shared_ptr<Vector3> c0, c1, c2;

        Matrix3(shared_ptr<Vector3> v0, shared_ptr<Vector3> v1, shared_ptr<Vector3> v2): v0(v0), v1(v1), v2(v2) {
            type = NODE_MATRIX3;

            generate_columns();
        }

        void generate_columns() {
            c0 = make_shared<Vector3>(shared_ptr<Expr>(v0->x), shared_ptr<Expr>(v1->x), shared_ptr<Expr>(v2->x));
            c1 = make_shared<Vector3>(shared_ptr<Expr>(v0->y), shared_ptr<Expr>(v1->y), shared_ptr<Expr>(v2->y));
            c2 = make_shared<Vector3>(shared_ptr<Expr>(v0->z), shared_ptr<Expr>(v1->z), shared_ptr<Expr>(v2->z));
        }
};

class Matrix4: public Expr {
    public:
        shared_ptr<Vector4> v0, v1, v2, v3;
        shared_ptr<Vector4> c0, c1, c2, c3;

        Matrix4(shared_ptr<Vector4> v0, shared_ptr<Vector4> v1, shared_ptr<Vector4> v2, shared_ptr<Vector4> v3): v0(v0), v1(v1), v2(v2), v3(v3) {
            type = NODE_MATRIX4;
            generate_columns();
        }

        void generate_columns() {
            c0 = make_shared<Vector4>(shared_ptr<Expr>(v0->x), shared_ptr<Expr>(v1->x), shared_ptr<Expr>(v2->x), shared_ptr<Expr>(v3->x));
            c1 = make_shared<Vector4>(shared_ptr<Expr>(v0->y), shared_ptr<Expr>(v1->y), shared_ptr<Expr>(v2->y), shared_ptr<Expr>(v3->y));
            c2 = make_shared<Vector4>(shared_ptr<Expr>(v0->z), shared_ptr<Expr>(v1->z), shared_ptr<Expr>(v2->z), shared_ptr<Expr>(v3->z));
            c3 = make_shared<Vector4>(shared_ptr<Expr>(v0->w), shared_ptr<Expr>(v1->w), shared_ptr<Expr>(v2->w), shared_ptr<Expr>(v3->w));
        }
};

class Index: public Expr {
    public:
        shared_ptr<Expr> source;
        shared_ptr<Expr> index;

        Index(shared_ptr<Expr> source, shared_ptr<Expr> index) {
            this->source = source;
            this->index = index;
            type = NODE_INDEX;
        }
};

class Stmt: public Node {
    public:
        Stmt() {
            type = NODE_STMT;
        }
};

class Stmts: public Node {
    public:
        vector<shared_ptr<Stmt>> list;

        Stmts(shared_ptr<Stmt> init) {
            if(init) list.insert(list.begin(), init);
            type = NODE_STMTS;
        }
};

class List: public Expr {
    public:
        vector<shared_ptr<Expr>> list;

        List(shared_ptr<Expr> init) {
            if(init) list.insert(list.begin(), init);
            type = NODE_LIST;
        }
};

class ArgList: public Node {
    public:
        vector<shared_ptr<Expr>> list;

        ArgList(shared_ptr<Expr> init) {
            if(init) list.push_back(init);
            type = NODE_ARGLIST;
        }
};

class ParamList: public Node {
    public:
        vector<shared_ptr<Ident>> list;

        ParamList(shared_ptr<Ident> init) {
            if(init) list.push_back(init);
            type = NODE_PARAMLIST;
        }
};

class Return: public Stmt {
    public:
        shared_ptr<Expr> value;

        Return(shared_ptr<Expr> value) {
            this->value = value;
            type = NODE_RETURN;
        }
};

class FuncDef: public Stmt {
    public:
        shared_ptr<Ident> ident;
        shared_ptr<ParamList> params;
        shared_ptr<Stmts> stmts;

        FuncDef(shared_ptr<Ident> ident, shared_ptr<ParamList> params, shared_ptr<Stmts> stmts) {
            this->ident = ident;
            this->params = params;
            this->stmts = stmts;
            type = NODE_FUNCDEF;
        }
};

class If: public Stmt {
    public:
        shared_ptr<Expr> condition;
        shared_ptr<Stmts> block;

        If(shared_ptr<Expr> condition, shared_ptr<Stmts> block) {
            this->condition = condition;
            this->block = block;
            type = NODE_IF;
        }
};

class While: public Stmt {
    public:
        shared_ptr<Expr> condition;
        shared_ptr<Stmts> block;

        While(shared_ptr<Expr> condition, shared_ptr<Stmts> block) {
            this->condition = condition;
            this->block = block;
            type = NODE_WHILE;
        }
}; 

class For: public Stmt {
    public:
        shared_ptr<Ident> iterator;
        shared_ptr<Expr> start, end, increment;
        shared_ptr<Stmts> block;

        For(shared_ptr<Ident> iterator, shared_ptr<Expr> start, shared_ptr<Expr> end, shared_ptr<Expr> increment, shared_ptr<Stmts> block) {
            this->iterator = iterator;
            this->start = start;
            this->end = end;
            this->increment = increment;
            this->block = block;
            type = NODE_FOR;
        }
};

class Assign: public Stmt {
    public:
        shared_ptr<Expr> lhs;
        shared_ptr<Expr> value;

        Assign(shared_ptr<Expr> ident, shared_ptr<Expr> value) {
            this->lhs = ident;
            this->value = value;
            type = NODE_ASSIGN;
        }
};

class Decl: public Stmt {
    public:
        shared_ptr<Ident> datatype, name;
        shared_ptr<Expr> value;

        Decl(shared_ptr<Ident> datatype, shared_ptr<Ident> name, shared_ptr<Expr> value) {
            this->datatype= datatype;
            this->name = name;
            this->value = value;
            type = NODE_DECL;
        }
};

class Alloc: public Stmt {
    public:
        shared_ptr<Ident> ident;

        Alloc(shared_ptr<Ident> ident) {
            this->ident = ident;
            type = NODE_ALLOC;
        }
};

class UploadList: public Expr {
    public:
        vector<shared_ptr<Expr>> list;

        UploadList(shared_ptr<Expr> init) {
            list.insert(list.begin(), init);
            type = NODE_UPLOADLIST;
        }
};

class Upload: public Stmt {
    public:
        shared_ptr<Ident> ident;
        shared_ptr<Ident> attrib;
        shared_ptr<UploadList> list;

        Upload(shared_ptr<Ident> ident, shared_ptr<Ident> attrib, shared_ptr<UploadList> list) {
            type = NODE_UPLOAD;
            this->ident = ident;
            this->attrib = attrib;
            this->list = list;
        }
};

class Draw: public Stmt {
    public:
        shared_ptr<Ident> ident;

        Draw(shared_ptr<Ident> ident) {
            this->ident = ident;
            type = NODE_DRAW;
        }
};

class Use: public Stmt {
    public:
        shared_ptr<Ident> ident;

        Use(shared_ptr<Ident> ident) {
            this->ident = ident;
            type = NODE_USE;
        }
};

class Invoke: public Node {
    public:
        shared_ptr<Ident> ident;
        shared_ptr<ArgList> args;

        Invoke(shared_ptr<Ident> ident, shared_ptr<ArgList> args) {
            this->ident = ident;
            this->args = args;
            type = NODE_INVOKE;
        }
};

class FuncExpr: public Expr {
    public:
        shared_ptr<Invoke> invoke;

        FuncExpr(shared_ptr<Invoke> invoke) {
            this->invoke = invoke;
            type = NODE_FUNCEXPR;
        }
};

class FuncStmt: public Stmt {
    public:
        shared_ptr<Invoke> invoke;

        FuncStmt(shared_ptr<Invoke> invoke) {
            this->invoke = invoke;
            type = NODE_FUNCSTMT;
        }
};


class ShaderSource: public Node {
    public:
        string name;
        string code;
        string shader_type;

        map<string, vector<string>> inputs;
        map<string, vector<string>> outputs;

        map<string, string> uniforms;

        regex uniform_regex = regex("uniform\\s(\\w+)\\s(.*);");
        regex sub_regex = regex("(\\w+)");

        ShaderSource(string name, string code, string shader_type) {
            this->name = name;
            this->code = code;
            this->shader_type = shader_type;
            type = NODE_SSOURCE;

            sregex_iterator uniform_begin = sregex_iterator(this->code.begin(), this->code.end(), uniform_regex), uniform_end = sregex_iterator();

            for(sregex_iterator i = uniform_begin; i != uniform_end; ++i) {
                smatch match = *i;
                string type = "";
                for(unsigned int j = 0; j < match.size(); j++) {
                    string group = match[j];
                    if(j == 1) {
                        type = group;
                    }
                    if(j == 2) {
                        sregex_iterator sub_begin = sregex_iterator(group.begin(), group.end(), sub_regex), sub_end = sregex_iterator();

                        for(sregex_iterator k = sub_begin; k != sub_end; ++k) {
                            uniforms[(*k)[0]] = type;
                        }
                    }
                }
            }
        }
};

struct ShaderPair {
    std::string name;
    shared_ptr<ShaderSource> vertex = NULL;
    shared_ptr<ShaderSource> fragment = NULL;
};

class Print: public Stmt {
    public:
        shared_ptr<Expr> expr;

        Print(shared_ptr<Expr> expr) {
            this->expr = expr;
            type = NODE_PRINT;
        }
};

#endif // NODES_H
