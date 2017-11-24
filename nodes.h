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
    NODE_EXPR, NODE_NULL, NODE_BINARY, NODE_UNARY, NODE_BOOL, NODE_INT, NODE_FLOAT, NODE_STRING, NODE_VECTOR2, NODE_VECTOR3, NODE_VECTOR4, NODE_MATRIX2, NODE_MATRIX3, NODE_MATRIX4, NODE_IDENT, NODE_DOT,
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
class Dot;
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
class Index;
class Stmt;
class Stmts;
class List;
class ArgList;
class ParamList;
class Return;
class FuncDef;
class If;
class While;
class For;
class Assign;
class Decl;
class Alloc;
class UploadList;
class Upload;
class Draw;
class Use;
class Print;
class Invoke;
class FuncExpr;
class FuncStmt;
class ShaderSource;
struct ShaderPair;

typedef shared_ptr<Node> Node_ptr;
typedef shared_ptr<Expr> Expr_ptr;
typedef shared_ptr<Ident> Ident_ptr;
typedef shared_ptr<Dot> Dot_ptr;
typedef shared_ptr<Binary> Binary_ptr;
typedef shared_ptr<Unary> Unary_ptr;
typedef shared_ptr<Bool> Bool_ptr;
typedef shared_ptr<Int> Int_ptr;
typedef shared_ptr<Float> Float_ptr;
typedef shared_ptr<String> String_ptr;
typedef shared_ptr<Vector2> Vector2_ptr;
typedef shared_ptr<Vector3> Vector3_ptr;
typedef shared_ptr<Vector4> Vector4_ptr;
typedef shared_ptr<Matrix2> Matrix2_ptr;
typedef shared_ptr<Matrix3> Matrix3_ptr;
typedef shared_ptr<Matrix4> Matrix4_ptr;
typedef shared_ptr<Index> Index_ptr;
typedef shared_ptr<Stmt> Stmt_ptr;
typedef shared_ptr<Stmts> Stmts_ptr;
typedef shared_ptr<List> List_ptr;
typedef shared_ptr<ArgList> ArgList_ptr;
typedef shared_ptr<ParamList> ParamList_ptr;
typedef shared_ptr<Return> Return_ptr;
typedef shared_ptr<FuncDef> FuncDef_ptr;
typedef shared_ptr<If> If_ptr;
typedef shared_ptr<While> While_ptr;
typedef shared_ptr<For> For_ptr;
typedef shared_ptr<Assign> Assign_ptr;
typedef shared_ptr<Decl> Decl_ptr;
typedef shared_ptr<Alloc> Alloc_ptr;
typedef shared_ptr<UploadList> UploadList_ptr;
typedef shared_ptr<Upload> Upload_ptr;
typedef shared_ptr<Draw> Draw_ptr;
typedef shared_ptr<Use> Use_ptr;
typedef shared_ptr<Print> Print_ptr;
typedef shared_ptr<Invoke> Invoke_ptr;
typedef shared_ptr<FuncExpr> FuncExpr_ptr;
typedef shared_ptr<FuncStmt> FuncStmt_ptr;
typedef shared_ptr<ShaderSource> ShaderSource_ptr;
typedef shared_ptr<ShaderPair> ShaderPair_ptr;

#define null_expr make_shared<Expr>(NODE_NULL)

class Node {
    public:
        unsigned int first_line, last_line;
        unsigned int first_column, last_column;
        NodeType type;

        Node(NodeType type): type(type) {}
};

class Expr: public Node {
    public:
        Expr(): Node(NODE_EXPR) {}
        Expr(NodeType type): Node(type) {}
};

class Ident: public Expr {
    public:
        string name;

        Ident(string name): Expr(NODE_IDENT) {
            this->name = name;
        }
};

class Dot: public Ident {
    public:
        string shader;

        Dot(Ident_ptr shader, Ident_ptr name): Ident(name->name) {
            this->shader = shader->name;
            type = NODE_DOT;
        }
};

class Binary: public Expr {
    public:
        OpType op;
        Expr_ptr lhs, rhs;

        Binary(Expr_ptr lhs, OpType op, Expr_ptr rhs): Expr(NODE_BINARY) {
            this->op = op;
            this->lhs = lhs;
            this->rhs = rhs;
        }
};

class Unary: public Expr {
    public:
        OpType op;
        Expr_ptr rhs;

        Unary(OpType op, Expr_ptr rhs): Expr(NODE_UNARY) {
            this->op = op;
            this->rhs = rhs;
        }
};

class Bool: public Expr {
    public:
        bool value;

        Bool(bool value): Expr(NODE_BOOL) {
            this->value = value;
        }
};

