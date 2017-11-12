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
    NODE_EXPR, NODE_BINARY, NODE_UNARY, NODE_BOOL, NODE_INT, NODE_FLOAT, NODE_VECTOR2, NODE_VECTOR3, NODE_VECTOR4, NODE_MATRIX2, NODE_MATRIX3, NODE_MATRIX4, NODE_IDENT, NODE_UNIFORM,
    NODE_UPLOADLIST, NODE_FUNCEXPR, NODE_ARGLIST, NODE_PARAMLIST,
    NODE_STMT, NODE_ASSIGN, NODE_ALLOC, NODE_UPLOAD, NODE_DRAW, NODE_USE, NODE_FUNCSTMT, NODE_STMTS, NODE_IF, NODE_WHILE, NODE_SSOURCE, NODE_PRINT, NODE_FUNCDEF, NODE_RETURN
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

class Uniform: public Ident {
    public:
        string shader;

        Uniform(Ident* shader, Ident* name): Ident(name->name) {
            this->shader = shader->name;
            type = NODE_UNIFORM;
        }
};

class Binary: public Expr {
    public:
        OpType op;
        Expr *lhs, *rhs;

        Binary(Expr *lhs, OpType op, Expr *rhs) {
            this->op = op;
            this->lhs = lhs;
            this->rhs = rhs;
            type = NODE_BINARY; 
        }

        ~Binary() {
            delete lhs;
            delete rhs;
        }
};

class Unary: public Expr {
    public:
        OpType op;
        Expr* rhs;

        Unary(OpType op, Expr *rhs) {
            this->op = op;
            this->rhs = rhs;
            type = NODE_UNARY;
        }

