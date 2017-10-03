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
void yyerror(Stmts** init, Stmts** loop, const char *s);
}

%debug

%defines "parser.h"
%output "parser.cpp"
%error-verbose

%parse-param { Stmts** init } { Stmts** loop }

%union {
	Expr* eval;
	Int* ival;
	Float* fval;
	Ident* idval;
	Vector3* vval;
	Stmt* sval;
    Stmts* svval;

	UploadList* lval;
}

%token<ival> INT 
%token<fval> FLOAT
%token<idval> IDENTIFIER

%token SEMICOLON OPEN_BRACE CLOSE_BRACE
%token PIPE
%token PLUS LEFT RIGHT
%token OPEN_PAREN CLOSE_PAREN LESS_THAN GREATER_THAN OPEN_BRACKET CLOSE_BRACKET COMMA PERIOD EQUALS
%token INIT LOOP ALLOCATE UPLOAD DRAW

%left PLUS MINUS
%left MULT DIV MOD

%type<eval> expr
%type<eval> scalar vector 
%type<vval> vec3

%type<sval> stmt
%type<svval> stmts block
%type<lval> upload_list

%start program 
%%

program:
    | INIT block LOOP block program { *init = $2; *loop = $4; }
	| expr SEMICOLON program { }
	| stmt SEMICOLON program { }
	;

expr: scalar { $$ = $1; }
	| vector { $$ = $1; }
	;

stmt: IDENTIFIER EQUALS expr { $$ = new Assign($1, $3); }
	| ALLOCATE IDENTIFIER { $$ = new Alloc($2); }
	| IDENTIFIER UPLOAD upload_list { $$ = new Upload($1, $3); }
    | DRAW IDENTIFIER { $$ = new Draw($2); }
    | IDENTIFIER PLUS EQUALS expr { $$ = new Assign($1, new Binary($1, OP_PLUS, $4)); }
    | IDENTIFIER MINUS EQUALS expr { $$ = new Assign($1, new Binary($1, OP_MINUS, $4)); }
    | IDENTIFIER MULT EQUALS expr { $$ = new Assign($1, new Binary($1, OP_MULT, $4)); }
    | IDENTIFIER DIV EQUALS expr { $$ = new Assign($1, new Binary($1, OP_DIV, $4)); }
    | IDENTIFIER MOD EQUALS expr { $$ = new Assign($1, new Binary($1, OP_MOD, $4)); }
	;

stmts: { $$ = new Stmts(0); }
    | stmt SEMICOLON { $$ = new Stmts($1); }
    | stmts stmt SEMICOLON { $1->list.insert($1->list.end(), $2); }
    ;

block: OPEN_BRACE stmts CLOSE_BRACE { $$ = $2; }

upload_list: vec3 { $$ = new UploadList($1); }
	| upload_list vec3 { $1->list.insert($1->list.end(), $2); } 
    | scalar { $$ = new UploadList($1); }
	| upload_list scalar { $1->list.insert($1->list.end(), $2); }
    | IDENTIFIER { $$ = new UploadList($1); }
    | upload_list IDENTIFIER { $1->list.insert($1->list.end(), $2); }
	;

scalar: INT { $$ = $1; }
	 | FLOAT { $$ = $1; }
	 | IDENTIFIER { $$ = $1; }
	 | scalar PLUS scalar { $$ = new Binary($1, OP_PLUS, $3); }
	 | scalar MINUS scalar { $$ = new Binary($1, OP_MINUS, $3); }
	 | scalar MULT scalar { $$ = new Binary($1, OP_MULT, $3); }
	 | scalar DIV scalar { $$ = new Binary($1, OP_DIV, $3); }
     | MINUS scalar { $$ = new Unary(OP_MINUS, $2); }
	 | vector MULT vector { $$ = new Binary($1, OP_MULT, $3); }
	 | OPEN_PAREN scalar CLOSE_PAREN { $$ = $2; }
	 ;

vector: vec3 { $$ = $1; }
	| IDENTIFIER { $$ = $1; }
	| vector PLUS vector { $$ = new Binary($1, OP_PLUS, $3); }
	| vector MINUS vector { $$ = new Binary($1, OP_MINUS, $3); }
	| vector MOD vector { $$ = new Binary($1, OP_MOD, $3); }
	| vector MULT scalar { $$ = new Binary($1, OP_MULT, $3); }
    | MINUS vector { $$ = new Unary(OP_MULT, $2); }
	| scalar MULT vector { $$ = new Binary($1, OP_MULT, $3); }
	| vector DIV scalar { $$ = new Binary($1, OP_DIV, $3); }
	| OPEN_PAREN vector CLOSE_PAREN { $$ = $2; }
	;

vec3: OPEN_BRACKET scalar COMMA scalar COMMA scalar CLOSE_BRACKET { $$ = new Vector3($2, $4, $6); }
	;

/*
iexpr: INT { $$ = $1; }
	| iexpr PLUS iexpr { $$ = new Binary($1, OP_PLUS, $3); }
	| iexpr MINUS iexpr { $$ = new Binary($1, OP_MINUS, $3); }
	| iexpr MULT iexpr { $$ = new Binary($1, OP_MULT, $3); }
	| OPEN_PAREN iexpr CLOSE_PAREN { $$ = $2; }
	;

fexpr: FLOAT { $$ = $1; }
	| fexpr PLUS fexpr { $$ = new Binary($1, OP_PLUS, $3); }
	| fexpr MINUS fexpr { $$ = new Binary($1, OP_MINUS, $3); }
	| fexpr MULT fexpr { $$ = new Binary($1, OP_MULT, $3); }
	| fexpr DIV fexpr { $$ = new Binary($1, OP_DIV, $3); }
	| iexpr DIV iexpr { $$ = new Binary($1, OP_DIV, $3); }
	| v3expr MULT v3expr { $$ = new Binary($1, OP_MULT, $3); }
	| OPEN_PAREN fexpr CLOSE_PAREN { $$ = $2; }
	;

v3expr: vec3 { $$ = $1; }
	| v3expr PLUS v3expr { $$ = new Binary($1, OP_PLUS, $3); }
	| v3expr MINUS v3expr { $$ = new Binary($1, OP_MINUS, $3); }
	| v3expr MULT fexpr { $$ = new Binary($1, OP_MULT, $3); }
	| fexpr MULT v3expr { $$ = new Binary($1, OP_MULT, $3); }
	| iexpr MULT v3expr { $$ = new Binary($1, OP_MULT, $3); }
	| v3expr MULT iexpr { $$ = new Binary($1, OP_MULT, $3); }
	| v3expr MOD v3expr { $$ = new Binary($1, OP_MOD, $3); }
	| v3expr DIV fexpr { $$ = new Binary($1, OP_DIV, $3); }
	| OPEN_PAREN v3expr CLOSE_PAREN { $$ = $2; }
	;
*/

%%

void yyerror(Stmts** init, Stmts** loop, const char* s) {
	fprintf(stderr, "Parse error: %s\n", s);
}
