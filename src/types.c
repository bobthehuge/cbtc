#include "../include/bth_alloc.h"
#include "../include/context.h"
#include "../include/types.h"
#include "../include/utils.h"

#include <stdio.h>

// HashTable *trait_table = NULL;
HashTable *type_table = NULL;

Type *empty_type(void)
{
    Type *res = smalloc(sizeof(Type));

    res->id = UNRESOLVED;
    res->poly = false;
    res->refc = 0;

    return res;
}

Type *self_type(void)
{
    Type *self = empty_type();

    self->id = VT_SELF;
    self->poly = true;

    return self;
}

TypeInfo *empty_typeinfo(void)
{
    TypeInfo *ti = smalloc(sizeof(TypeInfo));
    // Type *res = smalloc(sizeof(Type));

    ti->repr.id = UNRESOLVED;
    ti->repr.poly = false;
    ti->repr.refc = 0;

    ti->traits = NULL;

    return ti;
}

char *type2str(Type *t)
{
    char *res = smalloc(t->refc + 1);
    memset(res, '&', t->refc);
    res[t->refc] = 0;
    
    res = m_strapp(res, base2str(t));

    return res;
}

HashData *get_type(Type *t)
{
    HashData *hd = type_table->data[t->id];
    
    if (t->poly)
    {
        if (t->id < VT_CUSTOM)
            perr("type-id %zu isn't valid polymorphic");
        
        Node *ctx = ctx_peek();

        if (!ctx)
            perr("Invalid polymorphic scope");

        switch (ctx->kind)
        {
        case NK_IMPL_DECL:
            return ctx->as.impl->types->data[t->id - VT_CUSTOM + 1];
        case NK_FUN_DECL:
            return ctx->as.fdecl->types->data[t->id - VT_CUSTOM + 1];
        default:
            perr("Invalid polymorphic scope of kind %s",
                 NODEKIND_STRING[ctx->kind]);
        }
    }

    if (!hd)
        perr("'%zu' doesn't refer to a valid type-id", t->id);

    return hd;
}

TypeInfo *get_type_info(Type *t)
{
    HashData *hd = get_type(t);

    if (!hd->value)
        perr("Malformed TypeInfo for id '%zu'", t->id);

    return hd->value;
}

TypeInfo *get_id_type_info(uint id)
{
    if (!id || id > type_table->size)
        perr("Invalid type id %u", id);
    
    HashData *hd = type_table->data[id];

    if (!hd)
        perr("Invalid type id %u", id);

    return hd->value;
}

// HashData *get_trait(const char *name)
// {
//     HashData *hd = bth_htab_get(trait_table, name);

//     if (!hd)
//         perr("'%s' doesn't refer to a valid trait", name);

//     return hd;
// }

// TraitInfo *get_trait_info(const char *name)
// {
//     HashData *hd = get_trait(name);

//     if (!hd->value)
//         perr("Malformed TraitInfo for id '%s'", name);

//     return hd->value;
// }

char *base2str(Type *t)
{
    switch (t->id)
    {
    case UNRESOLVED:
        return m_strdup("unresolved");
    case VT_ANY:
        return m_strdup("any");
    case VT_CHAR:
        return m_strdup("char");
    case VT_INT:
        return m_strdup("int");
    default:
        if (t->poly)
            return base2str(get_type(t)->value);

        TODO();
        break;
    }

    return NULL;
}

Type *typeget(Node *n)
{
redo:
    switch (n->kind)
    {
    case NK_EXPR_ADD: case NK_EXPR_MUL:
        return n->as.binop->type;
    case NK_EXPR_DEREF: case NK_EXPR_REF:
        return n->as.unop->type;
    case NK_EXPR_IDENT:
        return n->as.ident->type;
    case NK_EXPR_LIT:
        return n->as.ident->type;
    case NK_EXPR_FUNCALL:
        {
            Node *fd = get_symbolv(n->as.fcall->name);

            if (!fd || fd->kind != NK_FUN_DECL)
            {
                // printf("trying to get: %s\n", n->as.fcall->name);
                // dump_symbols();
                return NULL;
            }

            Type *t = fd->as.fdecl->ret;

            if (t->poly)
                return fd->as.fdecl->types->data[t->id - VT_CUSTOM]->value;

            return t;
        }
        return NULL;
    case NK_VAR_DECL:
        return n->as.vdecl->type;
    case NK_FUN_DECL:
        return n->as.fdecl->ret;
    case NK_EXPR_ASSIGN:
        n = n->as.assign->dest;
        goto redo;
    default:
        return NULL;
    }
}

