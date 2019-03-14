#ifndef NODES_H
#define NODES_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <iostream>
#include <QOpenGLFunctions>

using namespace std;

enum NodeType {
    NODE_INVOKE,
    NODE_EXPR, NODE_NULL, NODE_BINARY, NODE_UNARY, NODE_BOOL, NODE_INT, NODE_FLOAT, NODE_STRING, NODE_VECTOR2, NODE_VECTOR3, NODE_VECTOR4, NODE_MATRIX2, NODE_MATRIX3, NODE_MATRIX4, NODE_IDENT, NODE_DOT, NODE_BUFFER, NODE_TEXTURE, NODE_PROGRAM,
    NODE_UPLOADLIST, NODE_FUNCEXPR, NODE_LIST, NODE_ARGLIST, NODE_PARAMLIST, NODE_INDEX,
    NODE_STMT, NODE_ASSIGN, NODE_DECL, NODE_ALLOC, NODE_COMPBINARY, NODE_UPLOAD, NODE_APPEND, NODE_DRAW, NODE_CLEAR, NODE_VIEWPORT, NODE_FUNCSTMT, NODE_STMTS, NODE_IF, NODE_WHILE, NODE_FOR, NODE_BREAK, NODE_SHADER, NODE_PRINT, NODE_FUNCDEF, NODE_RETURN
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
    OP_PLUS, OP_MINUS, OP_MULT, OP_EXP, OP_DIV, OP_MOD, OP_ABS, OP_AND, OP_OR, OP_NOT, OP_EQUAL, OP_LTHAN, OP_GTHAN, OP_NEQUAL, OP_LEQUAL, OP_GEQUAL
};

#define null_expr make_shared<Expr>(NODE_NULL)
#define DEFINE_PTR(type) typedef shared_ptr<type> ptr;

class Node {
    public:
        DEFINE_PTR(Node)

        unsigned int first_line, last_line;
        unsigned int first_column, last_column;
        NodeType type;

        Node(NodeType type): type(type) {}
};

class Expr: public Node {
    public:
        DEFINE_PTR(Expr)

        bool parenthesized = false;
        Expr(): Node(NODE_EXPR) { }
        Expr(NodeType type): Node(type) { }
};

class Ident: public Expr {
    public:
        DEFINE_PTR(Ident)

        string name;

        Ident(string name): Expr(NODE_IDENT), name(name) {}
};

class Dot: public Ident {
    public:
        DEFINE_PTR(Dot)

        Ident::ptr owner;

        Dot(Ident::ptr shader, Ident::ptr ident): Ident(ident->name), owner(shader) {
            type = NODE_DOT;
        }
};

class Binary: public Expr {
    public:
        DEFINE_PTR(Binary)

        OpType op;
        Expr::ptr lhs, rhs;

        Binary(Expr::ptr lhs, OpType op, Expr::ptr rhs): Expr(NODE_BINARY), op(op), lhs(lhs), rhs(rhs) {}
};

class Unary: public Expr {
    public:
        DEFINE_PTR(Unary)

        OpType op;
        Expr::ptr rhs;

        Unary(OpType op, Expr::ptr rhs): Expr(NODE_UNARY), op(op), rhs(rhs) {}
};

class Bool: public Expr {
    public:
        DEFINE_PTR(Bool)

        bool value;

        Bool(bool value): Expr(NODE_BOOL), value(value) {}
};

class Int: public Expr {
    public:
        DEFINE_PTR(Int)

        int value;

        Int(int value): Expr(NODE_INT), value(value) {}
};

class Float: public Expr {
    public:
        DEFINE_PTR(Float)

        float value;

        Float(float value): Expr(NODE_FLOAT), value(value) {}
};

class String: public Expr {
    public:
        DEFINE_PTR(String)

        string value;

        String(string value): Expr(NODE_STRING), value(value) {}
};

class Vector: public Expr {
    public:
        DEFINE_PTR(Vector)

        Vector(NodeType type): Expr(type) {}

        virtual Expr::ptr get(unsigned int i) = 0;
        virtual void set(unsigned int i, Expr::ptr val) = 0;
        virtual unsigned int size() = 0;
};

class Vector2: public Vector {
    public:
        DEFINE_PTR(Vector2)

        Expr::ptr x, y;

        Vector2(Expr::ptr x, Expr::ptr y): Vector(NODE_VECTOR2), x(x), y(y) {}

        Expr::ptr get(unsigned int i) {
            if(i == 0) return x;
            if(i == 1) return y;
            return nullptr;
        }

        void set(unsigned int i, Expr::ptr val) {
            if(i == 0) x = val;
            if(i == 1) y = val;
        }

        unsigned int size() {
            return 2;
        }
};

class Vector3: public Vector {
    public:
        DEFINE_PTR(Vector3)

