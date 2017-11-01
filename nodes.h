#include <string>
#include <vector>
#include <map>
#include <regex>

using namespace std;

enum NodeType {
    NODE_EXPR, NODE_BINARY, NODE_UNARY, NODE_BOOL, NODE_INT, NODE_FLOAT, NODE_VECTOR3, NODE_IDENT, NODE_UNIFORM, NODE_UPLOADLIST, 
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

        Uniform(Ident* name, Ident* shader): Ident(name->name) {
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
};

class Alloc: public Stmt {
    public:
        Ident* ident;

        Alloc(Ident* ident) {
            this->ident = ident;
            type = NODE_ALLOC;
        }
};

class UploadList: public Expr {
    public:
        vector<Expr*> list;

        UploadList(Expr* init) {
            list.insert(list.begin(), init);
            type = NODE_UPLOADLIST;
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
};

class Draw: public Stmt {
    public:
        Ident* ident;

        Draw(Ident* ident) {
            this->ident = ident;
            type = NODE_DRAW;
        }
};

class Use: public Stmt {
    public:
        Ident* ident;

        Use(Ident* ident) {
            this->ident = ident;
            type = NODE_USE;
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

class Print: public Stmt {
    public:
        Expr* expr;

        Print(Expr* expr) {
            this->expr = expr;
            type = NODE_PRINT;
        }
};
