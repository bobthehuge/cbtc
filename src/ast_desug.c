#include <assert.h>
#include <stdarg.h>

#include "../include/context.h"

#define derr(fmt, ...) \
do \
{ \
    perr( \
        "\n  in %s:\n    Faulty node at %zu:%zu: "fmt, \
        root->afile, root->row, root->col,##__VA_ARGS__); \
} while (0)

void ast_desug_node(Node *n);

void ast_desug_litnode(Node *root)
{}

void ast_desug_idnode(Node *root)
{
    struct IdentNode *i = root->as.ident;

    char *key = m_strcat(ctx_currname(), "_");
    key = m_strapp(key, i->name);

    if (i->type.base == UNRESOLVED)
    {
        HashData *hp = get_symbol(key);

        if (!hp)
            derr("Unknown symbol '%s' (%s)", i->name, key);
        
        i->type = *typeget(hp->value);
    }

    i->name = key;
}

void ast_desug_binop(Node *root)
{
    struct BinopNode *b = root->as.binop;
    ast_desug_node(b->lhs);
    ast_desug_node(b->rhs);

    Type *lt = typeget(b->lhs);
    Type *rt = typeget(b->rhs);

    if (!lt)
        derr("Invalid binary operation involving kind %zu", b->lhs->kind);
    if (!rt)
        derr("Invalid binary operation involving kind %zu", b->rhs->kind);

    char *str_lt = type2str(lt);
    char *str_rt = type2str(rt);
    
    b->type = *lt;

    switch (root->kind)
    {
    case NK_EXPR_ADD:
        if (lt->refc > 0 && (rt->refc > 0 || rt->base != VT_INT))
            derr("Can't add '%s' with '%s'", str_lt, str_rt);
        else if (rt->refc > 0)
        {
            if (lt->refc > 0 || lt->base != VT_INT)
                derr("Can't add '%s' with '%s'", str_lt, str_rt);

            b->type = *rt;
        }
        else if (lt->base != rt->base)
            derr("Can't add '%s' with '%s'", str_lt, str_rt);

        break;
    case NK_EXPR_MUL:
        if (lt->refc || rt->refc || lt->base != VT_INT || rt->base != VT_INT
            || lt->base != rt->base)
            derr("Can't mul '%s' with '%s'", str_lt, str_rt);
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
            derr("'%s' is not a pointer", type2str(&u->type));

        u->type.refc--;
        break;
    case NK_EXPR_REF:
        ast_desug_node(u->value);
        u->type = *typeget(u->value);

        if (u->type.base == VT_INT && u->value->kind == NK_EXPR_LIT)
            derr("Can't ref type '%s'", type2str(&u->type));

        u->type.refc++;
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

    Type *d = typeget(a->dest);
    Type *v = typeget(a->value);

    assert(d->base != UNRESOLVED && v->base != UNRESOLVED);
    
    if (typecmp(d, v))
        derr("Can't init '%s' from '%s'", type2str(d), type2str(v));
}

void ast_desug_vnode(Node *root)
{
    struct VarDeclNode *v = root->as.vdecl;

    assert(v->type.base != UNRESOLVED);

    char *key = m_strcat(ctx_currname(), "_");
    key = m_strapp(key, v->name);
    
    v->name = key;
    
    if (v->init)
    {
        ast_desug_node(v->init);
        Type *ti = typeget(v->init);

        if (typecmp(ti, &v->type))
            derr("Can't init '%s' from '%s'", type2str(ti), type2str(&v->type));
    }
}

void ast_desug_retnode(Node *root)
{
    Node *last = ctx_peek();

    if (!last)
        derr("Invalid return outside of function context");

    ast_desug_node(root->as.ret);

    Type funret = last->as.fdecl->ret;
    Type *ret = typeget(root->as.ret);

    if (typecmp(&funret, ret))
    {
        const char *str1 = type2str(&funret);
        const char *str2 = type2str(ret);
        derr("Can't return '%s' from '%s'", str1, str2);
    }
}

void ast_desug_fcall(Node *root)
{
    struct FunCallNode *fc = root->as.fcall;

    HashData *hp = get_symbol(fc->name);

    if (!hp)
        derr("Unknown symbol '%s'");

    Node *n = hp->value;

    if (n->kind != NK_FUN_DECL)
        derr("'%s' is not a function", fc->name);

    struct FunDeclNode *fun = n->as.fdecl;

    if (fun->argc != fc->argc)
        derr("Argc mismatch, expected %u", n->as.fdecl->argc);

    for (uint i = 0; i < fc->argc; i++)
    {
        ast_desug_node(fc->args[i]);

        Type *t1 = typeget(fun->args[i]);
        Type *t2 = typeget(fc->args[i]);

        if (typecmp(t1, t2))
        {
            const char *str1 = type2str(t1);
            const char *str2 = type2str(t2);
            derr("Arg[%u]: Can't init '%s' from '%s'", i, str1, str2);
        }
    }

    fc->type = fun->ret;
}

void ast_desug_fnode(Node *root)
{
    struct FunDeclNode *f = root->as.fdecl;

    ctx_push(root);

    Node **nodes = f->args;
    
    while (*nodes)
    {
        if ((*nodes)->kind != NK_VAR_DECL)
            derr("Unexpected node of kind '%zu'", (*nodes)->kind);

        ast_desug_node(*nodes);
        nodes++;
    }

    nodes = f->body;
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
        derr("%s is already declared", m->name);

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
    case NK_EXPR_DEREF: case NK_EXPR_REF:
        ast_desug_unop(n);
        break;
    case NK_EXPR_ASSIGN:
        ast_desug_assign(n);
        break;
    case NK_EXPR_FUNCALL:
        ast_desug_fcall(n);
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
    // symtable_reset(16);
    ast_desug_node(root);
}
