#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"

typedef struct {
    Lexer   lexer;
    Token   current;   /* token being examined  */
    int     had_error;
} Parser;

void    parser_init(Parser *p, const char *source);
Program parser_parse(Parser *p);  /* returns parsed program; check p->had_error */

#endif
