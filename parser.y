%skeleton "lalr1.cc"
%require "3.0"
%defines
%output "parser.cpp"
%define parser_class_name { Parser }

%define api.token.constructor
%define api.value.type variant
%define parse.assert
%define api.namespace { Prototype }
%code requires {
    #include <cstdio>
    #include <iostream>
    #include <vector>
    #include <cmath>
    #include <string>
    #include <memory>

    using namespace std;

    namespace Prototype {
        class Scanner;
    }

    #include "nodes.h"

    #define set_lines(a, b, c) \
        a->first_line = b.begin.line; \
        a->last_line = c.end.line; \
}

%code top {
    #include <iostream>
    #include "scanner.h"
    #include "parser.hpp"
    #include "interpreter.h"
    #include "location.hh"

    static Prototype::Parser::symbol_type yylex(Prototype::Scanner &scanner, unsigned int* line, unsigned int* column) {
        return scanner.get_next_token();
    }

    using namespace Prototype;
}

%lex-param { Prototype::Scanner &scanner }
%lex-param { unsigned int* line }
%lex-param { unsigned int* column }
%parse-param { Prototype::Scanner &scanner }
%parse-param { unsigned int* line }
%parse-param { unsigned int* column }
%parse-param { std::map<std::string, std::shared_ptr<FuncDef>>* functions }
%parse-param { std::map<std::string, std::shared_ptr<ShaderPair>>* shaders }
%locations
%define parse.trace
%define parse.error verbose

%define api.token.prefix {TOK_}

%token<shared_ptr<Bool>> BOOL;
%token<shared_ptr<Int>> INT;
%token<shared_ptr<Float>> FLOAT;
%token<shared_ptr<String>> STRING;
%token<shared_ptr<String>> SHADER;
%token<shared_ptr<Ident>> IDENTIFIER;

%token END 0 "eof";
%token SEMICOLON ";";
%token OPEN_BRACE "{";
%token CLOSE_BRACE "}";
%token PIPE "|";
%token OPEN_PAREN "(";
%token CLOSE_PAREN ")";
%token LESS_THAN "<";
%token GREATER_THAN ">";
%token OPEN_BRACKET "[";
%token CLOSE_BRACKET "]";
%token COMMA ",";
%token PERIOD  ".";
%token EQUALS "=";
%token COMP_PLUS "+=";
%token COMP_MINUS "-=";
%token COMP_MULT "*=";
%token COMP_DIV "/=";
%token COMP_MOD "%="; 
%token FUNC "func";
%token AND "and";
%token OR "or";
%token NOT "not";
%token IF "if";
%token WHILE "while";
%token FOR "for";
%token IN "in";
%token ALLOCATE "allocate"; 
%token UPLOAD "<-";
%token DRAW "draw";
%token VERTEX "vert";
%token FRAGMENT "frag";
%token PRINT "print";
%token USE "use";
%token RETURN "return";

%left PLUS MINUS
%left MULT DIV MOD
%left AND OR
%right UNARY

%nonassoc EQUAL NEQUAL GEQUAL LEQUAL
%nonassoc LESS_THAN GREATER_THAN

%type<shared_ptr<Invoke>> invoke;
%type<shared_ptr<Expr>> expr index
%type<shared_ptr<Vector2>> vec2
%type<shared_ptr<Vector3>> vec3
%type<shared_ptr<Vector4>> vec4
%type<shared_ptr<Dot>> dot;
%type<shared_ptr<List>> list;
%type<shared_ptr<Decl>> decl;

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

vert_shader: VERTEX IDENTIFIER SHADER SEMICOLON { $$ = make_shared<ShaderSource>($2->name, $3->value, "vert"); }
    ;

frag_shader: FRAGMENT IDENTIFIER SHADER SEMICOLON { $$ = make_shared<ShaderSource>($2->name, $3->value, "frag"); }
    ;

function: FUNC IDENTIFIER OPEN_PAREN param_list CLOSE_PAREN block { $$ = make_shared<FuncDef>($2, $4, $6); set_lines($$, @1, @6); }
    ;

