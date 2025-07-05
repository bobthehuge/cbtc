#include <err.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "../include/bth_alloc.h"
#include "../include/utils.h"

void *m_scalloc(size_t s)
{
    return scalloc(1, s);
}

char *m_strndup(const char *str, size_t n)
{
    size_t len = strlen(str);
    len = len > n ? n : len;

    char *res = malloc(len + 1);
    memcpy(res, str, len);

    res[len] = 0;

    return res;
}

char *m_strdup(const char *str)
{
    size_t len = strlen(str);
    char *res = malloc(len + 1);

    memcpy(res, str, len);
    res[len] = 0;

    return res;
}

char *m_strapp(char *dst, const char *src)
{
    size_t dlen = strlen(dst);
    size_t slen = strlen(src);

    char *res = realloc(dst, dlen + slen + 1);
    memcpy(res + dlen, src, slen);
    res[dlen + slen] = 0;

    return res;
}

char *m_strcat(const char *s1, const char *s2)
{
    size_t len1 = strlen(s1);
    size_t len2 = strlen(s2);

    char *res = malloc(len1 + len2 + 1);
    memcpy(res, s1, len1);
    memcpy(res + len1, s2, len2);
    res[len1 + len2] = 0;

    return res;
}

char *stresc(const char *str)
{
    const char table[7] = {
        ['\a'%7] = 'a',
        ['\b'%7] = 'b',
        ['\t'%7] = 't',
        ['\n'%7] = 'n',
        ['\v'%7] = 'v',
        ['\f'%7] = 'f',
        ['\r'%7] = 'r',
    };

    const char *iter;
    size_t flen = 0;

    for (iter = str; *iter; iter++)
    {
        if (*iter >= '\a' && *iter <= '\r')
            flen++;
        flen++;
    }

    char *res = malloc(flen + 1);
    char *ptr = res;
    
    for (iter = str; *iter; iter++)
    {
        if (*iter >= '\a' && *iter <= '\r')
        {
            *ptr++ = '\\';
            *ptr++ = table[(*iter) % 7];
            continue;
        }

        *ptr++ = *iter;
    }

    *ptr = 0;

    return res;
}

char *file_basename(const char *path)
{
    char *ext = strrchr(path, '.');

    if (!ext)
        perr("'%s' is not a valid filename", path);

    char *org = ext;
    while (org > path && *org != '/')
        org--;

    if (org > path)
        org++;

    return m_strndup(org, ext - org);
}

int is_valid_int(const char *s, long *res, int base)
{
    char *end = NULL;

    errno = 0;
    *res = strtol(s, &end, base);

    if (errno == EINVAL || (end && *end))
        return 0;

    if (errno == ERANGE)
        return -1;

    return 1;
}

char *type2str(Type *t)
{
    char *res = malloc(t->refc + 1);
    memset(res, '&', t->refc);
    res[t->refc] = 0;
    
    switch (t->base)
    {
    case UNRESOLVED:
        res = m_strapp(res, "unresolved");
        break;
    case VT_INT:
        res = m_strapp(res, "int");
        break;
    case VT_CHAR:
        res = m_strapp(res, "char");
        break;
    default:
        UNREACHABLE();
    }

    return res;
}

Type *typeget(Node *_n)
{
    Node *n = _n;

redo:
    switch (n->kind)
    {
    case NK_EXPR_ADD: case NK_EXPR_MUL:
        return &n->as.binop->type;
    case NK_EXPR_DEREF: case NK_EXPR_REF:
        return &n->as.unop->type;
    case NK_EXPR_IDENT:
        return &n->as.ident->type;
    case NK_EXPR_LIT:
        return &n->as.ident->type;
    case NK_EXPR_FUNCALL:
        return &n->as.fcall->type;
    case NK_VAR_DECL:
        return &n->as.vdecl->type;
    case NK_FUN_DECL:
        return &n->as.fdecl->ret;
    case NK_EXPR_ASSIGN:
        n = n->as.assign->dest;
        goto redo;
    default:
        return NULL;
    }
}

// returns:
// 0 if t1 == t2
// 1 else
// 
int typecmp(Type *t1, Type *t2)
{
    if (!t1 || !t2)
        perr("NULL type caught");

    return t1->refc != t2->refc || t1->base != t2->base;
}

// char *typehkey(Type *ty)
// {
//     char *sh = malloc(sizeof(Type) + 1);

//     char *src = (char *)ty;

//     for (uint i = 0; i < sizeof(Type); i++)
//         sh[i] = src[i] + 1;

//     sh[sizeof(Type)] = 0;

//     return sh;
// }

// get a valid cstr representation from delimited raw bytes
char *gethashkey(const char *raw, size_t len)
{
    char *hk = malloc(len + 1);

    for (size_t i = 0; i < len; i++)
        hk[i] = raw[i] % 255 + 1;

    hk[len] = 0;
    return hk;
}

void __perr(const char *fun, int l, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    fprintf(stderr, "at %s:%d: ", fun, l);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");

    va_end(ap);

    exit(1);
}
