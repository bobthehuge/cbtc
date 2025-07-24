#ifndef VTYPE_H
#define VTYPE_H

#include "bth_htab.h"
#include "ast.h"

extern HashTable *trait_table;
extern HashTable *type_table;

const char *base2str(Type *t);
char *type2str(Type *t);
Type *typeget(Node *n);
int typecmp(Type *t1, Type *t2);

Type *empty_type(void);
Type *self_type(void);

TypeInfo *empty_typeinfo(void);

HashData *get_type(Type *t);
TypeInfo *get_type_info(Type *t);
TypeInfo *get_id_type_info(uint id);

HashData *get_trait(const char *name);
TraitInfo *get_trait_info(const char *name);

char *impl2str(struct ImplDeclNode *im);

void define_trait(const char *name, TraitInfo *tr);
void reset_traits(void);
Node *create_impl_node(const char *name);
void impl_trait(Node *node);

void define_type(const char *name, TypeInfo *ti);
void reset_types(void);

void define_static_traits(void);
void define_static_types(void);

#endif
