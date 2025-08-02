#include "../include/ast.h"
#include "../include/bth_alloc.h"
#include "../include/context.h"
#include "../include/types.h"
#include "../include/utils.h"

#include <stdio.h>

// trait Add<Rhs, Output>
// where
//     Rhs: Any
//     Ouput: Any
// begin
//     Output add(self, Rhs rhs);
// end
// 
static void __define_add_trait(void)
{
    // TraitInfo *tr = smalloc(sizeof(TraitInfo));
    Node *tnode = new_node(NK_TRAIT_DECL, NULL);
    struct TraitDeclNode *tr = tnode->as.tdecl;

    tr->name = m_strdup("Add");
    tr->types = bth_htab_new(8, 0, 0);
    tr->funcs = bth_htab_new(8, 0, 0);

    Node *fnode = new_node(NK_FUN_DECL, NULL);
    struct FunDeclNode *Add_add = fnode->as.fdecl;

    Add_add->ret = empty_type();
    Add_add->ret->id = VT_CUSTOM + 2;
    Add_add->ret->poly = true;

    Add_add->name = m_strdup("add");
    Add_add->argc = 2;
    Add_add->args = smalloc(2 * sizeof(struct VarDeclNode *));

    TypeInfo *t1 = empty_typeinfo();
    TypeInfo *t2 = empty_typeinfo();
    TypeInfo *t3 = empty_typeinfo();

    t1->repr.id = VT_ANY;
    t2->repr.id = VT_ANY;
    t3->repr.id = VT_ANY;

    bth_htab_add(tr->types, "Self", t1);
    bth_htab_add(tr->types, "Rhs", t2);
    bth_htab_add(tr->types, "Output", t3);

    Node *n1 = new_node(NK_VAR_DECL, NULL);
    Node *n2 = new_node(NK_VAR_DECL, NULL);

    struct VarDeclNode *v1 = n1->as.vdecl;
    struct VarDeclNode *v2 = n2->as.vdecl;

    v1->init = NULL;
    v1->name = m_strdup("self");
    v1->type = empty_type();
    v1->type->id = VT_CUSTOM;
    v1->type->poly = true;
    
    v2->init = NULL;
    v2->name = m_strdup("rhs");
    v2->type = empty_type();
    v2->type->id = VT_CUSTOM + 1;
    v2->type->poly = true;

    Add_add->args[0] = n1;
    Add_add->args[1] = n1;

    bth_htab_add(tr->funcs, "add", fnode);
    // define_trait("Add", tr);

    add_symbol("Add", tnode);
}

void __define_char_type(void)
{
    TypeInfo *ti = empty_typeinfo();

    ti->repr.id = VT_CHAR;
    ti->traits = bth_htab_new(4, 1, 1);

    // bth_htab_add(ti->traits, "Add<int, int>", ti);

    define_type("Char", ti);
}

void __define_int_type(void)
{
    TypeInfo *ti = empty_typeinfo();

    ti->repr.id = VT_INT;
    ti->traits = bth_htab_new(4, 1, 1);

//  impl Add<Rhs, Output> for Int
//  where
//      Rhs: Any
//      Ouput: Any
//  begin
//      Output add(self, Rhs rhs);
//  end
// 
    Node *add_int_int = create_impl_node("Add");
    struct ImplDeclNode *im = add_int_int->as.impl;

    // im->types->data[1]->value = get_id_type_info(VT_CHAR);
    im->types->data[1]->value = ti;
    im->types->data[2]->value = ti;
    im->types->data[3]->value = ti;

    Node *node = im->funcs->data[1]->value;
    struct FunDeclNode *f1 = node->as.fdecl;

    f1->args[0]->as.vdecl->type = &ti->repr;
    f1->args[1]->as.vdecl->type = &ti->repr;
    f1->ret = &ti->repr;

    // char *name = impl2str(im);
    // bth_htab_add(ti->traits, name, add_int_int);

    f1->body = smalloc(sizeof(Node *));

    Node *body = new_node(NK_RETURN, NULL);
    body->as.ret = new_node(NK_EXPR_ADD, NULL);

    struct BinopNode *binop = body->as.ret->as.binop;
    binop->lhs = new_node(NK_EXPR_IDENT, NULL);
    binop->rhs = new_node(NK_EXPR_IDENT, NULL);
    binop->lhs->as.ident->name = m_strdup("self");
    binop->rhs->as.ident->name = m_strdup("rhs");
    
    define_type("Int", ti);
    impl_trait(add_int_int);
}

void __define_any_type(void)
{
}

// void define_static_traits(void)
// {
// }

// void define_static_types(void)
// {
// }

void init_cbtbase(void)
{
    __define_add_trait();

    __define_char_type();
    __define_int_type();
    // __define_any_type();
}
