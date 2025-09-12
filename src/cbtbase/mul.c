#include "../../include/ast.h"
#include "../../include/context.h"
#include "../../include/bth_alloc.h"
#include "../../include/types.h"
#include "../../include/utils.h"

static void __define_mul_trait(void)
{
    // TraitInfo *tr = smalloc(sizeof(TraitInfo));
    Node *tnode = new_node(NK_TRAIT_DECL, NULL);
    struct TraitDeclNode *tr = tnode->as.tdecl;

    tr->name = m_strdup("Mul");
    tr->types = bth_htab_new(8, 0, 0);
    tr->funcs = bth_htab_new(8, 0, 0);

    Node *fnode = new_node(NK_FUN_DECL, NULL);
    struct FunDeclNode *Mul_mul = fnode->as.fdecl;

    Mul_mul->ret = empty_type();
    Mul_mul->ret->id = VT_CUSTOM + 2;
    Mul_mul->ret->poly = true;
    Mul_mul->name = m_strdup("mul");

    Mul_mul->argc = 2;
    Mul_mul->args = m_smalloc(2 * sizeof(Node *));

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

    Mul_mul->args[0] = n1;
    Mul_mul->args[1] = n2;

    bth_htab_add(tr->funcs, "mul", fnode);
    // define_trait("Mul", tr);

    add_symbol("Mul", tnode);
}

void __impl_basic_mul(TypeInfo *self, TypeInfo *rhs, TypeInfo *output)
{
    Node *mul_basic = create_impl_node("Mul");
    struct ImplDeclNode *im = mul_basic->as.impl;

    im->types->data[1]->value = self;
    im->types->data[2]->value = rhs;
    im->types->data[3]->value = output;

    Node *node = im->funcs->data[1]->value;
    struct FunDeclNode *f1 = node->as.fdecl;

    f1->body = m_smalloc(sizeof(Node *) * 2);

    Node *body = new_node(NK_RETURN, NULL);
    body->as.ret = new_node(NK_EXPR_MUL, NULL);

    struct BinopNode *binop = body->as.ret->as.binop;

    binop->lhs = new_node(NK_EXPR_IDENT, NULL);
    binop->rhs = new_node(NK_EXPR_IDENT, NULL);

    binop->lhs->as.ident->name = m_strdup("self");
    binop->rhs->as.ident->name = m_strdup("rhs");

    f1->body[0] = body;
    f1->body[1] = NULL;

    impl_trait(mul_basic);
}