class Int: public Expr {
    public:
        int value;

        Int(int value): Expr(NODE_INT) {
            this->value = value;
        }
};

class Float: public Expr {
    public:
        float value;

        Float(float value): Expr(NODE_FLOAT) {
            this->value = value;
        }
};

class String: public Expr {
    public:
        string value;

        String(string value): Expr(NODE_STRING) {
            this->value = value;
        }
};

class Vector2: public Expr {
    public:
        Expr_ptr x, y;

        Vector2(Expr_ptr x, Expr_ptr y): Expr(NODE_VECTOR2) {
            this->x = x;
            this->y = y;
        }
};

class Vector3: public Expr {
    public:
        Expr_ptr x, y, z;

        Vector3(Expr_ptr x, Expr_ptr y, Expr_ptr z): Expr(NODE_VECTOR3) {
            this->x = x;
            this->y = y;
            this->z = z;
        }
};

class Vector4: public Expr {
    public:
        Expr_ptr x, y, z, w;

        Vector4(Expr_ptr x, Expr_ptr y, Expr_ptr z, Expr_ptr w): Expr(NODE_VECTOR4) {
            this->x = x;
            this->y = y;
            this->z = z;
            this->w = w;
        }
};

class Matrix2: public Expr {
    public:
        Vector2_ptr v0, v1;
        Vector2_ptr c0, c1;

        Matrix2(Vector2_ptr v0, Vector2_ptr v1): Expr(NODE_MATRIX2), v0(v0), v1(v1) {
            generate_columns();
        }

        void generate_columns() {
            c0 = make_shared<Vector2>(Expr_ptr(v0->x), Expr_ptr(v1->x));
            c1 = make_shared<Vector2>(Expr_ptr(v0->y), Expr_ptr(v1->y));
        }
};

class Matrix3: public Expr {
    public:
        Vector3_ptr v0, v1, v2;
        Vector3_ptr c0, c1, c2;

        Matrix3(Vector3_ptr v0, Vector3_ptr v1, Vector3_ptr v2): Expr(NODE_MATRIX3), v0(v0), v1(v1), v2(v2) {
            generate_columns();
        }

        void generate_columns() {
            c0 = make_shared<Vector3>(Expr_ptr(v0->x), Expr_ptr(v1->x), Expr_ptr(v2->x));
            c1 = make_shared<Vector3>(Expr_ptr(v0->y), Expr_ptr(v1->y), Expr_ptr(v2->y));
            c2 = make_shared<Vector3>(Expr_ptr(v0->z), Expr_ptr(v1->z), Expr_ptr(v2->z));
        }
};

class Matrix4: public Expr {
    public:
        Vector4_ptr v0, v1, v2, v3;
        Vector4_ptr c0, c1, c2, c3;

        Matrix4(Vector4_ptr v0, Vector4_ptr v1, Vector4_ptr v2, Vector4_ptr v3): Expr(NODE_MATRIX4), v0(v0), v1(v1), v2(v2), v3(v3) {
            generate_columns();
        }

        void generate_columns() {
            c0 = make_shared<Vector4>(Expr_ptr(v0->x), Expr_ptr(v1->x), Expr_ptr(v2->x), Expr_ptr(v3->x));
            c1 = make_shared<Vector4>(Expr_ptr(v0->y), Expr_ptr(v1->y), Expr_ptr(v2->y), Expr_ptr(v3->y));
            c2 = make_shared<Vector4>(Expr_ptr(v0->z), Expr_ptr(v1->z), Expr_ptr(v2->z), Expr_ptr(v3->z));
            c3 = make_shared<Vector4>(Expr_ptr(v0->w), Expr_ptr(v1->w), Expr_ptr(v2->w), Expr_ptr(v3->w));
        }
};

class Index: public Expr {
    public:
        Expr_ptr source;
        Expr_ptr index;

        Index(Expr_ptr source, Expr_ptr index): Expr(NODE_INDEX) {
            this->source = source;
            this->index = index;
        }
};

class Stmt: public Node {
    public:
        Stmt(): Node(NODE_STMT) {}
        Stmt(NodeType type): Node(type) {}
};

class Stmts: public Node {
    public:
        vector<Stmt_ptr> list;

        Stmts(Stmt_ptr init): Node(NODE_STMTS) {
            if(init) list.insert(list.begin(), init);
        }
};

class List: public Expr {
    public:
        vector<Expr_ptr> list;

        List(Expr_ptr init): Expr(NODE_LIST) {
            if(init) list.insert(list.begin(), init);
        }
};

class ArgList: public Node {
    public:
        vector<Expr_ptr> list;

