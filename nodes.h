#ifndef NODES_H
#define NODES_H

#include <string>
#include <vector>
#include <map>
#include <regex>
#include <iostream>
#include <QOpenGLFunctions>

using namespace std;

enum NodeType {
    NODE_INVOKE,
    NODE_EXPR, NODE_NULL, NODE_BINARY, NODE_UNARY, NODE_BOOL, NODE_INT, NODE_FLOAT, NODE_STRING, NODE_VECTOR2, NODE_VECTOR3, NODE_VECTOR4, NODE_MATRIX2, NODE_MATRIX3, NODE_MATRIX4, NODE_IDENT, NODE_DOT, NODE_BUFFER, NODE_TEXTURE, NODE_PROGRAM,
    NODE_UPLOADLIST, NODE_FUNCEXPR, NODE_LIST, NODE_ARGLIST, NODE_PARAMLIST, NODE_INDEX,
    NODE_STMT, NODE_ASSIGN, NODE_DECL, NODE_ALLOC, NODE_COMPBINARY, NODE_UPLOAD, NODE_APPEND, NODE_DRAW, NODE_CLEAR, NODE_FUNCSTMT, NODE_STMTS, NODE_IF, NODE_WHILE, NODE_FOR, NODE_BREAK, NODE_SHADER, NODE_PRINT, NODE_FUNCDEF, NODE_RETURN
};

