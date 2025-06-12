#ifndef AST_H
#define AST_H

#include "vtype.h"

#include "../include/bth_htab.h"
typedef struct bth_htab HashTable;
typedef struct bth_hpair HashPair;

typedef enum
{
    NK_VAR_DECL,
    NK_FUN_DECL,
    NK_MOD_DECL,
    NK_EXPR_LIT,
    NK_EXPR_IDENT,
    NK_EXPR_ASSIGN,
    NK_EXPR_DEREF,
    NK_EXPR_ADD,
    NK_EXPR_MUL,
    NK_RETURN,
} NodeKind;

struct VarDeclNode
{
    VType *type;
    const char *name;
    struct Node *init;
};

struct ModDeclNode
{
    const char *name;
    struct Node **nodes;
};

struct FunDeclNode
{
    VType *ret;
    const char *name;
    struct VarDeclNode **args;
    struct Node **body;
};

struct IdentNode
{
    VType *type;
    char *name;
};

struct AssignNode
{
    struct Node *dest;
    struct Node *value;
};

struct BinopNode
{
    struct Node *lhs;
    struct Node *rhs;
};

typedef struct Node
{
    NodeKind kind;
    union
    {
        struct VarDeclNode    *vdecl;
        struct FunDeclNode    *fdecl;
        struct ModDeclNode    *mdecl;
        struct Node           *ret;
        struct ValueNode      *lit;
        struct IdentNode      *ident;
        struct AssignNode     *assign;
        struct BinopNode      *binop;
        struct Node           *unop;
    } as;
} Node;

Node *parse_file(const char *filepath);
void ast_dump_node(Node *node, int padd);
struct IdentNode *get_ident(Node *n);

#endif
