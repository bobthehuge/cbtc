#include <assert.h>

#include "../include/bth_alloc.h"
#include "../include/context.h"

#include <stdio.h>

#define derr(fmt, ...) \
do \
{ \
    perr( \
        "\n  in %s:\n    Faulty node at %zu:%zu: "fmt, \
        root->afile, root->row, root->col,##__VA_ARGS__); \
} while (0)

Node *faulty = NULL;

void ast_desug_node(Node *n);

void ast_desug_litnode(Node *root)
{}

void ast_desug_idnode(Node *root)
{
    struct IdentNode *i = root->as.ident;

    char *key = ctx_currname();
    key = m_strapp_n(key, "::", i->name);

    if (i->type->id == UNRESOLVED)
    {
        HashData *hp = get_symbol(key);

        if (!hp)
            derr("Unknown symbol '%s' (%s)", i->name, key);
        
        i->type = typeget(hp->value);
    }

    i->name = key;
}

void ast_desug_binop(Node *root)
{
    Node *lhs = root->as.binop->lhs;
    Node *rhs = root->as.binop->rhs;
    uint kind = root->kind;
    
    m_free(root->as.binop);
    root->kind = NK_EXPR_FUNCALL;
    root->as.fcall = m_smalloc(sizeof(struct FunCallNode));

    struct FunCallNode *f = root->as.fcall;

    // f->funcref = NULL;
    f->argc = 2;
    f->args = m_smalloc(3 * sizeof(Node *));
    f->args[2] = NULL;

    ast_desug_node(lhs);
    ast_desug_node(rhs);

    Type *lt = typeget(lhs);
    Type *rt = typeget(rhs);

    if (!lt)
        derr("Invalid binary operation involving %s", NKSTR(lhs->kind));
    if (!rt)
        derr("Invalid binary operation involving %s", NKSTR(rhs->kind));
   
    char *str_lt = type2str(lt);
    char *str_rt = type2str(rt);

    TypeInfo *lti = get_type_info(lt);
    TypeInfo *rti = get_type_info(rt);

    switch (kind)
    {
    case NK_EXPR_ADD:
        {
            char *key = m_strdup("Add<");
            key = m_strapp_n(key, str_rt, ", ", str_lt, ">");

            Node *got = get_symbolv(key);
            f->name = m_strapp(key, "::add");

            if (got)
            {
                Node *ref = got->as.impl->funcs->data[1]->value;
                f->args[0] = lhs;
                f->args[1] = rhs;
                SET_STATES(ref, CALL);
                return;
            }

            key = m_strdup("Add<");
            key = m_strapp_n(key, str_lt, ", ", str_rt, ">");

            got = get_symbolv(key);
            f->name = m_strapp(key, "::add");

            if (!got)
                derr("Can't add %s with %s", str_lt, str_rt);

            Node *ref = got->as.impl->funcs->data[1]->value;
            f->args[0] = rhs;
            f->args[1] = lhs;
            SET_STATES(ref, CALL);
        }

        break;
    case NK_EXPR_MUL:
        // if (lt->refc || rt->refc || lt->id != VT_INT || rt->id != VT_INT
        //     || lt->id != rt->id)
        //     derr("Can't mul '%s' with '%s'", str_lt, str_rt);
        {
            char *key = m_strdup("Mul<");
            key = m_strapp_n(key, str_rt, ", ", str_lt, ">");

            Node *got = get_symbolv(key);
            f->name = m_strapp(key, "::mul");

            if (got)
            {
                Node *ref = got->as.impl->funcs->data[1]->value;
                f->args[0] = lhs;
                f->args[1] = rhs;
                SET_STATES(ref, CALL);
                return;
            }

            key = m_strdup("Mul<");
            key = m_strapp_n(key, str_lt, ", ", str_rt, ">");

            got = get_symbolv(key);
            f->name = m_strapp(key, "::mul");

            if (!got)
                derr("Can't mul %s with %s", str_lt, str_rt);

            Node *ref = got->as.impl->funcs->data[1]->value;
            f->args[0] = rhs;
            f->args[1] = lhs;
            SET_STATES(ref, CALL);
        }
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
        u->type = typeget(u->value);

        switch (u->type->refc)
        {
        case 0:
            derr("'%s' is not a pointer", type2str(u->type));
        case 1:
            {
                TypeInfo *base = findtype(u->type)->value;
                // m_free(u->type); // DO NOT UNCOMMENT NOW
                u->type = &base->repr;
            }
            break;
        default:
            u->type = typeclone(u->type);
            u->type->refc--;
            break;
        }

        // u->type->refc--;
        break;
    case NK_EXPR_REF:
        ast_desug_node(u->value);
        u->type = typeget(u->value);

        if (u->type->id == VT_INT && u->value->kind == NK_EXPR_LIT)
            derr("Can't ref type '%s'", type2str(u->type));

        u->type->refc++;
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

    assert(d->id != UNRESOLVED && v->id != UNRESOLVED);
    
    if (typecmp(d, v) != TCMP_EQS)
        derr("Can't init '%s' from '%s'", type2str(d), type2str(v));
}

void ast_desug_vnode(Node *root)
{
    struct VarDeclNode *v = root->as.vdecl;

    assert(v->type->id != UNRESOLVED);

    char *key = ctx_currname();
    key = m_strapp_n(key, "::", v->name);
    
    v->name = key;
    
    if (v->init)
    {
        ast_desug_node(v->init);
        Type *ti = typeget(v->init);

        if (typecmp(ti, v->type) != TCMP_EQS)
            derr("Can't init '%s' from '%s'", type2str(ti), type2str(v->type));
    }
}

void ast_desug_retnode(Node *root)
{
    Node *last = ctx_peek();

    if (!last)
        derr("Invalid return outside of function context");

    ast_desug_node(root->as.ret);

    Type *funret = last->as.fdecl->ret;
    Type *ret = typeget(root->as.ret);

    if (typecmp(funret, ret) < TCMP_INC)
    {
        char *str1 = type2str(funret);
        char *str2 = type2str(ret);
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

    SET_STATES(n, CALL);
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

    // fc->type = fun->ret;
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
    case NK_TRAIT_DECL:
        break;
    case NK_IMPL_DECL:
        break;
    default:
        err_unexp_node(n);
    }
}

void ast_desug(Node *root)
{
    // faulty = get_symbol("main::a")->value;

    // dump_symbols();
    // symtable_reset(16);
    ast_desug_node(root);
}
