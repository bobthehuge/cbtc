#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdbool.h>
#include "ast.h"
// #include "token.h"

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

#define STRLIST(...) ((const char *[]){__VA_ARGS__})

#define m_strapp_n(dst, ...) \
    __call_m_strapp_n(dst, ((const char *[]){__VA_ARGS__}))

#define m_strchg(dst, tok, val) __m_strchg(dst, tok, val, false);
#define m_strchg_all(dst, tok, val) __m_strchg(dst, tok, val, true);

#define m_strrep(dst, tok, val) __m_strrep(dst, tok, val, false);
#define m_strrep_all(dst, tok, val) __m_strrep(dst, tok, val, true);
    
#define __call_m_strapp_n(dst, strs) \
    __m_strapp_n(dst, sizeof(strs)/sizeof(char *), strs)

typedef struct
{
    size_t len;
    void *data;
} varray;

void *m_scalloc(size_t s);
void *m_memdup(const void *src, size_t len);
char *m_strndup(const char *str, size_t n);
char *m_strdup(const char *str);
char *m_strapp(char *dst, const char *src);
char *__m_strapp_n(char *dst, size_t len, const char *strs[len]);
char *m_strcat(const char *s1, const char *s2);
char *m_strpre(char *dst, const char *src);

// copies src and replace `tok` by `val`. Set `all` to false to do it once
char *__m_strchg(const char *src, const char *tok, const char *val, bool all);
// calls __str_chg then replace `dst` by the result
void __m_strrep(char **dst, const char *tok, const char *val, bool all);

char *stresc(const char *str);
char *file_basename(const char *path);
int is_valid_int(const char *s, long *res, int base);

char *varsig(struct VarDeclNode *vnode);
char *funcsig(struct FunDeclNode *fnode);

// get a valid cstr representation from delimited raw bytes
char *gethashkey(const char *raw, size_t len);

void __perr(const char *fun, int l, const char *fmt, ...);

#endif
