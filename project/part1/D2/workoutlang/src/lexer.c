#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"

/* ── helpers ─────────────────────────────────────────────────────────── */

static char cur(Lexer *l)  { return l->src[l->pos]; }
static char peek1(Lexer *l){ return l->src[l->pos + 1]; }
static void advance(Lexer *l){
    if (l->src[l->pos] == '\n') l->line++;
    l->pos++;
}

static void skip_whitespace_and_comments(Lexer *l) {
    for (;;) {
        /* whitespace */
        while (cur(l) && isspace((unsigned char)cur(l))) advance(l);
        /* // comment → skip to end of line */
        if (cur(l) == '/' && peek1(l) == '/') {
            while (cur(l) && cur(l) != '\n') advance(l);
            continue;
        }
        break;
    }
}

static Token make_tok(TokenType t, const char *lex, int line) {
    Token tok;
    tok.type = t;
    tok.line = line;
    strncpy(tok.lexeme, lex, 255);
    tok.lexeme[255] = '\0';
    return tok;
}

static Token error_tok(int line, const char *msg) {
    return make_tok(TOK_ERROR, msg, line);
}

/* ── keyword table ───────────────────────────────────────────────────── */

typedef struct { const char *word; TokenType type; } KW;

static KW keywords[] = {
    {"workout",  TOK_WORKOUT},
    {"routine",  TOK_ROUTINE},
    {"day",      TOK_DAY},
    {"set",      TOK_SET},
    {"reps",     TOK_REPS},
    {"rest",     TOK_REST},
    {"repeat",   TOK_REPEAT},
    {"weeks",    TOK_WEEKS},
    {"load",     TOK_LOAD},
    {"func",     TOK_FUNC},
    {"if",       TOK_IF},
    {"else",     TOK_ELSE},
    {"while",    TOK_WHILE},
    {"return",   TOK_RETURN},
    {"print",    TOK_PRINT},
    {"true",     TOK_BOOL},
    {"false",    TOK_BOOL},
    {"x",        TOK_X},
    {"int",      TOK_TINT},
    {"float",    TOK_TFLOAT},
    {"bool",     TOK_TBOOL},
    {"duration", TOK_TDURATION},
    {"weight",   TOK_TWEIGHT},
    {NULL,       TOK_IDENT}
};

static TokenType lookup_keyword(const char *word) {
    for (int i = 0; keywords[i].word; i++)
        if (strcmp(keywords[i].word, word) == 0)
            return keywords[i].type;
    return TOK_IDENT;
}

/* ── token_type_name ─────────────────────────────────────────────────── */

const char *token_type_name(TokenType t) {
    switch (t) {
        case TOK_INT:      return "INT_LIT";
        case TOK_FLOAT:    return "FLOAT_LIT";
        case TOK_DURATION: return "DURATION_LIT";
        case TOK_WEIGHT:   return "WEIGHT_LIT";
        case TOK_STRING:   return "STRING_LIT";
        case TOK_BOOL:     return "BOOL_LIT";
        case TOK_IDENT:    return "IDENT";
        case TOK_WORKOUT:  return "workout";
        case TOK_ROUTINE:  return "routine";
        case TOK_DAY:      return "day";
        case TOK_SET:      return "set";
        case TOK_REPS:     return "reps";
        case TOK_REST:     return "rest";
        case TOK_REPEAT:   return "repeat";
        case TOK_WEEKS:    return "weeks";
        case TOK_LOAD:     return "load";
        case TOK_FUNC:     return "func";
        case TOK_IF:       return "if";
        case TOK_ELSE:     return "else";
        case TOK_WHILE:    return "while";
        case TOK_RETURN:   return "return";
        case TOK_PRINT:    return "print";
        case TOK_X:        return "x";
        case TOK_TINT:     return "int";
        case TOK_TFLOAT:   return "float";
        case TOK_TBOOL:    return "bool";
        case TOK_TDURATION:return "duration";
        case TOK_TWEIGHT:  return "weight";
        case TOK_PLUS:     return "+";
        case TOK_MINUS:    return "-";
        case TOK_STAR:     return "*";
        case TOK_SLASH:    return "/";
        case TOK_EQ:       return "==";
        case TOK_NEQ:      return "!=";
        case TOK_LT:       return "<";
        case TOK_LE:       return "<=";
        case TOK_GT:       return ">";
        case TOK_GE:       return ">=";
        case TOK_AND:      return "&&";
        case TOK_OR:       return "||";
        case TOK_NOT:      return "!";
        case TOK_ASSIGN:   return "=";
        case TOK_LBRACE:   return "{";
        case TOK_RBRACE:   return "}";
        case TOK_LPAREN:   return "(";
        case TOK_RPAREN:   return ")";
        case TOK_COMMA:    return ",";
        case TOK_COLON:    return ":";
        case TOK_EOF:      return "EOF";
        case TOK_ERROR:    return "ERROR";
        default:           return "UNKNOWN";
    }
}

/* ── lexer_init ──────────────────────────────────────────────────────── */

void lexer_init(Lexer *l, const char *source) {
    l->src  = source;
    l->pos  = 0;
    l->line = 1;
}

/* ── lexer_next ──────────────────────────────────────────────────────── */

