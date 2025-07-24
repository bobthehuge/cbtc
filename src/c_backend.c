#include <stdio.h>

#include "../include/context.h"
#include "../include/c_backend.h"
#include "../include/utils.h"

static FILE *fout = NULL;

#include <stdarg.h>
#define err_emit(fmt, ...) \
do \
{ \
    fclose(fout); \
    perr(fmt,#__VA_ARGS__); \
} while (0)

static char *type2crep(Type *t)
{
    char *tmp = malloc(t->refc + 1);
    memset(tmp, '*', t->refc);
    tmp[t->refc] = 0;

    char *res;
    
    switch (t->id)
    {
    case UNRESOLVED:
        res = m_strcat("unresolved", tmp);
        break;
    case VT_INT:
        res = m_strcat("int", tmp);
        break;
    case VT_CHAR:
        res = m_strcat("char", tmp);
        break;
    default:
        UNREACHABLE();
    }

    free(tmp);
    return res;
}

static void bk_c_emit_node(Node *node);

static void bk_c_emit_ident(struct IdentNode *i)
{
    char *name = m_strchg(i->name, "::", "_");
    fprintf(fout, "%s", name);
    free(name);
}

static void bk_c_emit_literal(struct ValueNode *lit)
{
    fprintf(fout, "%d", lit->as.vt_int);
}

static void bk_c_emit_binop(Node *root)
{
    const char *op = NULL;

    switch (root->kind)
    {
    case NK_EXPR_ADD:
        op = "+";
        break;
    case NK_EXPR_MUL:
        op = "*";
        break;
    default:
        err_emit("unknown binary operator %u", root->kind);
    }
    
    bk_c_emit_node(root->as.binop->lhs);
    fprintf(fout, " %s ", op);
    bk_c_emit_node(root->as.binop->rhs);
}

static void bk_c_emit_unop(Node *root)
{
    const char *op = NULL;

    switch (root->kind)
    {
    case NK_EXPR_DEREF:
        op = "*";
        break;
    case NK_EXPR_REF:
        op = "&";
        break;
    default:
        err_emit("unknown unary operator %u", root->kind);
    }
    
    fprintf(fout, "%s", op);
    bk_c_emit_node(root->as.unop->value);
}

static void bk_c_emit_assign(struct AssignNode *a)
{
    fprintf(fout, "%*.s", (ctx_count - 1) * 4, "");
    bk_c_emit_node(a->dest);
    fprintf(fout, " = ");
    bk_c_emit_node(a->value);
    fprintf(fout, ";\n");
}

static void bk_c_emit_funcall(Node *root)
{
    struct FunCallNode *fc = root->as.fcall;

    char *name = m_strchg_all(fc->name, "::", "_");

    m_strrep_all(&name, "<", "_");
    m_strrep_all(&name, ">", "_");
    m_strrep_all(&name, ", ", "_");

    fprintf(fout, "%s(", name);
    free(name);

    Node **nodes = fc->args;

    while (*nodes)
    {
        bk_c_emit_node(*nodes++);

        if (*nodes)
            fprintf(fout, ", ");
    }

    fprintf(fout, ")");
}

static void bk_c_emit_function(Node *root)
{
    struct FunDeclNode *f = root->as.fdecl;
    
    char *ts = type2crep(f->ret);
    fprintf(fout, "%s %s", ts, f->name);
    free(ts);

    Node **nodes = f->args;
    fprintf(fout, "(");

    ctx_push(root);

    if (!f->argc)
    {
        fprintf(fout, "void)\n{\n");
    }
    else
    {
        fprintf(fout, "\n");

        while (*nodes)
        {
            bk_c_emit_node(*nodes);

            nodes++;
        }

        fprintf(fout, "){\n");
    }
    nodes = f->body;

    while (*nodes)
    {
        bk_c_emit_node(*nodes);
        nodes++;
    }

    (void)ctx_pop();

    fprintf(fout, "}\n");
}

static void bk_c_emit_vdecl(Node *root)
{
    struct VarDeclNode *v = root->as.vdecl;
    char *ts = type2crep(v->type);
    char *name = m_strchg_all(v->name, "::", "_");
    fprintf(fout, "%*s%s %s", (ctx_count - 1) * 4, "", ts, name);
    free(name);
    free(ts);

    if (v->init)
    {
        fprintf(fout, " = ");
        bk_c_emit_node(v->init);
    }

    Node *last = ctx_peek();

    if (last->kind == NK_FUN_DECL && last->as.fdecl->argc)
    {
        // shouldn't be useful anymore as caller's argc should already have
        // been checked in desug
        if (last->as.fdecl->argc-- > 1)
            fprintf(fout, ",");
        fprintf(fout, "\n");
    }
    else
        fprintf(fout, ";\n");
}

static void bk_c_emit_return(struct Node *r)
{
    fprintf(fout, "%*sreturn ", (ctx_count - 1) * 4, "");
    bk_c_emit_node(r);
    fprintf(fout, ";\n");
}

static void bk_c_emit_module(Node *root)
{
    struct ModDeclNode *m = root->as.mdecl;

    Node **nodes = m->nodes;
    ctx_push(root);

    while (*nodes)
    {
        bk_c_emit_node(*nodes);
        nodes++;

        if (*nodes)
            fprintf(fout, "\n");
    }

    (void)ctx_pop();
}

static void bk_c_emit_node(Node *node)
{
    switch (node->kind)
    {
    case NK_EXPR_IDENT:
        bk_c_emit_ident(node->as.ident);
        break;
    case NK_EXPR_LIT:
        bk_c_emit_literal(node->as.lit);
        break;
    case NK_EXPR_ADD: case NK_EXPR_MUL:
        bk_c_emit_binop(node);
        break;
    case NK_EXPR_DEREF: case NK_EXPR_REF:
        bk_c_emit_unop(node);
        break;
    case NK_EXPR_ASSIGN:
        bk_c_emit_assign(node->as.assign);
        break;
    case NK_EXPR_FUNCALL:
        bk_c_emit_funcall(node);
        break;
    case NK_FUN_DECL:
        bk_c_emit_function(node);
        break;
    case NK_MOD_DECL:
        bk_c_emit_module(node);
        break;
    case NK_VAR_DECL:
        bk_c_emit_vdecl(node);
        break;
    case NK_RETURN:
        bk_c_emit_return(node->as.ret);
        break;
    default:
        err_emit("Unexpected node kind %u", node->kind);
    }
}

void bk_c_emit(Node *ast, const char *fout_path)
{
    symtable_reset(16);
    fout = fopen(fout_path, "w");

    if (!fout)
        err(1, "Invalid out path");

    bk_c_emit_node(ast);
    fclose(fout);
}
