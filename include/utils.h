#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>
#include "ast.h"
#include "token.h"
#include "types.h"

#define perr(fmt, ...) __perr(__func__, __LINE__, fmt,##__VA_ARGS__)
#define err_unexp_node(n) \
    perr( \
        "in %s:%zu:%zu:\n\tUnexpected node of kind '%zu'", \
        g_lexer.filename, \
        n->row, \
        n->col, \
        n->kind \
    )

// lmao
#define HERE() printf("here\n")
#define UNREACHABLE() perr("UNREACHABLE (wtf \?\?)")
#define TODO() perr("TODO: UNIMPLEMENTED")

char *m_strndup(const char *str, size_t n);
char *m_strdup(const char *str);
char *m_strapp(char *dst, const char *src);
char *m_strcat(const char *s1, const char *s2);
void *m_scalloc(size_t s);
char *stresc(const char *str);
char *file_basename(const char *path);
int is_valid_int(const char *s, long *res, int base);

char *type2str(Type *t);
Type *typeget(Node *n);
int typecmp(Type *t1, Type *t2);
uint64_t typehash(Type *ty);

void __perr(const char *fun, int l, const char *fmt, ...);

#endif
