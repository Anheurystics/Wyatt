/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

#ifndef YY_YY_PARSER_H_INCLUDED
# define YY_YY_PARSER_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif
#if YYDEBUG
extern int yydebug;
#endif
/* "%code requires" blocks.  */
#line 8 "parser.y" /* yacc.c:1909  */

#include <string>
#include <vector>
#include "nodes.h"

int yylex();
void yyerror(Stmts** init, Stmts** loop, const char *s);

#line 53 "parser.h" /* yacc.c:1909  */

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    INT = 258,
    FLOAT = 259,
    IDENTIFIER = 260,
    SHADER = 261,
    SEMICOLON = 262,
    OPEN_BRACE = 263,
    CLOSE_BRACE = 264,
    PIPE = 265,
    PLUS = 266,
    LEFT = 267,
    RIGHT = 268,
    OPEN_PAREN = 269,
    CLOSE_PAREN = 270,
    LESS_THAN = 271,
    GREATER_THAN = 272,
    OPEN_BRACKET = 273,
    CLOSE_BRACKET = 274,
    COMMA = 275,
    PERIOD = 276,
    EQUALS = 277,
    INIT = 278,
    LOOP = 279,
    ALLOCATE = 280,
    UPLOAD = 281,
    DRAW = 282,
    VERTEX = 283,
    FRAGMENT = 284,
    MINUS = 285,
    MULT = 286,
    DIV = 287,
    MOD = 288
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 25 "parser.y" /* yacc.c:1909  */

	Expr* eval;
	Int* ival;
	Float* fval;
	Ident* idval;
	Vector3* vval;
	Stmt* sval;
    Stmts* svval;
    ShaderSource* ssval;

	UploadList* lval;

#line 112 "parser.h" /* yacc.c:1909  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (Stmts** init, Stmts** loop);

#endif /* !YY_YY_PARSER_H_INCLUDED  */
