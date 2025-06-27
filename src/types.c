#include "../include/types.h"
#include "../include/utils.h"

static HashTable *trait_table = NULL;

void reset_trait_table(void)
{
    if (trait_table)
        bth_htab_destroy(trait_table);
    trait_table = bth_htab_init(16);
}

int does_impl(Type *ty, const char *name)
{
    uint64_t th = typehash(ty);

    HashPair *root = trait_table->data[th % trait_table->cap];
    TypeInfo *ti = root->value;

    while (root)
    {
        ti = root->value;

        if (!typecmp(&ti->type, ty))
            break;

        root = root->next;
    }

    if (!root)
        return 0;

    ti = root->value;
    Trait **tr = ti->traits;

    while (*tr)
    {
        if (!strcmp((*(tr)++)->name, name))
            return 1;
    }

    return 0;
}