TypeCmpError typeicmp(TypeInfo *ti, TypeInfo *si)
{
    // can sample implement target
    if (ti->repr.poly || si->repr.poly)
    {
        uint incomp = 0;

        // are all target's traits in sample
        for (size_t i = 0; i < ti->traits->size; i++)
        {
            HashData *td = ti->traits->data[i];

            if (!td)
                perr("Malformed trait table for id '%zu'", ti->repr.id);

            struct TraitDeclNode *sd = get_symbolv(td->key)->as.tdecl;
        
            if (!sd)
                incomp++;

            // TODO: full trait table comparaison ??
        }

        if (incomp)
        {
            if (incomp == ti->traits->size)
                return TCMP_EXC;

            return TCMP_PIN;
        }

        if (ti->traits->size == si->traits->size)
            return TCMP_PEQ;

        return TCMP_INC;
    }

    if (ti->repr.refc != si->repr.refc)
        return TCMP_NEQ;

    if (ti->repr.id == si->repr.id)
        return TCMP_EQS;

    // avoid expensive lookup for standard types
    if (ti->repr.id < VT_CUSTOM && ti->repr.id < VT_CUSTOM)
    {
        switch (ti->repr.id)
        {
        case VT_CHAR:
            if (si->repr.id == VT_INT)
                return TCMP_RCA;
        case VT_INT:
            if (si->repr.id == VT_CHAR)
                return TCMP_LCA;
        case VT_ANY:
            return TCMP_LCA;
        default:
            UNREACHABLE();
        }
    }

    // TODO: check for From implementation in either target or sample
    TODO();

    return TCMP_NEQ;
}

// see ast.h for error values
// 
TypeCmpError typecmp(Type *target, Type *sample)
{
    if (!target || !sample ||
        target->id == UNRESOLVED || sample->id == UNRESOLVED)
        return TCMP_RES;
    
    TypeInfo *ti = get_type_info(target);
    TypeInfo *si = get_type_info(sample);

    return typeicmp(ti, si);
}

// checks if im's types can impl tr's types
// 
// similar to typecmp, type order must be respected
// 
int typetablecmp(HashTable *tr, HashTable *im, uint *idx)
{
    bool poly = false;
    uint equal = 0;
    uint count = 0;
    
    // TODO: may need additionnal checking when using "advanced" polymorphic
    // variants
    for (size_t i = 1; i < im->size; i++)
    {
        HashData *hdi = im->data[i];
        TypeInfo *tii = hdi->value;

        HashData *hdt = tr->data[i];

        if (!hdt)
        {
            if (idx)
                *idx = i;

            return TCMP_RES;
        }

        TypeInfo *tri = hdt->value;

        int err = typecmp(&tri->repr, &tii->repr);
        if (err < 0)
        {
            if (idx)
            {
                *idx = i;
                return err;
            }

            if (err == TCMP_RES)
                return TCMP_RES;

            poly = !poly && err < TCMP_NEQ;

            count++;
            continue;
        }

        if (err == TCMP_EQS)
            equal++;
    }

    if (count)
    {
        if (count == im->size)
        {
            if (poly)
                return TCMP_EXC;
            return TCMP_NEQ;
        }
        return TCMP_PIN;
    }

    if (idx)
        *idx = im->size;
    
    if (equal == im->size)
    {
        if (poly)
            return TCMP_PEQ;
        return TCMP_EQS;
    }

    return TCMP_INC;
}

char *impl2str(struct ImplDeclNode *im)
{
    char *res = m_strdup(im->trait);
    TypeInfo *ti;
    char *t;

    if (im->types->size > 1)
    {
        ti = im->types->data[2]->value;
        t = type2str(&ti->repr);

        // res = m_strapp(res, "<");
        // res = m_strapp(res, t);
        res = m_strapp_n(res, "<", t);

        for (uint i = 3; i < im->types->size; i++)
        {
            free(t);
            ti = im->types->data[i]->value;
            t = type2str(&ti->repr);
            // res = m_strapp(res, ", ");
            // res = m_strapp(res, t);
            res = m_strapp_n(res, ", ", t);
        }

        free(t);
        res = m_strapp(res, ">");
    }

    res = m_strapp(res, " for ");

    ti = im->types->data[1]->value;
    t = type2str(&ti->repr);
    res = m_strapp_n(res, " for ", t);
    free(t);

    return res;
}

