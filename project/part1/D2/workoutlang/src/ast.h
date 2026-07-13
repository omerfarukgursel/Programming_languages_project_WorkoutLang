#ifndef AST_H
#define AST_H

/* ── expression node types ──────────────────────────────────────────── */
typedef enum {
    EXPR_INT,       /* integer literal          */
    EXPR_FLOAT,     /* float literal            */
    EXPR_DURATION,  /* duration literal (90s)   */
    EXPR_WEIGHT,    /* weight literal (80kg)    */
    EXPR_BOOL,      /* bool literal             */
    EXPR_STRING,    /* string literal           */
    EXPR_IDENT,     /* variable reference       */
    EXPR_BINOP,     /* binary operation         */
    EXPR_UNOP,      /* unary operation          */
    EXPR_CALL       /* function call            */
} ExprKind;

typedef struct Expr Expr;
typedef struct ExprList ExprList;

struct ExprList {
    Expr     *expr;
    ExprList *next;
};

struct Expr {
    ExprKind kind;
    int      line;
    union {
        int    ival;
        double fval;
        char   sval[256];   /* string / ident / duration / weight */
        int    bval;        /* 0 = false, 1 = true                */
        struct { char op[4]; Expr *left; Expr *right; } binop;
        struct { char op[4]; Expr *operand;           } unop;
        struct { char name[256]; ExprList *args;      } call;
    };
};

/* ── statement node types ───────────────────────────────────────────── */
typedef enum {
    STMT_VAR_DECL,  /* int x = expr              */
    STMT_ASSIGN,    /* x = expr                  */
    STMT_IF,        /* if expr block [else block] */
    STMT_WHILE,     /* while expr block           */
    STMT_REPEAT,    /* repeat N weeks block       */
    STMT_LOAD,      /* load ident                 */
    STMT_PRINT,     /* print expr                 */
    STMT_RETURN,    /* return expr                */
    STMT_SET,       /* set name reps NxM rest Ds  */
    STMT_EXPR       /* bare expression statement  */
} StmtKind;

typedef struct Stmt Stmt;
typedef struct StmtList StmtList;

struct StmtList {
    Stmt     *stmt;
    StmtList *next;
};

struct Stmt {
    StmtKind kind;
    int      line;
    union {
        /* VAR_DECL */
        struct { char type_name[32]; char name[256]; Expr *init; } var_decl;
        /* ASSIGN */
        struct { char name[256]; Expr *value; } assign;
        /* IF */
        struct { Expr *cond; StmtList *then_body; StmtList *else_body; } if_stmt;
        /* WHILE */
        struct { Expr *cond; StmtList *body; } while_stmt;
        /* REPEAT */
        struct { int count; StmtList *body; } repeat_stmt;
        /* LOAD */
        struct { char name[256]; } load_stmt;
        /* PRINT */
        struct { Expr *value; } print_stmt;
        /* RETURN */
        struct { Expr *value; } return_stmt;
        /* SET  (domain-specific) */
        struct {
            char name[256]; /* exercise name    */
            int  sets;      /* left of x        */
            int  reps;      /* right of x       */
            char rest[64];  /* duration literal  */
        } set_stmt;
        /* EXPR */
        struct { Expr *expr; } expr_stmt;
    };
};

/* ── top-level declarations ─────────────────────────────────────────── */
typedef enum {
    DECL_WORKOUT,
    DECL_ROUTINE,
    DECL_FUNC
} DeclKind;

/* day block inside a workout */
typedef struct DayDecl DayDecl;
struct DayDecl {
    char      name[256];
    StmtList *sets;       /* list of STMT_SET nodes */
    DayDecl  *next;
};

/* parameter */
typedef struct Param Param;
struct Param {
    char  type_name[32];
    char  name[256];
    Param *next;
};

typedef struct Decl Decl;
struct Decl {
    DeclKind kind;
    int      line;
    char     name[256];
    union {
        struct { DayDecl  *days;   } workout;
        struct { StmtList *body;   } routine;
        struct { Param *params; char ret_type[32]; StmtList *body; } func;
    };
    Decl *next;
};

/* ── program root ───────────────────────────────────────────────────── */
typedef struct {
    Decl *decls;
} Program;

/* ── AST printer ────────────────────────────────────────────────────── */
void ast_print_program(Program *p);
void ast_free_program(Program *p);

#endif
