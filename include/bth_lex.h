// MIT No Attribution
// 
// Copyright (c) 2025 bobthehuge
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to 
// deal in the Software without restriction, including without limitation the 
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or 
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
// DEALINGS IN THE SOFTWARE.

#ifndef BTH_LEX_H
#define BTH_LEX_H

#include <stdlib.h>

enum BTH_LEX_KIND
{
    INVALID,
    LK_END,
    LK_IDENT,
    LK_SYMBOL,
    LK_DELIMITED,
    BTH_LEX_KIND_COUNT
};

struct bth_lex_token
{
    size_t kind;
    size_t idx; // index if SYMBOL or DELIM else 0
    const char *name;
    const char *filename;
    size_t row;
    size_t col;
    // currently unused but can be used to compress repeating tokens
    size_t repeat;
    const char *begin;
    const char *end;
};

struct bth_lexer
{
    const char *buffer;
    const char *filename;
    size_t size;

    size_t cur;
    size_t col;
    size_t row;

    const char **symbols;
    size_t symbols_count;
    const char **delims;
    size_t delims_count;
    const char **skips;
    size_t skips_count;

    // void *usrdata;
};

#ifndef BTH_LEX_LINESEP
#  define BTH_LEX_LINESEP "\n"
#endif

#ifndef BTH_LEX_STRNCMP
#  define BTH_LEX_STRNCMP(s1, s2, n) (strncmp(s1, s2, (n)))
#endif

#ifndef BTH_LEX_STRLEN
#  include <string.h>
#  define BTH_LEX_STRLEN(s) (strlen(s))
#endif

#ifndef BTH_LEX_SKIP
#  define BTH_LEX_DEFAULT_SKIP
#  define BTH_LEX_SKIP(l) bth_lex_skip(l)
#endif

#ifndef BTH_LEX_GET_DELIM
#  ifndef BTH_LEX_ERRX
#    include <err.h>
#    define BTH_LEX_ERRX(code, fmt, ...) errx((code), fmt,__VA_ARGS__)
#  endif

#  define BTH_LEX_DEFAULT_GET_DELIM
#  define BTH_LEX_GET_DELIM(l, t) bth_lex_get_delim(l, t)
#endif

#ifndef BTH_LEX_GET_SYMBOL
#  define BTH_LEX_DEFAULT_GET_SYMBOL
#  define BTH_LEX_GET_SYMBOL(l, t) bth_lex_get_symbol(l, t)
#endif

#ifndef BTH_LEX_GET_IDENT
#  ifndef BTH_LEX_ISVALID
#    include <ctype.h>
#    define BTH_LEX_DEFAULT_ISVALID
#    define BTH_LEX_ISVALID(c) bth_lex_isvalid((c))
#  endif

#  define BTH_LEX_DEFAULT_GET_IDENT
#  define BTH_LEX_GET_IDENT(l, t) bth_lex_get_ident(l, t)
#endif

const char *bth_lex_kind2str(size_t id);
struct bth_lex_token bth_lex_get_token(struct bth_lexer *lex);

#endif

#ifdef BTH_LEX_IMPLEMENTATION

// TODO: investigate alternatives to this
const char *bth_lex_kind2str(size_t id)
{
    switch (id)
    {
    case INVALID: return "INVALID";
    case LK_END: return "END";
    case LK_IDENT: return "IDENT";
    case LK_SYMBOL: return "SYMBOL";
    case LK_DELIMITED: return "DELIMITED";
    default: return "UNKNOWN";
    }
}

#ifdef BTH_LEX_DEFAULT_SKIP
int bth_lex_find_skip(struct bth_lexer *lex, const char *s2, size_t *idx)
{
    for (size_t i = 0; i < lex->skips_count; i++)
    {
        const char *s1 = lex->skips[i];
        if (!BTH_LEX_STRNCMP(s1, s2, BTH_LEX_STRLEN(s1)))
        {
            if (idx)
                *idx = i;
            return 1;
        }
    }

    return 0;
}

void bth_lex_skip(struct bth_lexer *lex)
{
    const char *curptr = lex->buffer + lex->cur;

    while (lex->cur < lex->size)
    {
        size_t idx = 0;
        if (!bth_lex_find_skip(lex, curptr, &idx))
            return;

        size_t len = BTH_LEX_STRLEN(lex->skips[idx]);

        if (!BTH_LEX_STRNCMP(BTH_LEX_LINESEP, curptr,
            BTH_LEX_STRLEN(BTH_LEX_LINESEP)))
        {
            lex->row++;
            lex->col = 1;
        }
                
        curptr += len;
        lex->cur += len;
    }
}
#endif

#ifdef BTH_LEX_DEFAULT_GET_DELIM
int bth_lex_find_delim(struct bth_lexer *lex, const char *s2, size_t *idx)
{
    for (size_t i = 0; i < lex->delims_count; i++)
    {
        const char *s1 = lex->delims[i * 3 + 1];
        if (!BTH_LEX_STRNCMP(s1, s2, BTH_LEX_STRLEN(s1)))
        {
            *idx = i * 3;
            return 1;
        }
    }

    return 0;
}

