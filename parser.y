%{
#include <cstdio>
#include <iostream>
#include <vector>
#include <cmath>
#include <string>

using namespace std;
%}

%code requires {
#include "nodes.h"
#include <memory>

int yylex();
void yyerror( std::map<std::string, std::shared_ptr<ShaderPair>> *shaders, std::map<std::string, std::shared_ptr<FuncDef>> *functions, const char *s);

#define set_lines(dst, start, end) dst->first_line = start.first_line; dst->last_line = end.last_line;
#define YYSTYPE shared_ptr<Node>
}

%debug

%language "c++"
%defines "parser.h"
%output "parser.cpp"
%define parse.error verbose
%define api.value.type variant
%locations

%parse-param { std::map<std::string, std::shared_ptr<ShaderPair>> *shaders } { std::map<std::string, std::shared_ptr<FuncDef>> *functions }

%token<shared_ptr<Bool>> T_BOOL
%token<shared_ptr<Int>> T_INT
%token<shared_ptr<Float>> T_FLOAT
%token<shared_ptr<String>> T_STRING T_SHADER
%token<shared_ptr<Ident>> T_IDENTIFIER

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

%type<shared_ptr<Invoke>> invoke;
%type<shared_ptr<Expr>> expr index
%type<shared_ptr<Vector2>> vec2
%type<shared_ptr<Vector3>> vec3
%type<shared_ptr<Vector4>> vec4
%type<shared_ptr<Dot>> dot;
%type<shared_ptr<List>> list;

%type<shared_ptr<Stmt>> stmt stmt_block
%type<shared_ptr<FuncDef>> function
%type<shared_ptr<Stmts>> stmts block
%type<shared_ptr<ShaderSource>> vert_shader frag_shader
%type<shared_ptr<UploadList>> upload_list
%type<shared_ptr<ArgList>> arg_list
%type<shared_ptr<ParamList>> param_list

%start program

%%

program:
    |
    function program  {
        if(functions->find($1->ident->name) == functions->end()) {
            functions->insert(std::pair<std::string, std::shared_ptr<FuncDef>>($1->ident->name, $1));
        } else {
            std::cout << "ERROR: Redefinition of function " << $1->ident->name << std::endl;
        }
    }
    |
    vert_shader program { 
        if(shaders->find($1->name) == shaders->end()) {
            std::shared_ptr<ShaderPair> pair = make_shared<ShaderPair>();
            pair->name = $1->name;
            pair->vertex = $1;

            shaders->insert(std::pair<std::string, std::shared_ptr<ShaderPair>>($1->name, pair));
        } else {
            (*shaders)[$1->name]->vertex = $1;
        }
    }
    |
    frag_shader program { 
        if(shaders->find($1->name) == shaders->end()) {
            std::shared_ptr<ShaderPair> pair = make_shared<ShaderPair>();
            pair->name = $1->name;
            pair->fragment = $1;

            shaders->insert(std::pair<std::string, std::shared_ptr<ShaderPair>>($1->name, pair));
        } else {
            (*shaders)[$1->name]->fragment = $1;
        }
    }
    ;

vert_shader: T_VERTEX T_IDENTIFIER T_SHADER T_SEMICOLON { $$ = make_shared<ShaderSource>($2->name, $3->value, "vert"); }
    ;

frag_shader: T_FRAGMENT T_IDENTIFIER T_SHADER T_SEMICOLON { $$ = make_shared<ShaderSource>($2->name, $3->value, "frag"); }
    ;

function: T_FUNC T_IDENTIFIER T_OPEN_PAREN param_list T_CLOSE_PAREN block { $$ = make_shared<FuncDef>($2, $4, $6); set_lines($$, @1, @6); }
    ;

