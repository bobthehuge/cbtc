#include <err.h>
#include <stdio.h>

#include "../include/ast.h"
#include "../include/utils.h"

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
    printf("%*sfunction \"%s\"\n", padd, "", f->name);
    char *frett = type2str(f->ret);
    printf("%*sreturns %s\n", padd + 2, "", frett);
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
    printf("%*svar \"%s\" of type %s\n", padd, "", v->name, vt);
    ast_dump_node(v->init, padd + 2);
}

void ast_dump_retnode(Node *ret, int padd)
{
    printf("%*sreturn\n", padd, " ");
    ast_dump_node(ret, padd + 2);
}

void ast_dump_idnode(struct IdentNode *id, int padd)
{
    printf("%*sident: %s\n", padd, "", id->value);
}

void ast_dump_litnode(struct ValueNode *l, int padd)
{
    char *lt = type2str(l->type);
    printf("%*slit of type %s = ", padd, "", lt);

    printf("%d\n", l->as.vt_int);
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
        errx(1, "Unexpected node kind %u", node->kind);
    }
}
