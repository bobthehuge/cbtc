#include <err.h>
#include <errno.h>
#include <string.h>
#include "../include/utils.h"

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
        errx(1, "'%s' is not a valid filename", path);

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

#include <stdio.h>
char *type2str(VType *bt)
{
    char *res = malloc(1);
    *res = 0;
redo:
    switch (*bt++)
    {
    case UNRESOLVED:
        {
            char *new = m_strcat("unresolved", res);
            free(res);
            res = new;
        }
        return res;
    case VT_INT:
        {
            char *new = m_strcat("int", res);
            free(res);
            res = new;
        }
        return res;
    case VT_PTR:
        res = m_strapp(res, "*");
        goto redo;
    default:
        errx(1, "broken type");
    }
}

// detecting UNRESOLVED always returns false
int typecmp(VType *t1, VType *t2)
{
    for (;;)
    {
        switch (*t1)
        {
        case VT_INT:
            return *t1 == *t2;
        case VT_PTR:
            if (*t1++ != *t2++)
                return 0;
            break;
        case UNRESOLVED:
            return 0;
        default:
            errx(1, "UNREACHABLE");
        }
    }
}
