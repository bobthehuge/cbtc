#ifndef VTYPE_H
#define VTYPE_H

#include "bth_htab.h"

typedef enum
{
    UNRESOLVED,
    VT_CHAR,
    VT_INT,
    VT_ANY,
    VT_CUSTOM, // need research in a type table
} VType;

typedef struct
{
    uint refc;
    uint poly:1; // polymophic: scope is local
    uint id:31;
} Type;

struct ValueNode
{
    Type type;
    union
    {
        int   vt_int;
        char  vt_char;
        char *vt_str;
    } as;
};

// typedef struct
// {
//     const char *name;
//     HashTable *types;
//     HashTable *impls;
// } TraitInfo;

typedef struct
{
    Type repr;
    HashTable *traits;
} TypeInfo;

#endif
