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
%locations

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
%type<eval> bool
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

function: FUNC IDENTIFIER OPEN_PAREN param_list CLOSE_PAREN block { $$ = new FuncDef($2, $4, $6); $$->first_line = @1.first_line; $$->last_line = @6.last_line; }
    ;

expr: INT { $$ = $1; $$->first_line = @1.first_line; $$->last_line = @1.last_line; }
    | FLOAT { $$ = $1; $$->first_line = @1.first_line; $$->last_line = @1.last_line; }
    | vec2 { $$ = $1; $$->first_line = @1.first_line; $$->last_line = @1.last_line; }
    | vec3 { $$ = $1; $$->first_line = @1.first_line; $$->last_line = @1.last_line; }
    | vec4 { $$ = $1; $$->first_line = @1.first_line; $$->last_line = @1.last_line; }
    | bool { $$ = $1; $$->first_line = @1.first_line; $$->last_line = @1.last_line; }
    | invoke { $$ = new FuncExpr($1); $$->first_line = @1.first_line; $$->last_line = @1.last_line; }
    | uniform { $$ = $1; $$->first_line = @1.first_line; $$->last_line = @1.last_line; }
    | IDENTIFIER { $$ = $1; $$->first_line = @1.first_line; $$->last_line = @1.last_line; }
    | expr PLUS expr { $$ = new Binary($1, OP_PLUS, $3); $$->first_line = @1.first_line; $$->last_line = @3.last_line; }
    | expr MINUS expr { $$ = new Binary($1, OP_MINUS, $3); $$->first_line = @1.first_line; $$->last_line = @3.last_line; }
    | expr MULT expr { $$ = new Binary($1, OP_MULT, $3); $$->first_line = @1.first_line; $$->last_line = @3.last_line; }
    | expr DIV expr { $$ = new Binary($1, OP_DIV, $3); $$->first_line = @1.first_line; $$->last_line = @3.last_line; }
    | expr MOD expr { $$ = new Binary($1, OP_MOD, $3); $$->first_line = @1.first_line; $$->last_line = @3.last_line; }
    | expr LESS_THAN expr { $$ = new Binary($1, OP_LESSTHAN, $3); $$->first_line = @1.first_line; $$->last_line = @3.last_line;}
    | expr GREATER_THAN expr { $$ = new Binary($1, OP_GREATERTHAN, $3); $$->first_line = @1.first_line; $$->last_line = @3.last_line; }
    | expr EQUAL expr { $$ = new Binary($1, OP_EQUAL, $3); $$->first_line = @1.first_line; $$->last_line = @3.last_line; }
    | expr NEQUAL expr { $$ = new Binary($1, OP_NEQUAL, $3); $$->first_line = @1.first_line; $$->last_line = @3.last_line; }
    | expr LEQUAL expr { $$ = new Binary($1, OP_LEQUAL, $3); $$->first_line = @1.first_line; $$->last_line = @3.last_line; }
    | expr GEQUAL expr { $$ = new Binary($1, OP_GEQUAL, $3); $$->first_line = @1.first_line; $$->last_line = @3.last_line; }
    | MINUS expr { $$ = new Unary(OP_MINUS, $2); $$->first_line = @1.first_line; $$->last_line = @2.last_line; } %prec UNARY
    | OPEN_PAREN expr CLOSE_PAREN { $$ = $2; $$->first_line = @1.first_line; $$->last_line = @3.last_line; }
    ;

uniform: IDENTIFIER PERIOD IDENTIFIER { $$ = new Uniform($1, $3); }
    ;

