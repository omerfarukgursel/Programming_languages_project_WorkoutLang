#include <stdio.h>
#include <stdlib.h>
#include "ast.h"

/* ── indentation helper ─────────────────────────────────────────────── */
static void indent(int depth) {
    for (int i = 0; i < depth; i++) printf("  ");
}

/* ── expression printer ─────────────────────────────────────────────── */
static void print_expr(Expr *e, int depth) {
    if (!e) return;
    indent(depth);
    switch (e->kind) {
        case EXPR_INT:      printf("IntLit(%d)\n",    e->ival);          break;
        case EXPR_FLOAT:    printf("FloatLit(%.6g)\n",e->fval);          break;
        case EXPR_DURATION: printf("DurationLit(%s)\n",e->sval);         break;
        case EXPR_WEIGHT:   printf("WeightLit(%s)\n", e->sval);          break;
        case EXPR_BOOL:     printf("BoolLit(%s)\n",   e->bval?"true":"false"); break;
        case EXPR_STRING:   printf("StringLit(\"%s\")\n", e->sval);      break;
        case EXPR_IDENT:    printf("Ident(%s)\n",     e->sval);          break;
        case EXPR_BINOP:
            printf("BinOp(%s)\n", e->binop.op);
            print_expr(e->binop.left,  depth + 1);
            print_expr(e->binop.right, depth + 1);
            break;
        case EXPR_UNOP:
            printf("UnOp(%s)\n", e->unop.op);
            print_expr(e->unop.operand, depth + 1);
            break;
        case EXPR_CALL:
            printf("Call(%s)\n", e->call.name);
            for (ExprList *a = e->call.args; a; a = a->next)
                print_expr(a->expr, depth + 1);
            break;
    }
}

/* ── statement printer ──────────────────────────────────────────────── */
static void print_stmts(StmtList *list, int depth);

static void print_stmt(Stmt *s, int depth) {
    if (!s) return;
    indent(depth);
    switch (s->kind) {
        case STMT_VAR_DECL:
            printf("VarDecl(%s %s)\n", s->var_decl.type_name, s->var_decl.name);
            print_expr(s->var_decl.init, depth + 1);
            break;
        case STMT_ASSIGN:
            printf("Assign(%s)\n", s->assign.name);
            print_expr(s->assign.value, depth + 1);
            break;
        case STMT_IF:
            printf("If\n");
            indent(depth + 1); printf("Condition:\n");
            print_expr(s->if_stmt.cond, depth + 2);
            indent(depth + 1); printf("Then:\n");
            print_stmts(s->if_stmt.then_body, depth + 2);
            if (s->if_stmt.else_body) {
                indent(depth + 1); printf("Else:\n");
                print_stmts(s->if_stmt.else_body, depth + 2);
            }
            break;
        case STMT_WHILE:
            printf("While\n");
            indent(depth + 1); printf("Condition:\n");
            print_expr(s->while_stmt.cond, depth + 2);
            indent(depth + 1); printf("Body:\n");
            print_stmts(s->while_stmt.body, depth + 2);
            break;
        case STMT_REPEAT:
            printf("Repeat(%d weeks)\n", s->repeat_stmt.count);
            print_stmts(s->repeat_stmt.body, depth + 1);
            break;
        case STMT_LOAD:
            printf("Load(%s)\n", s->load_stmt.name);
            break;
        case STMT_PRINT:
            printf("Print\n");
            print_expr(s->print_stmt.value, depth + 1);
            break;
        case STMT_RETURN:
            printf("Return\n");
            print_expr(s->return_stmt.value, depth + 1);
            break;
        case STMT_SET:
            printf("SetStmt(exercise=%s, sets=%d, reps=%d, rest=%s)\n",
                s->set_stmt.name, s->set_stmt.sets,
                s->set_stmt.reps, s->set_stmt.rest);
            break;
        case STMT_EXPR:
            printf("ExprStmt\n");
            print_expr(s->expr_stmt.expr, depth + 1);
            break;
    }
}

static void print_stmts(StmtList *list, int depth) {
    for (StmtList *sl = list; sl; sl = sl->next)
        print_stmt(sl->stmt, depth);
}

/* ── top-level printer ──────────────────────────────────────────────── */
void ast_print_program(Program *p) {
    printf("Program\n");
    for (Decl *d = p->decls; d; d = d->next) {
        switch (d->kind) {
            case DECL_WORKOUT:
                printf("  WorkoutDecl(%s)\n", d->name);
                for (DayDecl *day = d->workout.days; day; day = day->next) {
                    printf("    DayDecl(%s)\n", day->name);
                    print_stmts(day->sets, 3);
                }
                break;
            case DECL_ROUTINE:
                printf("  RoutineDecl(%s)\n", d->name);
                print_stmts(d->routine.body, 2);
                break;
            case DECL_FUNC:
                printf("  FuncDecl(%s) -> %s\n", d->name, d->func.ret_type);
                for (Param *pr = d->func.params; pr; pr = pr->next)
                    printf("    Param(%s: %s)\n", pr->name, pr->type_name);
                print_stmts(d->func.body, 2);
                break;
        }
    }
}

/* ── memory free ────────────────────────────────────────────────────── */
static void free_expr(Expr *e) {
    if (!e) return;
    if (e->kind == EXPR_BINOP) { free_expr(e->binop.left); free_expr(e->binop.right); }
    if (e->kind == EXPR_UNOP)  { free_expr(e->unop.operand); }
    if (e->kind == EXPR_CALL) {
        ExprList *a = e->call.args;
        while (a) { ExprList *nx = a->next; free_expr(a->expr); free(a); a = nx; }
    }
    free(e);
}

static void free_stmts(StmtList *list) {
    while (list) {
        StmtList *nx = list->next;
        Stmt *s = list->stmt;
        if (s) {
            switch (s->kind) {
                case STMT_VAR_DECL: free_expr(s->var_decl.init); break;
                case STMT_ASSIGN:   free_expr(s->assign.value);  break;
                case STMT_IF:
                    free_expr(s->if_stmt.cond);
                    free_stmts(s->if_stmt.then_body);
                    free_stmts(s->if_stmt.else_body);
                    break;
                case STMT_WHILE:
                    free_expr(s->while_stmt.cond);
                    free_stmts(s->while_stmt.body);
                    break;
                case STMT_REPEAT: free_stmts(s->repeat_stmt.body); break;
                case STMT_PRINT:  free_expr(s->print_stmt.value);  break;
                case STMT_RETURN: free_expr(s->return_stmt.value); break;
                case STMT_EXPR:   free_expr(s->expr_stmt.expr);    break;
                default: break;
            }
            free(s);
        }
        free(list);
        list = nx;
    }
}

void ast_free_program(Program *p) {
    Decl *d = p->decls;
    while (d) {
        Decl *nx = d->next;
        switch (d->kind) {
            case DECL_WORKOUT: {
                DayDecl *day = d->workout.days;
                while (day) { DayDecl *dn = day->next; free_stmts(day->sets); free(day); day = dn; }
                break;
            }
            case DECL_ROUTINE: free_stmts(d->routine.body); break;
            case DECL_FUNC: {
                Param *pr = d->func.params;
                while (pr) { Param *pn = pr->next; free(pr); pr = pn; }
                free_stmts(d->func.body);
                break;
            }
        }
        free(d);
        d = nx;
    }
}
