#include <stdio.h>
#include <stdlib.h>

#include "../include/bth_alloc.h"

#include "../include/ast.h"
#include "../include/context.h"
#include "../include/token.h"
#include "../include/utils.h"

#define EXPECT(k) tok_expect(&cur, k)

static Token cur = { .kind = LK_END };

Node *new_node(NodeKind k, Token *semholder)
{
    Node *root = smalloc(sizeof(Node));

    root->row = 0;
    root->col = 0;
    root->afile = m_strdup(g_lexer.filename);
    root->kind = k;
    
    if (semholder)
    {
        root->col = semholder->col;
        root->row = semholder->row;
    }

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
    case NK_EXPR_DEREF: case NK_EXPR_REF:
        dst = &root->as.unop;
        s = sizeof(struct UnopNode);
        break;
    case NK_EXPR_ASSIGN:
        dst = &root->as.assign;
        s = sizeof(struct AssignNode);
        break;
    case NK_EXPR_FUNCALL:
        dst = &root->as.fcall;
        s = sizeof(struct FunCallNode);
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

struct IdentNode *get_ident(Node *n)
{
    Node *root = n;

redo:
    switch (root->kind)
    {
    case NK_EXPR_DEREF: case NK_EXPR_REF:
        root = n->as.unop->value;
        goto redo;
    case NK_EXPR_IDENT:
        return root->as.ident;
    default:
        perr("Can't get ident from kind %u", n->kind);
        return NULL;
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

char *get_token_str(Token *tok)
{
    if (tok->end - tok->begin <= 2)
        return m_strdup("");
    return m_strndup(tok->begin+1, tok->end - tok->begin-1);
}

char get_token_char(Token *tok)
{
    if (tok->end - tok->begin > 3)
        perr("Invalid char literal %s", get_token_str(tok));

    return *(tok->begin + 1);
}

Type parse_type(void)
{
    Type res;
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
    Node *node = new_node(NK_EXPR_IDENT, &cur);
    struct IdentNode *root = node->as.ident;

    root->type.refc = 0;
    root->type.base = UNRESOLVED;
    root->name = get_token_content(&cur);

    cur = next_token();

    return node;
}

Node *parse_lit()
{
    Node *node = new_node(NK_EXPR_LIT, &cur);
    struct ValueNode *root = node->as.lit;

    switch (cur.idx)
    {
    case TK_INT_CST:
        root->type.refc = 0;
        root->type.base = VT_INT;
        root->as.vt_int = get_token_int(&cur);
        break;
    case TK_STR_CST:
        root->type.refc = 1;
        root->type.base = VT_CHAR;
        root->as.vt_str = get_token_str(&cur);
        break;
    case TK_CHAR_CST:
        root->type.refc = 0;
        root->type.base = VT_CHAR;
        root->as.vt_char = get_token_char(&cur);
        break;
    default:
        perr("Invalid literal constant of kind %zu", cur.idx);
    }

    cur = next_token();

    return node;
}

Node *parse_expr(void);

Node *parse_funcall(Node *n)
{
    Node *node = new_node(NK_EXPR_FUNCALL, &cur);
    struct FunCallNode *root = node->as.fcall;
       
    root->type.base = UNRESOLVED;
    root->type.refc = 0;
    root->args = smalloc(0);
    root->argc = 0;

    if (n->kind != NK_EXPR_IDENT)
        perr("Invalid function call (should be unreachable)");

    root->name = n->as.ident->name;

    cur = next_token();

    while (cur.idx != TK_CPAREN)
    {
        root->args = srealloc(root->args,
            (root->argc + 1) * sizeof(Node *));
        root->args[root->argc++] = parse_expr();

        // cur = next_token();
        if (cur.idx == TK_COMMA)
            cur = next_token();
    }

    cur = next_token(); // eat CPAREN

    root->args = srealloc(root->args, (root->argc + 1) * sizeof(Node *));
    root->args[root->argc] = NULL;

    return node;
}

Node *parse_value(void)
{
    Node *res;

    switch (cur.idx)
    {
    case TK_IDENTIFIER:
        res = parse_ident();
        if (cur.idx == TK_OPAREN)
            res = parse_funcall(res);
        break;
    case TK_INT_CST:
    case TK_CHAR_CST:
    case TK_STR_CST:
        res = parse_lit();
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
    case TK_STAR: case TK_UPPERSAND:
        {
            NodeKind kind = cur.idx == TK_STAR ? NK_EXPR_DEREF : NK_EXPR_REF;
            Node *new = new_node(kind, &cur);

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

    Node *sum = new_node(NK_EXPR_ADD, &cur);
    sum->as.binop->type.refc = 0;
    sum->as.binop->type.base = UNRESOLVED;
    sum->as.binop->lhs = n;
    sum->as.binop->rhs = parse_expr();

    return sum;
}

Node *parse_product(Node *n)
{
    cur = next_token(); // eat symbol

    Node *mul = new_node(NK_EXPR_MUL, &cur);
    mul->as.binop->type.refc = 0;
    mul->as.binop->type.base = UNRESOLVED;
    mul->as.binop->lhs = n;
    mul->as.binop->rhs = parse_unary();

    return mul;
}

Node *parse_assign(Node *n)
{
    Node *node = new_node(NK_EXPR_ASSIGN, &cur);
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
    case TK_PLUS:
        res = parse_sum(res);
        break;
    case TK_STAR:
        res = parse_product(res);
        goto redo;
    case TK_EQUAL:
        res = parse_assign(res);
        break;
    case TK_SEMICOLON: case TK_CPAREN: case TK_COMMA:
        break;
    default:
        err_tok_unexp(&cur);
    }

    return res;
}

Node *parse_var_decl(Type bt, Token *id)
{
    Node *node = new_node(NK_VAR_DECL, id);
    struct VarDeclNode *root = node->as.vdecl;

    root->name = get_token_content(id);
    root->type = bt;

    if (cur.idx == TK_EQUAL)
    {
        cur = next_token();
        root->init = parse_expr();
    }

    char *name = ctx_currname();
    if (*name)
        name = m_strapp(name, "_");
    name = m_strapp(name, root->name);
    add_symbol(name, node);

    return node;
}

Node *parse_fun_decl(Type bt, Token *id);

Node *parse_decl(Type bt)
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

Node **parse_fun_body(void)
{
    Node **nodes = smalloc(0);
    Node *nnode = NULL;
    size_t count = 0;
    
    for (;;)
    {
        cur = next_token();

        if (cur.idx == TK_END)
            break;

        switch (cur.idx)
        {
        case TK_INT: case TK_CHAR: case TK_UPPERSAND:
            nnode = parse_decl(parse_type());
            break;
        case TK_IDENTIFIER: case TK_STAR:
            nnode = parse_expr();
            break;
        case TK_RETURN:
            cur = next_token();
            nnode = new_node(NK_RETURN, &cur);
            nnode->as.ret = parse_expr();
            break;
        default:
            err_tok_unexp(&cur);
        }

        EXPECT(TK_SEMICOLON);

        nodes = srealloc(nodes, (count + 1) * sizeof(Node *));
        nodes[count++] = nnode;
    }

    nodes = srealloc(nodes, (count + 1) * sizeof(Node *));
    nodes[count] = NULL;

    return nodes;
}

Node *parse_fun_decl(Type bt, Token *id)
{
    Node *node = new_node(NK_FUN_DECL, id);
    struct FunDeclNode *root = node->as.fdecl;

    cur = next_token(); //skip OPAREN

    root->ret = bt;
    root->name = m_strdup(get_token_content(id));

    root->argc = 0;
    root->args = smalloc(0);

    ctx_push(node);

    while (cur.idx != TK_CPAREN)
    {
        root->args = srealloc(root->args,
            (root->argc + 1) * sizeof(Node *));
        root->args[root->argc++] = parse_var_decl(parse_type(), &cur);

        cur = next_token();
        if (cur.idx == TK_COMMA)
            cur = next_token();
    }

    root->args = srealloc(root->args, (root->argc + 1) * sizeof(Node *));
    root->args[root->argc] = NULL;
    
    root->body = parse_fun_body();

    (void)ctx_pop();

    char *name = ctx_currname();
    if (*name)
        name = m_strapp(name, "_");
    name = m_strapp(name, root->name);
    add_symbol(name, node);

    return node;
}

Node **parse_mod_body(Node *node)
{
    Node **nodes = malloc(0);
    Node *nnode = NULL;
    size_t count = 0;

    bool isfile = ctx_count == 0;
    
    ctx_push(node);

    for (;;)
    {
        cur = next_token();

        if ((isfile && cur.kind == LK_END) || (!isfile) && cur.idx == TK_END)
            break;
        
        switch (cur.idx)
        {
        case TK_INT: case TK_UPPERSAND:
            nnode = parse_decl(parse_type());
            break;
        default:
            err_tok_unexp(&cur);
        }

        nodes = srealloc(nodes, (count + 1) * sizeof(Node *));
        nodes[count++] = nnode;
    }

    (void)ctx_pop();
    
    nodes = srealloc(nodes, (count + 1) * sizeof(Node *));
    nodes[count] = NULL;

    return nodes;
}

Node *parse_file(const char *rpath)
{
    symtable_reset(16);
    set_lexer(rpath);

    Node *node = new_node(NK_MOD_DECL, NULL);
    struct ModDeclNode *root = node->as.mdecl;
    
    root->name = m_strdup(realpath(rpath, NULL));
    root->nodes = parse_mod_body(node);

    return node;
}
