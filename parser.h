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
    SEMICOLON = 261,
    OPEN_BRACE = 262,
    CLOSE_BRACE = 263,
    PIPE = 264,
    PLUS = 265,
    LEFT = 266,
    RIGHT = 267,
    OPEN_PAREN = 268,
    CLOSE_PAREN = 269,
    LESS_THAN = 270,
    GREATER_THAN = 271,
    COMMA = 272,
    EQUALS = 273,
    INIT = 274,
    LOOP = 275,
    ALLOCATE = 276,
    UPLOAD = 277,
    MINUS = 278,
    MULT = 279,
    DIV = 280,
    MOD = 281
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

	UploadList* lval;

#line 104 "parser.h" /* yacc.c:1909  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (Stmts** init, Stmts** loop);

#endif /* !YY_YY_PARSER_H_INCLUDED  */
