#ifndef VTYPE_H
#define VTYPE_H

typedef enum
{
    UNRESOLVED,
    VT_INT,
} VType;

typedef struct
{
    unsigned int refc;
    VType base;
} TypeInfo;

struct ValueNode
{
    TypeInfo type;
    union
    {
        int vt_int;
    } as;
};

#endif
