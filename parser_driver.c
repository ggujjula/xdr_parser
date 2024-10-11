#include "parser.h"
#include "lexer.h"
#include "codegen.h"
#include "token.h"
#include "symtab.h"
#include "dot_filter.h"
#include <stdio.h>

tok *result_tree;

void yyerror(char const *s) {
   printf("%s\n", s);
}

int main (void) {
   tok *filtered, *filtered2;
   yydebug=1;
   talloc_set_log_stderr();
   talloc_enable_null_tracking();
   //talloc_enable_leak_report();
   lexer_talloc_ctxt = talloc_init("lexer_talloc_ctxt");
   parser_talloc_ctxt = talloc_init("parser_talloc_ctxt");
   token_init();
   symtab_init();

   //TODO: Replace this with an input prelude.
   tok *false_ident = make_basic(parser_talloc_ctxt, IDENT, "FALSE");
   tok *false_value_string = make_basic(parser_talloc_ctxt, OCTAL, "0");
   tok *true_ident = make_basic(parser_talloc_ctxt, IDENT, "TRUE");
   tok *true_value_string = make_basic(parser_talloc_ctxt, OCTAL, "1");
   tok *false_value = make_value(parser_talloc_ctxt, false_value_string);
   tok *true_value = make_value(parser_talloc_ctxt, true_value_string);
   tok *false_enum_item = make_enum_item(parser_talloc_ctxt, false_ident, false_value);
   tok *true_enum_item = make_enum_item(parser_talloc_ctxt, true_ident, true_value);
   tok *tlb = tlb_new(parser_talloc_ctxt);
   tlb_append(parser_talloc_ctxt, tlb, false_enum_item);
   tlb_append(parser_talloc_ctxt, tlb, true_enum_item);
   tok *item_list = tlb_finish(parser_talloc_ctxt, tlb);
   tok *bool_tok = make_enum(parser_talloc_ctxt, item_list);
   symtab_insert_strict("bool", bool_tok);

   yyparse();
   //talloc_free(lexer_talloc_ctxt);
   fprintf(stderr, "PARSING COMPLETE! :)\n");
   fprintf(stderr, "Result tree is %p\n", result_tree);

   dot_filter_params p = {
      .filename = "testdot.dot"
   };
   walker *dot_walker = walker_init(&dot_filter_callbacks, &p);
   filtered = walker_walk(dot_walker, result_tree, 0);
   //fprintf(stderr, "???\n");
   walker_deinit(dot_walker);

   p = (dot_filter_params){
      .filename = "testdotfiltered.dot"
   };
   dot_walker = walker_init(&dot_filter_callbacks, &p);
   walker_walk(dot_walker, filtered, 0);
   walker_deinit(dot_walker);

   codegen_params p2 = (codegen_params){
      .filename = "generated.rs"
   };
   walker *codegen_walker = walker_init(&codegen_callbacks, &p2);
   filtered2 = walker_walk(codegen_walker, filtered, 0);
   walker_deinit(codegen_walker);

   //talloc_free(result_tree);

   return 0;
}
