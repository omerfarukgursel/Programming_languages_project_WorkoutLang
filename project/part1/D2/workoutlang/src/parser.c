#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

/* ── forward declarations ───────────────────────────────────────────── */
static Decl     *parse_top_decl(Parser *p);
static StmtList *parse_block(Parser *p);
static Stmt     *parse_stmt(Parser *p);
static Expr     *parse_expr(Parser *p);

/* ── error reporting ────────────────────────────────────────────────── */
static void error(Parser *p, const char *msg) {
    fprintf(stderr, "[line %d] Parse error at '%s': %s\n",
            p->current.line, p->current.lexeme, msg);
    p->had_error = 1;
}

/* ── token helpers ──────────────────────────────────────────────────── */
static void advance(Parser *p) {
    p->current = lexer_next(&p->lexer);
    /* skip and report any lexer errors */
    while (p->current.type == TOK_ERROR) {
        error(p, p->current.lexeme);
        p->current = lexer_next(&p->lexer);
    }
}

static int check(Parser *p, TokenType t) { return p->current.type == t; }

static int match(Parser *p, TokenType t) {
    if (check(p, t)) { advance(p); return 1; }
    return 0;
}

static Token expect(Parser *p, TokenType t, const char *ctx) {
    if (p->current.type != t) {
        char msg[256];
        snprintf(msg, sizeof(msg), "expected '%s' %s", token_type_name(t), ctx);
        error(p, msg);
    }
    Token tok = p->current;
    advance(p);
    return tok;
}

/* ── allocation helpers ─────────────────────────────────────────────── */
static Expr *new_expr(ExprKind k, int line) {
    Expr *e = calloc(1, sizeof(Expr));
    e->kind = k; e->line = line;
    return e;
}
static Stmt *new_stmt(StmtKind k, int line) {
    Stmt *s = calloc(1, sizeof(Stmt));
    s->kind = k; s->line = line;
    return s;
}
static StmtList *wrap_stmt(Stmt *s) {
    StmtList *sl = calloc(1, sizeof(StmtList));
    sl->stmt = s; sl->next = NULL;
    return sl;
}
static Decl *new_decl(DeclKind k, int line) {
    Decl *d = calloc(1, sizeof(Decl));
    d->kind = k; d->line = line; d->next = NULL;
    return d;
}

/* ── type keyword check ─────────────────────────────────────────────── */
static int is_type_kw(Parser *p) {
    TokenType t = p->current.type;
    return t == TOK_TINT || t == TOK_TFLOAT || t == TOK_TBOOL
        || t == TOK_TDURATION || t == TOK_TWEIGHT;
}

/* ══════════════════════════════════════════════════════════════════════
   EXPRESSIONS  (recursive descent matching the D1 EBNF grammar)
   ══════════════════════════════════════════════════════════════════════ */

/* <primary> */
static Expr *parse_primary(Parser *p) {
    int line = p->current.line;

    if (check(p, TOK_INT)) {
        Expr *e = new_expr(EXPR_INT, line);
        e->ival = atoi(p->current.lexeme);
        advance(p); return e;
    }
    if (check(p, TOK_FLOAT)) {
        Expr *e = new_expr(EXPR_FLOAT, line);
        e->fval = atof(p->current.lexeme);
        advance(p); return e;
    }
    if (check(p, TOK_DURATION)) {
        Expr *e = new_expr(EXPR_DURATION, line);
        strncpy(e->sval, p->current.lexeme, 255);
        advance(p); return e;
    }
    if (check(p, TOK_WEIGHT)) {
        Expr *e = new_expr(EXPR_WEIGHT, line);
        strncpy(e->sval, p->current.lexeme, 255);
        advance(p); return e;
    }
    if (check(p, TOK_BOOL)) {
        Expr *e = new_expr(EXPR_BOOL, line);
        e->bval = strcmp(p->current.lexeme, "true") == 0;
        advance(p); return e;
    }
    if (check(p, TOK_STRING)) {
        Expr *e = new_expr(EXPR_STRING, line);
        strncpy(e->sval, p->current.lexeme, 255);
        advance(p); return e;
    }
    /* identifier or function call */
    if (check(p, TOK_IDENT)) {
        char name[256];
        strncpy(name, p->current.lexeme, 255);
        advance(p);
        /* function call */
        if (check(p, TOK_LPAREN)) {
            advance(p);
            Expr *e = new_expr(EXPR_CALL, line);
            strncpy(e->call.name, name, 255);
            e->call.args = NULL;
            ExprList **tail = &e->call.args;
            if (!check(p, TOK_RPAREN)) {
                do {
                    ExprList *al = calloc(1, sizeof(ExprList));
                    al->expr = parse_expr(p);
                    al->next = NULL;
                    *tail = al; tail = &al->next;
                } while (match(p, TOK_COMMA));
            }
            expect(p, TOK_RPAREN, "after argument list");
            return e;
        }
        /* plain identifier */
        Expr *e = new_expr(EXPR_IDENT, line);
        strncpy(e->sval, name, 255);
        return e;
    }
    /* parenthesised expression */
    if (check(p, TOK_LPAREN)) {
        advance(p);
        Expr *e = parse_expr(p);
        expect(p, TOK_RPAREN, "after expression");
        return e;
    }

    error(p, "expected expression");
    /* return a dummy so parsing can continue */
    Expr *dummy = new_expr(EXPR_INT, line);
    dummy->ival = 0;
    advance(p);
    return dummy;
}