        Expr::ptr x, y, z;

        Vector3(Expr::ptr x, Expr::ptr y, Expr::ptr z): Vector(NODE_VECTOR3), x(x), y(y), z(z) {}

        Expr::ptr get(unsigned int i) {
            if(i == 0) return x;
            if(i == 1) return y;
            if(i == 2) return z;
            return nullptr;
        }

        void set(unsigned int i, Expr::ptr val) {
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
        DEFINE_PTR(Vector4)

        Expr::ptr x, y, z, w;

        Vector4(Expr::ptr x, Expr::ptr y, Expr::ptr z, Expr::ptr w): Vector(NODE_VECTOR4), x(x), y(y), z(z), w(w) {}

        Expr::ptr get(unsigned int i) {
            if(i == 0) return x;
            if(i == 1) return y;
            if(i == 2) return z;
            if(i == 3) return w;

            return nullptr;
        }

        void set(unsigned int i, Expr::ptr val) {
            if(i == 0) x = val;
            if(i == 1) y = val;
            if(i == 2) z = val;
            if(i == 3) w = val;
        }

        unsigned int size() {
            return 4;
        }
};

class Matrix: public Expr {
    public:
        DEFINE_PTR(Matrix)

        Matrix(NodeType type): Expr(type) {}

        virtual Expr::ptr get_row(unsigned int i) = 0;
        virtual Expr::ptr get_col(unsigned int i) = 0;
        virtual void generate_columns() = 0;
        virtual unsigned int size() = 0;
};

class Matrix2: public Matrix {
    public:
        DEFINE_PTR(Matrix2)

        Vector2::ptr v0, v1;
        Vector2::ptr c0, c1;

        Matrix2(Vector2::ptr v0, Vector2::ptr v1): Matrix(NODE_MATRIX2), v0(v0), v1(v1) {
            generate_columns();
        }

        Expr::ptr get_row(unsigned int i) {
            if(i == 0) return v0;
            if(i == 1) return v1;
            return nullptr;
        }

        Expr::ptr get_col(unsigned int i) {
            if(i == 0) return c0;
            if(i == 1) return c1;
            return nullptr;
        }

        void generate_columns() {
            c0 = make_shared<Vector2>(Expr::ptr(v0->x), Expr::ptr(v1->x));
            c1 = make_shared<Vector2>(Expr::ptr(v0->y), Expr::ptr(v1->y));
        }

        unsigned int size() {
            return 2;
        }
};

class Matrix3: public Matrix {
    public:
        DEFINE_PTR(Matrix3)

        Vector3::ptr v0, v1, v2;
        Vector3::ptr c0, c1, c2;

        Matrix3(Vector3::ptr v0, Vector3::ptr v1, Vector3::ptr v2): Matrix(NODE_MATRIX3), v0(v0), v1(v1), v2(v2) {
            generate_columns();
        }

        Expr::ptr get_row(unsigned int i) {
            if(i == 0) return v0;
            if(i == 1) return v1;
            if(i == 2) return v2;
            return nullptr;
        }

        Expr::ptr get_col(unsigned int i) {
            if(i == 0) return c0;
            if(i == 1) return c1;
            if(i == 2) return c2;
            return nullptr;
        }

        void generate_columns() {
            c0 = make_shared<Vector3>(Expr::ptr(v0->x), Expr::ptr(v1->x), Expr::ptr(v2->x));
            c1 = make_shared<Vector3>(Expr::ptr(v0->y), Expr::ptr(v1->y), Expr::ptr(v2->y));
            c2 = make_shared<Vector3>(Expr::ptr(v0->z), Expr::ptr(v1->z), Expr::ptr(v2->z));
        }

        unsigned int size() {
            return 3;
        }
};

class Matrix4: public Matrix {
    public:
        DEFINE_PTR(Matrix4)

        Vector4::ptr v0, v1, v2, v3;
        Vector4::ptr c0, c1, c2, c3;

        Matrix4(Vector4::ptr v0, Vector4::ptr v1, Vector4::ptr v2, Vector4::ptr v3): Matrix(NODE_MATRIX4), v0(v0), v1(v1), v2(v2), v3(v3) {
            generate_columns();
        }

        Expr::ptr get_row(unsigned int i) {
            if(i == 0) return v0;
            if(i == 1) return v1;
            if(i == 2) return v2;
            if(i == 3) return v3;
            return nullptr;
        }

        Expr::ptr get_col(unsigned int i) {
            if(i == 0) return c0;
            if(i == 1) return c1;
            if(i == 2) return c2;
            if(i == 3) return c3;
            return nullptr;
        }

