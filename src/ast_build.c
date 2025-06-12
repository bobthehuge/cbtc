#include <stdio.h>
#include <stdlib.h>

#include "../include/bth_alloc.h"

#include "../include/ast.h"
#include "../include/token.h"
#include "../include/utils.h"

#define EXPECT(k) tok_expect(&cur, k)

typedef struct
{
    int is_file:1;
    int is_mod:1;
} CtxFlags;

static CtxFlags ctx;
static Token cur = { .kind = LK_END };

static char *ipath = NULL;

struct IdentNode *get_ident(Node *n)
{
    Node *root = n;

    for (;;)
    {
        switch (n->kind)
        {
        case NK_EXPR_DEREF:
            root = n->as.unop;
            break;
        case NK_EXPR_IDENT:
            return root->as.ident;
        default:
            errx(1, "Can't get ident from kind %u", n->kind);
        }
    }
}

long get_token_int(Token *tok)
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
    VType *tt = smalloc(0);
    size_t len = 0;

redo:
    switch (cur.idx)
    {
    case TK_UPPERSAND:
        tt = realloc(tt, sizeof(VType) * (len + 1));
        tt[len++] = VT_PTR;
        cur = next_token();
        goto redo;
    case TK_INT: case TK_INT_CST:
        tt = realloc(tt, sizeof(VType) * (len + 1));
        tt[len++] = VT_INT;
        cur = next_token();
        break;
    default:
        tt = realloc(tt, sizeof(VType) * (len + 1));
        for (size_t i = len; i > 1; i--)
            tt[i] = tt[i - 1];
        tt[len++] = UNRESOLVED;
        cur = next_token();
        break;
    }

    return tt;
}

Node *parse_ident()
{
    Node *node = smalloc(sizeof(Node));
    struct IdentNode *root = smalloc(sizeof(struct IdentNode));

    node->kind = NK_EXPR_IDENT;
    node->as.ident = root;

    root->type = smalloc(sizeof(VType));
    *root->type = UNRESOLVED;
    root->name = get_token_content(&cur);

    cur = next_token();

    return node;
}

Node *parse_lit()
{
    Node *node = smalloc(sizeof(Node));
    struct ValueNode *root = smalloc(sizeof(struct ValueNode));

    node->kind = NK_EXPR_LIT;
    node->as.lit = root;

    root->type = smalloc(sizeof(VType));
    root->as.vt_int = get_token_int(&cur);

    cur = next_token();

    return node;
}

Node *parse_expr(void);

Node *parse_value(void)
{
    Node *res;

    switch (cur.idx)
    {
    case TK_IDENTIFIER:
        res = parse_ident();
        break;
    case TK_INT_CST:
        res = parse_lit();
        *res->as.lit->type = VT_INT;
        break;
    default:
        err_tok_unexp(&cur);
    }

    return res;
}

Node *parse_unary(void)
{
    Node *root = NULL;
    Node *low = NULL;

redo:
    switch (cur.idx)
    {
    case TK_STAR:
        {
            Node *new = smalloc(sizeof(Node));
            new->kind = NK_EXPR_DEREF;
            new->as.unop = NULL;

            if (low)
                low->as.unop = new;
            low = new;

            if (!root)
                root = new;

            cur = next_token();
        }
        goto redo;
    default:
        if (!root)
            root = parse_value();
        else
            low->as.unop = parse_value();
            
        return root;
    }
}

Node *parse_sum(Node *n)
{
    cur = next_token(); // eat symbol

    Node *sum = smalloc(sizeof(Node));
    sum->as.binop = smalloc(sizeof(struct BinopNode));
    sum->kind = NK_EXPR_ADD;
    sum->as.binop->lhs = n;
    sum->as.binop->rhs = parse_expr();

    return sum;
}

Node *parse_product(Node *n)
{
    cur = next_token(); // eat symbol

    Node *mul = smalloc(sizeof(Node));
    mul->as.binop = smalloc(sizeof(struct BinopNode));
    mul->kind = NK_EXPR_MUL;
    mul->as.binop->lhs = n;
    mul->as.binop->rhs = parse_unary();

    return mul;
}

Node *parse_assign(Node *n)
{
    Node *node = smalloc(sizeof(Node));

    node->kind = NK_EXPR_ASSIGN;

    struct AssignNode *root = smalloc(sizeof(struct AssignNode));
    node->as.assign = root;

    root->dest = n;
        
    if (cur.idx == TK_EQUAL)
    {
        cur = next_token();
        root->value = parse_expr();
    }

    return node;
}

Node *parse_expr(void)
{
    Node *res = parse_unary();

redo:
    switch (cur.idx)
    {
    case TK_EQUAL:
        res = parse_assign(res);
        break;
    case TK_PLUS:
        res = parse_sum(res);
        break;
    case TK_STAR:
        res = parse_product(res);
        goto redo;
    case TK_SEMICOLON:
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
        cur = next_token();
        root->init = parse_expr();
    }

    return node;
}

Node *parse_fun_decl(VType *bt, Token *id);
Node *parse_decl(VType *bt)
{
    Token id = cur;
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

// NOTE: all type definitions must have been parsed
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
        case TK_INT: case TK_UPPERSAND:
            nnode = parse_decl(parse_type());
            break;
        case TK_IDENTIFIER: case TK_STAR:
            nnode = parse_expr();
            break;
        case TK_RETURN:
            cur = next_token();
            nnode = smalloc(sizeof(Node *));
            nnode->kind = NK_RETURN;
            nnode->as.ret = parse_expr();
            break;
        default:
            err_tok_unexp(&cur);
        }

        EXPECT(TK_SEMICOLON);

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
