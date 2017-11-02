#include <string>
#include <vector>
#include <map>
#include <regex>

using namespace std;

enum NodeType {
    NODE_EXPR, NODE_BINARY, NODE_UNARY, NODE_BOOL, NODE_INT, NODE_FLOAT, NODE_VECTOR2, NODE_VECTOR3, NODE_VECTOR4, NODE_MATRIX2, NODE_MATRIX3, NODE_MATRIX4, NODE_IDENT, NODE_UNIFORM, NODE_UPLOADLIST, 
    NODE_STMT, NODE_ASSIGN, NODE_ALLOC, NODE_UPLOAD, NODE_DRAW, NODE_USE, NODE_STMTS, NODE_IF, NODE_WHILE, NODE_SSOURCE, NODE_PRINT
};

enum OpType {
    OP_PLUS, OP_MINUS, OP_MULT, OP_DIV, OP_MOD, OP_AND, OP_OR, OP_NOT, OP_EQUAL, OP_LESSTHAN, OP_GREATERTHAN, OP_NEQUAL, OP_LEQUAL, OP_GEQUAL
};

class Node {
    public:
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
        Expr *m00, *m01;
        Expr *m10, *m11;

        Matrix2(Expr *m00, Expr *m01, Expr *m10, Expr *m11) {
            this->m00 = m00; this->m01 = m01;
            this->m10 = m10; this->m11 = m11;
        }

        Matrix2(Vector2* v0, Vector2* v1): m00(v0->x), m01(v0->y), m10(v1->x), m11(v1->y) { }

        ~Matrix2() {
            delete m00; delete m10;
            delete m01; delete m11;
        }
};

class Matrix3: public Expr {
    public:
        Expr *m00, *m01, *m02;
        Expr *m10, *m11, *m12;
        Expr *m20, *m21, *m22;

        Matrix3(Expr *m00, Expr *m01, Expr *m02, Expr *m10, Expr *m11, Expr *m12, Expr *m20, Expr *m21, Expr *m22) {
            this->m00 = m00; this->m01 = m01; this->m02 = m02;
            this->m10 = m10; this->m11 = m11; this->m12 = m12;
            this->m20 = m20; this->m21 = m21; this->m22 = m22;
        }

        Matrix3(Vector3* v0, Vector3* v1, Vector3* v2): m00(v0->x), m01(v0->y), m02(v0->z), m10(v1->x), m11(v1->y), m12(v1->z), m20(v2->x), m21(v2->y), m22(v2->z) { }

        ~Matrix3() {
            delete m00; delete m01; delete m02;
            delete m10; delete m11; delete m12;
            delete m20; delete m21; delete m22;
        }
};

class Matrix4: public Expr {
    public:
        Expr *m00, *m01, *m02, *m03;
        Expr *m10, *m11, *m12, *m13;
        Expr *m20, *m21, *m22, *m23;
        Expr *m30, *m31, *m32, *m33;

        Matrix4(Expr *m00, Expr *m01, Expr *m02, Expr *m03, Expr *m10, Expr *m11, Expr *m12, Expr* m13, Expr *m20, Expr *m21, Expr *m22, Expr *m23, Expr *m30, Expr *m31, Expr *m32, Expr *m33) {
            this->m00 = m00; this->m01 = m01; this->m02 = m02; this->m03 = m03;
            this->m10 = m10; this->m11 = m11; this->m12 = m12; this->m13 = m13;
            this->m20 = m20; this->m21 = m21; this->m22 = m22; this->m23 = m23;
            this->m30 = m30; this->m31 = m31; this->m32 = m32; this->m33 = m33;
        }

        Matrix4(Vector4* v0, Vector4* v1, Vector4* v2, Vector4* v3): m00(v0->x), m01(v0->y), m02(v0->z), m03(v0->w), m10(v1->x), m11(v1->y), m12(v1->z), m13(v1->w), m20(v2->x), m21(v2->y), m22(v2->z), m23(v2->w), m30(v3->x), m31(v3->y), m32(v3->z), m33(v3->w) { }

        ~Matrix4() {
            delete m00; delete m01; delete m02; delete m03;
            delete m10; delete m11; delete m12; delete m13;
            delete m20; delete m21; delete m22; delete m23;
            delete m30; delete m31; delete m32; delete m33;
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