expr: T_INT { $$ = $1; set_lines($$,@1,@1); }
    | T_FLOAT { $$ = $1; set_lines($$, @1, @1); }
    | T_BOOL { $$ = $1; set_lines($$, @1, @1); }
    | T_IDENTIFIER { $$ = $1; set_lines($$, @1, @1); }
    | T_STRING { $$ = $1; set_lines($$, @1, @1); }
    | vec2 { $$ = $1; set_lines($$, @1, @1); }
    | vec3 { $$ = $1; set_lines($$, @1, @1); }
    | vec4 { $$ = $1; set_lines($$, @1, @1); }
    | invoke { $$ = make_shared<FuncExpr>($1); set_lines($$, @1, @1); }
    | dot { $$ = $1; set_lines($$, @1, @1); }
    | index { $$ = $1; set_lines($$, @1, @1); }
    | T_OPEN_BRACE list T_CLOSE_BRACE { $$ = $2; set_lines($$, @1, @3); }
    | expr T_PLUS expr { $$ = make_shared<Binary>($1, OP_PLUS, $3); set_lines($$, @1, @3); }
    | expr T_MINUS expr { $$ = make_shared<Binary>($1, OP_MINUS, $3); set_lines($$, @1, @3); }
    | expr T_MULT expr { $$ = make_shared<Binary>($1, OP_MULT, $3); set_lines($$, @1, @3); }
    | expr T_DIV expr { $$ = make_shared<Binary>($1, OP_DIV, $3); set_lines($$, @1, @3); }
    | expr T_MOD expr { $$ = make_shared<Binary>($1, OP_MOD, $3); set_lines($$, @1, @3); }
    | expr T_LESS_THAN expr { $$ = make_shared<Binary>($1, OP_LESSTHAN, $3); set_lines($$, @1, @3);}
    | expr T_GREATER_THAN expr { $$ = make_shared<Binary>($1, OP_GREATERTHAN, $3); set_lines($$, @1, @3); }
    | expr T_EQUAL expr { $$ = make_shared<Binary>($1, OP_EQUAL, $3); set_lines($$, @1, @3); }
    | expr T_NEQUAL expr { $$ = make_shared<Binary>($1, OP_NEQUAL, $3); set_lines($$, @1, @3); }
    | expr T_LEQUAL expr { $$ = make_shared<Binary>($1, OP_LEQUAL, $3); set_lines($$, @1, @3); }
    | expr T_GEQUAL expr { $$ = make_shared<Binary>($1, OP_GEQUAL, $3); set_lines($$, @1, @3); }
    | expr T_OR expr { $$ = make_shared<Binary>($1, OP_OR, $3); set_lines($$, @1, @3); }
    | expr T_AND expr { $$ = make_shared<Binary>($1, OP_AND, $3); set_lines($$, @1, @3); }
    | T_MINUS expr { $$ = make_shared<Unary>(OP_MINUS, $2); set_lines($$, @1, @2); } %prec UNARY
    | T_NOT expr { $$ = make_shared<Unary>(OP_NOT, $2); set_lines($$, @1, @2); } %prec UNARY
    | T_PIPE expr T_PIPE { $$ = make_shared<Unary>(OP_ABS, $2); set_lines($$, @1, @3); }
    | T_OPEN_PAREN expr T_CLOSE_PAREN { $$ = $2; set_lines($$, @1, @3); }
    ;

index: T_IDENTIFIER T_OPEN_BRACKET expr T_CLOSE_BRACKET { $$ = make_shared<Index>($1, $3); set_lines($$, @1, @4); }
    | index T_OPEN_BRACKET expr T_CLOSE_BRACKET { $$ = make_shared<Index>($1, $3); set_lines($$, @1, @4); }
    ;

dot: T_IDENTIFIER T_PERIOD T_IDENTIFIER { $$ = make_shared<Dot>($1, $3); }
    ;

stmt: T_IDENTIFIER T_EQUALS expr { $$ = make_shared<Assign>($1, $3); set_lines($$, @1, @3); }
    | T_IDENTIFIER T_IDENTIFIER { $$ = make_shared<Decl>($1, $2, nullptr); set_lines($$, @1, @2); }
    | T_IDENTIFIER T_IDENTIFIER T_EQUALS expr { $$ = make_shared<Decl>($1, $2, $4); set_lines($$, @1, @4); }
    | index T_EQUALS expr { $$ = make_shared<Assign>($1, $3); set_lines($$, @1, @3); }
    | dot T_EQUALS expr { $$ = make_shared<Assign>($1, $3); set_lines($$, @1, @3); }
    | T_ALLOCATE T_IDENTIFIER { $$ = make_shared<Alloc>($2); set_lines($$, @1, @2); }
    | T_IDENTIFIER T_PERIOD T_IDENTIFIER T_UPLOAD upload_list { $$ = make_shared<Upload>($1, $3, $5); set_lines($$, @1, @5); }
    | T_DRAW T_IDENTIFIER { $$ = make_shared<Draw>($2); set_lines($$, @1, @2); }
    | T_USE T_IDENTIFIER { $$ = make_shared<Use>($2); set_lines($$, @1, @2); }
    | T_PRINT expr { $$ = make_shared<Print>($2); set_lines($$, @1, @2); }
    | T_RETURN expr { $$ = make_shared<Return>($2); set_lines($$, @1, @2); }
    | invoke { $$ = make_shared<FuncStmt>($1); set_lines($$, @1, @1); }
    | T_IDENTIFIER T_COMP_PLUS expr { $$ = make_shared<Assign>($1, make_shared<Binary>($1, OP_PLUS, $3)); set_lines($$, @1, @3); }
    | T_IDENTIFIER T_COMP_MINUS expr { $$ = make_shared<Assign>($1, make_shared<Binary>($1, OP_MINUS, $3)); set_lines($$, @1, @3); }
    | T_IDENTIFIER T_COMP_MULT expr { $$ = make_shared<Assign>($1, make_shared<Binary>($1, OP_MULT, $3)); set_lines($$, @1, @3);}
    | T_IDENTIFIER T_COMP_DIV expr { $$ = make_shared<Assign>($1, make_shared<Binary>($1, OP_DIV, $3)); set_lines($$, @1, @3); }
    | T_IDENTIFIER T_COMP_MOD expr { $$ = make_shared<Assign>($1, make_shared<Binary>($1, OP_MOD, $3)); set_lines($$, @1, @3);}
    | index T_COMP_PLUS expr { $$ = make_shared<Assign>($1, make_shared<Binary>($1, OP_PLUS, $3)); set_lines($$, @1, @3); }
    | index T_COMP_MINUS expr { $$ = make_shared<Assign>($1, make_shared<Binary>($1, OP_MINUS, $3)); set_lines($$, @1, @3); }
    | index T_COMP_MULT expr { $$ = make_shared<Assign>($1, make_shared<Binary>($1, OP_MULT, $3)); set_lines($$, @1, @3);}
    | index T_COMP_DIV expr { $$ = make_shared<Assign>($1, make_shared<Binary>($1, OP_DIV, $3)); set_lines($$, @1, @3); }
    | index T_COMP_MOD expr { $$ = make_shared<Assign>($1, make_shared<Binary>($1, OP_MOD, $3)); set_lines($$, @1, @3);}
    ;

