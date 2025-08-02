#ifndef CONTEXT_H
#define CONTEXT_H

// #include "../include/bth_htab.h"
#include "../include/ast.h"
#include "../include/bth_htab.h"
#include "../include/utils.h"
#include "../include/types.h"

#define MAX_CONTEXT 4
extern unsigned int ctx_count;

void ctx_push(Node *c);
Node *ctx_peek(void);
Node *ctx_pop(void);
char *ctx_currname(void);

void symtable_set(size_t n);
void symtable_reset(size_t n);
void symtable_free(void);
int add_symbol(const char *bkey, Node *val);
int is_symbol(const char *bkey);
HashData *get_symbol(const char *bkey);
Node *get_symbolv(const char *key);
void dump_symbols(void);

#endif
