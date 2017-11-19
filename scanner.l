%{
#include <cstdlib>
#include <string>
#include <memory>
#include "scanner.h"
#include "parser.hpp"
#include "location.hh"

using namespace std;

#define yyterminate() Parser::make_END(location());
%}

%option noyywrap
%option c++
%option outfile="scanner.cpp"
%option yyclass="Scanner"
%option prefix="Prototype_"

%%
"vert"         { return Parser::make_VERTEX(location()); }
"frag"         { return Parser::make_FRAGMENT(location()); }
"`"(\\.|[^`])*"`" { std::string str = strdup(yytext); str = str.substr(1, str.length() - 2); return Parser::make_SHADER(make_shared<String>(str), location()); }
"\""(\\.|[^\\""])*"\"" { std::string str = strdup(yytext); str = str.substr(1, str.length() - 2); return Parser::make_STRING(make_shared<String>(str), location()); }
"//".*         ;
[ \t\n]        ;
[0-9]+\.[0-9]+ { return Parser::make_FLOAT(make_shared<Float>(float(atof(yytext))), location()); }
[0-9]+         { return Parser::make_INT(make_shared<Int>(atoi(yytext)), location()); }
"print"        { return Parser::make_PRINT(location()); }
"allocate"     { return Parser::make_ALLOCATE(location()); }
"func"         { return Parser::make_FUNC(location()); }
"return"       { return Parser::make_RETURN(location()); }
"draw"         { return Parser::make_DRAW(location()); }
"use"          { return Parser::make_USE(location()); }
"if"           { return Parser::make_IF(location()); }
"while"        { return Parser::make_WHILE(location()); }
"for"          { return Parser::make_FOR(location()); }
"in"           { return Parser::make_IN(location()); }
"and"          { return Parser::make_AND(location()); }
"or"           { return Parser::make_OR(location()); }
"true"         { return Parser::make_BOOL(make_shared<Bool>(true), location()); }
"false"        { return Parser::make_BOOL(make_shared<Bool>(false), location()); }
[a-zA-Z]+[a-zA-Z0-9_]* { return Parser::make_IDENTIFIER(make_shared<Ident>(strdup(yytext)), location()); }
"("            { return Parser::make_OPEN_PAREN(location()); }
")"            { return Parser::make_CLOSE_PAREN(location()); }
"<-"           { return Parser::make_UPLOAD(location()); }
"<"            { return Parser::make_LESS_THAN(location()); }
">"            { return Parser::make_GREATER_THAN(location()); }
">="           { return Parser::make_GEQUAL(location()); }
"<="           { return Parser::make_LEQUAL(location()); }
"=="           { return Parser::make_EQUAL(location()); }
"["            { return Parser::make_OPEN_BRACKET(location()); }
"]"            { return Parser::make_CLOSE_BRACKET(location()); }
"="            { return Parser::make_EQUALS(location()); }
"."            { return Parser::make_PERIOD(location()); }
","            { return Parser::make_COMMA(location()); }
"+"            { return Parser::make_PLUS(location()); }
"+="           { return Parser::make_COMP_PLUS(location()); }
"-"		       { return Parser::make_MINUS(location()); }
"-="           { return Parser::make_COMP_MINUS(location()); }
"*"            { return Parser::make_MULT(location()); }
"*="           { return Parser::make_COMP_MULT(location()); }
"!"            { return Parser::make_NOT(location()); }
"/"            { return Parser::make_DIV(location()); }
"/="           { return Parser::make_COMP_DIV(location()); }
"|"			   { return Parser::make_PIPE(location()); }
";"            { return Parser::make_SEMICOLON(location()); }
"{"            { return Parser::make_OPEN_BRACE(location()); }
"}"            { return Parser::make_CLOSE_BRACE(location()); }
"%"            { return Parser::make_MOD(location()); }
"%="           { return Parser::make_COMP_MOD(location()); }
<<EOF>>        { return yyterminate(); }
.			   { printf("Illegal token!\n"); }
%%

