%option noyywrap

%{
#include <cstdlib>
#include "parser.h"

int yylex();
void yyerror(const char *s);
%}

%option header-file="scanner.h"
%option outfile="scanner.cpp"
%option noyywrap
%option nounput

%%
"vert"         { return VERTEX; }
"frag"         { return FRAGMENT; }
"`"(\\.|[^`])*"`" { yylval.idval = new Ident(strdup(yytext));  return SHADER; }
[ \t\n]          ;
[0-9]+\.[0-9]+ { yylval.fval = new Float(float(atof(yytext))); return FLOAT; }
[0-9]+         { yylval.ival = new Int(atoi(yytext)); return INT; }
"allocate"     { return ALLOCATE; }
"init"         { return INIT; }
"loop"         { return LOOP; }
"draw"         { return DRAW; }
"and"          { return AND; }
"or"           { return OR; }
"true"         { yylval.bval = new Bool(true); return BOOL; }
"false"        { yylval.bval = new Bool(false); return BOOL; }
[a-zA-Z]+[a-zA-Z0-9]* { yylval.idval = new Ident(strdup(yytext)); return IDENTIFIER; }
"("            { return OPEN_PAREN; }
")"            { return CLOSE_PAREN; }
"<-"           { return UPLOAD; }
"<"            { return LESS_THAN; }
">"            { return GREATER_THAN; }
"["            { return OPEN_BRACKET; }
"]"            { return CLOSE_BRACKET; }
"="            { return EQUALS; }
"."            { return PERIOD; }
","            { return COMMA; }
"+"            { return PLUS; }
"-"		       { return MINUS; }
"*"            { return MULT; }
"!"            { return NOT; }
"/"            { return DIV; }
"|"			   { return PIPE; }
";"            { return SEMICOLON; }
"{"            { return OPEN_BRACE; }
"}"            { return CLOSE_BRACE; }
"%"            { return MOD; }
.			   { printf("Illegal token!\n"); }
%%

