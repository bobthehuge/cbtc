// MIT No Attribution
// 
// Copyright (c) 2025 bobthehuge
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to 
// deal in the Software without restriction, including without limitation the 
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or 
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
// DEALINGS IN THE SOFTWARE.

#ifndef BTH_HTAB_H
#define BTH_HTAB_H

#include <stdint.h>
#include <stdlib.h>

struct bth_hpair
{
    uint64_t hkey;
    char *key;
    void *value;
    struct bth_hpair *next;
};

struct bth_htab
{
    size_t cap;
    size_t size;
    struct bth_hpair **data;
};

// one at a time
uint32_t oaat(char *key);
// daniel j bernstein 2
uint32_t djb2(char *key);

#ifndef BTH_HTAB_HASH
#define BTH_HTAB_HASH(key) djb2(key)
#endif

#ifndef BTH_HTAB_STRLEN
#include <string.h>
#define BTH_HTAB_STRLEN(s) strlen(s)
#define BTH_HTAB_STRCMP(s1, s2) strcmp(s1, s2)
#define BTH_HTAB_MEMSET(dst, c, n) memset(dst, c, n)
#endif

#ifndef BTH_HTAB_ERRX
#include <err.h>
#define BTH_HTAB_ERRX(c, msg, ...) errx(c, msg, __VA_ARGS__)
#endif

#ifndef BTH_HTAB_ALLOC
#define BTH_HTAB_ALLOC(n) malloc(n)
#define BTH_HTAB_FREE(p) free(p)
#endif

struct bth_htab *bth_htab_init(size_t cap);
void bth_htab_resize(struct bth_htab **ht, size_t nc);
void bth_htab_clear(struct bth_htab *ht);
void bth_htab_destroy(struct bth_htab *ht);
struct bth_hpair *bth_htab_get(struct bth_htab *ht, char *key);
void bth_htab_put(struct bth_htab *ht, struct bth_hpair *p);
int bth_htab_insert(
    struct bth_htab *ht, struct bth_hpair *p, struct bth_hpair **dst);
int bth_htab_add(
    struct bth_htab *ht, char *key, void *value, struct bth_hpair **dst);
struct bth_hpair *bth_htab_remove(struct bth_htab *ht, char *key);

#endif

#ifdef BTH_HTAB_IMPLEMENTATION

uint32_t oaat(char *key)
{
    size_t i = 0;
    uint32_t hash = 0;
    size_t len = BTH_HTAB_STRLEN(key);

    while (i != len)
    {
        hash += key[i++];
        hash += hash << 10;
        hash ^= hash >> 6;
    }

    hash += hash << 3;
    hash ^= hash >> 11;
    hash += hash << 15;

    return hash;
}

uint32_t djb2(char *key)
{
    uint32_t hash = 5381;
    int c;

    while ((c = *key++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

struct bth_htab *bth_htab_init(size_t cap)
{
    struct bth_htab *res = BTH_HTAB_ALLOC(sizeof(struct bth_htab));

    if (!res)
        BTH_HTAB_ERRX(1, "Not enough memory for htab%s", "\n");

    res->cap = cap;
    res->size = 0;

    res->data = BTH_HTAB_ALLOC(sizeof(struct bth_hpair *) * cap);

    if (!res->data && cap)
        BTH_HTAB_ERRX(1, "Not enough memory for %zu elements", cap);

    BTH_HTAB_MEMSET(res->data, 0, sizeof(struct bth_hpair *) * cap);

    return res;
}

void bth_htab_resize(struct bth_htab **ht, size_t nc)
{
    struct bth_htab *dst = bth_htab_init(nc);

    for (size_t i = 0; i < (*ht)->cap; ++i)
    {
        struct bth_hpair *elt = (*ht)->data[i];

        while (elt)
        {
            struct bth_hpair *nxt = elt->next;
            size_t idx = elt->hkey % nc;
            elt->next = dst->data[idx];
            dst->data[idx] = elt;
            elt = nxt;
        }
    }

    dst->size = (*ht)->size;
    BTH_HTAB_FREE((*ht)->data);
    BTH_HTAB_FREE(*ht);
    *ht = dst;
}

void bth_htab_clear(struct bth_htab *ht)
{
    for (size_t i = 0; i < ht->cap; ++i)
    {
        struct bth_hpair *elt = ht->data[i];

        while (elt)
        {
            struct bth_hpair *nxt = elt->next;
            BTH_HTAB_FREE(elt);
            elt = nxt;
        }
    }

    BTH_HTAB_MEMSET(ht->data, 0, sizeof(struct bth_hpair *) * ht->cap);
    ht->size = 0;
}

void bth_htab_destroy(struct bth_htab *ht)
{
    bth_htab_clear(ht);
    BTH_HTAB_FREE(ht->data);
}

struct bth_hpair *bth_htab_get(struct bth_htab *ht, char *key)
{
    uint32_t h = BTH_HTAB_HASH(key);
    size_t idx = h % ht->cap;

    struct bth_hpair *elt = ht->data[idx];

    while (elt != NULL && BTH_HTAB_STRCMP(key, elt->key))
        elt = elt->next;

    return elt;
}

// force insert
void bth_htab_put(struct bth_htab *ht, struct bth_hpair *p)
{
    size_t idx = p->hkey % ht->cap;
    p->next = ht->data[idx];
    ht->data[idx] = p;
    ht->size++;
}

int bth_htab_insert(
    struct bth_htab *ht, struct bth_hpair *p, struct bth_hpair **dst)
{
    
    struct bth_hpair *old = bth_htab_get(ht, p->key);
    
    if (dst && old)
    {
        *dst = old;
        return 0;
    }

    bth_htab_put(ht, p);
    return 1;
}

int bth_htab_add(
    struct bth_htab *ht, char *key, void *value, struct bth_hpair **dst)
{
    uint64_t h = BTH_HTAB_HASH(key);
    size_t idx = h % ht->cap;

    struct bth_hpair *p = bth_htab_get(ht, key);
    
    if (dst)
    {
        *dst = p;
        return 0;
    }

    p = BTH_HTAB_ALLOC(sizeof(struct bth_hpair));

    if (!p)
        return 0;

    if (dst)
        *dst = p;

    p->hkey = h;
    p->key = key;
    p->value = value;
    p->next = ht->data[idx];
    ht->data[idx] = p;

    ht->size++;
    return 1;
}

struct bth_hpair *bth_htab_remove(struct bth_htab *ht, char *key)
{
    uint64_t h = BTH_HTAB_HASH(key);
    size_t idx = h % ht->cap;

    struct bth_hpair *elt = ht->data[idx];
    struct bth_hpair *prev = NULL;

    while (elt && BTH_HTAB_STRCMP(key, elt->key))
    {
        prev = elt;
        elt = elt->next;
    }
    
    if (elt == NULL)
        return NULL;

    ht->size--;

    if (prev == NULL)
        ht->data[idx] = elt->next;
    else
        prev->next = elt->next;

    return elt;
}
#endif
