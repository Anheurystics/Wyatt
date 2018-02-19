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

    namespace Prototype {
        class Scanner;
    }

    using namespace std;

    #include "nodes.h"
    #include "logwindow.h"

    #define set_lines(a, b, c) \
        a->first_line = b.begin.line; \
        a->last_line = c.end.line; \
}

%code top {
    #include "scanner.h"
    #include "parser.hpp"
    #include "interpreter.h"
    #include "location.hh"

    static Prototype::Parser::symbol_type yylex(Prototype::Scanner &scanner) {
        return scanner.get_next_token();
    }
}

%lex-param { Prototype::Scanner &scanner }
%parse-param { Prototype::Scanner &scanner }
%parse-param { LogWindow* logger }
%parse-param { unsigned int* line }
%parse-param { unsigned int* column }
%parse-param { vector<string>* imports }
%parse-param { vector<Decl_ptr>* globals }
%parse-param { map<string, FuncDef_ptr>* functions }
%parse-param { map<string, shared_ptr<ShaderPair>>* shaders }
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
%token ELSE "else";
%token WHILE "while";
%token FOR "for";
%token IN "in";
%token IMPORT "import";
%token ALLOCATE "allocate"; 
%token UPLOAD "<-";
%token DRAW "draw";
%token TO "to";
%token USING "using";
%token CLEAR "clear";
%token VERTEX "vert";
%token FRAGMENT "frag";
%token MAIN "main";
%token PRINT "print";
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
%type<shared_ptr<vector<pair<string, string>>>> shader_uniforms;
%type<shared_ptr<map<string, FuncDef_ptr>>> shader_functions;

%type<shared_ptr<Stmt>> stmt stmt_block
%type<shared_ptr<If>> if_stmt else_if_stmt
%type<shared_ptr<vector<shared_ptr<If>>>> else_if_chain
%type<shared_ptr<FuncDef>> function
%type<shared_ptr<Stmts>> stmts block
%type<shared_ptr<Shader>> vert_shader frag_shader
%type<shared_ptr<UploadList>> upload_list
%type<shared_ptr<ArgList>> arg_list
%type<shared_ptr<ParamList>> param_list

%start program

%%

program: imports globals funcshaders;

imports:
    |
    IMPORT STRING SEMICOLON imports {
        imports->push_back($2->value);
    }
    ;

globals:
    |
    decl SEMICOLON globals {
        globals->push_back($1);
    }
    |
    decl EQUALS expr SEMICOLON globals {
        $1->value = $3;
        globals->push_back($1);
    }
    ;

funcshaders:
    |
    function funcshaders{
        if(functions->find($1->ident->name) == functions->end()) {
            functions->insert(pair<string, shared_ptr<FuncDef>>($1->ident->name, $1));
        } else {
            cout << "ERROR: Redefinition of function " << $1->ident->name << endl;
        }
    }
    |
    vert_shader funcshaders{
        if(shaders->find($1->name) == shaders->end()) {
            shared_ptr<ShaderPair> shaderPair = make_shared<ShaderPair>();
            shaderPair->name = $1->name;
            shaderPair->vertex = $1;

            shaders->insert(pair<string, shared_ptr<ShaderPair>>($1->name, shaderPair));
        } else {
            (*shaders)[$1->name]->vertex = $1;
        }
    }
    |
    frag_shader funcshaders {
        if(shaders->find($1->name) == shaders->end()) {
            shared_ptr<ShaderPair> shaderPair = make_shared<ShaderPair>();
            shaderPair->name = $1->name;
            shaderPair->fragment = $1;

            shaders->insert(pair<string, shared_ptr<ShaderPair>>($1->name, shaderPair));
        } else {
            (*shaders)[$1->name]->fragment = $1;
        }
    }
    ;
    
shader_uniforms: { $$ = make_shared<vector<pair<string, string>>>(); }
    | shader_uniforms decl SEMICOLON { $$ = $1; $1->push_back(pair<string, string>($2->name->name, $2->datatype->name)); }
    ;

