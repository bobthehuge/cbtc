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

// static char *path = NULL;
static char *mpath = NULL;

Node *new_node(NodeKind k)
{
    Node *root = smalloc(sizeof(Node));

    root->col = 0;
    root->row = 0;
    root->kind = k;

    void *dst;
    size_t s;
    
    switch (k)
    {
    case NK_EXPR_IDENT:
        dst = &root->as.ident;
        s = sizeof(struct IdentNode);
        break;
    case NK_EXPR_LIT:
        dst = &root->as.lit;
        s = sizeof(struct ValueNode);
        break;
    case NK_EXPR_ADD: case NK_EXPR_MUL:
        dst = &root->as.binop;
        s = sizeof(struct BinopNode);
        break;
    case NK_EXPR_DEREF:
        dst = &root->as.unop;
        s = sizeof(struct UnopNode);
        break;
    case NK_EXPR_ASSIGN:
        dst = &root->as.assign;
        s = sizeof(struct AssignNode);
        break;
    case NK_FUN_DECL:
        dst = &root->as.fdecl;
        s = sizeof(struct FunDeclNode);
        break;
    case NK_MOD_DECL:
        dst = &root->as.mdecl;
        s = sizeof(struct ModDeclNode);
        break;
    case NK_VAR_DECL:
        dst = &root->as.vdecl;
        s = sizeof(struct VarDeclNode);
        break;
    case NK_RETURN:
        dst = &root->as.ret;
        s = 0;
        break;
    default:
        perr("Unexpected new node kind '%u'", k);
    }

    char **rdst = dst;

    *rdst = NULL;

    if (s)
        *rdst = m_scalloc(s);

    return root;
}

// struct IdentNode *get_ident(Node *n)
// {
//     Node *root = n;

// redo:
//     switch (root->kind)
//     {
//     case NK_EXPR_DEREF:
//         root = n->as.unop->value;
//         goto redo;
//     case NK_EXPR_IDENT:
//         return root->as.ident;
//     default:
//         perr("Can't get ident from kind %u", n->kind);
//         return NULL;
//     }
// }

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
            perr("Invalid int representation %s", src);
        }
    }

    int code = is_valid_int(src, &lnum, base);

    if (!code)
        perr("Invalid int representation %s", src);
    if (code < 0)
        warnx("%zu:%zu: long range error", tok->row, tok->col);

    free(src);
    return lnum;
}

TypeInfo parse_type(void)
{
    TypeInfo res;
    res.refc = 0;

redo:
    switch (cur.idx)
    {
    case TK_UPPERSAND:
        res.refc++;
        cur = next_token();
        goto redo;
    case TK_INT: case TK_INT_CST:
        res.base = VT_INT;
        cur = next_token();
        return res;
    default:
        res.base = UNRESOLVED;
        cur = next_token();
        return res;
    }
}

Node *parse_ident()
{
    Node *node = new_node(NK_EXPR_IDENT);
    struct IdentNode *root = node->as.ident;

    root->type.refc = 0;
    root->type.base = UNRESOLVED;
    root->name = get_token_content(&cur);

    cur = next_token();

    return node;
}

Node *parse_lit()
{
    Node *node = new_node(NK_EXPR_LIT);
    struct ValueNode *root = node->as.lit;

    root->type.refc = 0;
    root->type.base = VT_INT;
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
        res->as.lit->type.base = VT_INT;
        break;
    default:
        err_tok_unexp(cur);
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
            Node *new = new_node(NK_EXPR_DEREF);

            if (low)
                low->as.unop->value = new;
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
            low->as.unop->value = parse_value();
            
        return root;
    }
}

Node *parse_sum(Node *n)
{
    cur = next_token(); // eat symbol

    Node *sum = new_node(NK_EXPR_ADD);
    sum->as.binop->type.refc = 0;
    sum->as.binop->type.base = UNRESOLVED;
    sum->as.binop->lhs = n;
    sum->as.binop->rhs = parse_expr();

    return sum;
}

Node *parse_product(Node *n)
{
    cur = next_token(); // eat symbol

    Node *mul = new_node(NK_EXPR_MUL);
    mul->as.binop->type.refc = 0;
    mul->as.binop->type.base = UNRESOLVED;
    mul->as.binop->lhs = n;
    mul->as.binop->rhs = parse_unary();

    return mul;
}

Node *parse_assign(Node *n)
{
    Node *node = new_node(NK_EXPR_ASSIGN);
    struct AssignNode *root = node->as.assign;

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

Node *parse_var_decl(TypeInfo bt, Token *id)
{
    Node *node = new_node(NK_VAR_DECL);
    struct VarDeclNode *root = node->as.vdecl;

    root->name = get_token_content(id);
    root->type = bt;

    if (cur.idx == TK_EQUAL)
    {
        cur = next_token();
        root->init = parse_expr();
    }

    return node;
}

Node *parse_fun_decl(TypeInfo bt, Token *id);

Node *parse_decl(TypeInfo bt)
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
            nnode = new_node(NK_RETURN);
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

Node *parse_fun_decl(TypeInfo bt, Token *id)
{
    char *old = mpath;
    char *name = get_token_content(id);
    mpath = m_strcat(mpath, name);
    free(name);
    
    Node *res = new_node(NK_FUN_DECL);
    struct FunDeclNode *root = res->as.fdecl;

    cur = next_token(); //skip OPAREN

    if (cur.idx != TK_CPAREN)
    {
        fprintf(stderr, "TODO: function does not take argument\n");
        err_tok_unexp(&cur);
    }
    
    root->ret = bt;
    root->name = m_strdup(mpath);
    root->args = NULL;
    root->body = parse_fun_body();

    free(mpath);
    mpath = old;

    return res;
}

Node **parse_mod_body(void)
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

Node *parse_file(const char *rpath)
{
    set_lexer(rpath);

    Node *node = new_node(NK_MOD_DECL);
    struct ModDeclNode *root = node->as.mdecl;

    mpath = malloc(1);
    *mpath = 0;

    root->name = m_strdup(realpath(rpath, NULL));
    ctx.is_file = true;
    root->nodes = parse_mod_body();
    ctx.is_file = false;

    free(mpath);
    
    return node;
}
