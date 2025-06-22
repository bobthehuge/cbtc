#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

#include "../include/context.h"

void ast_desug_node(Node *n);

void ast_desug_litnode(Node *root)
{}

void ast_desug_idnode(Node *root)
{
    struct IdentNode *i = root->as.ident;

    char *key = m_strcat(ctx_currname(), "_");
    key = m_strapp(key, i->name);

    i->name = key;

    if (i->type.base == UNRESOLVED)
    {
        HashPair *hp = get_symbol(i->name);

        if (!hp)
            perr("Unknown symbol '%s' (%s)", i->name, key);
        
        i->type = *typeget(hp->value);
    }
}

void ast_desug_binop(Node *root)
{
    struct BinopNode *b = root->as.binop;
    ast_desug_node(b->lhs);
    ast_desug_node(b->rhs);

    TypeInfo *lt = typeget(b->lhs);
    TypeInfo *rt = typeget(b->rhs);

    if (!lt)
        perr("Invalid binary operation involving kind %zu", b->lhs->kind);
    if (!rt)
        perr("Invalid binary operation involving kind %zu", b->rhs->kind);

    char *str_lt = type2str(lt);
    char *str_rt = type2str(rt);
    
    b->type = *lt;

    switch (root->kind)
    {
    case NK_EXPR_ADD:
        if (lt->refc > 0 && (rt->refc > 0 || rt->base != VT_INT))
            perr("Can't add '%s' with '%s'", str_lt, str_rt);
        else if (rt->refc > 0)
        {
            if (lt->refc > 0 || lt->base != VT_INT)
                perr("Can't add '%s' with '%s'", str_lt, str_rt);

            b->type = *rt;
        }
        else if (lt->base != rt->base)
            perr("Can't add '%s' with '%s'", str_lt, str_rt);

        break;
    case NK_EXPR_MUL:
        if (lt->refc || rt->refc || lt->base != VT_INT || rt->base != VT_INT
            ||lt->base != rt->base)
            perr("Can't mul '%s' with '%s'", str_lt, str_rt);
        break;
    default:
        UNREACHABLE();
    }
}

void ast_desug_unop(Node *root)
{
    struct UnopNode *u = root->as.unop;
    switch (root->kind)
    {
    case NK_EXPR_DEREF:
        ast_desug_node(u->value);
        u->type = *typeget(u->value);

        if (u->type.refc == 0)
            perr("'%s' is not a pointer", type2str(&u->type));

        u->type.refc--;
        break;
    default:
        UNREACHABLE();
    }
}

void ast_desug_assign(Node *root)
{
    struct AssignNode *a = root->as.assign;

    ast_desug_node(a->dest);
    ast_desug_node(a->value);

    TypeInfo *d = typeget(a->dest);
    TypeInfo *v = typeget(a->value);

    assert(d->base != UNRESOLVED && v->base != UNRESOLVED);
    
    if (d->refc != v->refc || d->base != v->base)
        perr("Can't init '%s' from '%s'", type2str(d), type2str(v));
}

void ast_desug_vnode(Node *root)
{
    struct VarDeclNode *v = root->as.vdecl;

    assert(v->type.base != UNRESOLVED);

    char *key = m_strcat(ctx_currname(), "_");
    key = m_strapp(key, v->name);
    
    v->name = key;
    
    if (!add_symbol(key, root))
        perr("%s is already declared", v->name);
    
    if (v->init)
    {
        ast_desug_node(v->init);
        TypeInfo *ti = typeget(v->init);

        if (ti->refc != v->type.refc || ti->base != v->type.base)
            perr("Can't init '%s' from '%s'", type2str(ti), type2str(&v->type));
    }
}

void ast_desug_retnode(Node *root)
{
    ast_desug_node(root->as.ret);
}

void ast_desug_fnode(Node *root)
{
    struct FunDeclNode *f = root->as.fdecl;

    if (!add_symbol(f->name, root))
        perr("%s is already declared", f->name);
    
    ctx_push(root);

    Node **nodes = f->body;
    while (*nodes)
    {
        ast_desug_node(*nodes);
        nodes++;
    }

    (void)ctx_pop();
}

void ast_desug_mnode(Node *root)
{
    struct ModDeclNode *m = root->as.mdecl;

    // don't register file modules
    if (*m->name != '/' && !add_symbol(m->name, root))
        perr("%s is already declared", m->name);

    ctx_push(root);

    Node **nodes = m->nodes;
    while (*nodes)
    {
        ast_desug_node(*nodes);
        nodes++;
    }

    (void)ctx_pop();
}

void ast_desug_node(Node *n)
{
    switch (n->kind)
    {
    case NK_EXPR_IDENT:
        ast_desug_idnode(n);
        break;
    case NK_EXPR_LIT:
        ast_desug_litnode(n);
        break;
    case NK_EXPR_ADD: case NK_EXPR_MUL:
        ast_desug_binop(n);
        break;
    case NK_EXPR_DEREF:
        ast_desug_unop(n);
        break;
    case NK_EXPR_ASSIGN:
        ast_desug_assign(n);
        break;
    case NK_FUN_DECL:
        ast_desug_fnode(n);
        break;
    case NK_MOD_DECL:
        ast_desug_mnode(n);
        break;
    case NK_VAR_DECL:
        ast_desug_vnode(n);
        break;
    case NK_RETURN:
        ast_desug_retnode(n);
        break;
    default:
        err_unexp_node(n);
    }
}

void ast_desug(Node *root)
{
    symtable_reset(16);
    ast_desug_node(root);
}
