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

// tested on ASOIAF, should be robust by now

// memory layout example:
// 
// table.data = &{
//     [0] = RESERVED
//     [1] = &{ data 1 }
//     [2] = NULL
//     [3] = &{ data0 }
//     [4] = &{ data2 }
//     .
//     .
//     .
//     [table.size - 2] = &{ data3 }
//     [table.size - 1] = NULL
// }
//
// table.map = {
//     [0]: { 2, &[3, 0] }
//     [1]: { 4, &[2, 1, 0, 0] }
//     .
//     .
//     .
//     [table.cap - 1] : { 2, &[4, 0] }
// }

#ifndef BTH_HTAB_H
#define BTH_HTAB_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

struct bth_hdata
{
    uint64_t hash;
    const char *key;
    void *value;
};

struct bth_hbuck
{
    uint64_t cap;
    size_t *idx;
};

struct bth_htab
{
    bool noresize; // do not resize data array when full
    size_t cap; // map rows
    size_t size; // data len
    size_t nb; // additionnal elements to realloc when resizing bucket indices
    size_t nd; // additionnal elements to realloc when resizing data
    struct bth_hdata **data;
    struct bth_hbuck *map;
};

// one at a time
uint32_t oaat(const char *key);
// daniel j bernstein 2
uint32_t djb2(const char *key);

#ifndef BTH_HTAB_HASH
#define BTH_HTAB_HASH(key) djb2(key)
#endif

#ifndef BTH_HTAB_DATACMP
#define BTH_HTAB_DATACMP(d1, d2) \
    ((d1)->hash != (d2)->hash || strcmp((d1)->key, (d2)->key))
#endif

#ifndef BTH_HTAB_KEYDUP
#define BTH_HTAB_KEYDUP(k) strdup(k)
#endif

#ifndef BTH_HTAB_STRLEN
#include <string.h>
#define BTH_HTAB_STRLEN(s) strlen(s)
#define BTH_HTAB_MEMSET(dst, c, n) memset(dst, c, n)
#define BTH_HTAB_MEMMOVE(dst, src, n) memmove(dst, src, n)
#define BTH_HTAB_MEMCPY(dst, src, n) memcpy(dst, src, n)
#endif

#ifndef BTH_HTAB_ERRX
#include <err.h>
#define BTH_HTAB_ERRX(c, msg, ...) errx(c, msg, __VA_ARGS__)
#endif

#ifndef BTH_HTAB_ALLOC
#define BTH_HTAB_ALLOC(n) malloc(n)
#define BTH_HTAB_CALLOC(nmemb, size) calloc(nmemb, size)
#define BTH_HTAB_REALLOC(ptr, size) realloc(ptr, size)
#define BTH_HTAB_FREE(p) free(p)
#endif

#ifndef BTH_HTAB_NOTYPEDEFS
typedef struct bth_htab HashTable;
typedef struct bth_hdata HashData;
#endif

struct bth_htab *bth_htab_new(size_t cap, size_t nd, size_t nb);

size_t bth_htab_add(struct bth_htab *ht, const char *k, void *val);
void *bth_htab_delete(struct bth_htab *ht, const char *key);

// resize bucket list
void bth_htab_recap(struct bth_htab *ht, size_t newcap);
// resize data array
void bth_htab_resize(struct bth_htab *ht, size_t s);
// reput hdata at idx inside the map (idx MUST be valid)
void bth_htab_reput(struct bth_htab *ht, size_t idx);

void bth_htab_reset(struct bth_htab *ht);
void bth_htab_destroy(struct bth_htab *ht);

struct bth_hdata *bth_htab_get(struct bth_htab *ht, const char *key);
void *bth_htab_vget(struct bth_htab *ht, const char *key);

// return a pointer to data-idx of (key, hash)
size_t *bth_htab_get_idxp(struct bth_htab *ht, const char *key, uint64_t hash);

size_t bth_htab__dputd(struct bth_htab *ht, struct bth_hdata *hd);

#endif