Token lexer_next(Lexer *l) {
    skip_whitespace_and_comments(l);
    int line = l->line;

    if (!cur(l)) return make_tok(TOK_EOF, "EOF", line);

    /* ── string literal ──────────────────────────────── */
    if (cur(l) == '"') {
        advance(l);
        char buf[256]; int i = 0;
        while (cur(l) && cur(l) != '"' && cur(l) != '\n') {
            if (i < 254) buf[i++] = cur(l);
            advance(l);
        }
        if (cur(l) != '"')
            return error_tok(line, "Unterminated string literal");
        advance(l); /* closing " */
        buf[i] = '\0';
        return make_tok(TOK_STRING, buf, line);
    }

    /* ── number: int, float, duration, weight ────────── */
    if (isdigit((unsigned char)cur(l))) {
        char buf[64]; int i = 0;
        while (isdigit((unsigned char)cur(l))) {
            if (i < 62) buf[i++] = cur(l);
            advance(l);
        }
        /* float */
        if (cur(l) == '.' && isdigit((unsigned char)peek1(l))) {
            if (i < 62) { buf[i++] = cur(l); } advance(l);
            while (isdigit((unsigned char)cur(l))) {
                if (i < 62) buf[i++] = cur(l);
                advance(l);
            }
            buf[i] = '\0';
            return make_tok(TOK_FLOAT, buf, line);
        }
        /* duration suffix: s, min, hr */
        if (cur(l) == 's' && !isalnum((unsigned char)peek1(l)) && peek1(l) != '_') {
            buf[i++] = 's'; buf[i] = '\0'; advance(l);
            return make_tok(TOK_DURATION, buf, line);
        }
        if (cur(l) == 'm' && peek1(l) == 'i' && l->src[l->pos+2] == 'n') {
            buf[i++]='m'; buf[i++]='i'; buf[i++]='n'; buf[i]='\0';
            advance(l); advance(l); advance(l);
            return make_tok(TOK_DURATION, buf, line);
        }
        if (cur(l) == 'h' && peek1(l) == 'r') {
            buf[i++]='h'; buf[i++]='r'; buf[i]='\0';
            advance(l); advance(l);
            return make_tok(TOK_DURATION, buf, line);
        }
        /* weight suffix: kg, lb */
        if (cur(l) == 'k' && peek1(l) == 'g') {
            buf[i++]='k'; buf[i++]='g'; buf[i]='\0';
            advance(l); advance(l);
            return make_tok(TOK_WEIGHT, buf, line);
        }
        if (cur(l) == 'l' && peek1(l) == 'b') {
            buf[i++]='l'; buf[i++]='b'; buf[i]='\0';
            advance(l); advance(l);
            return make_tok(TOK_WEIGHT, buf, line);
        }
        buf[i] = '\0';
        return make_tok(TOK_INT, buf, line);
    }

    /* ── identifier or keyword ───────────────────────── */
    if (isalpha((unsigned char)cur(l)) || cur(l) == '_') {
        char buf[256]; int i = 0;
        while (isalnum((unsigned char)cur(l)) || cur(l) == '_') {
            if (i < 254) buf[i++] = cur(l);
            advance(l);
        }
        buf[i] = '\0';
        TokenType kw = lookup_keyword(buf);
        return make_tok(kw, buf, line);
    }

    /* ── two-character operators ─────────────────────── */
    char c = cur(l);
    if (c == '=' && peek1(l) == '=') { advance(l); advance(l); return make_tok(TOK_EQ,  "==", line); }
    if (c == '!' && peek1(l) == '=') { advance(l); advance(l); return make_tok(TOK_NEQ, "!=", line); }
    if (c == '<' && peek1(l) == '=') { advance(l); advance(l); return make_tok(TOK_LE,  "<=", line); }
    if (c == '>' && peek1(l) == '=') { advance(l); advance(l); return make_tok(TOK_GE,  ">=", line); }
    if (c == '&' && peek1(l) == '&') { advance(l); advance(l); return make_tok(TOK_AND, "&&", line); }
    if (c == '|' && peek1(l) == '|') { advance(l); advance(l); return make_tok(TOK_OR,  "||", line); }

    /* ── single-character tokens ─────────────────────── */
    advance(l);
    switch (c) {
        case '+': return make_tok(TOK_PLUS,   "+", line);
        case '-': return make_tok(TOK_MINUS,  "-", line);
        case '*': return make_tok(TOK_STAR,   "*", line);
        case '/': return make_tok(TOK_SLASH,  "/", line);
        case '<': return make_tok(TOK_LT,     "<", line);
        case '>': return make_tok(TOK_GT,     ">", line);
        case '!': return make_tok(TOK_NOT,    "!", line);
        case '=': return make_tok(TOK_ASSIGN, "=", line);
        case '{': return make_tok(TOK_LBRACE, "{", line);
        case '}': return make_tok(TOK_RBRACE, "}", line);
        case '(': return make_tok(TOK_LPAREN, "(", line);
        case ')': return make_tok(TOK_RPAREN, ")", line);
        case ',': return make_tok(TOK_COMMA,  ",", line);
        case ':': return make_tok(TOK_COLON,  ":", line);
        default: {
            char msg[32];
            snprintf(msg, sizeof(msg), "Unknown char '%c'", c);
            return error_tok(line, msg);
        }
    }
}

/* ── lexer_peek ──────────────────────────────────────────────────────── */

Token lexer_peek(Lexer *l) {
    int saved_pos  = l->pos;
    int saved_line = l->line;
    Token t = lexer_next(l);
    l->pos  = saved_pos;
    l->line = saved_line;
    return t;
}
