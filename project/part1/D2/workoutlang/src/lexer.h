#ifndef LEXER_H
#define LEXER_H

#include "token.h"

typedef struct {
    const char *src;   /* full source text          */
    int         pos;   /* current character index   */
    int         line;  /* current line number       */
} Lexer;

void  lexer_init(Lexer *l, const char *source);
Token lexer_next(Lexer *l);          /* consume and return next token */
Token lexer_peek(Lexer *l);          /* look ahead without consuming  */

#endif
