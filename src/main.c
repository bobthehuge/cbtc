// #include <assert.h>
#include <error.h>
#include <stdio.h>

#include "../include/ast.h"
#include "../include/utils.h"

#define BTH_LEX_IMPLEMENTATION
#include "../include/bth_lex.h"

#define BTH_OPTION_IMPLEMENTATION
#include "../include/bth_option.h"
OPTION_TYPEDEF(size_t);

#define BTH_IO_IMPLEMENTATION
#include "../include/bth_io.h"

#define BTH_ALLOC_IMPLEMENTATION
#include "../include/bth_alloc.h"

#define BTH_HTAB_IMPLEMENTATION
#include "../include/bth_htab.h"

// void gen_sym_table(struct ModDeclNode *m)
// {
//     Node **ch = m->nodes;
//     while (*ch)
//     {
//         if ((*ch)->kind == NK_EXPR_)

//     }
// }
static FILE *fout = NULL;
static HashTable *symbols = NULL;

#define MAX_CONTEXT 4
static Node *ctx_stack[MAX_CONTEXT];
static int ctx_count = 0;

#include <stdarg.h>
void err_emit(const char *fmt, ...)
{
    fclose(fout);

    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(1);
}

void ctx_push(Node *c)
{
    if (ctx_count >= 4)
        err_emit("Max active contexts reached");

    ctx_stack[ctx_count++] = c;
}

Node *ctx_peak(void)
{
    if (ctx_count <= 0)
        err_emit("Invalid current context (none)");

    return ctx_stack[ctx_count-1];
}

Node *ctx_pop(void)
{
    Node *res = ctx_peak();
    ctx_count--;
    return res;
}

char *ctx_currname(void)
{
    char *tmp = NULL;
    char *res = malloc(1);
    *res = 0;

    int i = ctx_count;
    while (i--)
    {
        Node *ctx = ctx_stack[i];
        switch (ctx->kind)
        {
        case NK_MOD_DECL:
            if (ctx->as.mdecl->name[0] == '/')
                break;
            
            tmp = m_strcat("_", res);
            free(res);
            res = m_strcat(ctx->as.mdecl->name, tmp);
            free(tmp);
            break;
        case NK_FUN_DECL:
            tmp = m_strcat("_", res);
            free(res);
            res = m_strcat(ctx->as.fdecl->name, tmp);
            free(tmp);
            break;
        default:
            err_emit("Invalid context detected");
        }
    }

    size_t len = strlen(res);
    if (res[len - 1] == '_')
    {
        res = realloc(res, len);
        res[len - 1] = 0;
    }
    
    return res;
}

void bk_c_emit_node(Node *node);

int add_symbol(const char *bkey, Node *val)
{
    char *key = ctx_currname();
    if (*key)
       key = m_strapp(key, "_");
    key = m_strapp(key, bkey);

    // printf("adding %s\n", key);
    int e = bth_htab_add(symbols, key, val, NULL);

    return e;
}

int is_symbol(const char *bkey)
{
    char *key = m_strcat(ctx_currname(), "_");
    key = m_strapp(key, bkey);

    // printf("searching %s\n", key);
    int e = bth_htab_get(symbols, key) != NULL;
    free(key);

    return e;
}

void bk_c_emit_ident(struct IdentNode *i)
{
    char *key = m_strcat(ctx_currname(), "_");
    key = m_strapp(key, i->value);

    if (*i->type == UNRESOLVED && !is_symbol(i->value))
        err_emit("unknown symbol '%s' (%s)", i->value, key);
    
    fprintf(fout, "%s", key);
    free(key);
}

void bk_c_emit_literal(struct ValueNode *lit)
{
    fprintf(fout, "%d", lit->as.vt_int);
}

void bk_c_emit_function(Node *root)
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

void bk_c_emit_variable(Node *root)
{
    struct VarDeclNode *v = root->as.vdecl;

    char *key = m_strcat(ctx_currname(), "_");
    key = m_strapp(key, v->name);

    if (!add_symbol(v->name, root))
        err_emit("%s is already declared", v->name);

    char *ts = type2str(v->type);
    fprintf(fout, "%*s%s %s", (ctx_count - 1) * 4, "", ts, key);
    free(key);
    free(ts);

    if (v->init)
    {
        fprintf(fout, " = ");
        bk_c_emit_node(v->init);
    }

    fprintf(fout, ";\n");
}

void bk_c_emit_return(struct Node *r)
{
    fprintf(fout, "%*sreturn ", (ctx_count - 1) * 4, "");
    bk_c_emit_node(r);
    fprintf(fout, ";\n");
}

void bk_c_emit_module(Node *root)
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
        errx(1, "Unexpected node kind %u", node->kind);
    }
}

int main(int argc, char **argv)
{
#if CHECK_PREFIX_COLLISIONS
    {
        size_t hay;
        size_t sub;

        if (check_prefix_collisions(&hay, &sub))
            errx(1, "Collisions detected: idx(%s) > idx(%s)",
                 KEYWORD_TABLE[hay*2+1], KEYWORD_TABLE[sub*2+1]);
    }
#endif

    const char *file = NULL;

    if (argc > 1)
        file = argv[1];

    Node *ast_file1 = parse_file(file);
    ast_dump_node(ast_file1, 0);

    char *fname = file_basename(file);
    char *fout_path = m_strdup(getenv("PWD"));
    fout_path = m_strapp(fout_path, "/res.c");
    // fout_path = m_strapp(fout_path, fname);
    // fout_path = m_strapp(fout_path, ".c");
    
    symbols = bth_htab_init(16);
    
    fout = fopen(fout_path, "w");
    bk_c_emit_node(ast_file1);
    fclose(fout);

    return 0;
}
