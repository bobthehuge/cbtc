#include <stdio.h>

#include "../include/context.h"
#include "../include/c_backend.h"
#include "../include/utils.h"

static FILE *fout = NULL;

#include <stdarg.h>
#define err_emit(fmt, ...) \
    __err_emit(__func__, __LINE__, fmt,##__VA_ARGS__)

void __err_emit(const char *fun, int line, const char *fmt, ...)
{
    fclose(fout);
    
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "at %s:%u: ", fun, line);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(1);
}

void bk_c_emit_node(Node *node);
static void bk_c_emit_ident(struct IdentNode *i)
{
    char *key = m_strcat(ctx_currname(), "_");
    key = m_strapp(key, i->name);

    if (*i->type == UNRESOLVED && !is_symbol(i->name))
        err_emit("unknown symbol '%s' (%s)", i->name, key);
    
    fprintf(fout, "%s", key);
    free(key);
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
    default:
        err_emit("unknown unary operator %u", root->kind);
    }
    
    fprintf(fout, "%s", op);
    bk_c_emit_node(root->as.unop);
}

static void bk_c_emit_assign(struct AssignNode *a)
{
    fprintf(fout, "    ");
    bk_c_emit_node(a->dest);
    fprintf(fout, " = ");
    bk_c_emit_node(a->value);
    fprintf(fout, "\n");
}

static void bk_c_emit_function(Node *root)
{
    struct FunDeclNode *f = root->as.fdecl;
    if (!add_symbol(f->name, root))
        err_emit("%s is already declared", f->name);
    
    char *ts = type2str(f->ret);
    fprintf(fout, "%s %s", ts, f->name);
    free(ts);
    fprintf(fout, "(void)\n{\n");

    ctx_push(root);

    Node **nodes = f->body;
    while (*nodes)
    {
        bk_c_emit_node(*nodes);
        nodes++;
    }
    fprintf(fout, "}\n");

    (void)ctx_pop();
}

static void bk_c_emit_variable(Node *root)
{
    struct VarDeclNode *v = root->as.vdecl;

    char *key = m_strcat(ctx_currname(), "_");
    key = m_strapp(key, v->name);

    if (!add_symbol(v->name, root))
        err_emit("%s is already declared", v->name);

    char *ts = type2str(v->type);
    fprintf(fout, "%*s%s %s", (ctx_count - 1) * 4, "", ts, key);
    free(key);

    if (v->init)
    {
        fprintf(fout, " = ");
        bk_c_emit_node(v->init);
    }

    free(ts);
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

    // don't register file modules
    if (*m->name != '/' && !add_symbol(m->name, root))
        err_emit("%s is already declared", m->name);

    ctx_push(root);
    
    Node **nodes = m->nodes;
    while (*nodes)
    {
        bk_c_emit_node(*nodes);
        nodes++;
    }

    (void)ctx_pop();
}

void bk_c_emit_node(Node *node)
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
    case NK_EXPR_DEREF:
        bk_c_emit_unop(node);
        break;
    case NK_EXPR_ASSIGN:
        bk_c_emit_assign(node->as.assign);
        break;
    case NK_FUN_DECL:
        bk_c_emit_function(node);
        break;
    case NK_MOD_DECL:
        bk_c_emit_module(node);
        break;
    case NK_VAR_DECL:
        bk_c_emit_variable(node);
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