expr: INT { $$ = $1; set_lines($$,@1,@1); }
    | FLOAT { $$ = $1; set_lines($$, @1, @1); }
    | BOOL { $$ = $1; set_lines($$, @1, @1); }
    | IDENTIFIER { $$ = $1; set_lines($$, @1, @1); }
    | STRING { $$ = $1; set_lines($$, @1, @1); }
    | vec2 { $$ = $1; set_lines($$, @1, @1); }
    | vec3 { $$ = $1; set_lines($$, @1, @1); }
    | vec4 { $$ = $1; set_lines($$, @1, @1); }
    | invoke { $$ = make_shared<FuncExpr>($1); set_lines($$, @1, @1); }
    | dot { $$ = $1; set_lines($$, @1, @1); }
    | index { $$ = $1; set_lines($$, @1, @1); }
    | OPEN_BRACE list CLOSE_BRACE { $$ = $2; set_lines($$, @1, @3); }
    | expr PLUS expr { $$ = make_shared<Binary>($1, OP_PLUS, $3); set_lines($$, @1, @3); }
    | expr MINUS expr { $$ = make_shared<Binary>($1, OP_MINUS, $3); set_lines($$, @1, @3); }
    | expr MULT expr { $$ = make_shared<Binary>($1, OP_MULT, $3); set_lines($$, @1, @3); }
    | expr DIV expr { $$ = make_shared<Binary>($1, OP_DIV, $3); set_lines($$, @1, @3); }
    | expr MOD expr { $$ = make_shared<Binary>($1, OP_MOD, $3); set_lines($$, @1, @3); }
    | expr LESS_THAN expr { $$ = make_shared<Binary>($1, OP_LESSTHAN, $3); set_lines($$, @1, @3);}
    | expr GREATER_THAN expr { $$ = make_shared<Binary>($1, OP_GREATERTHAN, $3); set_lines($$, @1, @3); }
    | expr EQUAL expr { $$ = make_shared<Binary>($1, OP_EQUAL, $3); set_lines($$, @1, @3); }
    | expr NEQUAL expr { $$ = make_shared<Binary>($1, OP_NEQUAL, $3); set_lines($$, @1, @3); }
    | expr LEQUAL expr { $$ = make_shared<Binary>($1, OP_LEQUAL, $3); set_lines($$, @1, @3); }
    | expr GEQUAL expr { $$ = make_shared<Binary>($1, OP_GEQUAL, $3); set_lines($$, @1, @3); }
    | expr OR expr { $$ = make_shared<Binary>($1, OP_OR, $3); set_lines($$, @1, @3); }
    | expr AND expr { $$ = make_shared<Binary>($1, OP_AND, $3); set_lines($$, @1, @3); }
    | MINUS expr { $$ = make_shared<Unary>(OP_MINUS, $2); set_lines($$, @1, @2); } %prec UNARY
    | NOT expr { $$ = make_shared<Unary>(OP_NOT, $2); set_lines($$, @1, @2); } %prec UNARY
    | PIPE expr PIPE { $$ = make_shared<Unary>(OP_ABS, $2); set_lines($$, @1, @3); }
    | OPEN_PAREN expr CLOSE_PAREN { $$ = $2; set_lines($$, @1, @3); }
    ;

index: IDENTIFIER OPEN_BRACKET expr CLOSE_BRACKET { $$ = make_shared<Index>($1, $3); set_lines($$, @1, @4); }
    | index OPEN_BRACKET expr CLOSE_BRACKET { $$ = make_shared<Index>($1, $3); set_lines($$, @1, @4); }
    ;

dot: IDENTIFIER PERIOD IDENTIFIER { $$ = make_shared<Dot>($1, $3); }
    ;

decl: IDENTIFIER IDENTIFIER { $$ = make_shared<Decl>($1, $2, nullptr); set_lines($$, @1, @2); }
    ;