/* <unary_expr> */
static Expr *parse_unary(Parser *p) {
    int line = p->current.line;
    if (check(p, TOK_MINUS)) {
        advance(p);
        Expr *e = new_expr(EXPR_UNOP, line);
        strcpy(e->unop.op, "-");
        e->unop.operand = parse_primary(p);
        return e;
    }
    if (check(p, TOK_NOT)) {
        advance(p);
        Expr *e = new_expr(EXPR_UNOP, line);
        strcpy(e->unop.op, "!");
        e->unop.operand = parse_primary(p);
        return e;
    }
    return parse_primary(p);
}

/* generic left-associative binary level */
static Expr *parse_binary(Parser *p, Expr *(*sub)(Parser*),
                           TokenType ops[], const char *op_strs[], int n) {
    Expr *left = sub(p);
    for (;;) {
        int matched = -1;
        for (int i = 0; i < n; i++)
            if (check(p, ops[i])) { matched = i; break; }
        if (matched < 0) break;
        int line = p->current.line;
        advance(p);
        Expr *right = sub(p);
        Expr *e = new_expr(EXPR_BINOP, line);
        strncpy(e->binop.op, op_strs[matched], 3);
        e->binop.left  = left;
        e->binop.right = right;
        left = e;
    }
    return left;
}

static Expr *parse_mul(Parser *p) {
    TokenType ops[]      = {TOK_STAR, TOK_SLASH};
    const char *strs[]   = {"*", "/"};
    return parse_binary(p, parse_unary, ops, strs, 2);
}
static Expr *parse_add(Parser *p) {
    TokenType ops[]      = {TOK_PLUS, TOK_MINUS};
    const char *strs[]   = {"+", "-"};
    return parse_binary(p, parse_mul, ops, strs, 2);
}
static Expr *parse_rel(Parser *p) {
    TokenType ops[]      = {TOK_LT, TOK_LE, TOK_GT, TOK_GE};
    const char *strs[]   = {"<", "<=", ">", ">="};
    return parse_binary(p, parse_add, ops, strs, 4);
}
static Expr *parse_eq(Parser *p) {
    TokenType ops[]      = {TOK_EQ, TOK_NEQ};
    const char *strs[]   = {"==", "!="};
    return parse_binary(p, parse_rel, ops, strs, 2);
}
static Expr *parse_and(Parser *p) {
    TokenType ops[]      = {TOK_AND};
    const char *strs[]   = {"&&"};
    return parse_binary(p, parse_eq, ops, strs, 1);
}
static Expr *parse_or(Parser *p) {
    TokenType ops[]      = {TOK_OR};
    const char *strs[]   = {"||"};
    return parse_binary(p, parse_and, ops, strs, 1);
}

static Expr *parse_expr(Parser *p) { return parse_or(p); }

/* ══════════════════════════════════════════════════════════════════════
   STATEMENTS
   ══════════════════════════════════════════════════════════════════════ */

/* set benchpress reps 4 x 10 rest 90s */
static Stmt *parse_set_stmt(Parser *p) {
    int line = p->current.line;
    expect(p, TOK_SET, "at start of set statement");
    Token name = expect(p, TOK_IDENT, "exercise name");
    expect(p, TOK_REPS, "after exercise name");
    Token sets_tok = expect(p, TOK_INT, "set count");
    expect(p, TOK_X, "between sets and reps");
    Token reps_tok = expect(p, TOK_INT, "rep count");
    expect(p, TOK_REST, "after rep count");
    Token rest_tok = expect(p, TOK_DURATION, "rest duration");

    Stmt *s = new_stmt(STMT_SET, line);
    strncpy(s->set_stmt.name, name.lexeme, 255);
    s->set_stmt.sets = atoi(sets_tok.lexeme);
    s->set_stmt.reps = atoi(reps_tok.lexeme);
    strncpy(s->set_stmt.rest, rest_tok.lexeme, 63);
    return s;
}

