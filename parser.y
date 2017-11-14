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

#define set_lines(dst, start, end) dst->first_line = start.first_line; dst->last_line = end.last_line;
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
    String* strval;
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

%token<bval> T_BOOL
%token<ival> T_INT
%token<fval> T_FLOAT
%token<strval> T_STRING T_SHADER
%token<idval> T_IDENTIFIER

%token T_SEMICOLON T_OPEN_BRACE T_CLOSE_BRACE
%token T_PIPE
%token T_OPEN_PAREN T_CLOSE_PAREN T_LESS_THAN T_GREATER_THAN T_OPEN_BRACKET T_CLOSE_BRACKET T_COMMA T_PERIOD T_EQUALS T_COMP_PLUS T_COMP_MINUS T_COMP_MULT T_COMP_DIV T_COMP_MOD
%token T_FUNC T_AND T_OR T_NOT T_IF T_WHILE T_FOR T_IN T_ALLOCATE T_UPLOAD T_DRAW T_VERTEX T_FRAGMENT T_PRINT T_USE T_RETURN

%left T_PLUS T_MINUS
%left T_MULT T_DIV T_MOD
%left T_AND T_OR
%right UNARY

%nonassoc T_EQUAL T_NEQUAL T_GEQUAL T_LEQUAL
%nonassoc T_LESS_THAN T_GREATER_THAN

%type<inval> invoke;

%type<eval> expr index
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

vert_shader: T_VERTEX T_IDENTIFIER T_SHADER T_SEMICOLON { $$ = new ShaderSource($2->name, $3->value, "vert"); }
    ;

frag_shader: T_FRAGMENT T_IDENTIFIER T_SHADER T_SEMICOLON { $$ = new ShaderSource($2->name, $3->value, "frag"); }
    ;

function: T_FUNC T_IDENTIFIER T_OPEN_PAREN param_list T_CLOSE_PAREN block { $$ = new FuncDef($2, $4, $6); set_lines($$, @1, @6); }
    ;

expr: T_INT { $$ = $1; set_lines($$,@1,@1); }
    | T_FLOAT { $$ = $1; set_lines($$, @1, @1); }
    | T_BOOL { $$ = $1; set_lines($$, @1, @1); }
    | T_IDENTIFIER { $$ = $1; set_lines($$, @1, @1); }
    | T_STRING { $$ = $1; set_lines($$, @1, @1); }
    | vec2 { $$ = $1; set_lines($$, @1, @1); }
    | vec3 { $$ = $1; set_lines($$, @1, @1); }
    | vec4 { $$ = $1; set_lines($$, @1, @1); }
    | invoke { $$ = new FuncExpr($1); set_lines($$, @1, @1); }
    | uniform { $$ = $1; set_lines($$, @1, @1); }
    | index { $$ = $1; set_lines($$, @1, @1); }
    | expr T_PLUS expr { $$ = new Binary($1, OP_PLUS, $3); set_lines($$, @1, @3); }
    | expr T_MINUS expr { $$ = new Binary($1, OP_MINUS, $3); set_lines($$, @1, @3); }
    | expr T_MULT expr { $$ = new Binary($1, OP_MULT, $3); set_lines($$, @1, @3); }
    | expr T_DIV expr { $$ = new Binary($1, OP_DIV, $3); set_lines($$, @1, @3); }
    | expr T_MOD expr { $$ = new Binary($1, OP_MOD, $3); set_lines($$, @1, @3); }
    | expr T_LESS_THAN expr { $$ = new Binary($1, OP_LESSTHAN, $3); set_lines($$, @1, @3);}
    | expr T_GREATER_THAN expr { $$ = new Binary($1, OP_GREATERTHAN, $3); set_lines($$, @1, @3); }
    | expr T_EQUAL expr { $$ = new Binary($1, OP_EQUAL, $3); set_lines($$, @1, @3); }
    | expr T_NEQUAL expr { $$ = new Binary($1, OP_NEQUAL, $3); set_lines($$, @1, @3); }
    | expr T_LEQUAL expr { $$ = new Binary($1, OP_LEQUAL, $3); set_lines($$, @1, @3); }
    | expr T_GEQUAL expr { $$ = new Binary($1, OP_GEQUAL, $3); set_lines($$, @1, @3); }
    | expr T_OR expr { $$ = new Binary($1, OP_OR, $3); set_lines($$, @1, @3); }
    | expr T_AND expr { $$ = new Binary($1, OP_AND, $3); set_lines($$, @1, @3); }
    | T_MINUS expr { $$ = new Unary(OP_MINUS, $2); set_lines($$, @1, @2); } %prec UNARY
    | T_NOT expr { $$ = new Unary(OP_NOT, $2); set_lines($$, @1, @2); } %prec UNARY
    | T_PIPE expr T_PIPE { $$ = new Unary(OP_ABS, $2); set_lines($$, @1, @3); }
    | T_OPEN_PAREN expr T_CLOSE_PAREN { $$ = $2; set_lines($$, @1, @3); }
    ;

