// #include <assert.h>
#include <error.h>
#include <stdio.h>

#include "../include/ast.h"
#include "../include/c_backend.h"
#include "../include/context.h"
#include "../include/utils.h"

#define BTH_LEX_IMPLEMENTATION
#include "../include/bth_lex.h"

#define BTH_OPTION_IMPLEMENTATION
#include "../include/bth_option.h"
// OPTION_TYPEDEF(size_t);

#define BTH_IO_IMPLEMENTATION
#include "../include/bth_io.h"

#define BTH_ALLOC_IMPLEMENTATION
#include "../include/bth_alloc.h"

#define BTH_HTAB_IMPLEMENTATION
#include "../include/bth_htab.h"

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

    g_print_token = false;

    const char *fin_path = NULL;
    char *fout_path = NULL;

    bool dump_ast = false;
    bool dump_irep = false;
    bool dump_syms = false;

    argv++;
    argc--;

    while (argc > 0)
    {
        if (!fout_path && !strcmp(*argv, "-o"))
        {
            argc -= 2;
            argv++;
            fout_path = *argv++;
        }
        else if (!g_print_token && !strcmp(*argv, "--tok"))
        {
            argc--;
            argv++;
            g_print_token = true;
        }
        else if (!dump_ast && !strcmp(*argv, "--ast"))
        {
            argc--;
            argv++;
            dump_ast = true;
        }
        else if (!dump_irep && !strcmp(*argv, "--irep"))
        {
            argc--;
            argv++;
            dump_irep = true;
        }
        else if (!dump_syms && !strcmp(*argv, "--syms"))
        {
            argc--;
            argv++;
            dump_syms = true;
        }
        else if (!fin_path)
        {
            argc--;
            fin_path = *argv++;
        }
        else
            errx(1, "Invalid CMD argument %s", *argv);
    }

    if (dump_irep && dump_ast)
        errx(1, "Can't dump ast and ir at the same time");

    Node *ast_file1 = parse_file(fin_path);

    // char *t = m_strdup("foo");
    // t = m_strapp_n(t, "bar", "baz");
    // printf("%s : %zu\n", t, strlen(t));

    // char *t1 = m_strchg(t, "bar", "ewe");
    // printf("%s : %zu\n", t1, strlen(t1));

    // char *t2 = m_strchg(t, "bar", "bite");
    // printf("%s : %zu\n", t2, strlen(t2));

    // char *t3 = m_strchg(t2, "bite", "bar");
    // printf("%s : %zu\n", t3, strlen(t3));
    
    // free(t);
    // free(t1);
    // free(t2);
    // free(t3);

    if (dump_ast)
    {
        ast_dump(ast_file1);
        exit(0);
    }
    
    ast_desug(ast_file1);
    
    if (dump_syms)
    {
        printf("---- SYMBOLS ----\n");
        dump_symbols();
        printf("----   END   ----\n");
    }

    if (dump_irep)
    {
        ast_dump(ast_file1);
        exit(0);
    }

    if (!fout_path)
    {
        // char *fname = file_basename(fin_path);
        fout_path = m_strdup(getenv("PWD"));
        fout_path = m_strapp(fout_path, "/res.c");
    }
    
    bk_c_emit(ast_file1, fout_path);
    
    return 0;
}
