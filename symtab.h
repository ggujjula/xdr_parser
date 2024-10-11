#ifndef _SYMTAB_H
#define _SYMTAB_H

#include "token.h"

void symtab_init(void);
void symtab_insert(char *symbol, tok *t);
void symtab_insert_strict(char *symbol, tok *t);
tok *symtab_lookup(char *symbol);

#endif
