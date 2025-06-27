#ifndef VTYPE_H
#define VTYPE_H

typedef enum
{
    UNRESOLVED,
    VT_INT,
    VT_CHAR,
} VType;

typedef struct
{
    unsigned int refc;
    VType base;
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

typedef struct
{
    const char *name;
} Trait;

typedef struct
{
    Type type;
    Trait **traits;
} TypeInfo;

#endif
