#include "lexer.h"

int main (void) {
   lexer_talloc_ctxt = talloc_init("lexer_talloc_ctxt");
   YYSTYPE yylval = 0;
   int ret = yylex(&yylval);
   while(ret > 0) {
      talloc_unlink(lexer_talloc_ctxt, yylval);
      talloc_unlink(lexer_talloc_ctxt, yylval);
      ret = yylex(&yylval);
   }
   talloc_unlink(lexer_talloc_ctxt, yylval);
   talloc_unlink(lexer_talloc_ctxt, yylval);
   talloc_free(lexer_talloc_ctxt);
   return 0;
}
