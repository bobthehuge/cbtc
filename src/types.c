#include "../include/bth_alloc.h"
#include "../include/types.h"
#include "../include/utils.h"

enum BaseTraitIdx
{
    ADD_IDX,
    SUB_IDX,
    MUL_IDX,
    DIV_IDX,
};

enum BaseTypesIdx
{
    INT_IDX,
    CHAR_IDX,
    ANY_PTR_IDX,
};

static HashTable *trait_table = NULL;
static HashTable *type_table = NULL;

// trait Add<K, L, M>
//     M add(K, L);
// end
//
static void __define_add_trait(void)
{
    struct TraitNode *add = smalloc(sizeof(struct TraitNode));

    add->types = bth_htab_new(8, 3, 0);
    add->funs = bth_htab_new(8, 1, 0);

    struct FunDeclNode *Add_add = smalloc(sizeof(struct FunDeclNode));
    Add_add->ret.id = VT_ANY;
    Add_add->ret.poly = true;
    Add_add->ret.refc = 0;
    Add_add->name = m_strdup("add");
    Add_add->argc = 2;
    Add_add->args = smalloc(2 * sizeof(struct VarDeclNode *));

    TypeInfo *t1 = smalloc(sizeof(TypeInfo));
    TypeInfo *t2 = smalloc(sizeof(TypeInfo));
    TypeInfo *t3 = smalloc(sizeof(TypeInfo));

    t1->repr.id = VT_ANY;
    t1->repr.poly = false;
    t1->repr.refc = 0;
    t1->traits = NULL;

    t2->repr.id = VT_ANY;
    t2->repr.poly = false;
    t2->repr.refc = 0;
    t2->traits = NULL;

    t3->repr.id = VT_ANY;
    t3->repr.poly = false;
    t3->repr.refc = 0;
    t3->traits = NULL;

    bth_htab_add(add->types, "K", t1);
    bth_htab_add(add->types, "L", t2);
    bth_htab_add(add->types, "M", t3);

    struct VarDeclNode *v1 = smalloc(sizeof(struct VarDeclNode));
    struct VarDeclNode *v2 = smalloc(sizeof(struct VarDeclNode));

    v1->init = NULL;
    v1->name = m_strdup("lhs");
    v1->type.id = VT_CUSTOM;
    v1->type.poly = true;
    v1->type.refc = 0;

    v2->init = NULL;
    v2->name = m_strdup("rhs");
    v2->type.id = VT_CUSTOM + 1;
    v2->type.id = true;
    v2->type.refc = 0;

    bth_htab_add(trait_table, "Add", add);
}

void __impl_trait(const char *name, varray _types, varray _funs)
{
    struct ImplNode *im = smalloc(sizeof(struct ImplNode));
    HashData *hd = bth_htab_get(trait_table, name);

    if (!hd)
        perr("'%s' is not a valid trait", name);

    struct TraitNode *tr = hd->value;
    if (_types.len != tr->types->size)
        perr("Missing type information while impl '%s'", name);

    if (_funs.len != tr->funs->size)
        perr("Missing function while impl '%s'", name);

    im->trait = m_strdup(hd->key);

    Type **types = _types.data;
    struct FunDeclNode **funs = _funs.data;

    for (size_t i = 0; i < _types.len; i++)
    {
        if ()
    }
}

void define_trait(const char *name, struct TraitNode *tr)
{
    if (!bth_htab_add(trait_table, name, tr))
        perr("Trait '%s' already declared", name);
}

void reset_traits(void)
{
    trait_table = bth_htab_new(16, 2, 1);

    __define_add_trait();
}
