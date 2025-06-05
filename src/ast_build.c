#include <stdio.h>
#include <stdlib.h>

#include "../include/bth_alloc.h"

#include "../include/ast.h"
#include "../include/token.h"
#include "../include/utils.h"

typedef struct
{
    int is_file:1;
    int is_mod:1;
} CtxFlags;

static CtxFlags ctx;
static Token cur = { .kind = LK_END };

static char *ipath = NULL;

Node *parse_var_assign(VType *bt, Token *id)
{
    if (*bt == UNRESOLVED)
        errx(1, "TODO: var assign isn't supported yet");

    Node *res = smalloc(sizeof(Node));
    res->kind = NK_EXPR_LIT;

    res->as.lit = smalloc(sizeof(struct ValueNode));
    res->as.lit->type = smalloc(sizeof(VType));

    *res->as.lit->type = *bt;
    cur = next_token();
    res->as.lit->as.vt_int = atoi(get_token_content(&cur));

    return res;
}

long token_get_int(Token *tok)
{
    char *src = get_token_content(tok);

    long lnum;
    int base = 10;

    if (*(src + 1) && *src == '0')
    {
        switch (*(src + 1))
        {
        case 'x': case 'X':
            base = 16;
            break;
        case 'b': case 'B':
            base = 2;
            break;
        default:
            errx(1, "invalid int representation %s", src);
        }
    }

    int code = is_valid_int(src, &lnum, base);

    if (!code)
        errx(1, "invalid int representation %s", src);
    if (code < 0)
        warnx("%zu:%zu: long range error", tok->row, tok->col);

    free(src);
    return lnum;
}

VType *parse_type(void)
{
    VType *tt = smalloc(sizeof(VType));
    *tt = VT_INT;

    return tt;
}

Node *parse_expr(void)
{
    cur = next_token();
    Node *res = smalloc(sizeof(Node));
    
    switch (cur.idx)
    {
    case TK_RETURN:
        res->kind = NK_RETURN;
        res->as.ret = parse_expr();
        break;
    case TK_IDENTIFIER:
        res->kind = NK_EXPR_IDENT;
        res->as.ident = smalloc(sizeof(struct IdentNode));
        res->as.ident->type = smalloc(sizeof(VType));
        *res->as.ident->type = UNRESOLVED;
        res->as.ident->value = get_token_content(&cur);
        break;
    case TK_INT_CST:
        res->kind = NK_EXPR_LIT;
        res->as.lit = smalloc(sizeof(struct ValueNode));
        res->as.lit->type = smalloc(sizeof(VType));
        *res->as.lit->type = VT_INT;
        res->as.lit->as.vt_int = token_get_int(&cur);
        break;
    default:
        err_tok_unexp(&cur);
    }

    return res;
}

Node *parse_var_decl(VType *bt, Token *id)
{
    Node *node = smalloc(sizeof(Node));

    node->kind = NK_VAR_DECL;
    
    struct VarDeclNode *root = smalloc(sizeof(struct VarDeclNode));
    node->as.vdecl = root;
    root->name = get_token_content(id);
    root->type = bt;

    if (cur.idx == TK_EQUAL)
    {
        root->init = parse_expr();
        cur = next_token();
    }
    else if (cur.idx != TK_SEMICOLON)
        err_tok_unexp(&cur);

    return node;
}

Node *parse_fun_decl(VType *bt, Token *id);
Node *parse_decl(VType *bt)
{
    Token id = next_token();
    cur = next_token();

    switch (cur.idx)
    {
    case TK_OPAREN:
        return parse_fun_decl(bt, &id);
    case TK_EQUAL:
    case TK_SEMICOLON:
        return parse_var_decl(bt, &id);
    default:
        err_tok_unexp(&cur);
    }

    return NULL;
}

Node **parse_fun_body(void)
{
    Node **nodes = malloc(0);
    Node *nnode = NULL;
    size_t count = 0;

    for (;;)
    {
        cur = next_token();

        if (cur.idx == TK_END)
            break;

        switch (cur.idx)
        {
        case TK_INT:
            nnode = parse_decl(parse_type());
            break;
        case TK_RETURN:
            nnode = smalloc(sizeof(Node *));
            nnode->kind = NK_RETURN;
            nnode->as.ret = parse_expr();
            cur = next_token();
            break;
        default:
            err_tok_unexp(&cur);
        }

        nodes = realloc(nodes, (count + 1) * sizeof(Node *));
        nodes[count++] = nnode;
    }

    nodes = realloc(nodes, (count + 1) * sizeof(Node *));
    nodes[count++] = NULL;

    return nodes;
}

Node *parse_fun_decl(VType *bt, Token *id)
{
    char *old = ipath;
    char *name = get_token_content(id);
    ipath = m_strcat(ipath, name);
    free(name);
    
    Node *res = smalloc(sizeof(Node));
    struct FunDeclNode *root = smalloc(sizeof(struct FunDeclNode));
    res->kind = NK_FUN_DECL;
    res->as.fdecl = root;

    cur = next_token(); //skip OPAREN

    if (cur.idx != TK_CPAREN)
    {
        fprintf(stderr, "TODO: function does not take argument\n");
        err_tok_unexp(&cur);
    }
    
    root->ret = bt;
    root->name = m_strdup(ipath);
    root->args = NULL;
    root->body = parse_fun_body();

    free(ipath);
    ipath = old;

    return res;
}

Node **parse_module_body(void)
{
    Node **nodes = malloc(0);
    Node *nnode = NULL;
    size_t count = 0;

    for (;;)
    {
        cur = next_token();

        if ((ctx.is_file && cur.kind == LK_END) ||
            (ctx.is_mod && cur.idx == TK_END)
        )
            break;

        switch (cur.idx)
        {
        case TK_INT:
            nnode = parse_decl(parse_type());
            break;
        default:
            err_tok_unexp(&cur);
        }

        nodes = realloc(nodes, (count + 1) * sizeof(Node *));
        nodes[count++] = nnode;
    }
    
    nodes = realloc(nodes, (count + 1) * sizeof(Node *));
    nodes[count++] = NULL;

    return nodes;
}

Node *parse_file(const char *path)
{
    set_lexer(path);

    Node *root = smalloc(sizeof(Node));
    root->kind = NK_MOD_DECL;

    ipath = malloc(1);
    *ipath = 0;

    root->as.mdecl = smalloc(sizeof(struct ModDeclNode));
    root->as.mdecl->name = realpath(path, NULL);
    ctx.is_file = true;
    root->as.mdecl->nodes = parse_module_body();
    ctx.is_file = false;

    free(ipath);
    
    return root;
}
