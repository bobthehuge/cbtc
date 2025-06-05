#ifndef VTYPE_H
#define VTYPE_H

typedef enum
{
    UNRESOLVED,
    VT_INT,
} VType;

struct ValueNode
{
    VType *type;
    union
    {
        int vt_int;
    } as;
};

#endif