        ~Unary() {
            delete rhs;
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

class Vector2: public Expr {
    public:
        Expr *x, *y;

        Vector2(Expr *x, Expr *y) {
            this->x = x;
            this->y = y;
            type = NODE_VECTOR2;
        }

        ~Vector2() {
            delete x;
            delete y;
        }
};

class Vector3: public Expr {
    public:
        Expr *x, *y, *z;

        Vector3(Expr *x, Expr *y, Expr *z) {
            this->x = x;
            this->y = y;
            this->z = z;
            type = NODE_VECTOR3; 
        }

        ~Vector3() {
            delete x;
            delete y;
            delete z;
        }
};

class Vector4: public Expr {
    public:
        Expr *x, *y, *z, *w;

        Vector4(Expr *x, Expr *y, Expr *z, Expr *w) {
            this->x = x;
            this->y = y;
            this->z = z;
            this->w = w;
            type = NODE_VECTOR4;
        }

        ~Vector4() {
            delete x;
            delete y;
            delete z;
            delete w;
        }
};

class Matrix2: public Expr {
    public:
        Vector2 *v0, *v1;
        Vector2 *c0, *c1;

        Matrix2(Vector2* v0, Vector2* v1): v0(v0), v1(v1) {
            type = NODE_MATRIX2;

            c0 = new Vector2(v0->x, v1->x);
            c1 = new Vector2(v0->y, v1->y);
        }

        ~Matrix2() {
            delete v0;
            delete v1;

            c0 = NULL;
            c1 = NULL;
        }
};

class Matrix3: public Expr {
    public:
        Vector3 *v0, *v1, *v2;
        Vector3 *c0, *c1, *c2;

        Matrix3(Vector3* v0, Vector3* v1, Vector3* v2): v0(v0), v1(v1), v2(v2) {
            type = NODE_MATRIX3;

            c0 = new Vector3(v0->x, v1->x, v2->x);
            c1 = new Vector3(v0->y, v1->y, v2->y);
            c2 = new Vector3(v0->z, v1->z, v2->z);
        }

        ~Matrix3() {
            delete v0;
            delete v1;
            delete v2;

            c0 = NULL;
            c1 = NULL;
            c2 = NULL;
        }
};

class Matrix4: public Expr {
    public:
        Vector4 *v0, *v1, *v2, *v3;
        Vector4 *c0, *c1, *c2, *c3;

        Matrix4(Vector4* v0, Vector4* v1, Vector4* v2, Vector4* v3): v0(v0), v1(v1), v2(v2), v3(v3) {
            type = NODE_MATRIX4;

            c0 = new Vector4(v0->x, v1->x, v2->x, v3->x);
            c1 = new Vector4(v0->y, v1->y, v2->y, v3->y);
            c2 = new Vector4(v0->z, v1->z, v2->z, v3->z);
            c3 = new Vector4(v0->w, v1->w, v2->w, v3->w);
        }

        ~Matrix4() {
            delete v0;
            delete v1;
            delete v2;
            delete v3;

            c0 = NULL;
            c1 = NULL;
            c2 = NULL;
            c3 = NULL;
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
        vector<Stmt*> list;

        Stmts(Stmt* init) {
            if(init) list.insert(list.begin(), init);
            type = NODE_STMTS;
        }

        ~Stmts() {
            for(unsigned int i = 0; i < list.size(); i++) {
                delete list[i];
            }
        }
};

class ArgList: public Node {
    public:
        vector<Expr*> list;

        ArgList(Expr* init) {
            if(init) list.push_back(init);
            type = NODE_ARGLIST;
        }

        ~ArgList() {
            for(unsigned int i = 0; i < list.size(); i++) {
                delete list[i];
            }
        }
};

class ParamList: public Node {
    public:
        vector<Ident*> list;

        ParamList(Ident* init) {
            if(init) list.push_back(init);
            type = NODE_PARAMLIST;
        }

        ~ParamList() {
            for(unsigned int i = 0; i < list.size(); i++) {
                delete list[i];
            }
        }
};

class Return: public Stmt {
    public:
        Expr* value;

        Return(Expr* value) {
            this->value = value;
            type = NODE_RETURN;
        }

        ~Return() {
            delete value;
        }
};

class FuncDef: public Stmt {
    public:
        Ident* ident;
        ParamList* params;
        Stmts* stmts;

        FuncDef(Ident* ident, ParamList* params, Stmts* stmts) {
            this->ident = ident;
            this->params = params;
            this->stmts = stmts;
            type = NODE_FUNCDEF;
        }

        ~FuncDef() {
            delete ident;
            delete params;
            delete stmts;
        }
};

class If: public Stmt {
    public:
        Expr* condition;
        Stmts* block;

        If(Expr* condition, Stmts* block) {
            this->condition = condition;
            this->block = block;
            type = NODE_IF;
        }

        ~If() {
            delete condition;
            delete block;
        }
};

class While: public Stmt {
    public:
        Expr* condition;
        Stmts* block;

        While(Expr* condition, Stmts* block) {
            this->condition = condition;
            this->block = block;
            type = NODE_WHILE;
        }

        ~While() {
            delete condition;
            delete block;
        }
}; 

class Assign: public Stmt {
    public:
        Ident* ident;
        Expr* value; 

        Assign(Ident* ident, Expr* value) {
            this->ident = ident;
            this->value = value;
            type = NODE_ASSIGN;
        }

        ~Assign() {
            delete ident;
            delete value;
        }
};

class Alloc: public Stmt {
    public:
        Ident* ident;

        Alloc(Ident* ident) {
            this->ident = ident;
            type = NODE_ALLOC;
        }

        ~Alloc() {
            delete ident;
        }
};

class UploadList: public Expr {
    public:
        vector<Expr*> list;

        UploadList(Expr* init) {
            list.insert(list.begin(), init);
            type = NODE_UPLOADLIST;
        }

        ~UploadList() {
            for(unsigned int i = 0; i < list.size(); i++) {
                delete list[i];
            }
        }
};

class Upload: public Stmt {
    public:
        Ident* ident;
        Ident* attrib;
        UploadList *list;

        Upload(Ident* ident, Ident* attrib, UploadList *list) {
            type = NODE_UPLOAD;
            this->ident = ident;
            this->attrib = attrib;
            this->list = list;
        }

        ~Upload() {
            delete ident;
            delete attrib;
            delete list;
        }
};

class Draw: public Stmt {
    public:
        Ident* ident;

        Draw(Ident* ident) {
            this->ident = ident;
            type = NODE_DRAW;
        }

        ~Draw() {
            delete ident;
        }
};

class Use: public Stmt {
    public:
        Ident* ident;

        Use(Ident* ident) {
            this->ident = ident;
            type = NODE_USE;
        }

        ~Use() {
            delete ident;
        }
};

class Invoke: public Node {
    public:
        Ident* ident;
        ArgList* args;

        Invoke(Ident* ident, ArgList* args) {
            this->ident = ident;
            this->args = args;
            type = NODE_INVOKE;
        }

        ~Invoke() {
            delete ident;
            delete args;
        }
};

class FuncExpr: public Expr {
    public:
        Invoke* invoke;

        FuncExpr(Invoke* invoke) {
            this->invoke = invoke;
            type = NODE_FUNCEXPR;
        }

        ~FuncExpr() {
            delete invoke;
        }
};

class FuncStmt: public Stmt {
    public:
        Invoke* invoke;

        FuncStmt(Invoke* invoke) {
            this->invoke = invoke;
            type = NODE_FUNCSTMT;
        }

        ~FuncStmt() {
            delete invoke;
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
            this->code = code.substr(1, code.length() - 2);
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
    ShaderSource* vertex = NULL;
    ShaderSource* fragment = NULL;
};

class Print: public Stmt {
    public:
        Expr* expr;

        Print(Expr* expr) {
            this->expr = expr;
            type = NODE_PRINT;
        }

        ~Print() {
            delete expr;
        }
};

#endif // NODES_H
