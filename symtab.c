#include "symtab.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "external/include/talloc.h"
#include "external/include/uthash.h"

typedef struct sym_entry {
        char *symbol;
        tok *t;
        UT_hash_handle hh;
} sym_entry;

static sym_entry *symtab = NULL;

void symtab_init(void)
{
   symtab = NULL;
}

void symtab_insert(char *symbol, tok *t)
{
   assert(t != NULL);
   assert(tok_next(t) == NULL);
   sym_entry *entry = talloc(NULL, sym_entry);
   entry->symbol = talloc_strdup(entry, symbol);
   entry->t = t;
   HASH_ADD_KEYPTR(hh, symtab, entry->symbol, strlen(entry->symbol), entry);
}

tok *symtab_lookup(char *symbol)
{
        printf("Lookup for %p (%s)\n", symbol, symbol);
        sym_entry *entry;
        HASH_FIND(hh, symtab, symbol, strlen(symbol), entry);
        return entry == NULL ? NULL : entry->t;
}

void symtab_insert_strict(char *symbol, tok *t)
{
        tok *symentry = symtab_lookup(symbol);
        if (symentry != NULL) {
                printf("Symbol %s previously declared\n", symbol);
                tok_dump(symentry);
                exit(1);
        }
        symtab_insert(symbol, t);
}
