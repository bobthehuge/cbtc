#include <err.h>
#include <stdio.h>

#include "../include/ast.h"
#include "../include/utils.h"

#define err_unexp_node(n) __err_unexp_node(n, __func__, __LINE__)
void __err_unexp_node(Node *n, const char *fun, int line)
{
    if (!n)
        errx(1, "Unexpected NULL node");

    errx(1, "%s:%d: Unexpected node %u", fun, line, n->kind);
}

void ast_dump_mnode(struct ModDeclNode *m, int padd)
{
    printf("%*smodule \"%s\"\n", padd, "", m->name);

    Node **nodes = m->nodes;
    while (*nodes)
    {
        ast_dump_node(*nodes, padd + 2);
        nodes++;
    }
        
    printf("%*send\n", padd, "");
}

void ast_dump_fnode(struct FunDeclNode *f, int padd)
{
    char *frett = type2str(f->ret);
    printf("%*sfunction \"%s\" -> %s\n", padd, "", f->name, frett);
    free(frett);

    Node **nodes = f->body;
    while (*nodes)
    {
        ast_dump_node(*nodes, padd + 2);
        nodes++;
    }
        
    printf("%*send\n", padd, " ");
}

void ast_dump_vnode(struct VarDeclNode *v, int padd)
{
    char *vt = type2str(v->type);
    printf("%*svar \"%s\": %s\n", padd, "", v->name, vt);
    if (v->init)
        ast_dump_node(v->init, padd + 2);
}

void ast_dump_assign(Node *n, int padd)
{
    struct IdentNode *id = get_ident(n->as.assign->dest);
    char *vt = type2str(id->type);
    printf("%*sassign \"%s\": %s\n", padd, "", id->name, vt);
    ast_dump_node(n->as.assign->value, padd + 2);
}

void ast_dump_retnode(Node *ret, int padd)
{
    printf("%*sreturn\n", padd, " ");
    ast_dump_node(ret, padd + 2);
}

void ast_dump_idnode(struct IdentNode *id, int padd)
{
    printf("%*sident \"%s\"\n", padd, "", id->name);
}

void ast_dump_litnode(struct ValueNode *l, int padd)
{
    char *lt = type2str(l->type);
    printf("%*slit: %s = ", padd, "", lt);

    printf("%d\n", l->as.vt_int);
}

void ast_dump_binop(Node *b, int padd)
{
    const char *op = NULL;

    switch (b->kind)
    {
    case NK_EXPR_ADD:
        op = "'+'";
        break;
    case NK_EXPR_MUL:
        op = "'*'";
        break;
    default:
        err_unexp_node(b);
    }
    
    printf("%*s(%s\n", padd, "", op);
    ast_dump_node(b->as.binop->lhs, padd + 2);
    ast_dump_node(b->as.binop->rhs, padd + 2);
    printf("%*s)\n", padd, "");
}

void ast_dump_unop(Node *u, int padd)
{
    const char *op = NULL;

    switch (u->kind)
    {
    case NK_EXPR_DEREF:
        op = "deref";
        break;
    default:
        err_unexp_node(u);
    }
    
    printf("%*s(%s\n", padd, "", op);
    ast_dump_node(u->as.unop, padd + 2);
    printf("%*s)\n", padd, "");
}

void ast_dump_node(Node *node, int padd)
{
    switch (node->kind)
    {
    case NK_EXPR_IDENT:
        ast_dump_idnode(node->as.ident, padd);
        break;
    case NK_EXPR_LIT:
        ast_dump_litnode(node->as.lit, padd);
        break;
    case NK_EXPR_ADD: case NK_EXPR_MUL:
        ast_dump_binop(node, padd);
        break;
    case NK_EXPR_DEREF:
        ast_dump_unop(node, padd);
        break;
    case NK_EXPR_ASSIGN:
        ast_dump_assign(node, padd);
        break;
    case NK_FUN_DECL:
        ast_dump_fnode(node->as.fdecl, padd);
        break;
    case NK_MOD_DECL:
        ast_dump_mnode(node->as.mdecl, padd);
        break;
    case NK_VAR_DECL:
        ast_dump_vnode(node->as.vdecl, padd);
        break;
    case NK_RETURN:
        ast_dump_retnode(node->as.ret, padd);
        break;
    default:
        err_unexp_node(node);
    }
}