Node *clone_vdecl_node(Node *base)
{
    struct VarDeclNode *ref = base->as.vdecl;

    Node *new = new_node(NK_VAR_DECL, NULL);
    struct VarDeclNode *var = new->as.vdecl;

    var->init = ref->init;
    var->name = m_strdup(ref->name);

    // NOTE: this should be ok as type are initially refs to a known table
    var->type = ref->type;

    return new;
}
    
// NOTE: doesn't clone the body, only the "head"
// 
Node *clone_fdecl_node(Node *base)
{
    struct FunDeclNode *ref = base->as.fdecl;

    Node *new = new_node(NK_FUN_DECL, NULL);
    struct FunDeclNode *fun = new->as.fdecl;
    
    fun->argc = ref->argc;
    fun->ret = ref->ret;
    fun->types = NULL;
    fun->name = m_strdup(ref->name);

    fun->args = smalloc(ref->argc * sizeof(Node *));
    for (uint i = 0; i < ref->argc; i++)
        fun->args[i] = clone_vdecl_node(ref->args[i]);

    fun->body = NULL;

    return new;
}

// NOTE: investigate when "complex" types are used as local type table content
// is only copied, it's pointers are kept intact. Idk man...
// 
Node *create_impl_node(const char *name)
{
    Node *res = new_node(NK_IMPL_DECL, NULL);
    struct ImplDeclNode *im = res->as.impl;

    Node *node = get_symbolv(name);
    if (!node)
        perr("Unresolved trait '%s'\n", name);

    struct TraitDeclNode *tr = node->as.tdecl;

    im->trait = m_strdup(name);
    im->funcs = bth_htab_clone(tr->funcs);
    im->types = bth_htab_clone(tr->types);

    for (uint i = 1; i < im->funcs->size; i++)
    {
        Node *base = tr->funcs->data[i]->value;
        Node *cloned = clone_fdecl_node(base);

        cloned->as.fdecl->types = im->types;
        im->funcs->data[i]->value = cloned;
    }
    
    return res;
}

// <
// ..,
// Any, <- reccall
// Any => VT_CHAR, VT_INT, ...
// >
//
// TODO: try to find a way to "precompute" all type's str into a matrix and
// then expand for every combinaison
// 
void poly_expand(Node *node, TypeInfo *target, const char *ckey, uint start)
{
    struct ImplDeclNode *im = node->as.impl;

    if (start >= im->types->size)
        return;

    HashData *chd = im->types->data[start];
    TypeInfo *cti = chd->value;
    
    if (start == im->types->size - 1)
    {
        if (cti->repr.id != VT_ANY)
        {
            char *cur = type2str(&cti->repr);
            cur = m_strpre(cur, ckey);
            cur = m_strapp(cur, ">");

            // bth_htab_add(target->traits, cur, node);
            add_symbol(cur, node);

            cur = m_strapp(cur, "::");

            for (uint j = 1; j < im->funcs->size; j++)
            {
                Node *fn = im->funcs->data[j]->value;
                char *entry = m_strcat(cur, fn->as.fdecl->name);
                fn->as.fdecl->name = entry;
                add_symbol(entry, fn);
            }

            free(cur);
            return;
        }

        for (uint i = 1; i < type_table->size; i++)
        {
            HashData *hd = type_table->data[i];
            TypeInfo *ti = hd->value;

            if (typeicmp(cti, ti) < TCMP_INC)
                continue;

            char *cur = type2str(&cti->repr);
            // cur = m_strpre(cur, ", ");
            cur = m_strpre(cur, ckey);

            cur = m_strapp(cur, ">");

            // bth_htab_add(target->traits, cur, node);
            add_symbol(cur, node);

            cur = m_strapp(cur, "::");

            for (uint j = 1; j < im->funcs->size; j++)
            {
                Node *fn = im->funcs->data[j]->value;
                char *entry = m_strcat(cur, fn->as.fdecl->name);
                fn->as.fdecl->name = entry;
                add_symbol(entry, fn);
            }

            // TODO: add poly expansion for function
            free(cur);
        }

        return;
    }

    if (cti->repr.id != VT_ANY)
    {
        char *cur = type2str(&cti->repr);
        cur = m_strpre(cur, ckey);
        cur = m_strapp(cur, ", ");

        poly_expand(node, target, cur, start + 1);
        free(cur);

        return;
    }

    for (uint i = start; i < type_table->size; i++)
    {
        HashData *hd = type_table->data[i];
        TypeInfo *ti = hd->value;

        if (typeicmp(cti, ti) < TCMP_INC)
            continue;

        char *cur = type2str(&cti->repr);
        cur = m_strpre(cur, ckey);
        cur = m_strapp(cur, ", ");

        poly_expand(node, target, cur, start + 1);
        free(cur);
    }
}

