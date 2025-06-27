#ifndef AST_H
#define AST_H

#include "token.h"
#include "types.h"

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
    NK_EXPR_REF,
    NK_EXPR_ADD,
    NK_EXPR_MUL,
    NK_EXPR_FUNCALL,
    NK_RETURN,
} NodeKind;

struct VarDeclNode
{
    Type type;
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
    Type ret;
    uint argc;
    const char *name;
    struct Node **args;
    struct Node **body;
};

struct IdentNode
{
    Type type;
    char *name;
};

struct FunCallNode
{
    Type type;
    uint argc;
    const char *name;
    struct Node **args;
};

struct AssignNode
{
    struct Node *dest;
    struct Node *value;
};

struct BinopNode
{
    Type type;
    struct Node *lhs;
    struct Node *rhs;
};

struct UnopNode
{
    Type type;
    struct Node *value;
};

typedef struct Node
{
    NodeKind kind;

    const char *afile;
    size_t row;
    size_t col;

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
        struct UnopNode       *unop;
        struct FunCallNode    *fcall;
    } as;
} Node;

Node *parse_file(const char *filepath);
Node *new_node(NodeKind k, Token *semholder);
void ast_dump(Node *node);
void ast_desug(Node *node);
struct IdentNode *get_ident(Node *n);

#endif
