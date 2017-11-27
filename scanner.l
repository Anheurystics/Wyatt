%{
#include <cstdlib>
#include <string>
#include <memory>
#include "scanner.h"
#include "parser.hpp"
#include "location.hh"

using namespace std;

#define yyterminate() Parser::make_END(curr_location);
#define YY_USER_ACTION (*column) += yyleng;
#define curr_location location(0, *line, *column)
%}

%option noyywrap
%option c++
%option outfile="scanner.cpp"
%option yyclass="Scanner"
%option prefix="Prototype_"

%%
vert           { return Parser::make_VERTEX(curr_location); }
frag           { return Parser::make_FRAGMENT(curr_location); }
`(\\.|[^`])*` { std::string str = strdup(yytext); str = str.substr(1, str.length() - 2); return Parser::make_SHADER(make_shared<String>(str), curr_location); }
\"(\\.|[^\\"])*\" { std::string str = strdup(yytext); str = str.substr(1, str.length() - 2); return Parser::make_STRING(make_shared<String>(str), curr_location); }
[ \t]          ;
"//".*         ;
\n             { (*line)++; (*column) = 0; }
[0-9]+\.[0-9]+ { return Parser::make_FLOAT(make_shared<Float>(float(atof(yytext))), curr_location); }
[0-9]+         { return Parser::make_INT(make_shared<Int>(atoi(yytext)), curr_location); }
import         { return Parser::make_IMPORT(curr_location); }
print          { return Parser::make_PRINT(curr_location); }
allocate       { return Parser::make_ALLOCATE(curr_location); }
func           { return Parser::make_FUNC(curr_location); }
return         { return Parser::make_RETURN(curr_location); }
draw           { return Parser::make_DRAW(curr_location); }
use            { return Parser::make_USE(curr_location); }
if             { return Parser::make_IF(curr_location); }
while          { return Parser::make_WHILE(curr_location); }
for            { return Parser::make_FOR(curr_location); }
in             { return Parser::make_IN(curr_location); }
and            { return Parser::make_AND(curr_location); }
or             { return Parser::make_OR(curr_location); }
true           { return Parser::make_BOOL(make_shared<Bool>(true), curr_location); }
false          { return Parser::make_BOOL(make_shared<Bool>(false), curr_location); }
[a-zA-Z]+[a-zA-Z0-9_]* { return Parser::make_IDENTIFIER(make_shared<Ident>(strdup(yytext)), curr_location); }
"("            { return Parser::make_OPEN_PAREN(curr_location); }
")"            { return Parser::make_CLOSE_PAREN(curr_location); }
"<-"           { return Parser::make_UPLOAD(curr_location); }
"<"            { return Parser::make_LESS_THAN(curr_location); }
">"            { return Parser::make_GREATER_THAN(curr_location); }
">="           { return Parser::make_GEQUAL(curr_location); }
"<="           { return Parser::make_LEQUAL(curr_location); }
"=="           { return Parser::make_EQUAL(curr_location); }
"["            { return Parser::make_OPEN_BRACKET(curr_location); }
"]"            { return Parser::make_CLOSE_BRACKET(curr_location); }
"="            { return Parser::make_EQUALS(curr_location); }
"."            { return Parser::make_PERIOD(curr_location); }
","            { return Parser::make_COMMA(curr_location); }
"+"            { return Parser::make_PLUS(curr_location); }
"+="           { return Parser::make_COMP_PLUS(curr_location); }
"-"            { return Parser::make_MINUS(curr_location); }
"-="           { return Parser::make_COMP_MINUS(curr_location); }
"*"            { return Parser::make_MULT(curr_location); }
"*="           { return Parser::make_COMP_MULT(curr_location); }
"!"            { return Parser::make_NOT(curr_location); }
"/"            { return Parser::make_DIV(curr_location); }
"/="           { return Parser::make_COMP_DIV(curr_location); }
"|"            { return Parser::make_PIPE(curr_location); }
";"            { return Parser::make_SEMICOLON(curr_location); }
"{"            { return Parser::make_OPEN_BRACE(curr_location); }
"}"            { return Parser::make_CLOSE_BRACE(curr_location); }
"%"            { return Parser::make_MOD(curr_location); }
"%="           { return Parser::make_COMP_MOD(curr_location); }
<<EOF>>        { return yyterminate(); }
.              { printf("Illegal token!\n"); }
%%

