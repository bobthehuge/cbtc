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

#ifndef BTH_ALLOC_H 
#define BTH_ALLOC_H

#include <stdlib.h>

#ifndef BTH_ALLOC_ERR
#include <err.h>
#define BTH_ALLOC_ERR(c, msg, ...) err(c, msg, __VA_ARGS__)
#endif

#ifdef BTH_BALLOC

#define BALLOC_LOCKED (1 << 0)
#define BALLOC_STATIC (1 << 1)

struct bth_arena
{
    size_t cap;
    size_t len;
    void *data;
    char flags;
};

void bth_balloc_resize(struct bth_arena *t);
void bth_balloc_free(struct bth_arena *t);
void *bth_balloc(struct bth_arena *t, size_t s);

#endif

void *smalloc(size_t size);
void *srealloc(void *ptr, size_t size);
void *scalloc(size_t nmemb, size_t size);

#endif

#ifdef BTH_ALLOC_IMPLEMENTATION
#include <err.h>
#include <errno.h>

void *smalloc(size_t size)
{
    void *d = malloc(size);

    if (!d && size)
        BTH_ALLOC_ERR(1, "Cannot malloc of size %zu", size);

    return d;
}

void *srealloc(void *ptr, size_t size)
{
    void *d = realloc(ptr, size);

    if (!d && size)
        BTH_ALLOC_ERR(1, "Cannot realloc of size %zu", size);

    return d;
}

void *scalloc(size_t nmemb, size_t size)
{
    void *d = calloc(nmemb, size);

    if (!d && size)
        BTH_ALLOC_ERR(1, "Cannot calloc of size %zu", size);

    return d;
}

#ifdef BTH_BALLOC
// resize arena capacity to len
void bth_balloc_resize(struct bth_arena *t)
{
    void *d = srealloc(t->data, t->len);
    t->data = d;
    t->cap = t->len;
}

void bth_balloc_free(struct bth_arena *t)
{
    t->cap = 0;
    t->len = 0;
    free(t->data);
    t->data = NULL;
}

void *bth_balloc(struct bth_arena *t, size_t size)
{
    if (t->cap <= t->len + size)
    {
        errno = ENOMEM;
        BTH_ALLOC_ERR(1, "Cannot balloc of size %zu", size);
        return NULL;
    }

    errno = 0;
    void *d = (char *)t->data + t->len;

    t->len += size;

    return d;
}
#endif

#endif /* ! */