arg_list: { $$ = make_shared<ArgList>(nullptr); }
    | expr { $$ = make_shared<ArgList>($1); }
    | arg_list T_COMMA expr { $1->list.push_back($3); }
    ;

param_list: { $$ = make_shared<ParamList>(nullptr); }
    | T_IDENTIFIER { $$ = make_shared<ParamList>($1); }
    | param_list T_COMMA T_IDENTIFIER { $1->list.push_back($3); }
    ;

list: { $$ = make_shared<List>(nullptr); }
    | expr { $$ = make_shared<List>($1); }
    | list T_COMMA expr { $1->list.push_back($3); }
    ;

invoke: T_IDENTIFIER T_OPEN_PAREN arg_list T_CLOSE_PAREN { $$ = make_shared<Invoke>($1, $3); set_lines($$, @1, @1); }
    ;

stmt_block: T_IF T_OPEN_PAREN expr T_CLOSE_PAREN block { $$ = make_shared<If>($3, $5); }
    | T_IF T_OPEN_PAREN expr T_CLOSE_PAREN stmt T_SEMICOLON { $$ = make_shared<If>($3, make_shared<Stmts>($5)); }
    | T_WHILE T_OPEN_PAREN expr T_CLOSE_PAREN block { $$ = make_shared<While>($3, $5); }
    | T_FOR T_OPEN_PAREN T_IDENTIFIER T_IN expr T_COMMA expr T_COMMA expr T_CLOSE_PAREN block { $$ = make_shared<For>($3, $5, $7, $9, $11); }
    ;

stmts: { $$ = make_shared<Stmts>(nullptr); }
    | stmts stmt T_SEMICOLON { $1->list.insert($1->list.end(), $2); }
    | stmts stmt_block { $1->list.insert($1->list.end(), $2); }
    ;

block: T_OPEN_BRACE stmts T_CLOSE_BRACE { $$ = $2; }

upload_list: expr { $$ = make_shared<UploadList>($1); }
    | upload_list T_COMMA expr { $1->list.insert($1->list.end(), $3); }
    ;

vec2: T_OPEN_BRACKET expr T_COMMA expr T_CLOSE_BRACKET { $$ = make_shared<Vector2>($2, $4); set_lines($$, @1, @5); }
    ;

vec3: T_OPEN_BRACKET expr T_COMMA expr T_COMMA expr T_CLOSE_BRACKET { $$ = make_shared<Vector3>($2, $4, $6); set_lines($$, @1, @7); }
    ;

vec4: T_OPEN_BRACKET expr T_COMMA expr T_COMMA expr T_COMMA expr T_CLOSE_BRACKET { $$ = make_shared<Vector4>($2, $4, $6, $8); set_lines($$, @1, @9); }
    ;

%%

void yyerror(std::map<std::string, std::shared_ptr<ShaderPair>> *shaders, std::map<std::string, std::shared_ptr<FuncDef>> *functions, const std::shared_ptr<char> s) {
    std::cerr << "shaders: " << shaders << "\nfunctions: " << functions << std::endl;
    fprintf(stderr, "Parse error: %s\n", s);
}