/* if expr block [else block] */
static Stmt *parse_if_stmt(Parser *p) {
    int line = p->current.line;
    expect(p, TOK_IF, "");
    Stmt *s = new_stmt(STMT_IF, line);
    s->if_stmt.cond      = parse_expr(p);
    s->if_stmt.then_body = parse_block(p);
    s->if_stmt.else_body = NULL;
    /* dangling-else: greedily consume else */
    if (match(p, TOK_ELSE))
        s->if_stmt.else_body = parse_block(p);
    return s;
}

/* while expr block */
static Stmt *parse_while_stmt(Parser *p) {
    int line = p->current.line;
    expect(p, TOK_WHILE, "");
    Stmt *s = new_stmt(STMT_WHILE, line);
    s->while_stmt.cond = parse_expr(p);
    s->while_stmt.body = parse_block(p);
    return s;
}

/* repeat INT weeks block */
static Stmt *parse_repeat_stmt(Parser *p) {
    int line = p->current.line;
    expect(p, TOK_REPEAT, "");
    Token count = expect(p, TOK_INT, "week count");
    expect(p, TOK_WEEKS, "after count");
    Stmt *s = new_stmt(STMT_REPEAT, line);
    s->repeat_stmt.count = atoi(count.lexeme);
    s->repeat_stmt.body  = parse_block(p);
    return s;
}

/* <type> IDENT = expr  OR  IDENT = expr */
static Stmt *parse_assign_or_decl(Parser *p) {
    int line = p->current.line;
    if (is_type_kw(p)) {
        /* variable declaration */
        char type_name[32];
        strncpy(type_name, p->current.lexeme, 31);
        advance(p);
        Token name = expect(p, TOK_IDENT, "variable name");
        expect(p, TOK_ASSIGN, "after variable name");
        Expr *init = parse_expr(p);
        Stmt *s = new_stmt(STMT_VAR_DECL, line);
        strncpy(s->var_decl.type_name, type_name, 31);
        strncpy(s->var_decl.name, name.lexeme, 255);
        s->var_decl.init = init;
        return s;
    }
    /* must be IDENT */
    if (!check(p, TOK_IDENT)) {
        error(p, "expected statement");
        advance(p);
        return NULL;
    }
    char name[256];
    strncpy(name, p->current.lexeme, 255);
    advance(p);

    /* assignment: IDENT = expr */
    if (check(p, TOK_ASSIGN)) {
        advance(p);
        Stmt *s = new_stmt(STMT_ASSIGN, line);
        strncpy(s->assign.name, name, 255);
        s->assign.value = parse_expr(p);
        return s;
    }
    /* function call as statement: IDENT ( args ) */
    if (check(p, TOK_LPAREN)) {
        /* re-parse as expression starting from the ident */
        Expr *e = new_expr(EXPR_CALL, line);
        strncpy(e->call.name, name, 255);
        advance(p); /* consume ( */
        e->call.args = NULL;
        ExprList **tail = &e->call.args;
        if (!check(p, TOK_RPAREN)) {
            do {
                ExprList *al = calloc(1, sizeof(ExprList));
                al->expr = parse_expr(p); al->next = NULL;
                *tail = al; tail = &al->next;
            } while (match(p, TOK_COMMA));
        }
        expect(p, TOK_RPAREN, "after arguments");
        Stmt *s = new_stmt(STMT_EXPR, line);
        s->expr_stmt.expr = e;
        return s;
    }
    error(p, "expected '=' for assignment or '(' for function call");
    return NULL;
}

static Stmt *parse_stmt(Parser *p) {
    int line = p->current.line;

    if (check(p, TOK_SET))    return parse_set_stmt(p);
    if (check(p, TOK_IF))     return parse_if_stmt(p);
    if (check(p, TOK_WHILE))  return parse_while_stmt(p);
    if (check(p, TOK_REPEAT)) return parse_repeat_stmt(p);

    if (check(p, TOK_LOAD)) {
        advance(p);
        Token name = expect(p, TOK_IDENT, "workout name");
        Stmt *s = new_stmt(STMT_LOAD, line);
        strncpy(s->load_stmt.name, name.lexeme, 255);
        return s;
    }
    if (check(p, TOK_PRINT)) {
        advance(p);
        Stmt *s = new_stmt(STMT_PRINT, line);
        s->print_stmt.value = parse_expr(p);
        return s;
    }
    if (check(p, TOK_RETURN)) {
        advance(p);
        Stmt *s = new_stmt(STMT_RETURN, line);
        s->return_stmt.value = parse_expr(p);
        return s;
    }
    return parse_assign_or_decl(p);
}

