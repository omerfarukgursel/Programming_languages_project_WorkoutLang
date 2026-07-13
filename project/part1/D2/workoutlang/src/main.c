#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "ast.h"

/* read entire file into a malloc'd string */
static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "Error: cannot open file '%s'\n", path); exit(1); }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    char *buf = malloc(size + 1);
    if (!buf) { fprintf(stderr, "Error: out of memory\n"); exit(1); }
    fread(buf, 1, size, f);
    buf[size] = '\0';
    fclose(f);
    return buf;
}

static void usage(const char *argv0) {
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  %s <source.wl>            -- parse and report result\n", argv0);
    fprintf(stderr, "  %s --dump-ast <source.wl>  -- parse and print AST\n", argv0);
    fprintf(stderr, "  %s --lex <source.wl>        -- print token stream\n", argv0);
}

int main(int argc, char **argv) {
    if (argc < 2) { usage(argv[0]); return 1; }

    int dump_ast = 0, dump_lex = 0;
    const char *filepath = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--dump-ast") == 0) dump_ast = 1;
        else if (strcmp(argv[i], "--lex") == 0) dump_lex = 1;
        else filepath = argv[i];
    }

    if (!filepath) { usage(argv[0]); return 1; }

    char *source = read_file(filepath);

    /* ── lex-only mode ── */
    if (dump_lex) {
        Lexer l;
        lexer_init(&l, source);
        Token tok;
        while ((tok = lexer_next(&l)).type != TOK_EOF) {
            printf("[line %2d] %-14s '%s'\n",
                   tok.line, token_type_name(tok.type), tok.lexeme);
            if (tok.type == TOK_ERROR) { free(source); return 1; }
        }
        printf("[line %2d] EOF\n", tok.line);
        free(source);
        return 0;
    }

    /* ── parse ── */
    Parser p;
    parser_init(&p, source);
    Program prog = parser_parse(&p);

    if (p.had_error) {
        fprintf(stderr, "Parsing failed — see errors above.\n");
        ast_free_program(&prog);
        free(source);
        return 1;
    }

    printf("Parsing successful.\n");

    if (dump_ast) {
        printf("\n");
        ast_print_program(&prog);
    }

    ast_free_program(&prog);
    free(source);
    return 0;
}
