#include "../include/ast.h"
#include "../include/bth_alloc.h"
#include "../include/types.h"
#include "../include/utils.h"

#include <stdio.h>

static void __define_add_trait(void)
{
    TraitInfo *tr = smalloc(sizeof(TraitInfo));

    tr->types = bth_htab_new(8, 0, 0);
    tr->funcs = bth_htab_new(8, 0, 0);

    struct FunDeclNode *Add_add = smalloc(sizeof(struct FunDeclNode));

    Add_add->ret = empty_type();
    Add_add->ret->id = VT_CUSTOM + 2;
    Add_add->ret->poly = true;

    Add_add->name = m_strdup("add");
    Add_add->argc = 2;
    Add_add->args = smalloc(2 * sizeof(struct VarDeclNode *));

    // TypeInfo *t1 = smalloc(sizeof(TypeInfo));
    // TypeInfo *t2 = smalloc(sizeof(TypeInfo));
    // TypeInfo *t3 = smalloc(sizeof(TypeInfo));

    TypeInfo *t1 = empty_typeinfo();
    TypeInfo *t2 = empty_typeinfo();
    TypeInfo *t3 = empty_typeinfo();

    t1->repr.id = VT_ANY;
    // t1->repr.poly = false;
    // t1->repr.refc = 0;
    // t1->traits = NULL;

    t2->repr.id = VT_ANY;
    // t2->repr.poly = false;
    // t2->repr.refc = 0;
    // t2->traits = NULL;

    t3->repr.id = VT_ANY;
    // t3->repr.poly = false;
    // t3->repr.refc = 0;
    // t3->traits = NULL;

    bth_htab_add(tr->types, "Self", t1);
    bth_htab_add(tr->types, "Rhs", t2);
    bth_htab_add(tr->types, "Output", t3);

    struct VarDeclNode *v1 = smalloc(sizeof(struct VarDeclNode));
    struct VarDeclNode *v2 = smalloc(sizeof(struct VarDeclNode));

    v1->init = NULL;
    v1->name = m_strdup("self");
    v1->type = empty_type();
    v1->type->id = VT_CUSTOM;
    v1->type->poly = true;
    // v1->type->refc = 0;
    
    v2->init = NULL;
    v2->name = m_strdup("rhs");
    v2->type = empty_type();
    v2->type->id = VT_CUSTOM + 1;
    v2->type->poly = true;
    // v2->type->refc = 0;

    // bth_htab_add(trait_table, "Add", tr);
    define_trait("Add", tr);
}

void __define_char_type(void)
{
    TypeInfo *ti = empty_typeinfo();

    ti->repr.id = VT_CHAR;
    ti->traits = bth_htab_new(4, 1, 1);

    // bth_htab_add(ti->traits, "Add<int, int>", ti);

    define_type("char", ti);
}

void __define_int_type(void)
{
    TypeInfo *ti = empty_typeinfo();

    ti->repr.id = VT_INT;
    ti->traits = bth_htab_new(4, 1, 1);

    Node *add_int_int = create_impl_node("Add");
    struct ImplDeclNode *im = add_int_int->as.impl;

    im->types->data[1]->value = ti;
    im->types->data[2]->value = ti;
    im->types->data[3]->value = ti;

    struct FunDeclNode *f1 = im->funcs->data[1]->value;

    f1->args[0]->as.vdecl->type = &ti->repr;
    f1->args[1]->as.vdecl->type = &ti->repr;

    char *name = impl2str(im);

    bth_htab_add(ti->traits, name, add_int_int);

    define_type("int", ti);
}

void define_static_traits(void)
{
    __define_add_trait();
}

void define_static_types(void)
{
    __define_char_type();
    __define_int_type();
}
