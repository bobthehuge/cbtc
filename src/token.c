#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#include "../include/token.h"
#include "../include/utils.h"

#include "../include/bth_io.h"
#include "../include/bth_lex.h"
#include "../include/bth_types.h"

extern bool g_print_token;
static Lexer l_lexer;

const char *KEYWORD_TABLE[] = {
    "TK_INT",          "int",
    "TK_END",          "end",
    "TK_RETURN",       "return",

    "TK_EQUAL",        "=",
    "TK_SEMICOLON",    ";",
    "TK_OPAREN",       "(",
    "TK_CPAREN",       ")",
    "TK_PLUS",         "+",
    "TK_STAR",         "*",
    "TK_UPPERSAND",    "&",
};

const size_t KEYWORD_COUNT = sizeof(KEYWORD_TABLE) / (sizeof(char *) * 2);

const char *DELIM_TABLE[] = {
    "TK_STRING_LITERAL", "\"", "\"",
    "TK_CHAR_LITERAL",   "\'", "\'",
    "TK_COMMENT",        "//", "\n",
    "TK_CPP_COMMENT",    "/*", "*/",
};

const size_t DELIM_COUNT = sizeof(DELIM_TABLE) / (sizeof(char *) * 3);

const char *SKIP_TABLE[] = {
    "\n",
    "\t",
    " ",
};

const size_t SKIP_COUNT = sizeof(SKIP_TABLE) / sizeof(char *);

// checks if a substring of a symbol is placed before it in the table
int check_prefix_collisions(size_t *h, size_t *s)
{
    bool mark[KEYWORD_COUNT];
    memset(mark, false, KEYWORD_COUNT * sizeof(bool));
    
    for (size_t i = 0; i < KEYWORD_COUNT; i++)
    {
        if (mark[i])
            continue;

        for (size_t j = 0; j < KEYWORD_COUNT; j++)
        {
            if (mark[j] || i == j)
                continue;

            size_t hidx = i;
            size_t sidx = j;

            if (j > i)
            {
                hidx ^= sidx;
                sidx ^= hidx;
                hidx ^= sidx;
            }
            
            const char *hay = KEYWORD_TABLE[hidx*2+1];
            const char *sub = KEYWORD_TABLE[sidx*2+1];

            if (!strncmp(hay, sub, strlen(sub)))
            {
                *h = hidx;
                *s = sidx;
                return 1;
            }
        }

        mark[i] = true;
    }
    return 0;
}

void set_lexer(const char *path)
{
    str buf;
    OPTION(size_t) buflen = readfn(&buf, 0, path);
    CHECK(buflen);
    
    l_lexer = (Lexer){
        .filename = path,
        .buffer = buf, .size = buflen.some,
        .col = 1, .row = 1,
        .cur = 0,
        .symbols = KEYWORD_TABLE,
        .symbols_count = KEYWORD_COUNT,
        .delims = DELIM_TABLE,
        .delims_count = DELIM_COUNT,
        .skips = SKIP_TABLE,
        .skips_count = SKIP_COUNT,
    };
}

void identify_token(Token *tok)
{
    if (*tok->begin >= '0' && *tok->begin <= '9')
        tok->idx = TK_INT_CST;
}

Token next_token(void)
{
    for (;;)
    {
        Token tok = bth_lex_get_token(&l_lexer);

        if (tok.kind == INVALID)
            errx(1, "at %zu:%zu: INVALID", tok.col, tok.row);

        // skips comment
        if (tok.kind == LK_DELIMITED)
            continue;

        identify_token(&tok);

        if (g_print_token)
        {
            char *tmp = get_token_content(&tok);
            printf("token of '%s' (idx: %zu)\n", tmp, tok.idx);
            free(tmp);
        }

        return tok;
    }
}

Token peak_token(void)
{
    size_t cur = l_lexer.cur;
    size_t row = l_lexer.row;
    size_t col = l_lexer.col;

    Token tok = next_token();

    l_lexer.cur = cur;
    l_lexer.row = row;
    l_lexer.col = col;

    return tok;
}

Token *collect_tokens(void)
{
    Token *toks = malloc(0);
    size_t c = 0;
    Token tok;

    do
    {
        tok = next_token();

        switch (tok.kind)
        {
        case LK_END:
        // FALLTHROUGH
        case LK_IDENT:
        // FALLTHROUGH
        case LK_SYMBOL:
        // FALLTHROUGH
        case LK_DELIMITED:
            if (c > 0 && tok.kind == toks[c - 1].kind
                && !strncmp(tok.begin, toks[c - 1].begin, tok.end - tok.begin))
            {
                toks[c - 1].repeat++;
            }
            else
            {
                toks = realloc(toks, (c + 1) * sizeof(Token));
                toks[c] = tok;
                c++;
            }
            break;
        default:
            errx(1, "UNREACHABLE");
        }
    } while (tok.kind != LK_END);

    return toks;
}

char *get_token_content(Token *t)
{
    return m_strndup(t->begin, t->end - t->begin);
}

void __err_tok_unexp(Token *t, const char *fun, int line)
{
    if (!t)
        errx(1, "Unexpected NULL token");

    char *d = get_token_content(t);
    errx(1, "%s:%d: Unexpected token '%s' at %zu:%zu", fun, line,
         d, t->row, t->col);
}

void __tok_expect(Token *t, size_t v, const char *fun, int line)
{
    if (t->idx != v)
        __err_tok_unexp(t, fun, line);
}