inline string type_to_name(NodeType type) {
    switch(type) {
        case NODE_BOOL: return "bool";
        case NODE_INT: return "int";
        case NODE_FLOAT: return "float";
        case NODE_STRING: return "string";
        case NODE_LIST: return "list";
        case NODE_VECTOR2: return "vec2";
        case NODE_VECTOR3: return "vec3";
        case NODE_VECTOR4: return "vec4";
        case NODE_MATRIX2: return "mat2";
        case NODE_MATRIX3: return "mat3";
        case NODE_MATRIX4: return "mat4";
        case NODE_BUFFER: return "buffer";
        case NODE_TEXTURE: return "texture2D";
        case NODE_PROGRAM: return "program";
        case NODE_NULL: return "null";
        default: return "undefined";
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
class Vector;
class Vector2;
class Vector3;
class Vector4;
class Matrix2;
class Matrix3;
class Matrix4;
class Index;
class Buffer;
class Texture;
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
class Break;
class Assign;
class Decl;
class Alloc;
class UploadList;
class Upload;
class CompBinary;
class Draw;
class Clear;
class Print;
class Invoke;
class FuncExpr;
class FuncStmt;
class Shader;
class Program;
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
typedef shared_ptr<Vector> Vector_ptr;
typedef shared_ptr<Vector2> Vector2_ptr;
typedef shared_ptr<Vector3> Vector3_ptr;
typedef shared_ptr<Vector4> Vector4_ptr;
typedef shared_ptr<Matrix2> Matrix2_ptr;
typedef shared_ptr<Matrix3> Matrix3_ptr;
typedef shared_ptr<Matrix4> Matrix4_ptr;
typedef shared_ptr<Index> Index_ptr;
typedef shared_ptr<Buffer> Buffer_ptr;
typedef shared_ptr<Texture> Texture_ptr;
typedef shared_ptr<Stmt> Stmt_ptr;
typedef shared_ptr<Stmts> Stmts_ptr;
typedef shared_ptr<List> List_ptr;
typedef shared_ptr<ArgList> ArgList_ptr;
typedef shared_ptr<ParamList> ParamList_ptr;
typedef shared_ptr<Return> Return_ptr;
typedef shared_ptr<FuncDef> FuncDef_ptr;
typedef shared_ptr<If> If_ptr;
typedef shared_ptr<While> While_ptr;
typedef shared_ptr<Break> Break_ptr;
typedef shared_ptr<For> For_ptr;
typedef shared_ptr<Assign> Assign_ptr;
typedef shared_ptr<Decl> Decl_ptr;
typedef shared_ptr<Alloc> Alloc_ptr;
typedef shared_ptr<UploadList> UploadList_ptr;
typedef shared_ptr<Upload> Upload_ptr;
typedef shared_ptr<CompBinary> CompBinary_ptr;
typedef shared_ptr<Draw> Draw_ptr;
typedef shared_ptr<Clear> Clear_ptr;
typedef shared_ptr<Print> Print_ptr;
typedef shared_ptr<Invoke> Invoke_ptr;
typedef shared_ptr<FuncExpr> FuncExpr_ptr;
typedef shared_ptr<FuncStmt> FuncStmt_ptr;
typedef shared_ptr<Shader> Shader_ptr;
typedef shared_ptr<Program> Program_ptr;
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
        Ident_ptr owner;

        Dot(Ident_ptr shader, Ident_ptr name): Ident(name->name) {
            this->owner = shader;
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

class Vector: public Expr {
    public:
        Vector(NodeType type): Expr(type) {}

        virtual Expr_ptr get(unsigned int i) = 0;
        virtual void set(unsigned int i, Expr_ptr val) = 0;
        virtual unsigned int size() = 0;
};

class Vector2: public Vector {
    public:
        Expr_ptr x, y;

        Vector2(Expr_ptr x, Expr_ptr y): Vector(NODE_VECTOR2) {
            this->x = x;
            this->y = y;
        }

        Expr_ptr get(unsigned int i) {
            if(i == 0) return x;
            if(i == 1) return y;
            return nullptr;
        }

        void set(unsigned int i, Expr_ptr val) {
            if(i == 0) x = val;
            if(i == 1) y = val;
        }

        unsigned int size() {
            return 2;
        }
};

class Vector3: public Vector {
    public:
        Expr_ptr x, y, z;

        Vector3(Expr_ptr x, Expr_ptr y, Expr_ptr z): Vector(NODE_VECTOR3) {
            this->x = x;
            this->y = y;
            this->z = z;
        }

        Expr_ptr get(unsigned int i) {
            if(i == 0) return x;
            if(i == 1) return y;
            if(i == 2) return z;
            return nullptr;
        }

        void set(unsigned int i, Expr_ptr val) {
            if(i == 0) x = val;
            if(i == 1) y = val;
            if(i == 2) z = val;
        }

        unsigned int size() {
            return 3;
        }

};

class Vector4: public Vector {
    public:
        Expr_ptr x, y, z, w;

        Vector4(Expr_ptr x, Expr_ptr y, Expr_ptr z, Expr_ptr w): Vector(NODE_VECTOR4) {
            this->x = x;
            this->y = y;
            this->z = z;
            this->w = w;
        }

        Expr_ptr get(unsigned int i) {
            if(i == 0) return x;
            if(i == 1) return y;
            if(i == 2) return z;
            if(i == 3) return w;

            return nullptr;
        }

        void set(unsigned int i, Expr_ptr val) {
            if(i == 0) x = val;
            if(i == 1) y = val;
            if(i == 2) z = val;
            if(i == 3) w = val;
        }

        unsigned int size() {
            return 4;
        }
};

//TODO: Refactor MatrixN class to derive from VectorN class
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

struct Layout {
    map<string, unsigned int> attributes;
    vector<string> list;
};

class Buffer: public Expr {
    public:
        GLuint handle, indexHandle;
        map<string, vector<float>> data;
        map<string, unsigned int> sizes;
        vector<unsigned int> indices;

        shared_ptr<Layout> layout;

        Buffer(): Expr(NODE_BUFFER) {}
};

class Texture: public Expr {
    public:
        GLuint handle;
        GLuint framebuffer;

        int width, height, channels;
        unsigned char* image;

        Texture(): Expr(NODE_TEXTURE) {
            handle = 0;
            framebuffer = 0;
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
        bool literal = true;

        List(Expr_ptr init): Expr(NODE_LIST) {
            if(init != nullptr) {
                list.insert(list.begin(), init);
            } 
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
        vector<Decl_ptr> list;

        ParamList(Decl_ptr init): Node(NODE_PARAMLIST) {
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
        Stmts_ptr elseBlock;

        shared_ptr<vector<If_ptr>> elseIfBlocks;

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

        Expr_ptr list;

        For(Ident_ptr iterator, Expr_ptr start, Expr_ptr end, Expr_ptr increment, Stmts_ptr block): Stmt(NODE_FOR) {
            this->iterator = iterator;
            this->start = start;
            this->end = end;
            this->increment = increment;
            this->block = block;
        }

        For(Ident_ptr iterator, Expr_ptr list, Stmts_ptr block): Stmt(NODE_FOR) {
            this->iterator = iterator;
            this->list = list;
            this->block = block;
        }
};

class Break: public Stmt {
    public:
        Break(): Stmt(NODE_BREAK) {}
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

class CompBinary: public Stmt {
    public:
        OpType op;
        Expr_ptr lhs, rhs;

        CompBinary(Expr_ptr lhs, OpType op, Expr_ptr rhs): Stmt(NODE_COMPBINARY), op(op), lhs(lhs), rhs(rhs) {}
};

class Draw: public Stmt {
    public:
        Ident_ptr ident;
        Ident_ptr target;
        Ident_ptr program;

        Draw(Ident_ptr ident, Ident_ptr target = nullptr, Ident_ptr program = nullptr): Stmt(NODE_DRAW) {
            this->ident = ident;
            this->target = target;
            this->program = program;
        }
};

class Clear: public Stmt {
    public:
        Expr_ptr color;

        Clear(Expr_ptr color): Stmt(NODE_CLEAR) {
            this->color = color;
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

class Shader: public Node {
    public:
        string name;
        shared_ptr<map<string, string>> uniforms;
        shared_ptr<vector<string>> textureSlots;
        shared_ptr<map<string, FuncDef_ptr>> functions;
        ParamList_ptr inputs;
        ParamList_ptr outputs;

        Shader(string name, shared_ptr<vector<pair<string, string>>> uniforms, shared_ptr<map<string, FuncDef_ptr>> functions, ParamList_ptr inputs, ParamList_ptr outputs): Node(NODE_SHADER) {
            this->name = name;
            this->uniforms = make_shared<map<string, string>>();
            this->textureSlots = make_shared<vector<string>>();
            for(auto it = uniforms->begin(); it != uniforms->end(); ++it) {
                this->uniforms->insert(pair<string, string>(it->first, it->second));
                if(it->second == "texture2D") {
                    this->textureSlots->push_back(it->first);
                }
            }
            this->functions = functions;
            this->inputs = inputs;
            this->outputs = outputs;
        }
};

class Program: public Expr {
    public:
        GLuint handle;
        GLuint vert, frag;
        Shader_ptr vertSource, fragSource;

        Program(): Expr(NODE_PROGRAM) { }
};

struct ShaderPair {
    std::string name;
    Shader_ptr vertex = nullptr;
    Shader_ptr fragment = nullptr;
};

#endif // NODES_H
