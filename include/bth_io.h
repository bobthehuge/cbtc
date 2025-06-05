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

#ifndef BTH_IO_H
#define BTH_IO_H

#include <stdio.h>
#include <stdlib.h>

#ifndef BTH_IO_ALLOC
#define BTH_IO_ALLOC(t) malloc(t)
#endif

#ifndef OPTION
#define OPTION(t) t
#endif

OPTION(size_t) readfn(char **buf, size_t n, const char *path);

#ifdef BTH_IO_IMPLEMENTATION
OPTION(size_t) readfn(char **buf, size_t n, const char *path)
{
    FILE *f = fopen(path, "r");

    if (!f)
        return NONE(size_t);

    size_t len = n;
    if (!n)
    {
        fseek(f, 0, SEEK_END);
        len = ftell(f);
        fseek(f, 0, SEEK_SET);
    }

    char *d = (char *)BTH_IO_ALLOC(len + 1);
    size_t count = fread(d, 1, len, f);
    fclose(f);
    
    d[count] = 0;
    *buf = d;

    return SOME(size_t, count);
}
#endif
#endif
