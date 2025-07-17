#include "../include/bth_alloc.h"
#include "../include/context.h"
#include "../include/types.h"
#include "../include/utils.h"

static HashTable *trait_table = NULL;
static HashTable *type_table = NULL;

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
        Node *ctx = ctx_peek();

        if (ctx->kind != NK_IMPL_DECL)
            perr("Invalid polymorphic scope");

        hd = ctx->as.impl->types->data[t->id];
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

HashData *get_trait(const char *name)
{
    HashData *hd = bth_htab_get(trait_table, name);

    if (!hd)
        perr("'%s' doesn't refer to a valid trait", name);

    return hd;
}

TraitInfo *get_trait_info(const char *name)
{
    HashData *hd = get_trait(name);

    if (!hd->value)
        perr("Malformed TraitInfo for id '%s'", name);

    return hd->value;
}

const char *base2str(Type *t)
{
    switch (t->id)
    {
    case UNRESOLVED:
        return "unresolved";
    case VT_INT:
        return "int";
    case VT_CHAR:
        return "char";
    case VT_ANY:
        return "any";
    case VT_CUSTOM:
        TODO();
        break;
    default:
        UNREACHABLE();
    }

    return NULL;
}

Type *typeget(Node *_n)
{
    Node *n = _n;

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
        return n->as.fcall->type;
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

// see ast.h for error values
// 
TypeCmpError typecmp(Type *target, Type *sample)
{
    if (!target || !sample ||
        target->id == UNRESOLVED || sample->id == UNRESOLVED)
        return TCMP_RES;
    
    TypeInfo *ti = get_type_info(target);
    TypeInfo *si = get_type_info(sample);

    // can sample implement target
    if (target->poly || sample->poly)
    {
        uint incomp = 0;

        // are all target's traits in sample
        for (size_t i = 0; i < ti->traits->size; i++)
        {
            HashData *td = ti->traits->data[i];

            if (!td)
                perr("Malformed trait table for id '%zu'", target->id);

            TraitInfo *sd = get_trait_info(td->key);
        
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

    if (target->refc != sample->refc)
        return TCMP_NEQ;

    if (target->id == sample->id)
        return TCMP_EQS;

    // avoid expensive lookup for standard types
    if (target->id < VT_CUSTOM && target->id < VT_CUSTOM)
    {
        switch (target->id)
        {
        case VT_CHAR:
            if (sample->id == VT_INT)
                return TCMP_RCA;
        case VT_INT:
            if (sample->id == VT_CHAR)
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

        res = m_strapp(res, "<");
        res = m_strapp(res, t);

        for (uint i = 3; i < im->types->size; i++)
        {
            free(t);
            ti = im->types->data[i]->value;
            t = type2str(&ti->repr);
            res = m_strapp(res, ", ");
            res = m_strapp(res, t);
        }

        free(t);
        res = m_strapp(res, ">");
    }

    res = m_strapp(res, " for ");

    ti = im->types->data[1]->value;
    t = type2str(&ti->repr);
    res = m_strapp(res, t);
    free(t);

    return res;
}

Node *create_impl_node(const char *name)
{
    Node *res = new_node(NK_IMPL_DECL, NULL);
    struct ImplDeclNode *im = res->as.impl;

    TraitInfo *tr = get_trait_info(name);

    im->trait = m_strdup(name);
    im->funcs = bth_htab_clone(tr->funcs);
    im->types = bth_htab_clone(tr->types);

    return res;
}

// 1st type should always be the target type
// void impl_trait(const char *name, varray _types, varray _funcs)
void impl_trait(Node *node)
{
    struct ImplDeclNode *im = node->as.impl;

    TraitInfo *tr = bth_htab_vget(trait_table, im->trait);

    if (!tr)
        perr("'%s' is not a valid trait", im->trait);

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

    for (uint i = 0; i < im->funcs->size; i++)
    {
        HashData *hd = im->funcs->data[i];
        struct FunDeclNode *tif = hd->value;
        struct FunDeclNode *trf = bth_htab_vget(tr->funcs, tif->name);

        if (!trf)
            perr("No function named %s in %s definition", tif->name, im->trait);

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
    char *key = impl2str(im);

    // add polymorphic expansion
    // => replace polymorphics by all possible matching types
    //
    // maybe call this function only when desugaring to avoid missing types
    // 
    TODO();
    
    bth_htab_add(target->traits, key, im);
    free(key);
}

void define_trait(const char *name, TraitInfo *tr)
{
    if (!bth_htab_add(trait_table, name, tr))
        perr("Trait '%s' already declared", name);
}

void reset_traits(void)
{
    if (trait_table)
        bth_htab_destroy(trait_table);

    trait_table = bth_htab_new(16, 2, 1);

    define_static_traits();
}

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

    define_static_types();
}
