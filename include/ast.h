#ifndef AST_H
#define AST_H

#include "bth_htab.h"
#include "token.h"

typedef enum
{
    UNRESOLVED,
    VT_ANY,
    VT_CHAR,
    VT_INT,
    VT_SELF,
    VT_CUSTOM,
} VType;

typedef struct
{
    uint refc;
    uint poly:1; // polymophic: scope is local
    uint id:31;
} Type;

struct ValueNode
{
    Type *type;
    union
    {
        int   vt_int;
        char  vt_char;
        char *vt_str;
    } as;
};

typedef struct
{
    Type repr;
    HashTable *traits;
} TypeInfo;

typedef enum
{
    TCMP_RES = -4, // cmp with UNRESOLVED or undefined
    TCMP_EXC = -3, // total exclusion
    TCMP_PIN = -2, // partial inclusion
    TCMP_NEQ = -1, // strict inequality
    TCMP_LCA =  0, // can cast target into sample
    TCMP_RCA =  1, // can cast sample into target
    TCMP_INC =  2, // total inclusion
    TCMP_PEQ =  3, // polymorphic equality
    TCMP_EQS =  4, // strict equality
} TypeCmpError;

// typedef enum
// {
    // NK_VAR_DECL,
    // NK_FUN_DECL,
    // NK_MOD_DECL,
    // NK_TRAIT_DECL,
    // NK_IMPL_DECL,
    // NK_EXPR_LIT,
    // NK_EXPR_IDENT,
    // NK_EXPR_ASSIGN,
    // NK_EXPR_DEREF,
    // NK_EXPR_REF,
    // NK_EXPR_ADD,
    // NK_EXPR_MUL,
    // NK_EXPR_FUNCALL,
    // NK_RETURN,
// } NodeKind;

#define FOREACH_NODEKIND(FUNC) \
    FUNC(NK_VAR_DECL)      \
    FUNC(NK_FUN_DECL)      \
    FUNC(NK_MOD_DECL)      \
    FUNC(NK_TRAIT_DECL)    \
    FUNC(NK_IMPL_DECL)     \
    FUNC(NK_EXPR_LIT)      \
    FUNC(NK_EXPR_IDENT)    \
    FUNC(NK_EXPR_ASSIGN)   \
    FUNC(NK_EXPR_DEREF)    \
    FUNC(NK_EXPR_REF)      \
    FUNC(NK_EXPR_ADD)      \
    FUNC(NK_EXPR_MUL)      \
    FUNC(NK_EXPR_FUNCALL)  \
    FUNC(NK_RETURN)        \

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

typedef enum
{
    FOREACH_NODEKIND(GENERATE_ENUM)
} NodeKind;

extern const char *NODEKIND_STRING[];

#define NKSTR(name) (NODEKIND_STRING[name])

// typedef enum
// {
//     NS_UNUSED,
//     NS_USED_ONCE,
//     NS_USED_MULT,
//     NS_UNREACHABLE,
//     NS_REACHABLE,
// } NodeState;

#define REACHABLE      1 << 0
#define READ           1 << 1
#define WRITE          1 << 2
#define CALL           1 << 3

#define SET_STATES(st, x) (*(uint8_t *)(&st->states) = x)

typedef struct
{
    uint8_t reachable:1;
    uint8_t read:1;
    uint8_t write:1;
    uint8_t call:1;
} NodeStates;

struct VarDeclNode
{
    Type *type;
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
    uint argc;
    Type *ret;
    HashTable *types;
    const char *name;
    struct Node **args;
    struct Node **body;
};

struct IdentNode
{
    Type *type;
    const char *name;
};

struct FunCallNode
{
    uint argc;
    // Type *type;
    const char *name;
    // struct Node *funcref;
    struct Node **args;
};

struct AssignNode
{
    struct Node *dest;
    struct Node *value;
};

struct BinopNode
{
    Type *type;
    struct Node *lhs;
    struct Node *rhs;
};

struct UnopNode
{
    Type *type;
    struct Node *value;
};

struct ImplDeclNode
{
    const char *trait;
    HashTable *types;
    HashTable *funcs;
};

struct TraitDeclNode
{
    const char *name;
    HashTable *types;
    HashTable *funcs;
};

typedef struct Node
{
    NodeKind kind;

    const char *afile;
    size_t row;
    size_t col;
    NodeStates states;

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
        struct TraitDeclNode  *tdecl;
        struct ImplDeclNode   *impl;
    } as;
} Node;

Node *parse_file(const char *filepath);
Node *new_node(NodeKind k, Token *semholder);
void ast_dump(Node *node);
void ast_desug(Node *node);
struct IdentNode *get_ident(Node *n);

#endif
