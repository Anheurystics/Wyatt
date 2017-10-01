%{
#include <cstdio>
#include <iostream>
#include <cmath>
%}

%code requires {
#include <string>
#include <vector>
#include "nodes.h"

int yylex();
void yyerror(std::vector<Node*>* nodes, const char *s);
}

%debug

%defines "parser.h"
%output "parser.cpp"
%error-verbose

%parse-param { std::vector<Node*>* nodes }

%union {
	Expr* eval;
	Int* ival;
	Float* fval;
	Ident* idval;
	Vector3* vval;
	Stmt* sval;

	UploadList* lval;
}

%token<ival> INT 
%token<fval> FLOAT
%token<idval> IDENTIFIER

%token PIPE
%token PLUS LEFT RIGHT
%token OPEN_PAREN CLOSE_PAREN LESS_THAN GREATER_THAN COMMA EQUALS
%token ALLOCATE UPLOAD
%token<eval> NEWLINE

%left PLUS MINUS
%left MULT DIV MOD

%type<eval> expr
%type<eval> scalar vector 
%type<vval> vec3

%type<sval> stmt
%type<lval> upload_list

%start program 
%%

program:
	| expr NEWLINE program { nodes->insert(nodes->begin(), $1); }
	| stmt NEWLINE program { nodes->insert(nodes->begin(), $1); }
	;

expr: scalar { $$ = $1; }
	| vector { $$ = $1; }
	;

stmt: IDENTIFIER EQUALS expr { $$ = new Assign($1, $3); }
	| ALLOCATE IDENTIFIER { $$ = new Alloc($2); }
	| IDENTIFIER UPLOAD upload_list { $$ = new Upload($1, $3); }
	;

upload_list: vec3 { $$ = new UploadList($1); }
	| upload_list vec3 { $1->list.insert($1->list.end(), $2); } 
    | scalar { $$ = new UploadList($1); }
	| upload_list scalar { $1->list.insert($1->list.end(), $2); }
	;

scalar: INT { $$ = $1; }
	 | FLOAT { $$ = $1; }
	 | IDENTIFIER { $$ = $1; }
	 | scalar PLUS scalar { $$ = new Binary($1, OP_PLUS, $3); }
	 | scalar MINUS scalar { $$ = new Binary($1, OP_MINUS, $3); }
	 | scalar MULT scalar { $$ = new Binary($1, OP_MULT, $3); }
	 | scalar DIV scalar { $$ = new Binary($1, OP_DIV, $3); }
	 | vector MULT vector { $$ = new Binary($1, OP_MULT, $3); }
	 | OPEN_PAREN scalar CLOSE_PAREN { $$ = $2; }
	 ;

vector: vec3 { $$ = $1; }
	| IDENTIFIER { $$ = $1; }
	| vector PLUS vector { $$ = new Binary($1, OP_PLUS, $3); }
	| vector MINUS vector { $$ = new Binary($1, OP_MINUS, $3); }
	| vector MOD vector { $$ = new Binary($1, OP_MOD, $3); }
	| vector MULT scalar { $$ = new Binary($1, OP_MULT, $3); }
	| scalar MULT vector { $$ = new Binary($1, OP_MULT, $3); }
	| vector DIV scalar { $$ = new Binary($1, OP_DIV, $3); }
	| OPEN_PAREN vector CLOSE_PAREN { $$ = $2; }
	;

vec3: LESS_THAN scalar COMMA scalar COMMA scalar GREATER_THAN { $$ = new Vector3($2, $4, $6); }
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

void yyerror(std::vector<Node*>* nodes, const char* s) {
	fprintf(stderr, "Parse error: %s\n", s);
}