        void generate_columns() {
            c0 = make_shared<Vector4>(Expr::ptr(v0->x), Expr::ptr(v1->x), Expr::ptr(v2->x), Expr::ptr(v3->x));
            c1 = make_shared<Vector4>(Expr::ptr(v0->y), Expr::ptr(v1->y), Expr::ptr(v2->y), Expr::ptr(v3->y));
            c2 = make_shared<Vector4>(Expr::ptr(v0->z), Expr::ptr(v1->z), Expr::ptr(v2->z), Expr::ptr(v3->z));
            c3 = make_shared<Vector4>(Expr::ptr(v0->w), Expr::ptr(v1->w), Expr::ptr(v2->w), Expr::ptr(v3->w));
        }

        unsigned int size() {
            return 4;
        }
};

class Index: public Expr {
    public:
        DEFINE_PTR(Index)

        Expr::ptr source;
        Expr::ptr index;

        Index(Expr::ptr source, Expr::ptr index): Expr(NODE_INDEX), source(source), index(index) {}
};

struct Layout {
    DEFINE_PTR(Layout)

    map<string, unsigned int> attributes;
    vector<string> list;
};

class Buffer: public Expr {
    public:
        DEFINE_PTR(Buffer)

        GLuint handle, indexHandle;
        map<string, vector<float>> data;
        map<string, unsigned int> sizes;
        vector<unsigned int> indices;

        Layout* layout;

        Buffer(): Expr(NODE_BUFFER) {}

        ~Buffer() {
            delete layout;
        }
};

class Texture: public Expr {
    public:
        DEFINE_PTR(Texture)

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
        DEFINE_PTR(Stmt)

        Stmt(): Node(NODE_STMT) {}
        Stmt(NodeType type): Node(type) {}
};

class Stmts: public Node {
    public:
        DEFINE_PTR(Stmts)

        vector<Stmt::ptr> list;

        Stmts(Stmt::ptr init): Node(NODE_STMTS) {
            if(init) list.insert(list.begin(), init);
        }
};

class List: public Expr {
    public:
        DEFINE_PTR(List)

        vector<Expr::ptr> list;
        bool literal = true;
        bool evaluated = false;

        List(Expr::ptr init): Expr(NODE_LIST) {
            if(init != nullptr) {
                list.insert(list.begin(), init);
            } 
        }
};

class Return: public Stmt {
    public:
        DEFINE_PTR(Return)

        Expr::ptr value;

        Return(Expr::ptr value): Stmt(NODE_RETURN), value(value) {}
};

class If: public Stmt {
    public:
        DEFINE_PTR(If)

        Expr::ptr condition;
        Stmts::ptr block;
        Stmts::ptr elseBlock;

        shared_ptr<vector<If::ptr>> elseIfBlocks;

        If(Expr::ptr condition, Stmts::ptr block): Stmt(NODE_IF), condition(condition), block(block) {}
};

class While: public Stmt {
    public:
        DEFINE_PTR(While)

        Expr::ptr condition;
        Stmts::ptr block;

        While(Expr::ptr condition, Stmts::ptr block): Stmt(NODE_WHILE), condition(condition), block(block) {}
}; 

class For: public Stmt {
    public:
        DEFINE_PTR(For)

        Ident::ptr iterator;
        Expr::ptr start, end, increment;
        Expr::ptr list;
        Stmts::ptr block;

        For(Ident::ptr iterator, Expr::ptr start, Expr::ptr end, Expr::ptr increment, Stmts::ptr block): Stmt(NODE_FOR), iterator(iterator), start(start), end(end), increment(increment), block(block) {}
        For(Ident::ptr iterator, Expr::ptr list, Stmts::ptr block): Stmt(NODE_FOR), iterator(iterator), list(list), block(block) {}
        For(Ident::ptr iterator, Dot::ptr list, Stmts::ptr block): Stmt(NODE_FOR), iterator(iterator), list(list), block(block) {}
};

class Break: public Stmt {
    public:
        DEFINE_PTR(Break)

        Break(): Stmt(NODE_BREAK) {}
};

class Assign: public Stmt {
    public:
        DEFINE_PTR(Assign)

        Expr::ptr lhs;
        Expr::ptr value;

        Assign(Expr::ptr lhs, Expr::ptr value): Stmt(NODE_ASSIGN), lhs(lhs), value(value) {}
};

class Decl: public Stmt {
    public:
        DEFINE_PTR(Decl)

        Ident::ptr datatype, ident;
        Expr::ptr value;
        bool constant = false;

        Decl(Ident::ptr datatype, Ident::ptr ident, Expr::ptr value): Stmt(NODE_DECL), datatype(datatype), ident(ident), value(value) {}
};

class ArgList: public Node {
    public:
        DEFINE_PTR(ArgList)

        vector<Expr::ptr> list;

        ArgList(Expr::ptr init): Node(NODE_ARGLIST) {
            if(init) list.push_back(init);
        }
};

class ParamList: public Node {
    public:
        DEFINE_PTR(ParamList)

