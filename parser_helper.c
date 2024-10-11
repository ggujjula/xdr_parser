#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "parser_helper.h"
#include "symtab.h"

void add_enum_items_to_symtab(tok *t) {
   tok *iter = tok_child(tok_child(t));
   while (iter != NULL) {
      tok_dump(iter);
      char *symbol = tok_str(tok_child(iter));
      tok *entry = tok_next(tok_child(iter));
      printf("STR to insert: %s (%p)\n", symbol, symbol);
      symtab_insert_strict(symbol, entry);
      iter = tok_next(iter);
   }
}

