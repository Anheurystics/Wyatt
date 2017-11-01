%{
#include <cstdio>
#include <iostream>
#include <vector>
#include <cmath>
%}

%code requires {
#include <string>
#include <vector>
#include "nodes.h"

int yylex();
void yyerror( std::map<std::string, ShaderPair*> *shaders, Stmts** init, Stmts** loop, const char *s);
}

%debug

%defines "parser.h"
%output "parser.cpp"
%define parse.lac full
%define parse.error verbose

%parse-param { std::map<std::string, ShaderPair*> *shaders } { Stmts** init } { Stmts** loop }

%union {
	Expr* eval;
    Bool* bval;
	Int* ival;
	Float* fval;
	Ident* idval;
	Vector3* vval;
	Stmt* sval;
    Stmts* svval;
    ShaderSource* ssval;

	UploadList* lval;
}

%token<bval> BOOL
%token<ival> INT 
%token<fval> FLOAT
%token<idval> IDENTIFIER SHADER

%token SEMICOLON OPEN_BRACE CLOSE_BRACE
%token PIPE
%token OPEN_PAREN CLOSE_PAREN LESS_THAN GREATER_THAN OPEN_BRACKET CLOSE_BRACKET COMMA PERIOD EQUALS AND OR NOT IF WHILE EQUAL NEQUAL GEQUAL LEQUAL COMP_PLUS COMP_MINUS COMP_MULT COMP_DIV COMP_MOD
%token INIT LOOP ALLOCATE UPLOAD DRAW VERTEX FRAGMENT PRINT USE

%left PLUS MINUS
%left MULT DIV MOD
%left AND OR
%left UNARY

%type<eval> expr uniform
%type<eval> scalar bool
%type<vval> vec3

%type<sval> stmt stmt_block
%type<svval> stmts block
%type <ssval> vert_shader frag_shader
%type<lval> upload_list

%start program 
%%

program: INIT block LOOP block { *init = $2; *loop = $4; }
    | vert_shader program { 
        if(shaders->find($1->name) == shaders->end()) {
            ShaderPair* pair = new ShaderPair;
            pair->name = $1->name;
            pair->vertex = $1;

            shaders->insert(std::pair<std::string, ShaderPair*>($1->name, pair));
        } else {
            (*shaders)[$1->name]->vertex = $1;
        }
    }
    | frag_shader program { 
        if(shaders->find($1->name) == shaders->end()) {
            ShaderPair* pair = new ShaderPair;
            pair->name = $1->name;
            pair->fragment = $1;

            shaders->insert(std::pair<std::string, ShaderPair*>($1->name, pair));
        } else {
            (*shaders)[$1->name]->fragment = $1;
        }
    }
	;

vert_shader: VERTEX IDENTIFIER SHADER SEMICOLON { $$ = new ShaderSource($2->name, $3->name, "vert"); }
    ;

frag_shader: FRAGMENT IDENTIFIER SHADER SEMICOLON { $$ = new ShaderSource($2->name, $3->name, "frag"); }
    ;

expr: scalar { $$ = $1; }
	| vec3 { $$ = $1; }
    | bool { $$ = $1; }
    | IDENTIFIER { $$ = $1; }
    | uniform { $$ = $1; }
	| expr PLUS expr { $$ = new Binary($1, OP_PLUS, $3); }
	| expr MINUS expr { $$ = new Binary($1, OP_MINUS, $3); }
	| expr MULT expr { $$ = new Binary($1, OP_MULT, $3); }
	| expr DIV expr { $$ = new Binary($1, OP_DIV, $3); }
    | expr MOD expr { $$ = new Binary($1, OP_MOD, $3); }
    | MINUS expr { $$ = new Unary(OP_MINUS, $2); } %prec UNARY
    | OPEN_PAREN expr OPEN_PAREN { $$ = $2; }
	;

uniform: IDENTIFIER PERIOD IDENTIFIER { $$ = new Uniform($1, $3); }
    ;

stmt: IDENTIFIER EQUALS expr { $$ = new Assign($1, $3); }
	| ALLOCATE IDENTIFIER { $$ = new Alloc($2); }
	| IDENTIFIER OPEN_BRACKET IDENTIFIER CLOSE_BRACKET UPLOAD upload_list { $$ = new Upload($1, $3, $6); }
    | DRAW IDENTIFIER { $$ = new Draw($2); }
    | USE IDENTIFIER { $$ = new Use($2); }
    | PRINT expr { $$ = new Print($2); }
    | IDENTIFIER COMP_PLUS expr { $$ = new Assign($1, new Binary($1, OP_PLUS, $3)); }
    | IDENTIFIER COMP_MINUS expr { $$ = new Assign($1, new Binary($1, OP_MINUS, $3)); }
    | IDENTIFIER COMP_MULT expr { $$ = new Assign($1, new Binary($1, OP_MULT, $3)); }
    | IDENTIFIER COMP_DIV expr { $$ = new Assign($1, new Binary($1, OP_DIV, $3)); }
    | IDENTIFIER COMP_MOD expr { $$ = new Assign($1, new Binary($1, OP_MOD, $3)); }
	;

stmt_block: IF OPEN_PAREN expr CLOSE_PAREN block { $$ = new If($3, $5); }
    | WHILE OPEN_PAREN expr CLOSE_PAREN block { $$ = new While($3, $5); }
    ;

stmts: { $$ = new Stmts(0); }
    | stmts stmt SEMICOLON { $1->list.insert($1->list.end(), $2); }
    | stmts stmt_block { $1->list.insert($1->list.end(), $2); }
    ;

block: OPEN_BRACE stmts CLOSE_BRACE { $$ = $2; }

upload_list: expr { $$ = new UploadList($1); }
	| upload_list COMMA expr { $1->list.insert($1->list.end(), $3); }
	;

bool: BOOL { $$ = $1; }
    | bool AND bool { $$ = new Binary($1, OP_AND, $3); }
    | bool OR bool { $$ = new Binary($1, OP_OR, $3); }
    | NOT bool { $$ = new Unary(OP_NOT, $2); } %prec UNARY
    | scalar EQUAL scalar { $$ = new Binary($1, OP_EQUAL, $3); }
    | scalar LESS_THAN scalar { $$ = new Binary($1, OP_LESSTHAN, $3); }
    | scalar GREATER_THAN scalar { $$ = new Binary($1, OP_GREATERTHAN, $3); }
    | scalar NEQUAL scalar { $$ = new Binary($1, OP_NEQUAL, $3); }
    | scalar LEQUAL scalar { $$ = new Binary($1, OP_LEQUAL, $3); }
    | scalar GEQUAL scalar { $$ = new Binary($1, OP_GEQUAL, $3); }
    ;

scalar: INT { $$ = $1; }
	| FLOAT { $$ = $1; }
	;

vec3: OPEN_BRACKET expr COMMA expr COMMA expr CLOSE_BRACKET { $$ = new Vector3($2, $4, $6); }
	;

%%

void yyerror(std::map<std::string, ShaderPair*> *shaders, Stmts** init, Stmts** loop, const char* s) {
    std::cerr << "shaders: " << shaders << "\ninit: " << *init << "\nloop: " << *loop << std::endl;
	fprintf(stderr, "Parse error: %s\n", s);
}
