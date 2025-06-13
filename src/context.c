#include "../include/context.h"

static HashTable *symbols = NULL;
static Node *ctx_stack[MAX_CONTEXT];
unsigned int ctx_count = 0;

void ctx_push(Node *c)
{
    if (ctx_count >= 4)
        errx(1, "Max active contexts reached");

    ctx_stack[ctx_count++] = c;
}

Node *ctx_peak(void)
{
    if (ctx_count <= 0)
        errx(1, "Invalid current context (none)");

    return ctx_stack[ctx_count-1];
}

Node *ctx_pop(void)
{
    Node *res = ctx_peak();
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
            errx(1, "Invalid context detected");
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
}

int add_symbol(const char *bkey, Node *val)
{
    char *key = ctx_currname();
    if (*key)
       key = m_strapp(key, "_");
    key = m_strapp(key, bkey);

    // printf("adding %s\n", key);
    int e = bth_htab_add(symbols, key, val, NULL);

    return e;
}

int is_symbol(const char *bkey)
{
    char *key = m_strcat(ctx_currname(), "_");
    key = m_strapp(key, bkey);

    // printf("searching %s\n", key);
    int e = bth_htab_get(symbols, key) != NULL;
    free(key);

    return e;
}

HashPair *get_symbol(const char *bkey)
{
    char *key = m_strcat(ctx_currname(), "_");
    key = m_strapp(key, bkey);

    // printf("searching %s\n", key);
    void *res = bth_htab_get(symbols, key);
    free(key);

    return res;
}
