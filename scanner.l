%option noyywrap

%{
#include <cstdlib>
#include <string>
#include <memory>
#include "parser.h"

using namespace std;

#define YY_USER_ACTION \
    if(yylloc.first_line != yylineno) yycolumn = 0; \
    yylloc.first_line = yylloc.last_line = yylineno; \
    yylloc.first_column = yycolumn; yylloc.last_column = yycolumn + yyleng - 1; \
    yycolumn += yyleng; \

int yycolumn = 0;
int yylex();
void yyerror(const char *s);
%}

%option header-file="scanner.h"
%option outfile="scanner.cpp"
%option yylineno
%option noyywrap
%option nounput

%%
"vert"         { return T_VERTEX; }
"frag"         { return T_FRAGMENT; }
"`"(\\.|[^`])*"`" { std::string str = strdup(yytext); str = str.substr(1, str.length() - 2); yylval = make_shared<String>(str); return T_SHADER; }
"\""(\\.|[^\\""])*"\"" { std::string str = strdup(yytext); str = str.substr(1, str.length() - 2); yylval = make_shared<String>(str); return T_STRING; }
"//".*         ;
[ \t\n]        ;
[0-9]+\.[0-9]+ { yylval = make_shared<Float>(float(atof(yytext))); return T_FLOAT; }
[0-9]+         { yylval = make_shared<Int>(atoi(yytext)); return T_INT; }
"print"        { return T_PRINT; }
"allocate"     { return T_ALLOCATE; }
"func"         { return T_FUNC; }
"return"       { return T_RETURN; }
"draw"         { return T_DRAW; }
"use"          { return T_USE; }
"if"           { return T_IF; }
"while"        { return T_WHILE; }
"for"          { return T_FOR; }
"in"           { return T_IN; }
"and"          { return T_AND; }
"or"           { return T_OR; }
"true"         { yylval = make_shared<Bool>(true); return T_BOOL; }
"false"        { yylval = make_shared<Bool>(false); return T_BOOL; }
[a-zA-Z]+[a-zA-Z0-9_]* { yylval = make_shared<Ident>(strdup(yytext)); return T_IDENTIFIER; }
"("            { return T_OPEN_PAREN; }
")"            { return T_CLOSE_PAREN; }
"<-"           { return T_UPLOAD; }
"<"            { return T_LESS_THAN; }
">"            { return T_GREATER_THAN; }
">="           { return T_GEQUAL; }
"<="           { return T_LEQUAL; }
"=="           { return T_EQUAL; }
"["            { return T_OPEN_BRACKET; }
"]"            { return T_CLOSE_BRACKET; }
"="            { return T_EQUALS; }
"."            { return T_PERIOD; }
","            { return T_COMMA; }
"+"            { return T_PLUS; }
"+="           { return T_COMP_PLUS; }
"-"		       { return T_MINUS; }
"-="           { return T_COMP_MINUS; }
"*"            { return T_MULT; }
"*="           { return T_COMP_MULT; }
"!"            { return T_NOT; }
"/"            { return T_DIV; }
"/="           { return T_COMP_DIV; }
"|"			   { return T_PIPE; }
";"            { return T_SEMICOLON; }
"{"            { return T_OPEN_BRACE; }
"}"            { return T_CLOSE_BRACE; }
"%"            { return T_MOD; }
"%="           { return T_COMP_MOD; }
.			   { printf("Illegal token!\n"); }
%%

