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
void yyerror( std::map<std::string, ShaderPair*> *shaders, std::map<std::string, FuncDef*> *functions, const char *s);
}

%debug

%defines "parser.h"
%output "parser.cpp"
%define parse.lac full
%define parse.error verbose

%parse-param { std::map<std::string, ShaderPair*> *shaders } { std::map<std::string, FuncDef*> *functions }

%union {
    Expr* eval;
    Bool* bval;
    Int* ival;
    Float* fval;
    Ident* idval;
    Vector2* v2val;
    Vector3* v3val;
    Vector4* v4val;
    Stmt* sval;
    Stmts* svval;
    Invoke* inval;
    ShaderSource* ssval;

    UploadList* ulval;
    ArgList* alval;
    ParamList* pmval;
    FuncDef* fdval;
}

%token<bval> BOOL
%token<ival> INT 
%token<fval> FLOAT
%token<idval> IDENTIFIER SHADER

%token SEMICOLON OPEN_BRACE CLOSE_BRACE
%token PIPE
%token OPEN_PAREN CLOSE_PAREN LESS_THAN GREATER_THAN OPEN_BRACKET CLOSE_BRACKET COMMA PERIOD EQUALS COMP_PLUS COMP_MINUS COMP_MULT COMP_DIV COMP_MOD
%token FUNC AND OR NOT IF WHILE ALLOCATE UPLOAD DRAW VERTEX FRAGMENT PRINT USE RETURN

%left PLUS MINUS
%left MULT DIV MOD
%left AND OR
%right UNARY

%nonassoc EQUAL NEQUAL GEQUAL LEQUAL
%nonassoc LESS_THAN GREATER_THAN

%type<inval> invoke;

%type<eval> expr
%type<eval> scalar bool
%type<v2val> vec2
%type<v3val> vec3
%type<v4val> vec4
%type<idval> uniform;

%type<sval> stmt stmt_block
%type<fdval> function
%type<svval> stmts block
%type<ssval> vert_shader frag_shader
%type<ulval> upload_list
%type<alval> arg_list
%type<pmval> param_list

%start program 
%%

program:
    |
    function program  {
        if(functions->find($1->ident->name) == functions->end()) {
            functions->insert(std::pair<std::string, FuncDef*>($1->ident->name, $1));
        } else {
            std::cout << "ERROR: Redefinition of function " << $1->ident->name << std::endl;
        }
    }
    |
    vert_shader program { 
        if(shaders->find($1->name) == shaders->end()) {
            ShaderPair* pair = new ShaderPair;
            pair->name = $1->name;
            pair->vertex = $1;

            shaders->insert(std::pair<std::string, ShaderPair*>($1->name, pair));
        } else {
            (*shaders)[$1->name]->vertex = $1;
        }
    }
    |
    frag_shader program { 
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

function: FUNC IDENTIFIER OPEN_PAREN param_list CLOSE_PAREN block { $$ = new FuncDef($2, $4, $6); }
    ;

expr: scalar { $$ = $1; }
    | vec2 { $$ = $1; }
    | vec3 { $$ = $1; }
    | vec4 { $$ = $1; }
    | bool { $$ = $1; }
    | invoke { $$ = new FuncExpr($1); }
    | uniform { $$ = $1; }
    | IDENTIFIER { $$ = $1; }
    | expr PLUS expr { $$ = new Binary($1, OP_PLUS, $3); }
    | expr MINUS expr { $$ = new Binary($1, OP_MINUS, $3); }
    | expr MULT expr { $$ = new Binary($1, OP_MULT, $3); }
    | expr DIV expr { $$ = new Binary($1, OP_DIV, $3); }
    | expr MOD expr { $$ = new Binary($1, OP_MOD, $3); }
    | expr LESS_THAN expr { $$ = new Binary($1, OP_LESSTHAN, $3); }
    | expr GREATER_THAN expr { $$ = new Binary($1, OP_GREATERTHAN, $3); }
    | expr EQUAL expr { $$ = new Binary($1, OP_EQUAL, $3); }
    | expr NEQUAL expr { $$ = new Binary($1, OP_NEQUAL, $3); }
    | expr LEQUAL expr { $$ = new Binary($1, OP_LEQUAL, $3); }
    | expr GEQUAL expr { $$ = new Binary($1, OP_GEQUAL, $3); }
    | MINUS expr { $$ = new Unary(OP_MINUS, $2); } %prec UNARY
    | OPEN_PAREN expr CLOSE_PAREN { $$ = $2; }
    ;

uniform: IDENTIFIER PERIOD IDENTIFIER { $$ = new Uniform($1, $3); }
    ;

stmt: IDENTIFIER EQUALS expr { $$ = new Assign($1, $3); }
    | uniform EQUALS expr { $$ = new Assign($1, $3); }
    | ALLOCATE IDENTIFIER { $$ = new Alloc($2); }
    | IDENTIFIER OPEN_BRACKET IDENTIFIER CLOSE_BRACKET UPLOAD upload_list { $$ = new Upload($1, $3, $6); }
    | DRAW IDENTIFIER { $$ = new Draw($2); }
    | USE IDENTIFIER { $$ = new Use($2); }
    | PRINT expr { $$ = new Print($2); }
    | RETURN expr { $$ = new Return($2); }
    | invoke { $$ = new FuncStmt($1) ; }
    | IDENTIFIER COMP_PLUS expr { $$ = new Assign($1, new Binary($1, OP_PLUS, $3)); }
    | IDENTIFIER COMP_MINUS expr { $$ = new Assign($1, new Binary($1, OP_MINUS, $3)); }
    | IDENTIFIER COMP_MULT expr { $$ = new Assign($1, new Binary($1, OP_MULT, $3)); }
    | IDENTIFIER COMP_DIV expr { $$ = new Assign($1, new Binary($1, OP_DIV, $3)); }
    | IDENTIFIER COMP_MOD expr { $$ = new Assign($1, new Binary($1, OP_MOD, $3)); }
    ;

arg_list: { $$ = new ArgList(0); }
    | expr { $$ = new ArgList($1); }
    | arg_list COMMA expr { $1->list.push_back($3); }
    ;

param_list: { $$ = new ParamList(0); }
    | IDENTIFIER { $$ = new ParamList($1); }
    | param_list COMMA IDENTIFIER { $1->list.push_back($3); }
    ;

invoke: IDENTIFIER OPEN_PAREN arg_list CLOSE_PAREN { $$ = new Invoke($1, $3); }
    ;

stmt_block: IF OPEN_PAREN expr CLOSE_PAREN block { $$ = new If($3, $5); }
    | IF OPEN_PAREN expr CLOSE_PAREN stmt SEMICOLON { $$ = new If($3, new Stmts($5)); }
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
    ;

scalar: INT { $$ = $1; }
    | FLOAT { $$ = $1; }
    ;

vec2: OPEN_BRACKET expr COMMA expr CLOSE_BRACKET { $$ = new Vector2($2, $4); }
    ;

vec3: OPEN_BRACKET expr COMMA expr COMMA expr CLOSE_BRACKET { $$ = new Vector3($2, $4, $6); }
    ;

vec4: OPEN_BRACKET expr COMMA expr COMMA expr COMMA expr CLOSE_BRACKET { $$ = new Vector4($2, $4, $6, $8); }
    ;

%%

void yyerror(std::map<std::string, ShaderPair*> *shaders, std::map<std::string, FuncDef*> *functions, const char* s) {
    std::cerr << "shaders: " << shaders << "\nfunctions: " << functions << std::endl;
    fprintf(stderr, "Parse error: %s\n", s);
}
