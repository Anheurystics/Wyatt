#include <string>
#include <vector>

enum NodeType {
	NODE_EXPR, NODE_BINARY, NODE_INT, NODE_FLOAT, NODE_VECTOR3, NODE_IDENT, NODE_UPLOADLIST, 
	NODE_STMT, NODE_ASSIGN, NODE_ALLOC, NODE_UPLOAD, NODE_STMTS
};

enum OpType {
	OP_PLUS, OP_MINUS, OP_MULT, OP_DIV, OP_MOD
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
	std::string name;

	Ident(std::string name) {
		this->name = name;
		type = NODE_IDENT;
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
    std::vector<Stmt*> list;

    Stmts(Stmt* init) {
        list.insert(list.begin(), init);
        type = NODE_STMTS;
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
	std::vector<Expr*> list;

	UploadList(Expr* init) {
		list.insert(list.begin(), init);
		type = NODE_UPLOADLIST;
	}
};

class Upload: public Stmt {
public:
	Ident* ident;
	UploadList *list;

	Upload(Ident* ident, UploadList *list) {
		type = NODE_UPLOAD;
		this->ident = ident;
		this->list = list;
	}
};