int bth_lex_get_delim(struct bth_lexer *lex, struct bth_lex_token *t)
{
    const char *curptr = lex->buffer + lex->cur;
    size_t idx = 0;

    if (!bth_lex_find_delim(lex, curptr, &idx))
        return 0;
    
    const char **delim = lex->delims + idx;

    // size_t clen = 0;
    size_t clen = BTH_LEX_STRLEN(delim[1]);
    size_t lend = BTH_LEX_STRLEN(delim[2]);

    t->kind = LK_DELIMITED;
    t->idx = idx;
    t->begin = curptr;
    t->row = lex->row;
    t->col = lex->col;

    while (BTH_LEX_STRNCMP(delim[2], curptr + clen, lend))
    {
        if (lex->cur + clen >= lex->size - lend)
            // BTH_LEX_ERRX(1, "Unclosed delimiter %s at l:%zu c:%zu", 
            //         delim[0], lex->row, lex->col);
            return 0;

        if (!BTH_LEX_STRNCMP(BTH_LEX_LINESEP, (curptr + clen),
            BTH_LEX_STRLEN(BTH_LEX_LINESEP)))
        {
            lex->row++;
            lex->col = 1;
        }

        clen++;
    }

    t->name = delim[0];
    t->end = curptr + clen + lend;
    lex->col += clen + lend;
    lex->cur += clen + lend;

        if (!BTH_LEX_STRNCMP(BTH_LEX_LINESEP, (curptr + clen),
            BTH_LEX_STRLEN(BTH_LEX_LINESEP)))
        {
            lex->row++;
            lex->col = 1;
        }

    return 1;
}
#endif

#ifdef BTH_LEX_DEFAULT_GET_SYMBOL
int bth_lex_find_symbol(struct bth_lexer *lex, const char *s2, size_t *idx)
{
    for (size_t i = 0; i < lex->symbols_count; i++)
    {
        const char *s1 = lex->symbols[i * 2 + 1];
        if (!BTH_LEX_STRNCMP(s1, s2, BTH_LEX_STRLEN(s1)))
        {
            *idx = i;
            return 1;
        }
    }

    return 0;
}

int bth_lex_get_symbol(struct bth_lexer *lex, struct bth_lex_token *t)
{
    const char *curptr = lex->buffer + lex->cur;
    size_t idx = 0;

    if (!bth_lex_find_symbol(lex, curptr, &idx))
        return 0;
    
    const char **symbol = lex->symbols + idx * 2;
    size_t slen = BTH_LEX_STRLEN(symbol[1]);

    t->kind = LK_SYMBOL;
    t->idx = idx;
    t->name = symbol[0];
    t->row = lex->row;
    t->col = lex->col;
    t->begin = curptr;
    t->end = curptr + slen;

    lex->col += slen;

    if (!BTH_LEX_STRNCMP(BTH_LEX_LINESEP, symbol[1], slen))
    {
        lex->row++;
        lex->col = 1;
    }

    lex->cur += slen;
    
    return 1;
}
#endif


#ifdef BTH_LEX_DEFAULT_GET_IDENT

#ifdef BTH_LEX_DEFAULT_ISVALID
int bth_lex_isvalid(char c)
{
    return isalnum(c) || c == '_';
}
#endif

int bth_lex_get_ident(struct bth_lexer *lex, struct bth_lex_token *t)
{
    const char *curptr = lex->buffer + lex->cur;
    const char *lastptr = lex->buffer + lex->size;
    size_t off = 0;

    while (curptr + off < lastptr && BTH_LEX_ISVALID(*(curptr + off)))
        off++;

    if (off == 0)
        return 0;
    
    t->kind = LK_IDENT;
    t->idx = lex->symbols_count;
    t->name = "IDENT";
    t->row = lex->row;
    t->col = lex->col;
    t->begin = curptr;
    t->end = curptr + off;

    lex->cur += off;
    lex->col += off;

    return 1;
}
#endif

// is in bounds ?
// is delim ?
// is symbol ?
// is valid ident ?

struct bth_lex_token bth_lex_get_token(struct bth_lexer *lex)
{
    struct bth_lex_token tok = {
        .kind = INVALID,
        .row = lex->row,
        .col = lex->col,
        .begin = lex->buffer + lex->cur,
    };

    BTH_LEX_SKIP(lex);

    if (lex->cur >= lex->size)
    {
        tok.kind = LK_END;
        tok.idx = 0;
        tok.name = "END";
        tok.row = lex->row;
        tok.col = lex->col;
        tok.begin = lex->buffer + lex->size;
        tok.end = tok.begin;
        return tok;
    }

    if (BTH_LEX_GET_DELIM(lex, &tok))
        return tok;

    if (BTH_LEX_GET_SYMBOL(lex, &tok))
        return tok;

    if (BTH_LEX_GET_IDENT(lex, &tok))
        return tok;

    return tok;
}
#endif /* ! */