shader_functions: FUNC MAIN OPEN_PAREN CLOSE_PAREN block { $$ = make_shared<map<string, FuncDef_ptr>>(); $$->insert(pair<string, FuncDef_ptr>("main", make_shared<FuncDef>(make_shared<Ident>("main"), make_shared<ParamList>(nullptr), $5)));}
    | function shader_functions { $2->insert(pair<string, FuncDef_ptr>($1->ident->name, $1)); }
    ;

vert_shader: VERTEX IDENTIFIER OPEN_PAREN param_list CLOSE_PAREN OPEN_BRACE shader_uniforms shader_functions CLOSE_BRACE OPEN_PAREN param_list CLOSE_PAREN {
        $$ = make_shared<Shader>($2->name, $7, $8, $4, $11);
    }
    ;
frag_shader: FRAGMENT IDENTIFIER OPEN_PAREN param_list CLOSE_PAREN OPEN_BRACE shader_uniforms shader_functions CLOSE_BRACE OPEN_PAREN param_list CLOSE_PAREN {
        $$ = make_shared<Shader>($2->name, $7, $8, $4, $11);
    }
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
    | decl EQUALS expr { $$ = $1; $1->value = $3; set_lines($$, @1, @3); }
    | index EQUALS expr { $$ = make_shared<Assign>($1, $3); set_lines($$, @1, @3); }
    | dot EQUALS expr { $$ = make_shared<Assign>($1, $3); set_lines($$, @1, @3); }
    | ALLOCATE IDENTIFIER { $$ = make_shared<Alloc>($2); set_lines($$, @1, @2); }
    | IDENTIFIER PERIOD IDENTIFIER COMP_PLUS upload_list { $$ = make_shared<Upload>($1, $3, $5); set_lines($$, @1, @5); }
    | CLEAR { $$ = make_shared<Clear>(nullptr); set_lines($$, @1, @1); }
    | CLEAR expr { $$ = make_shared<Clear>($2); set_lines($$, @1, @2); }
    | DRAW IDENTIFIER { $$ = make_shared<Draw>($2); set_lines($$, @1, @2); }
    | DRAW IDENTIFIER TO IDENTIFIER { $$ = make_shared<Draw>($2, $4); set_lines($$, @1, @4); }
    | DRAW IDENTIFIER USING IDENTIFIER { $$ = make_shared<Draw>($2, nullptr, $4); set_lines($$, @1, @4); }
    | DRAW IDENTIFIER TO IDENTIFIER USING IDENTIFIER { $$ = make_shared<Draw>($2, $4, $6); set_lines($$, @1, @6); }
    | PRINT expr { $$ = make_shared<Print>($2); set_lines($$, @1, @2); }
    | RETURN expr { $$ = make_shared<Return>($2); set_lines($$, @1, @2); }
    | invoke { $$ = make_shared<FuncStmt>($1); set_lines($$, @1, @1); }
    | IDENTIFIER COMP_PLUS upload_list { $$ = make_shared<CompBinary>($1, OP_PLUS, $3); set_lines($$, @1, @3); }
    | IDENTIFIER COMP_MINUS expr { $$ = make_shared<CompBinary>($1, OP_MINUS, $3); set_lines($$, @1, @3); }
    | IDENTIFIER COMP_MULT expr { $$ = make_shared<CompBinary>($1, OP_MULT, $3); set_lines($$, @1, @3); }
    | IDENTIFIER COMP_DIV expr { $$ = make_shared<CompBinary>($1, OP_DIV, $3); set_lines($$, @1, @3); }
    | IDENTIFIER COMP_MOD expr { $$ = make_shared<CompBinary>($1, OP_MOD, $3); set_lines($$, @1, @3); }
    | index COMP_PLUS expr { $$ = make_shared<CompBinary>($1, OP_PLUS, $3); set_lines($$, @1, @3); }
    | index COMP_MINUS expr { $$ = make_shared<CompBinary>($1, OP_MINUS, $3); set_lines($$, @1, @3); }
    | index COMP_MULT expr { $$ = make_shared<CompBinary>($1, OP_MULT, $3); set_lines($$, @1, @3); }
    | index COMP_DIV expr { $$ = make_shared<CompBinary>($1, OP_DIV, $3); set_lines($$, @1, @3); }
    | index COMP_MOD expr { $$ = make_shared<CompBinary>($1, OP_MOD, $3); set_lines($$, @1, @3); }
    ;