index: T_IDENTIFIER T_OPEN_BRACKET expr T_CLOSE_BRACKET { $$ = new Index($1, $3); set_lines($$, @1, @4); }
    | index T_OPEN_BRACKET expr T_CLOSE_BRACKET { $$ = new Index($1, $3); set_lines($$, @1, @4); }
    ;

uniform: T_IDENTIFIER T_PERIOD T_IDENTIFIER { $$ = new Uniform($1, $3); }
    ;

stmt: T_IDENTIFIER T_EQUALS expr { $$ = new Assign($1, $3); set_lines($$, @1, @3); }
    | uniform T_EQUALS expr { $$ = new Assign($1, $3); set_lines($$, @1, @3); }
    | T_ALLOCATE T_IDENTIFIER { $$ = new Alloc($2); set_lines($$, @1, @2); }
    | T_IDENTIFIER T_PERIOD T_IDENTIFIER T_UPLOAD upload_list { $$ = new Upload($1, $3, $5); set_lines($$, @1, @5); }
    | T_DRAW T_IDENTIFIER { $$ = new Draw($2); set_lines($$, @1, @2); }
    | T_USE T_IDENTIFIER { $$ = new Use($2); set_lines($$, @1, @2); }
    | T_PRINT expr { $$ = new Print($2); set_lines($$, @1, @2); }
    | T_RETURN expr { $$ = new Return($2); set_lines($$, @1, @2); }
    | invoke { $$ = new FuncStmt($1); set_lines($$, @1, @1); }
    | T_IDENTIFIER T_COMP_PLUS expr { $$ = new Assign($1, new Binary($1, OP_PLUS, $3)); set_lines($$, @1, @3); }
    | T_IDENTIFIER T_COMP_MINUS expr { $$ = new Assign($1, new Binary($1, OP_MINUS, $3)); set_lines($$, @1, @3); }
    | T_IDENTIFIER T_COMP_MULT expr { $$ = new Assign($1, new Binary($1, OP_MULT, $3)); set_lines($$, @1, @3);}
    | T_IDENTIFIER T_COMP_DIV expr { $$ = new Assign($1, new Binary($1, OP_DIV, $3)); set_lines($$, @1, @3); }
    | T_IDENTIFIER T_COMP_MOD expr { $$ = new Assign($1, new Binary($1, OP_MOD, $3)); set_lines($$, @1, @3);}
    ;

arg_list: { $$ = new ArgList(0); }
    | expr { $$ = new ArgList($1); }
    | arg_list T_COMMA expr { $1->list.push_back($3); }
    ;

param_list: { $$ = new ParamList(0); }
    | T_IDENTIFIER { $$ = new ParamList($1); }
    | param_list T_COMMA T_IDENTIFIER { $1->list.push_back($3); }
    ;

invoke: T_IDENTIFIER T_OPEN_PAREN arg_list T_CLOSE_PAREN { $$ = new Invoke($1, $3); set_lines($$, @1, @1); }
    ;

stmt_block: T_IF T_OPEN_PAREN expr T_CLOSE_PAREN block { $$ = new If($3, $5); }
    | T_IF T_OPEN_PAREN expr T_CLOSE_PAREN stmt T_SEMICOLON { $$ = new If($3, new Stmts($5)); }
    | T_WHILE T_OPEN_PAREN expr T_CLOSE_PAREN block { $$ = new While($3, $5); }
    | T_FOR T_OPEN_PAREN T_IDENTIFIER T_IN expr T_COMMA expr T_COMMA expr T_CLOSE_PAREN block { $$ = new For($3, $5, $7, $9, $11); }
    ;

stmts: { $$ = new Stmts(0); }
    | stmts stmt T_SEMICOLON { $1->list.insert($1->list.end(), $2); }
    | stmts stmt_block { $1->list.insert($1->list.end(), $2); }
    ;

block: T_OPEN_BRACE stmts T_CLOSE_BRACE { $$ = $2; }

upload_list: expr { $$ = new UploadList($1); }
    | upload_list T_COMMA expr { $1->list.insert($1->list.end(), $3); }
    ;

vec2: T_OPEN_BRACKET expr T_COMMA expr T_CLOSE_BRACKET { $$ = new Vector2($2, $4); set_lines($$, @1, @5); }
    ;

vec3: T_OPEN_BRACKET expr T_COMMA expr T_COMMA expr T_CLOSE_BRACKET { $$ = new Vector3($2, $4, $6); set_lines($$, @1, @7); }
    ;

vec4: T_OPEN_BRACKET expr T_COMMA expr T_COMMA expr T_COMMA expr T_CLOSE_BRACKET { $$ = new Vector4($2, $4, $6, $8); set_lines($$, @1, @9); }
    ;

%%

void yyerror(std::map<std::string, ShaderPair*> *shaders, std::map<std::string, FuncDef*> *functions, const char* s) {
    std::cerr << "shaders: " << shaders << "\nfunctions: " << functions << std::endl;
    fprintf(stderr, "Parse error: %s\n", s);
}
