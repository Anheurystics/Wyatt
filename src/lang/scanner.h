#ifndef SCANNER_H
#define SCANNER_H

#if !defined(yyFlexLexerOnce)
#undef yyFlexLexer
#define yyFlexLexer Wyatt_FlexLexer
#include <FlexLexer.h>
#endif

#undef YY_DECL
#define YY_DECL Wyatt::Parser::symbol_type Wyatt::Scanner::get_next_token()

#include "parser.hpp"

namespace Wyatt
{

class Scanner : public yyFlexLexer
{
  public:
    Scanner(unsigned int *line, unsigned int *column) : line(line), column(column) {}
    virtual ~Scanner() {}
    virtual Parser::symbol_type get_next_token();
    unsigned int *line;
    unsigned int *column;
};
}

#endif
