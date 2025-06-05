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

#ifndef BTH_OPTION_H
#define BTH_OPTION_H

#include <stdbool.h>

#define OPTION_TYPEDEF(t) typedef struct{bool none; t some;} Option_##t
#define OPTION(t) Option_##t
#define IS_NONE(o) ((o).none)
#define IS_SOME(o) (!(o).none)
#define NONE(c) ((Option_##c){.none=true})
#define SOME(c, v) ((Option_##c){.none=false,.some=(c)(v)})

#define CHECK(v) \
    __bth_assertf(!(v).none,"%s:%d:None Caught",__FILE__,__LINE__)

void __bth_assertf(int d, char *fmt, ...);

#endif

#ifdef BTH_OPTION_IMPLEMENTATION

#ifndef __BTH_ASSERTF
#define __BTH_ASSERTF
#include <stdarg.h>
#include <err.h>
void __bth_assertf(int d, char *fmt, ...)
{
    if (!d)
    {
        va_list args;
        va_start(args, fmt);
        verrx(1, fmt, args);
        va_end(args);
    }
}
#endif

#endif