stmt: IDENTIFIER EQUALS expr { $$ = make_shared<Assign>($1, $3); set_lines($$, @1, @3); }
    | decl { $$ = $1; }
    | decl EQUALS expr { $1->value = $3; set_lines($$, @1, @3); }
    | index EQUALS expr { $$ = make_shared<Assign>($1, $3); set_lines($$, @1, @3); }
    | dot EQUALS expr { $$ = make_shared<Assign>($1, $3); set_lines($$, @1, @3); }
    | ALLOCATE IDENTIFIER { $$ = make_shared<Alloc>($2); set_lines($$, @1, @2); }
    | IDENTIFIER PERIOD IDENTIFIER UPLOAD upload_list { $$ = make_shared<Upload>($1, $3, $5); set_lines($$, @1, @5); }
    | DRAW IDENTIFIER { $$ = make_shared<Draw>($2); set_lines($$, @1, @2); }
    | USE IDENTIFIER { $$ = make_shared<Use>($2); set_lines($$, @1, @2); }
    | PRINT expr { $$ = make_shared<Print>($2); set_lines($$, @1, @2); }
    | RETURN expr { $$ = make_shared<Return>($2); set_lines($$, @1, @2); }
    | invoke { $$ = make_shared<FuncStmt>($1); set_lines($$, @1, @1); }
    | IDENTIFIER COMP_PLUS expr { $$ = make_shared<Assign>($1, make_shared<Binary>($1, OP_PLUS, $3)); set_lines($$, @1, @3); }
    | IDENTIFIER COMP_MINUS expr { $$ = make_shared<Assign>($1, make_shared<Binary>($1, OP_MINUS, $3)); set_lines($$, @1, @3); }
    | IDENTIFIER COMP_MULT expr { $$ = make_shared<Assign>($1, make_shared<Binary>($1, OP_MULT, $3)); set_lines($$, @1, @3);}
    | IDENTIFIER COMP_DIV expr { $$ = make_shared<Assign>($1, make_shared<Binary>($1, OP_DIV, $3)); set_lines($$, @1, @3); }
    | IDENTIFIER COMP_MOD expr { $$ = make_shared<Assign>($1, make_shared<Binary>($1, OP_MOD, $3)); set_lines($$, @1, @3);}
    | index COMP_PLUS expr { $$ = make_shared<Assign>($1, make_shared<Binary>($1, OP_PLUS, $3)); set_lines($$, @1, @3); }
    | index COMP_MINUS expr { $$ = make_shared<Assign>($1, make_shared<Binary>($1, OP_MINUS, $3)); set_lines($$, @1, @3); }
    | index COMP_MULT expr { $$ = make_shared<Assign>($1, make_shared<Binary>($1, OP_MULT, $3)); set_lines($$, @1, @3);}
    | index COMP_DIV expr { $$ = make_shared<Assign>($1, make_shared<Binary>($1, OP_DIV, $3)); set_lines($$, @1, @3); }
    | index COMP_MOD expr { $$ = make_shared<Assign>($1, make_shared<Binary>($1, OP_MOD, $3)); set_lines($$, @1, @3);}
    ;

arg_list: { $$ = make_shared<ArgList>(nullptr); }
    | expr { $$ = make_shared<ArgList>($1); }
    | arg_list COMMA expr { $$ = $1; $1->list.push_back($3); }
    ;

param_list: { $$ = make_shared<ParamList>(nullptr); }
    | IDENTIFIER { $$ = make_shared<ParamList>($1); }
    | param_list COMMA IDENTIFIER { $$ = $1; $1->list.push_back($3); }
    ;

list: { $$ = make_shared<List>(nullptr); }
    | expr { $$ = make_shared<List>($1); }
    | list COMMA expr { $$ = $1; $1->list.push_back($3); }
    ;

invoke: IDENTIFIER OPEN_PAREN arg_list CLOSE_PAREN { $$ = make_shared<Invoke>($1, $3); set_lines($$, @1, @1); }
    ;

stmt_block: IF OPEN_PAREN expr CLOSE_PAREN block { $$ = make_shared<If>($3, $5); }
    | IF OPEN_PAREN expr CLOSE_PAREN stmt SEMICOLON { $$ = make_shared<If>($3, make_shared<Stmts>($5)); }
    | WHILE OPEN_PAREN expr CLOSE_PAREN block { $$ = make_shared<While>($3, $5); }
    | FOR OPEN_PAREN IDENTIFIER IN expr COMMA expr COMMA expr CLOSE_PAREN block { $$ = make_shared<For>($3, $5, $7, $9, $11); }
    ;

stmts: { $$ = make_shared<Stmts>(nullptr); }
    | stmts stmt SEMICOLON { $$ = $1; $1->list.insert($1->list.end(), $2); }
    | stmts stmt_block { $$ = $1; $1->list.insert($1->list.end(), $2); }
    ;

block: OPEN_BRACE stmts CLOSE_BRACE { $$ = $2; }

upload_list: expr { $$ = make_shared<UploadList>($1); }
    | upload_list COMMA expr { $$ = $1; $1->list.insert($1->list.end(), $3); }
    ;

vec2: OPEN_BRACKET expr COMMA expr CLOSE_BRACKET { $$ = make_shared<Vector2>($2, $4); set_lines($$, @1, @5); }
    ;

vec3: OPEN_BRACKET expr COMMA expr COMMA expr CLOSE_BRACKET { $$ = make_shared<Vector3>($2, $4, $6); set_lines($$, @1, @7); }
    ;

vec4: OPEN_BRACKET expr COMMA expr COMMA expr COMMA expr CLOSE_BRACKET { $$ = make_shared<Vector4>($2, $4, $6, $8); set_lines($$, @1, @9); }
    ;

%%

void Parser::error(const location &loc, const string &message) {
    cerr << "Error: " << message << " at line " << loc.begin.line << endl;
}

