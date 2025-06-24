#include <stdio.h>

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
        perr("Invalid current context (none)");

    return ctx_stack[ctx_count-1];
}

Node *ctx_pop(void)
{
    Node *res = ctx_peek();
    ctx_count--;
    return res;
}

char *ctx_currname(void)
{
    char *tmp = NULL;
    char *res = malloc(1);
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
        res = realloc(res, len);
        res[len - 1] = 0;
    }
    
    return res;
}

void symtable_set(size_t n)
{
    symbols = bth_htab_init(n);
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
    return bth_htab_add(symbols, (char *)key, val, NULL);
}

int is_symbol(const char *key)
{
    return bth_htab_get(symbols, (char *)key) != NULL;
}

HashPair *get_symbol(const char *key)
{
    return bth_htab_get(symbols, (char *)key);
}

void dump_symbols(void)
{
    for (size_t i = 0; i < symbols->cap; i++)
    {
        if (!symbols->data[i])
            continue;
        
        HashPair *hp = symbols->data[i];
        Node *val = hp->value;

        printf("  [ \"%s\"(%u)", hp->key, val->kind);

        hp = hp->next;

        while (hp)
        {
            val = hp->value;
            printf(", \"%s\"(%u)", hp->key, val->kind);
            hp = hp->next;
        }

        printf("]\n");
    }
}