// 1st type should always be the target type
// void impl_trait(const char *name, varray _types, varray _funcs)
void impl_trait(Node *node)
{
    struct ImplDeclNode *im = node->as.impl;

    Node *query = get_symbolv(im->trait);

    if (!query)
        perr("'%s' is not a valid trait", im->trait);

    struct TraitDeclNode *tr = query->as.tdecl;
    uint erridx = 0;
    int cmp = typetablecmp(tr->types, im->types, &erridx);

    if (cmp < TCMP_INC)
    {
        HashData *hdi = im->types->data[erridx];

        if (cmp == TCMP_RES)
            perr("No type %s in trait %s definition", hdi->key, im->trait);

        HashData *hdt = tr->types->data[erridx];
        
        perr("Can't init %s from %s in %s impl", hdt->key, hdi->key, im->trait);
    }

    // push implementation scoped polymorphic types
    ctx_push(node);
    
    // implement funcs check
    // TODO();

    for (uint i = 1; i < im->funcs->size; i++)
    {
        HashData *hd = im->funcs->data[i];
        struct FunDeclNode *tif = ((Node *)hd->value)->as.fdecl;

        Node *query2 = bth_htab_vget(tr->funcs, tif->name);
        if (!query2)
            perr("No function named %s in %s definition", tif->name, im->trait);

        struct FunDeclNode *trf = query2->as.fdecl;

        // useless as polymorphs are declared at impl definition
        // cmp = typetablecmp(trf->types, tyf->types, &erridx);

        for (uint j = 0; j < tif->argc; j++)
        {
            struct VarDeclNode *tiv = tif->args[i]->as.vdecl;
            struct VarDeclNode *trv = trf->args[i]->as.vdecl;

            if (!trv)
                perr("No type %s in %s definition", tiv->name, trf->name);

            cmp = typecmp(trv->type, tiv->type);
            if (cmp < TCMP_INC)
            {
                char *strv = type2str(tiv->type);
                char *stri = type2str(trv->type);

                perr("Can't init %s from %s in %s impl",
                     strv, stri, im->trait);
            }
        }
    }

    TypeInfo *target = im->types->data[1]->value;

    char *key = m_strdup(im->trait);
    key = m_strapp(key, "<");

    // add polymorphic expansion
    // => replace polymorphics by all possible matching types
    //
    // maybe call this function only when desugaring to avoid missing types
    // 

    poly_expand(node, target, key, 2);
    free(key);

    (void)ctx_pop();
    
    // TODO();
    // bth_htab_add(target->traits, im->trait, im);
    // free(key);
}

// void define_trait(const char *name, TraitInfo *tr)
// {
//     if (!bth_htab_add(trait_table, name, tr))
//         perr("Trait '%s' already declared", name);
// }

// void reset_traits(void)
// {
//     if (trait_table)
//         bth_htab_destroy(trait_table);

//     trait_table = bth_htab_new(16, 2, 1);

//     define_static_traits();
// }

void define_type(const char *name, TypeInfo *ti)
{
    if (!bth_htab_add(type_table, name, ti))
        perr("Type '%s' already declared", name);
}

void reset_types(void)
{
    if (type_table)
        bth_htab_destroy(type_table);

    type_table = bth_htab_new(16, 2, 1);
}
