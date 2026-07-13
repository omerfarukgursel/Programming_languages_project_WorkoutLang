#ifndef TOKEN_H
#define TOKEN_H

typedef enum {
    /* Literals */
    TOK_INT,        /* 42          */
    TOK_FLOAT,      /* 3.14        */
    TOK_DURATION,   /* 90s, 2min, 1hr */
    TOK_WEIGHT,     /* 80kg, 135lb */
    TOK_STRING,     /* "hello"     */
    TOK_BOOL,       /* true false  */
    TOK_IDENT,      /* myWorkout   */

    /* Keywords */
    TOK_WORKOUT,    /* workout     */
    TOK_ROUTINE,    /* routine     */
    TOK_DAY,        /* day         */
    TOK_SET,        /* set         */
    TOK_REPS,       /* reps        */
    TOK_REST,       /* rest        */
    TOK_REPEAT,     /* repeat      */
    TOK_WEEKS,      /* weeks       */
    TOK_LOAD,       /* load        */
    TOK_FUNC,       /* func        */
    TOK_IF,         /* if          */
    TOK_ELSE,       /* else        */
    TOK_WHILE,      /* while       */
    TOK_RETURN,     /* return      */
    TOK_PRINT,      /* print       */
    TOK_X,          /* x  (sets × reps separator) */

    /* Type keywords */
    TOK_TINT,       /* int         */
    TOK_TFLOAT,     /* float       */
    TOK_TBOOL,      /* bool        */
    TOK_TDURATION,  /* duration    */
    TOK_TWEIGHT,    /* weight      */

    /* Operators */
    TOK_PLUS,       /* +  */
    TOK_MINUS,      /* -  */
    TOK_STAR,       /* *  */
    TOK_SLASH,      /* /  */
    TOK_EQ,         /* == */
    TOK_NEQ,        /* != */
    TOK_LT,         /* <  */
    TOK_LE,         /* <= */
    TOK_GT,         /* >  */
    TOK_GE,         /* >= */
    TOK_AND,        /* && */
    TOK_OR,         /* || */
    TOK_NOT,        /* !  */
    TOK_ASSIGN,     /* =  */

    /* Separators */
    TOK_LBRACE,     /* {  */
    TOK_RBRACE,     /* }  */
    TOK_LPAREN,     /* (  */
    TOK_RPAREN,     /* )  */
    TOK_COMMA,      /* ,  */
    TOK_COLON,      /* :  */

    TOK_EOF,
    TOK_ERROR
} TokenType;

typedef struct {
    TokenType type;
    char      lexeme[256];  /* raw text of the token   */
    int       line;         /* 1-based source line      */
} Token;

const char *token_type_name(TokenType t);

#endif
