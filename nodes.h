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
class Invoke;
class FuncExpr;
class FuncStmt;
class ShaderSource;
struct ShaderPair;
class Print;

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
typedef shared_ptr<Invoke> Invoke_ptr;
typedef shared_ptr<FuncExpr> FuncExpr_ptr;
typedef shared_ptr<FuncStmt> FuncStmt_ptr;
typedef shared_ptr<ShaderSource> ShaderSource_ptr;
typedef shared_ptr<ShaderPair> ShaderPair_ptr;
typedef shared_ptr<Print> Print_ptr;

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

        Dot(Ident_ptr shader, Ident_ptr name): Ident(name->name) {
            this->shader = shader->name;
            type = NODE_DOT;
        }
};

class Binary: public Expr {
    public:
        OpType op;
        Expr_ptr lhs, rhs;

        Binary(Expr_ptr lhs, OpType op, Expr_ptr rhs) {
            this->op = op;
            this->lhs = lhs;
            this->rhs = rhs;
            type = NODE_BINARY; 
        }
};

class Unary: public Expr {
    public:
        OpType op;
        Expr_ptr rhs;

        Unary(OpType op, Expr_ptr rhs) {
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
        Expr_ptr x, y;

        Vector2(Expr_ptr x, Expr_ptr y) {
            this->x = x;
            this->y = y;
            type = NODE_VECTOR2;
        }
};

class Vector3: public Expr {
    public:
        Expr_ptr x, y, z;

        Vector3(Expr_ptr x, Expr_ptr y, Expr_ptr z) {
            this->x = x;
            this->y = y;
            this->z = z;
            type = NODE_VECTOR3; 
        }
};

class Vector4: public Expr {
    public:
        Expr_ptr x, y, z, w;

        Vector4(Expr_ptr x, Expr_ptr y, Expr_ptr z, Expr_ptr w) {
            this->x = x;
            this->y = y;
            this->z = z;
            this->w = w;
            type = NODE_VECTOR4;
        }
};

class Matrix2: public Expr {
    public:
        Vector2_ptr v0, v1;
        Vector2_ptr c0, c1;

        Matrix2(Vector2_ptr v0, Vector2_ptr v1): v0(v0), v1(v1) {
            type = NODE_MATRIX2;
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

        Matrix3(Vector3_ptr v0, Vector3_ptr v1, Vector3_ptr v2): v0(v0), v1(v1), v2(v2) {
            type = NODE_MATRIX3;

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

        Matrix4(Vector4_ptr v0, Vector4_ptr v1, Vector4_ptr v2, Vector4_ptr v3): v0(v0), v1(v1), v2(v2), v3(v3) {
            type = NODE_MATRIX4;
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

        Index(Expr_ptr source, Expr_ptr index) {
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
        vector<Stmt_ptr> list;

        Stmts(Stmt_ptr init) {
            if(init) list.insert(list.begin(), init);
            type = NODE_STMTS;
        }
};

class List: public Expr {
    public:
        vector<Expr_ptr> list;

        List(Expr_ptr init) {
            if(init) list.insert(list.begin(), init);
            type = NODE_LIST;
        }
};

class ArgList: public Node {
    public:
        vector<Expr_ptr> list;

        ArgList(Expr_ptr init) {
            if(init) list.push_back(init);
            type = NODE_ARGLIST;
        }
};

class ParamList: public Node {
    public:
        vector<Ident_ptr> list;

        ParamList(Ident_ptr init) {
            if(init) list.push_back(init);
            type = NODE_PARAMLIST;
        }
};

class Return: public Stmt {
    public:
        Expr_ptr value;

        Return(Expr_ptr value) {
            this->value = value;
            type = NODE_RETURN;
        }
};

class FuncDef: public Stmt {
    public:
        Ident_ptr ident;
        ParamList_ptr params;
        Stmts_ptr stmts;

        FuncDef(Ident_ptr ident, ParamList_ptr params, Stmts_ptr stmts) {
            this->ident = ident;
            this->params = params;
            this->stmts = stmts;
            type = NODE_FUNCDEF;
        }
};

class If: public Stmt {
    public:
        Expr_ptr condition;
        Stmts_ptr block;

        If(Expr_ptr condition, Stmts_ptr block) {
            this->condition = condition;
            this->block = block;
            type = NODE_IF;
        }
};

class While: public Stmt {
    public:
        Expr_ptr condition;
        Stmts_ptr block;

        While(Expr_ptr condition, Stmts_ptr block) {
            this->condition = condition;
            this->block = block;
            type = NODE_WHILE;
        }
}; 

class For: public Stmt {
    public:
        Ident_ptr iterator;
        Expr_ptr start, end, increment;
        Stmts_ptr block;

        For(Ident_ptr iterator, Expr_ptr start, Expr_ptr end, Expr_ptr increment, Stmts_ptr block) {
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
        Expr_ptr lhs;
        Expr_ptr value;

        Assign(Expr_ptr ident, Expr_ptr value) {
            this->lhs = ident;
            this->value = value;
            type = NODE_ASSIGN;
        }
};

class Decl: public Stmt {
    public:
        Ident_ptr datatype, name;
        Expr_ptr value;

        Decl(Ident_ptr datatype, Ident_ptr name, Expr_ptr value) {
            this->datatype= datatype;
            this->name = name;
            this->value = value;
            type = NODE_DECL;
        }
};

class Alloc: public Stmt {
    public:
        Ident_ptr ident;

        Alloc(Ident_ptr ident) {
            this->ident = ident;
            type = NODE_ALLOC;
        }
};

class UploadList: public Expr {
    public:
        vector<Expr_ptr> list;

        UploadList(Expr_ptr init) {
            list.insert(list.begin(), init);
            type = NODE_UPLOADLIST;
        }
};

class Upload: public Stmt {
    public:
        Ident_ptr ident;
        Ident_ptr attrib;
        UploadList_ptr list;

        Upload(Ident_ptr ident, Ident_ptr attrib, UploadList_ptr list) {
            type = NODE_UPLOAD;
            this->ident = ident;
            this->attrib = attrib;
            this->list = list;
        }
};

class Draw: public Stmt {
    public:
        Ident_ptr ident;

        Draw(Ident_ptr ident) {
            this->ident = ident;
            type = NODE_DRAW;
        }
};

class Use: public Stmt {
    public:
        Ident_ptr ident;

        Use(Ident_ptr ident) {
            this->ident = ident;
            type = NODE_USE;
        }
};

class Invoke: public Node {
    public:
        Ident_ptr ident;
        ArgList_ptr args;

        Invoke(Ident_ptr ident, ArgList_ptr args) {
            this->ident = ident;
            this->args = args;
            type = NODE_INVOKE;
        }
};

class FuncExpr: public Expr {
    public:
        Invoke_ptr invoke;

        FuncExpr(Invoke_ptr invoke) {
            this->invoke = invoke;
            type = NODE_FUNCEXPR;
        }
};

class FuncStmt: public Stmt {
    public:
        Invoke_ptr invoke;

        FuncStmt(Invoke_ptr invoke) {
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
    ShaderSource_ptr vertex = NULL;
    ShaderSource_ptr fragment = NULL;
};

class Print: public Stmt {
    public:
        Expr_ptr expr;

        Print(Expr_ptr expr) {
            this->expr = expr;
            type = NODE_PRINT;
        }
};

#endif // NODES_H
