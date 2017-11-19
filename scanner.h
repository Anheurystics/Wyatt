#ifndef SCANNER_H
#define SCANNER_H

#if ! defined(yyFlexLexerOnce)
#undef yyFlexLexer
#define yyFlexLexer Prototype_FlexLexer
#include <FlexLexer.h>
#endif

#undef YY_DECL
#define YY_DECL Prototype::Parser::symbol_type Prototype::Scanner::get_next_token()

#include "parser.hpp"

namespace Prototype {

class Scanner: public yyFlexLexer {
    public:
        Scanner() {}
        virtual ~Scanner() {}
        virtual Parser::symbol_type get_next_token();
};

}

#endif
