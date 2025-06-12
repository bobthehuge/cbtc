#ifndef TOKEN_H
#define TOKEN_H

#include <stddef.h>

#include "bth_lex.h"
#include "bth_option.h"
OPTION_TYPEDEF(size_t);

#define err_tok_unexp(t) __err_tok_unexp(t, __func__, __LINE__)
#define tok_expect(t, v) __tok_expect(t, v, __func__, __LINE__)

typedef struct bth_lexer Lexer;
typedef struct bth_lex_token Token;

typedef enum {
    // Keywords
    TK_INT,
    TK_END,
    TK_RETURN,

    // punctuators
    TK_EQUAL,
    TK_SEMICOLON,
    TK_OPAREN,
    TK_CPAREN,
    TK_PLUS,
    TK_STAR,
    TK_UPPERSAND,

    // other
    TK_IDENTIFIER,
    TK_INT_CST,
    TK_UNKNOWN
} TokenKind;

extern const char *KEYWORD_TABLE[];
extern const size_t KEYWORD_COUNT;
extern const char *DELIM_TABLE[];
extern const size_t DELIM_COUNT;
extern const char *SKIP_TABLE[];
extern const size_t SKIP_COUNT;

int check_prefix_collisions(size_t *h, size_t *s);
void set_lexer(const char *path);
Token next_token(void);
Token peak_token(void);
Token *collect_tokens(void);
char *get_token_content(Token *tok);
void __err_tok_unexp(Token *tok, const char *fun, int line);
void __tok_expect(Token *tok, size_t value, const char *fun, int line);

#endif