        vector<Decl::ptr> list;

        ParamList(Decl::ptr init): Node(NODE_PARAMLIST) {
            if(init) list.push_back(init);
        }
};

class FuncDef: public Stmt {
    public:
        DEFINE_PTR(FuncDef)

        Ident::ptr ident;
        ParamList::ptr params;
        Stmts::ptr stmts;

        FuncDef(Ident::ptr ident, ParamList::ptr params, Stmts::ptr stmts): Stmt(NODE_FUNCDEF), ident(ident), params(params), stmts(stmts) {}
};

class Alloc: public Stmt {
    public:
        DEFINE_PTR(Alloc)

        Ident::ptr ident;

        Alloc(Ident::ptr ident): Stmt(NODE_ALLOC) {
            this->ident = ident;
        }
};

class UploadList: public Expr {
    public:
        DEFINE_PTR(UploadList)

        vector<Expr::ptr> list;

        UploadList(Expr::ptr init): Expr(NODE_UPLOADLIST) {
            list.insert(list.begin(), init);
        }
};

class Upload: public Stmt {
    public:
        DEFINE_PTR(Upload)

        Ident::ptr ident;
        Ident::ptr attrib;
        UploadList::ptr list;

        Upload(Ident::ptr ident, Ident::ptr attrib, UploadList::ptr list): Stmt(NODE_UPLOAD), ident(ident), attrib(attrib), list(list) {}
};

class CompBinary: public Stmt {
    public:
        DEFINE_PTR(CompBinary)

        OpType op;
        Expr::ptr lhs, rhs;

        CompBinary(Expr::ptr lhs, OpType op, Expr::ptr rhs): Stmt(NODE_COMPBINARY), op(op), lhs(lhs), rhs(rhs) {}
};

class Draw: public Stmt {
    public:
        DEFINE_PTR(Draw)

        Ident::ptr ident;
        Ident::ptr target;
        Ident::ptr program;

        Draw(Ident::ptr ident, Ident::ptr target = nullptr, Ident::ptr program = nullptr): Stmt(NODE_DRAW), ident(ident), target(target), program(program) {}
};

class Clear: public Stmt {
    public:
        DEFINE_PTR(Clear)

        Expr::ptr color;

        Clear(Expr::ptr color): Stmt(NODE_CLEAR), color(color) {}
};

class Viewport: public Stmt {
    public:
        DEFINE_PTR(Viewport)

        Expr::ptr bounds;

        Viewport(Expr::ptr bounds): Stmt(NODE_VIEWPORT), bounds(bounds) {}
};

class Print: public Stmt {
    public:
        DEFINE_PTR(Print)

        Expr::ptr expr;

        Print(Expr::ptr expr): Stmt(NODE_PRINT), expr(expr) {}
};

class Invoke: public Node {
    public:
        DEFINE_PTR(Invoke)

        Ident::ptr ident;
        ArgList::ptr args;

        Invoke(Ident::ptr ident, ArgList::ptr args): Node(NODE_INVOKE), ident(ident), args(args) {}
};

class FuncExpr: public Expr {
    public:
        DEFINE_PTR(FuncExpr)

        Invoke::ptr invoke;

        FuncExpr(Invoke::ptr invoke): Expr(NODE_FUNCEXPR), invoke(invoke) {}
};

class FuncStmt: public Stmt {
    public:
        DEFINE_PTR(FuncStmt)

        Invoke::ptr invoke;

        FuncStmt(Invoke::ptr invoke): Stmt(NODE_FUNCSTMT), invoke(invoke) {}
};

class Shader: public Node {
    public:
        DEFINE_PTR(Shader)

        string name;
        shared_ptr<map<string, string>> uniforms;
        shared_ptr<vector<string>> textureSlots;
        shared_ptr<map<string, FuncDef::ptr>> functions;
        ParamList::ptr inputs;
        ParamList::ptr outputs;

        Shader(string name, shared_ptr<vector<pair<string, string>>> uniforms, shared_ptr<map<string, FuncDef::ptr>> functions, ParamList::ptr inputs, ParamList::ptr outputs): Node(NODE_SHADER), name(name) {
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
        DEFINE_PTR(Program)

        GLuint handle;
        GLuint vert, frag;
        Shader::ptr vertSource, fragSource;

        Program(): Expr(NODE_PROGRAM) {}
};

class ProgramLayout: public Stmt {
    public:
        DEFINE_PTR(ProgramLayout)

        Ident::ptr ident;
        shared_ptr<vector<Decl::ptr>> attribs;
};

struct ShaderPair {
    DEFINE_PTR(ShaderPair)

    std::string name;
    Shader::ptr vertex = nullptr;
    Shader::ptr fragment = nullptr;
};

#endif // NODES_H
