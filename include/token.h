#ifndef TOKEN_H
#define TOKEN_H

#include <stddef.h>

#include "bth_lex.h"
#include "bth_option.h"
OPTION_TYPEDEF(size_t);

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
Token *collect_tokens(void);
char *get_token_content(Token *tok);
void err_tok_unexp(Token *tok);
void tok_expect(Token *tok, size_t value);

#endif