stmt: IDENTIFIER EQUALS expr { $$ = new Assign($1, $3); $$->first_line = @1.first_line; $$->last_line = @3.last_line; }
    | uniform EQUALS expr { $$ = new Assign($1, $3); $$->first_line = @1.first_line; $$->last_line = @3.last_line; }
    | ALLOCATE IDENTIFIER { $$ = new Alloc($2); $$->first_line = @1.first_line; $$->last_line = @2.last_line; }
    | IDENTIFIER OPEN_BRACKET IDENTIFIER CLOSE_BRACKET UPLOAD upload_list { $$ = new Upload($1, $3, $6); $$->first_line = @1.first_line; $$->last_line = @6.last_line; }
    | DRAW IDENTIFIER { $$ = new Draw($2); $$->first_line = @1.first_line; $$->last_line = @2.last_line; }
    | USE IDENTIFIER { $$ = new Use($2); $$->first_line = @1.first_line; $$->last_line = @2.last_line; }
    | PRINT expr { $$ = new Print($2); $$->first_line = @1.first_line; $$->last_line = @2.last_line; }
    | RETURN expr { $$ = new Return($2); $$->first_line = @1.first_line; $$->last_line = @2.last_line; }
    | invoke { $$ = new FuncStmt($1); $$->first_line = @1.first_line; $$->last_line = @1.last_line; }
    | IDENTIFIER COMP_PLUS expr { $$ = new Assign($1, new Binary($1, OP_PLUS, $3)); $$->first_line = @1.first_line; $$->last_line = @3.last_line; }
    | IDENTIFIER COMP_MINUS expr { $$ = new Assign($1, new Binary($1, OP_MINUS, $3)); $$->first_line = @1.first_line; $$->last_line = @3.last_line; }
    | IDENTIFIER COMP_MULT expr { $$ = new Assign($1, new Binary($1, OP_MULT, $3)); $$->first_line = @1.first_line; $$->last_line = @3.last_line;}
    | IDENTIFIER COMP_DIV expr { $$ = new Assign($1, new Binary($1, OP_DIV, $3)); $$->first_line = @1.first_line; $$->last_line = @3.last_line; }
    | IDENTIFIER COMP_MOD expr { $$ = new Assign($1, new Binary($1, OP_MOD, $3)); $$->first_line = @1.first_line; $$->last_line = @3.last_line;}
    ;

arg_list: { $$ = new ArgList(0); }
    | expr { $$ = new ArgList($1); }
    | arg_list COMMA expr { $1->list.push_back($3); }
    ;

param_list: { $$ = new ParamList(0); }
    | IDENTIFIER { $$ = new ParamList($1); }
    | param_list COMMA IDENTIFIER { $1->list.push_back($3); }
    ;

invoke: IDENTIFIER OPEN_PAREN arg_list CLOSE_PAREN { $$ = new Invoke($1, $3); $$->first_line = @1.first_line; $$->last_line = @1.last_line; }
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
    | bool AND bool { $$ = new Binary($1, OP_AND, $3); $$->first_line = @1.first_line; $$->last_line = @3.last_line; }
    | bool OR bool { $$ = new Binary($1, OP_OR, $3); $$->first_line = @1.first_line; $$->last_line = @3.last_line; }
    | NOT bool { $$ = new Unary(OP_NOT, $2); $$->first_line = @1.first_line; $$->last_line = @2.last_line; } %prec UNARY
    ;

vec2: OPEN_BRACKET expr COMMA expr CLOSE_BRACKET { $$ = new Vector2($2, $4); $$->first_line = @1.first_line; $$->last_line = @5.last_line; }
    ;

vec3: OPEN_BRACKET expr COMMA expr COMMA expr CLOSE_BRACKET { $$ = new Vector3($2, $4, $6); $$->first_line = @1.first_line; $$->last_line = @7.last_line; }
    ;

vec4: OPEN_BRACKET expr COMMA expr COMMA expr COMMA expr CLOSE_BRACKET { $$ = new Vector4($2, $4, $6, $8); $$->first_line = @1.first_line; $$->last_line = @9.last_line; }
    ;

%%

void yyerror(std::map<std::string, ShaderPair*> *shaders, std::map<std::string, FuncDef*> *functions, const char* s) {
    std::cerr << "shaders: " << shaders << "\nfunctions: " << functions << std::endl;
    fprintf(stderr, "Parse error: %s\n", s);
}