/* { stmt* } */
static StmtList *parse_block(Parser *p) {
    expect(p, TOK_LBRACE, "at start of block");
    StmtList *head = NULL, **tail = &head;
    while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF)) {
        Stmt *s = parse_stmt(p);
        if (s) { *tail = wrap_stmt(s); tail = &(*tail)->next; }
    }
    expect(p, TOK_RBRACE, "at end of block");
    return head;
}

/* ══════════════════════════════════════════════════════════════════════
   TOP-LEVEL DECLARATIONS
   ══════════════════════════════════════════════════════════════════════ */

/* day IDENT { set_stmt* } */
static DayDecl *parse_day(Parser *p) {
    expect(p, TOK_DAY, "");
    Token name = expect(p, TOK_IDENT, "day name");
    expect(p, TOK_LBRACE, "after day name");

    DayDecl *day = calloc(1, sizeof(DayDecl));
    strncpy(day->name, name.lexeme, 255);
    day->sets = NULL;
    StmtList **tail = &day->sets;

    while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF)) {
        Stmt *s = parse_set_stmt(p);
        if (s) { *tail = wrap_stmt(s); tail = &(*tail)->next; }
    }
    expect(p, TOK_RBRACE, "at end of day block");
    return day;
}

/* workout IDENT { day* } */
static Decl *parse_workout(Parser *p) {
    int line = p->current.line;
    expect(p, TOK_WORKOUT, "");
    Token name = expect(p, TOK_IDENT, "workout name");
    expect(p, TOK_LBRACE, "after workout name");

    Decl *d = new_decl(DECL_WORKOUT, line);
    strncpy(d->name, name.lexeme, 255);
    d->workout.days = NULL;
    DayDecl **tail = &d->workout.days;

    while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF)) {
        DayDecl *day = parse_day(p);
        *tail = day; tail = &day->next;
    }
    expect(p, TOK_RBRACE, "at end of workout");
    return d;
}

/* routine IDENT { stmt* } */
static Decl *parse_routine(Parser *p) {
    int line = p->current.line;
    expect(p, TOK_ROUTINE, "");
    Token name = expect(p, TOK_IDENT, "routine name");
    Decl *d = new_decl(DECL_ROUTINE, line);
    strncpy(d->name, name.lexeme, 255);
    d->routine.body = parse_block(p);
    return d;
}

/* func IDENT ( params ) : type block */
static Decl *parse_func(Parser *p) {
    int line = p->current.line;
    expect(p, TOK_FUNC, "");
    Token name = expect(p, TOK_IDENT, "function name");
    expect(p, TOK_LPAREN, "after function name");

    Decl *d = new_decl(DECL_FUNC, line);
    strncpy(d->name, name.lexeme, 255);
    d->func.params = NULL;
    Param **ptail = &d->func.params;

    if (!check(p, TOK_RPAREN)) {
        do {
            Token pname = expect(p, TOK_IDENT, "parameter name");
            expect(p, TOK_COLON, "after parameter name");
            if (!is_type_kw(p)) error(p, "expected type keyword");
            Param *pr = calloc(1, sizeof(Param));
            strncpy(pr->name, pname.lexeme, 255);
            strncpy(pr->type_name, p->current.lexeme, 31);
            advance(p);
            pr->next = NULL;
            *ptail = pr; ptail = &pr->next;
        } while (match(p, TOK_COMMA));
    }
    expect(p, TOK_RPAREN, "after parameters");
    expect(p, TOK_COLON,  "before return type");
    if (!is_type_kw(p)) error(p, "expected return type");
    strncpy(d->func.ret_type, p->current.lexeme, 31);
    advance(p);
    d->func.body = parse_block(p);
    return d;
}

static Decl *parse_top_decl(Parser *p) {
    if (check(p, TOK_WORKOUT)) return parse_workout(p);
    if (check(p, TOK_ROUTINE)) return parse_routine(p);
    if (check(p, TOK_FUNC))    return parse_func(p);
    error(p, "expected 'workout', 'routine', or 'func'");
    advance(p);
    return NULL;
}

/* ══════════════════════════════════════════════════════════════════════
   PUBLIC API
   ══════════════════════════════════════════════════════════════════════ */

void parser_init(Parser *p, const char *source) {
    lexer_init(&p->lexer, source);
    p->had_error = 0;
    advance(p);  /* prime the first token */
}

Program parser_parse(Parser *p) {
    Program prog;
    prog.decls = NULL;
    Decl **tail = &prog.decls;

    while (!check(p, TOK_EOF)) {
        Decl *d = parse_top_decl(p);
        if (d) { *tail = d; tail = &d->next; }
    }
    return prog;
}