arg_list: { $$ = make_shared<ArgList>(nullptr); }
    | expr { $$ = make_shared<ArgList>($1); }
    | arg_list COMMA expr { $$ = $1; $1->list.push_back($3); }
    ;

param_list: { $$ = make_shared<ParamList>(nullptr); }
    | decl { $$ = make_shared<ParamList>($1); }
    | param_list COMMA decl { $$ = $1; $1->list.push_back($3); }
    ;

list: { $$ = make_shared<List>(nullptr); }
    | expr { $$ = make_shared<List>($1); }
    | list COMMA expr { $$ = $1; $1->list.push_back($3); }
    ;

invoke: IDENTIFIER OPEN_PAREN arg_list CLOSE_PAREN { $$ = make_shared<Invoke>($1, $3); set_lines($$, @1, @1); }
    ;

stmt_block: if_stmt { $$ = $1; }
    | if_stmt ELSE stmt SEMICOLON { $$ = $1; $1->elseBlock = make_shared<Stmts>($3); }
    | if_stmt ELSE block { $$ = $1; $1->elseBlock = $3; }
    | if_stmt else_if_chain { $$ = $1; $1->elseIfBlocks = $2; }
    | if_stmt else_if_chain ELSE stmt SEMICOLON { $$ = $1; $1->elseIfBlocks = $2; $1->elseBlock = make_shared<Stmts>($4); }
    | if_stmt else_if_chain ELSE block { $$ = $1; $1->elseIfBlocks = $2; $1->elseBlock = $4; }
    | WHILE OPEN_PAREN expr CLOSE_PAREN block { $$ = make_shared<While>($3, $5); }
    | FOR OPEN_PAREN IDENTIFIER IN expr COMMA expr COMMA expr CLOSE_PAREN block { $$ = make_shared<For>($3, $5, $7, $9, $11); }
    | FOR OPEN_PAREN IDENTIFIER IN IDENTIFIER CLOSE_PAREN block { $$ = make_shared<For>($3, $5, $7); }
    ;

if_stmt: IF OPEN_PAREN expr CLOSE_PAREN block { $$ = make_shared<If>($3, $5); }
    | IF OPEN_PAREN expr CLOSE_PAREN stmt SEMICOLON { $$ = make_shared<If>($3, make_shared<Stmts>($5)); }
   ;

else_if_chain: else_if_stmt { $$ = make_shared<vector<shared_ptr<If>>>(); $$->push_back($1); }
    | else_if_chain else_if_stmt { $$ = $1; $1->push_back($2); }
    ;

else_if_stmt: ELSE if_stmt { $$ = $2; }
    ;

stmts: { $$ = make_shared<Stmts>(nullptr); }
    | stmts stmt SEMICOLON { $$ = $1; $1->list.push_back($2); }
    | stmts stmt_block { $$ = $1; $1->list.push_back($2); }
    ;

block: OPEN_BRACE stmts CLOSE_BRACE { $$ = $2; }

upload_list: expr { $$ = make_shared<UploadList>($1); }
    | upload_list COMMA expr { $$ = $1; $1->list.push_back($3); }
    ;

vec2: OPEN_BRACKET expr COMMA expr CLOSE_BRACKET { $$ = make_shared<Vector2>($2, $4); set_lines($$, @1, @5); }
    ;

vec3: OPEN_BRACKET expr COMMA expr COMMA expr CLOSE_BRACKET { $$ = make_shared<Vector3>($2, $4, $6); set_lines($$, @1, @7); }
    ;

vec4: OPEN_BRACKET expr COMMA expr COMMA expr COMMA expr CLOSE_BRACKET { $$ = make_shared<Vector4>($2, $4, $6, $8); set_lines($$, @1, @9); }
    ;

%%

void Prototype::Parser::error(const Prototype::location &loc, const string &message) {
    logger->log("ERROR: " + message + " at line " + to_string(loc.begin.line) + "\n");
}
