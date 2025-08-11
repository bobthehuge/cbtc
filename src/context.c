#include <stdio.h>

#include "../include/bth_alloc.h"
#include "../include/context.h"

static HashTable *symbols = NULL;

static Node *ctx_stack[MAX_CONTEXT];
unsigned int ctx_count = 0;

void ctx_push(Node *c)
{
    if (ctx_count >= 4)
        perr("Max active contexts reached");

    ctx_stack[ctx_count++] = c;
}

Node *ctx_peek(void)
{
    if (ctx_count <= 0)
        // perr("No current context");
        return NULL;

    return ctx_stack[ctx_count-1];
}

Node *ctx_pop(void)
{
    Node *res = ctx_peek();

    if (ctx_count <= 0)
        perr("No current context");
    
    ctx_count--;
    return res;
}

char *ctx_currname(void)
{
    char *tmp = NULL;
    char *res = smalloc(1);
    *res = 0;

    int i = ctx_count;
    while (i--)
    {
        Node *ctx = ctx_stack[i];
        switch (ctx->kind)
        {
        case NK_MOD_DECL:
            if (ctx->as.mdecl->name[0] == '/')
                break;
            
            tmp = m_strcat("_", res);
            free(res);
            res = m_strcat(ctx->as.mdecl->name, tmp);
            free(tmp);
            break;
        case NK_FUN_DECL:
            tmp = m_strcat("_", res);
            free(res);
            res = m_strcat(ctx->as.fdecl->name, tmp);
            free(tmp);
            break;
        default:
            perr("Invalid context detected");
        }
    }

    size_t len = strlen(res);
    if (res[len - 1] == '_')
    {
        res = srealloc(res, len);
        res[len - 1] = 0;
    }
    
    return res;
}

void symtable_set(size_t n)
{
    symbols = bth_htab_new(n, 2, 2);
}

void symtable_reset(size_t n)
{
    if (symbols)
        symtable_free();
    symtable_set(n);
}

void symtable_free(void)
{
    bth_htab_destroy(symbols);
    symbols = NULL;
}

int add_symbol(const char *key, Node *val)
{
    return bth_htab_add(symbols, (char *)key, val) > 0;
}

int is_symbol(const char *key)
{
    return bth_htab_get(symbols, (char *)key) != NULL;
}

HashData *get_symbol(const char *key)
{
    return bth_htab_get(symbols, (char *)key);
}

Node *get_symbolv(const char *key)
{
    return bth_htab_vget(symbols, (char *)key);
}

// NOTE: should only be used during the backend process
HashTable *__get_symtable(void)
{
    return symbols;
}

void dump_symbols(void)
{
    printf("\nSymbols = [\n");
    for (size_t i = 1; i < symbols->size; i++)
    {
        if (!symbols->data[i] || !symbols->data[i])
            continue;
        
        HashData *hp = symbols->data[i];
        Node *val = hp->value;

        printf("  %zu: \"%s\"  (%u: %s)\n", i, hp->key, val->kind,
               NODEKIND_STRING[val->kind]);
    }
    printf("]\n\n");
}