        ArgList(Expr_ptr init): Node(NODE_ARGLIST) {
            if(init) list.push_back(init);
        }
};

class ParamList: public Node {
    public:
        vector<Ident_ptr> list;

        ParamList(Ident_ptr init): Node(NODE_PARAMLIST) {
            if(init) list.push_back(init);
        }
};

class Return: public Stmt {
    public:
        Expr_ptr value;

        Return(Expr_ptr value): Stmt(NODE_RETURN) {
            this->value = value;
        }
};

class FuncDef: public Stmt {
    public:
        Ident_ptr ident;
        ParamList_ptr params;
        Stmts_ptr stmts;

        FuncDef(Ident_ptr ident, ParamList_ptr params, Stmts_ptr stmts): Stmt(NODE_FUNCDEF) {
            this->ident = ident;
            this->params = params;
            this->stmts = stmts;
        }
};

class If: public Stmt {
    public:
        Expr_ptr condition;
        Stmts_ptr block;

        If(Expr_ptr condition, Stmts_ptr block): Stmt(NODE_IF) {
            this->condition = condition;
            this->block = block;
        }
};

class While: public Stmt {
    public:
        Expr_ptr condition;
        Stmts_ptr block;

        While(Expr_ptr condition, Stmts_ptr block): Stmt(NODE_WHILE) {
            this->condition = condition;
            this->block = block;
        }
}; 

class For: public Stmt {
    public:
        Ident_ptr iterator;
        Expr_ptr start, end, increment;
        Stmts_ptr block;

        For(Ident_ptr iterator, Expr_ptr start, Expr_ptr end, Expr_ptr increment, Stmts_ptr block): Stmt(NODE_FOR) {
            this->iterator = iterator;
            this->start = start;
            this->end = end;
            this->increment = increment;
            this->block = block;
        }
};

class Assign: public Stmt {
    public:
        Expr_ptr lhs;
        Expr_ptr value;

        Assign(Expr_ptr ident, Expr_ptr value): Stmt(NODE_ASSIGN) {
            this->lhs = ident;
            this->value = value;
        }
};

class Decl: public Stmt {
    public:
        Ident_ptr datatype, name;
        Expr_ptr value;

        Decl(Ident_ptr datatype, Ident_ptr name, Expr_ptr value): Stmt(NODE_DECL) {
            this->datatype= datatype;
            this->name = name;
            this->value = value;
        }
};

class Alloc: public Stmt {
    public:
        Ident_ptr ident;

        Alloc(Ident_ptr ident): Stmt(NODE_ALLOC) {
            this->ident = ident;
        }
};

class UploadList: public Expr {
    public:
        vector<Expr_ptr> list;

        UploadList(Expr_ptr init): Expr(NODE_UPLOADLIST) {
            list.insert(list.begin(), init);
        }
};

class Upload: public Stmt {
    public:
        Ident_ptr ident;
        Ident_ptr attrib;
        UploadList_ptr list;

        Upload(Ident_ptr ident, Ident_ptr attrib, UploadList_ptr list): Stmt(NODE_UPLOAD) {
            this->ident = ident;
            this->attrib = attrib;
            this->list = list;
        }
};

class Draw: public Stmt {
    public:
        Ident_ptr ident;

        Draw(Ident_ptr ident): Stmt(NODE_DRAW) {
            this->ident = ident;
        }
};

class Use: public Stmt {
    public:
        Ident_ptr ident;

        Use(Ident_ptr ident): Stmt(NODE_USE) {
            this->ident = ident;
        }
};

class Print: public Stmt {
    public:
        Expr_ptr expr;

        Print(Expr_ptr expr): Stmt(NODE_PRINT) {
            this->expr = expr;
        }
};

class Invoke: public Node {
    public:
        Ident_ptr ident;
        ArgList_ptr args;

        Invoke(Ident_ptr ident, ArgList_ptr args): Node(NODE_INVOKE) {
            this->ident = ident;
            this->args = args;
        }
};

class FuncExpr: public Expr {
    public:
        Invoke_ptr invoke;

        FuncExpr(Invoke_ptr invoke): Expr(NODE_FUNCEXPR) {
            this->invoke = invoke;
        }
};

class FuncStmt: public Stmt {
    public:
        Invoke_ptr invoke;

        FuncStmt(Invoke_ptr invoke): Stmt(NODE_FUNCSTMT) {
            this->invoke = invoke;
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

        ShaderSource(string name, string code, string shader_type): Node(NODE_SSOURCE) {
            this->name = name;
            this->code = code;
            this->shader_type = shader_type;

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
    ShaderSource_ptr vertex = NULL;
    ShaderSource_ptr fragment = NULL;
};

#endif // NODES_H