#ifdef BTH_HTAB_IMPLEMENTATION

#include <assert.h>
#include <errno.h>

uint32_t oaat(const char *key)
{
    size_t i = 0;
    uint32_t hash = 0;
    size_t len = BTH_HTAB_STRLEN(key);

    while (i < len)
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

uint32_t djb2(const char *key)
{
    uint32_t hash = 5381;
    int c;

    while ((c = *key++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

struct bth_htab *bth_htab_new(size_t cap, size_t nd, size_t nb)
{
    struct bth_htab *ht = BTH_HTAB_ALLOC(sizeof(struct bth_htab));

    ht->cap = cap;
    ht->size = nd;
    ht->nd = nd;

    if (!nd)
        nd++;

    ht->nb = nb;
    ht->map = BTH_HTAB_ALLOC(cap * sizeof(struct bth_hbuck));

    for (size_t i = 0; i < cap; i++)
    {
        ht->map[i].idx = BTH_HTAB_CALLOC(nb, sizeof(size_t));
        ht->map[i].cap = nb;
    }

    ht->data = BTH_HTAB_CALLOC(nd, sizeof(struct bth_hdata *));
    // BTH_HTAB_MEMSET(ht->data, 0, nd * sizeof(struct bth_hdata *));
    ht->data[0] = (void *)0xdeadcafebeefbabe;

    return ht;
}

size_t bth_htab_add(struct bth_htab *ht, const char *k, void *val)
{
    errno = 0;
    uint64_t hash = BTH_HTAB_HASH(k);

    size_t *idxp = bth_htab_get_idxp(ht, k, hash);

    if (errno != ENOENT)
        return *idxp;

    char *key = BTH_HTAB_KEYDUP(k);
    struct bth_hdata *hd = BTH_HTAB_ALLOC(sizeof(struct bth_hdata));

    if (errno == ENOMEM)
        return 0;

    hd->hash = hash;
    hd->key = key;
    hd->value = val;

    size_t idx = bth_htab__dputd(ht, hd);

    if (errno == ENOMEM)
    {
        BTH_HTAB_FREE(key);
        BTH_HTAB_FREE(hd);
        return 0;
    }
    
    bth_htab_reput(ht, idx);

    errno = ENOENT;
    return idx;
}

void *bth_htab_delete(struct bth_htab *ht, const char *key)
{
    size_t *idxp = bth_htab_get_idxp(ht, key, BTH_HTAB_HASH(key));

    if (errno == ENOENT)
        return NULL;

    struct bth_hdata *d = ht->data[*idxp];

    ht->data[*idxp] = NULL;

    struct bth_hbuck *hb = ht->map + d->hash % ht->cap;
    size_t *last = hb->idx + hb->cap - 1;
    
    if (idxp < last)
        BTH_HTAB_MEMMOVE(idxp, idxp + 1, (last - idxp) * sizeof(size_t));

    if (*last)
        *last = 0;

    void *res = d->value;

    BTH_HTAB_FREE((char *)d->key);
    BTH_HTAB_FREE(d);

    return res;
}

void bth_htab_recap(struct bth_htab *ht, size_t newcap)
{
    for (size_t i = 0; i < ht->cap; i++)
        BTH_HTAB_FREE(ht->map[i].idx);

    ht->map = BTH_HTAB_REALLOC(ht->map, newcap * sizeof(struct bth_hbuck));
    ht->cap = newcap;

    for (size_t i = 0; i < ht->cap; i++)
    {
        ht->map[i].idx = BTH_HTAB_CALLOC(ht->nb, sizeof(size_t));
        ht->map[i].cap = ht->nb;
    }

    for (size_t i = 1; i < ht->size; i++)
    {
        if (!ht->data[i])
            continue;

        bth_htab_reput(ht, i);
    }
}

void bth_htab_resize(struct bth_htab *ht, size_t s)
{
    ht->data = BTH_HTAB_REALLOC(ht->data, s * sizeof(struct bth_hdata *));

    if (errno == ENOMEM)
        return;

    size_t old = ht->size;
    ht->size = s;

    if (s > old)
        BTH_HTAB_MEMSET(ht->data + old, 0, (s - old - 1)
            * sizeof(struct bth_hdata *));
}

void bth_htab_reput(struct bth_htab *ht, size_t idx)
{
    struct bth_hdata *hd = ht->data[idx];

    struct bth_hbuck *hb = ht->map + (hd->hash % ht->cap);
    size_t *last = hb->idx + hb->cap - 1;

    if (*last) // resize bucket idx
    {
        size_t inc = ht->nb ? ht->nb : hb->cap;
        
        hb->idx =
            BTH_HTAB_REALLOC(hb->idx, (hb->cap + inc) * sizeof(size_t));

        if (errno == ENOMEM)
            return;

        last = hb->idx + hb->cap;
        BTH_HTAB_MEMSET(last, 0, inc * sizeof(size_t));
        hb->cap += inc;
    }
    else // find last zero index
    {
        while (last > hb->idx && *(last - 1) == 0)
            last--;
    }

    *last = idx;
}

void bth_htab_reset(struct bth_htab *ht)
{
    size_t newds = ht->nd ? ht->nd : 1;
    size_t newbs = ht->nb ? ht->nb : 0;

    for (size_t i = 0; i < ht->cap; i++)
    {
        struct bth_hbuck *hb = ht->map + i;
        hb->idx = BTH_HTAB_REALLOC(hb->idx, newbs);
        hb->cap = newbs;
    }

    ht->data = BTH_HTAB_REALLOC(ht->data, newds);
    BTH_HTAB_MEMSET(ht->data + 1, 0, (newds - 1) * sizeof(struct bth_hdata *));
}

void bth_htab_destroy(struct bth_htab *ht)
{
    for (size_t i = 0; i < ht->cap; i++)
        BTH_HTAB_FREE(ht->map[i].idx);

    for (size_t i = 1; i < ht->size; i++)
    {
        if (!ht->data[i])
            continue;
        BTH_HTAB_FREE(ht->data[i]);
    }

    BTH_HTAB_FREE(ht->map);
    BTH_HTAB_FREE(ht->data);
    BTH_HTAB_FREE(ht);
}

struct bth_hdata *bth_htab_get(struct bth_htab *ht, const char *key)
{
    size_t *idxp = bth_htab_get_idxp(ht, key, BTH_HTAB_HASH(key));

    if (!idxp)
        return NULL;
    
    return ht->data[*idxp];
}

void *bth_htab_vget(struct bth_htab *ht, const char *key)
{
    size_t *idxp = bth_htab_get_idxp(ht, key, BTH_HTAB_HASH(key));

    if (!idxp)
        return NULL;

    return ht->data[*idxp]->value;
}

size_t *bth_htab_get_idxp(struct bth_htab *ht, const char *key, uint64_t hash)
{
    struct bth_hbuck *hb = ht->map + hash % ht->cap;
    struct bth_hdata hd1 = {.hash = hash, .key = key};

    size_t *midx = hb->idx;
    errno = 0;
    
    for (size_t i = 0; i < hb->cap; i++)
    {
        if (!hb->idx[i])
            break;
        
        struct bth_hdata *hd2 = ht->data[*midx];

        if (!BTH_HTAB_DATACMP(hd2, &hd1))
            return midx;

        midx++;
    }

    errno = ENOENT;
    return NULL;
}

size_t bth_htab__dputd(struct bth_htab *ht, struct bth_hdata *hd)
{
    struct bth_hdata **data = ht->data + 1;

    while (data - ht->data < ht->size && *data)
        data++;

    if (data - ht->data >= ht->size)
    {
        if (ht->noresize)
        {
            errno = ENOMEM;
            return 0;
        }

        size_t inc = ht->nd ? ht->nd : ht->size;
        bth_htab_resize(ht, ht->size + inc);
        data = ht->data + ht->size - inc;
    }

    *data = hd;
    return data - ht->data;
}

#endif
