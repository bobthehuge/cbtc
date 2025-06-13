#ifndef C_BACKEND_H
#define C_BACKEND_H

#include "../include/ast.h"

void bk_c_emit(Node *ast, const char *fout_path);

#endif
