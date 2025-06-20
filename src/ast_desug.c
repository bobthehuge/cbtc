#include <stdarg.h>
#include <stdio.h>

#include "../include/context.h"

#define err_check(fmt, ...) \
    __err_check(__func__, __LINE__, fmt,##__VA_ARGS__)

void __err_check(const char *fun, int line, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "at %s:%u: ", fun, line);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(1);
}

// NOTE: will be useful when custom typing will be
// returns if a node can initialize a t2 type
int can_init(Node *n, VType *t2)
{
    switch (n->kind)
    {
    case NK_EXPR_ADD: case NK_EXPR_MUL:
        return can_init(n->as.binop->lhs, t2);
    case NK_EXPR_DEREF:
        return can_init(n->as.unop, t2 + 1);
    case NK_EXPR_LIT:
        return typecmp(n->as.lit->type, t2);
    case NK_EXPR_ASSIGN:
        switch(n->as.assign->dest->kind)
        {
        case NK_EXPR_DEREF:
            t2 += 1;
        // FALLTHROUGH
        default:
            return can_init(n->as.assign->value, t2);
        }
    case NK_EXPR_IDENT:
        {
            HashPair *v = get_symbol(n->as.ident->name);
            if (!v)
                errx(1, "unknown symbol '%s'", n->as.ident->name);

            return can_init(v->value, t2);
        }
    default:
        return 0;
    }
}

void ast_desug_module()

void ast_desug_node(Node *n)
{
    switch (n->kind)
    {
    case NK_MOD_DECL:
        break;
    default:
        err_check("Can't check node kind %u", n->kind);
    }
}
