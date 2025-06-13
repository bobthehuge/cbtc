#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>
#include "vtype.h"

// lmao
#define HERE() printf("here\n")

char *m_strndup(const char *str, size_t n);
char *m_strdup(const char *str);
char *m_strapp(char *dst, const char *src);
char *m_strcat(const char *s1, const char *s2);
char *stresc(const char *str);
char *file_basename(const char *path);
int is_valid_int(const char *s, long *res, int base);

char *type2str(VType *bt);
// detecting UNRESOLVED always returns false
int typecmp(VType *t1, VType *t2);

#endif
