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
%option yyclass="Scanner"
%option prefix="Wyatt_"

%x BLOCK_COMMENT

%%
"/*"           { BEGIN(BLOCK_COMMENT); }
<BLOCK_COMMENT>"*/" { BEGIN(INITIAL); }
<BLOCK_COMMENT>. {}
<BLOCK_COMMENT>\n { (*line)++; (*column) = 0; }
vert           { return Parser::make_VERTEX(curr_location); }
frag           { return Parser::make_FRAGMENT(curr_location); }
layout         { return Parser::make_LAYOUT(curr_location); }
`(\\.|[^`])*`  { 
                 std::string str = strdup(yytext);
                 str = str.substr(1, str.length() - 2);
                 return Parser::make_SHADER(make_shared<String>(str), curr_location);
               }
\"(\\.|[^\\"])*\" {
                 std::string str = strdup(yytext);
                 str = str.substr(1, str.length() - 2);
                 String::ptr s = make_shared<String>(str);
                 s->first_line = s->last_line = *line;
                 return Parser::make_STRING(s, curr_location);
               }
[ \t]          ;
"//".*         ;
\n             { (*line)++; (*column) = 0; }
[0-9]+\.[0-9]+ {
                 Float::ptr f = make_shared<Float>(float(atof(yytext)));
                 f->first_line = f->last_line = *line;
                 return Parser::make_FLOAT(f, curr_location);
               }
[0-9]+         {
                 Int::ptr i = make_shared<Int>(atoi(yytext));
                 i->first_line = i->last_line = *line;
                 return Parser::make_INT(i, curr_location);
               }
import         { return Parser::make_IMPORT(curr_location); }
print          { return Parser::make_PRINT(curr_location); }
func           { return Parser::make_FUNC(curr_location); }
main           { return Parser::make_MAIN(curr_location); }
return         { return Parser::make_RETURN(curr_location); }
draw           { return Parser::make_DRAW(curr_location); }
to             { return Parser::make_TO(curr_location); }
using          { return Parser::make_USING(curr_location); }
clear          { return Parser::make_CLEAR(curr_location); }
viewport       { return Parser::make_VIEWPORT(curr_location); }
if             { return Parser::make_IF(curr_location); }
else           { return Parser::make_ELSE(curr_location); }
while          { return Parser::make_WHILE(curr_location); }
for            { return Parser::make_FOR(curr_location); }
break          { return Parser::make_BREAK(curr_location); }
in             { return Parser::make_IN(curr_location); }
and            { return Parser::make_AND(curr_location); }
or             { return Parser::make_OR(curr_location); }
true           { return Parser::make_BOOL(make_shared<Bool>(true), curr_location); }
false          { return Parser::make_BOOL(make_shared<Bool>(false), curr_location); }
const          { return Parser::make_CONST(curr_location); }
[a-zA-Z]+[a-zA-Z0-9_]* {
                 Ident::ptr id = make_shared<Ident>(strdup(yytext));
                 id->first_line = id->last_line = *line;
                 return Parser::make_IDENTIFIER(id, curr_location);
               }
"("            { return Parser::make_OPEN_PAREN(curr_location); }
")"            { return Parser::make_CLOSE_PAREN(curr_location); }
"<-"           { return Parser::make_UPLOAD(curr_location); }
"<"            { return Parser::make_LTHAN(curr_location); }
">"            { return Parser::make_GTHAN(curr_location); }
">="           { return Parser::make_GEQUAL(curr_location); }
"<="           { return Parser::make_LEQUAL(curr_location); }
"=="           { return Parser::make_EQUAL(curr_location); }
"!="           { return Parser::make_NEQUAL(curr_location); }
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
"**"           { return Parser::make_EXP(curr_location); }
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

