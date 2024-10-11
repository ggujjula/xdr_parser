/*
 * parser.y:
 * Bison input file to create an XDR frontend.
 */
%code requires {

#include "token.h"

}

%code {

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "lexer.h"
#include "symtab.h"
#include "parser_helper.h"
#include "external/include/talloc.h"

#define UNLINK_ALL_SYMS(n)             \
   assert(n > 0);                      \
   for(int i=-n+1; i <= 0; i++) {    \
      talloc_unlink(lexer_talloc_ctxt, yyvsp[i]);   \
   }

void *parser_talloc_ctxt = NULL;

}

%code provides{
extern void *parser_talloc_ctxt;
}

%{

#include "parser_driver.h"

%}

%define api.value.type {tok*}
%define api.pure full

%token LPAREN RPAREN COMMA COLON SEMICOLON EQ LBRACE RBRACE BOOL CASE CONST
%token DEFAULT DOUBLE ENUM FLOAT HYPER INT OPAQUE QUADRUPLE STRING STRUCT
%token SWITCH TYPEDEF UNION UNSIGNED VOID IDENT DECIMAL HEX OCTAL LT GT STAR
%token LBRACKET RBRACKET PROGRAM VERSION

%start specification

%%
declaration:
     type-specifier IDENT
   {
      $$ = make_decl(parser_talloc_ctxt, $2, $1);
      UNLINK_ALL_SYMS(2);
   }
   | type-specifier IDENT LBRACKET value RBRACKET
   {
      $$ = make_number(parser_talloc_ctxt, TOK_UINT, $4);
      $$ = make_array(parser_talloc_ctxt, $1, $$);
      $$ = make_decl(parser_talloc_ctxt, $2, $$);
      UNLINK_ALL_SYMS(5);
   }
   | type-specifier IDENT LT GT
   {
      $$ = make_dynarray(parser_talloc_ctxt, $1, NULL);
      $$ = make_decl(parser_talloc_ctxt, $2, $$);
      UNLINK_ALL_SYMS(4);
   }
   | type-specifier IDENT LT value GT
   {
      $$ = make_number(parser_talloc_ctxt, TOK_UINT, $4);
      $$ = make_dynarray(parser_talloc_ctxt, $1, $$);
      $$ = make_decl(parser_talloc_ctxt, $2, $$);
      UNLINK_ALL_SYMS(5);
   }
   | OPAQUE IDENT LBRACKET value RBRACKET
   {  $$ = make_number(parser_talloc_ctxt, TOK_UINT, $4);
      $$ = make_opaque(parser_talloc_ctxt, $$);
      $$ = make_decl(parser_talloc_ctxt, $2, $$);
      UNLINK_ALL_SYMS(5);
   }
   | OPAQUE IDENT LT GT
   {
      $$ = make_dynopaque(parser_talloc_ctxt, NULL);
      $$ = make_decl(parser_talloc_ctxt, $2, $$);
      UNLINK_ALL_SYMS(4);
   }
   | OPAQUE IDENT LT value GT
   {
      $$ = make_number(parser_talloc_ctxt, TOK_UINT, $4);
      $$ = make_dynopaque(parser_talloc_ctxt, $$);
      $$ = make_decl(parser_talloc_ctxt, $2, $$);
      UNLINK_ALL_SYMS(5);
   }
   | STRING IDENT LT GT
   {
      $$ = make_string(parser_talloc_ctxt, NULL);
      $$ = make_decl(parser_talloc_ctxt, $2, $$);
      UNLINK_ALL_SYMS(4);
   }
   | STRING IDENT LT value GT
   {
      $$ = make_number(parser_talloc_ctxt, TOK_UINT, $4);
      $$ = make_string(parser_talloc_ctxt, $$);
      $$ = make_decl(parser_talloc_ctxt, $2, $$);
      UNLINK_ALL_SYMS(5);
   }
   | type-specifier STAR IDENT
   {
      $$ = make_optional(parser_talloc_ctxt, $1);
      $$ = make_decl(parser_talloc_ctxt, $3, $$);
      UNLINK_ALL_SYMS(3);
   }
   | VOID
   {
      $$ = tok_get_void();
      tok *ident = make_basic(parser_talloc_ctxt, IDENT, "_void");
      $$ = make_typeref(parser_talloc_ctxt, ident, $$);
      $$ = make_decl(parser_talloc_ctxt, ident, $$);
      UNLINK_ALL_SYMS(1);
   }

value:
   constant
   | IDENT
   {
      $$ = symtab_lookup(tok_str($1));
      if($$ == NULL) {
         YYERROR;
      }
      $$ = make_valref(parser_talloc_ctxt, $1, $$);
      UNLINK_ALL_SYMS(1);
   }

constant:
   DECIMAL
   {
      $$ = make_value(parser_talloc_ctxt, $1);
      UNLINK_ALL_SYMS(1);
   }
   |HEX
   {
      $$ = make_value(parser_talloc_ctxt, $1);
      UNLINK_ALL_SYMS(1);
   }
   |OCTAL
   {
      $$ = make_value(parser_talloc_ctxt, $1);
      UNLINK_ALL_SYMS(1);
   }

type-specifier:
   INT                { $$ = tok_get_int(); $$ = make_typeref(parser_talloc_ctxt, $1, $$); UNLINK_ALL_SYMS(1); }
   | UNSIGNED INT     { $$ = tok_get_uint(); $$ = make_typeref(parser_talloc_ctxt, $1, $$); UNLINK_ALL_SYMS(2); }
   | HYPER            { $$ = tok_get_hyp(); $$ = make_typeref(parser_talloc_ctxt, $1, $$); UNLINK_ALL_SYMS(1); }
   | UNSIGNED HYPER   { $$ = tok_get_uhyp(); $$ = make_typeref(parser_talloc_ctxt, $1, $$); UNLINK_ALL_SYMS(2); }
   | FLOAT            { $$ = tok_get_float(); $$ = make_typeref(parser_talloc_ctxt, $1, $$); UNLINK_ALL_SYMS(1); }
   | DOUBLE           { $$ = tok_get_double(); $$ = make_typeref(parser_talloc_ctxt, $1, $$); UNLINK_ALL_SYMS(1); }
   | QUADRUPLE        { $$ = tok_get_quad(); $$ = make_typeref(parser_talloc_ctxt, $1, $$); UNLINK_ALL_SYMS(1); }
   | BOOL             { $$ = symtab_lookup("bool"); $$ = make_typeref(parser_talloc_ctxt, $1, $$); UNLINK_ALL_SYMS(1); }
   | enum-type-spec
   | struct-type-spec
   | union-type-spec
   | IDENT
   {
      $$ = symtab_lookup(tok_str($1));
      if ($$ == NULL) {
         YYERROR;
      }
      $$ = make_typeref(parser_talloc_ctxt, $1, $$);
      UNLINK_ALL_SYMS(1);
   }

enum-type-spec:
   ENUM enum-body { $$ = $2; talloc_unlink(lexer_talloc_ctxt, $1); }

enum-body:
   LBRACE enum-body-inner RBRACE
   {
      fprintf(stderr, "%lu\n", talloc_reference_count($1));
      talloc_show_parents($1, stderr);
      $$ = tlb_finish(parser_talloc_ctxt, $2);
      $$ = make_enum(parser_talloc_ctxt, $$);
      UNLINK_ALL_SYMS(3);
   }

enum-body-inner:
   IDENT EQ value
   {
      $$ = tlb_new(parser_talloc_ctxt);
      $$ = tlb_append(parser_talloc_ctxt, $$, make_enum_item(parser_talloc_ctxt, $1, $3));
      UNLINK_ALL_SYMS(3);
   }
   | enum-body-inner COMMA IDENT EQ value
   {
      $$ = tlb_append(parser_talloc_ctxt, $$, make_enum_item(parser_talloc_ctxt, $3, $5));
      UNLINK_ALL_SYMS(5);
   }

struct-type-spec:
   STRUCT struct-body    { $$ = $2; talloc_unlink(lexer_talloc_ctxt, $1); }

struct-body:
   LBRACE struct-body-inner RBRACE   { $$ = tlb_finish(parser_talloc_ctxt, $2); $$ = make_struct(parser_talloc_ctxt, $$); UNLINK_ALL_SYMS(3); }

struct-body-inner:
   declaration SEMICOLON                      { $$ = tlb_new(parser_talloc_ctxt); $$ = tlb_append(parser_talloc_ctxt, $$, $1); UNLINK_ALL_SYMS(2); }
   | struct-body-inner declaration SEMICOLON  { $$ = tlb_append(parser_talloc_ctxt, $1, $2); UNLINK_ALL_SYMS(3); }

union-type-spec:
   UNION union-body { $$ = $2; talloc_unlink(lexer_talloc_ctxt, $1); }

union-body:
   SWITCH LPAREN declaration RPAREN LBRACE union-body-inner RBRACE
   {
      $$ = tlb_finish(parser_talloc_ctxt, $6);
      $$ = make_union(parser_talloc_ctxt, $3, $$, NULL);
      UNLINK_ALL_SYMS(7);
   }
   | SWITCH LPAREN declaration RPAREN LBRACE union-body-inner DEFAULT COLON
     declaration SEMICOLON RBRACE
   {
      $$ = tlb_finish(parser_talloc_ctxt, $6);
      $$ = make_union(parser_talloc_ctxt, $3, $$, $9);
      UNLINK_ALL_SYMS(11);
   }

union-body-inner:
   case-spec                      { $$ = tlb_new(parser_talloc_ctxt); $$ = tlb_append(parser_talloc_ctxt, $$, $1); UNLINK_ALL_SYMS(1); }
   | union-body-inner case-spec   { $$ = tlb_append(parser_talloc_ctxt, $1, $2); UNLINK_ALL_SYMS(2); }

case-spec:
   case-spec-inner declaration SEMICOLON { $$ = tlb_finish(parser_talloc_ctxt, $1); $$ = make_case(parser_talloc_ctxt, $$, $2); UNLINK_ALL_SYMS(3); }

case-spec-inner:
   CASE value COLON                     { $$ = tlb_new(parser_talloc_ctxt); $$ = tlb_append(parser_talloc_ctxt, $$, $2); UNLINK_ALL_SYMS(3); }
   | case-spec-inner CASE value COLON   { $$ = tlb_append(parser_talloc_ctxt, $1, $3); UNLINK_ALL_SYMS(4); }

constant-def:
   CONST IDENT EQ constant SEMICOLON
   {
      $$ = make_constant(parser_talloc_ctxt, $2, $4);
      symtab_insert_strict(tok_str($2), tok_child($$));
      UNLINK_ALL_SYMS(5);
   }

type-def:
   TYPEDEF declaration SEMICOLON
   {
      $$ = make_typedef(parser_talloc_ctxt, tok_child($2), tok_next(tok_child($2)));
      symtab_insert_strict(tok_str(tok_child($2)), tok_child($$));
      if (tok_type(tok_next(tok_child($2))) == TOK_ENUM) {
         add_enum_items_to_symtab(tok_next(tok_child($2)));
      }
      UNLINK_ALL_SYMS(3);
   }
   | ENUM IDENT enum-body SEMICOLON
   {
      add_enum_items_to_symtab($3);
      $$ = make_typedef(parser_talloc_ctxt, $2, $3);
      tok_dump($$);
      symtab_insert_strict(tok_str($2), tok_child($$));
      UNLINK_ALL_SYMS(4);
   }
   | STRUCT IDENT struct-body SEMICOLON
   {
      $$ = make_typedef(parser_talloc_ctxt, $2, $3);
      symtab_insert_strict(tok_str($2), tok_child($$));
      UNLINK_ALL_SYMS(4);
   }
   | UNION IDENT union-body SEMICOLON
   {
      $$ = make_typedef(parser_talloc_ctxt, $2, $3);
      symtab_insert_strict(tok_str($2), tok_child($$));
      UNLINK_ALL_SYMS(4);
   }

definition:
   type-def
   | constant-def
   | program-def

specification:
   specification-inner  { $$ = tlb_finish(parser_talloc_ctxt, $1); result_tree = $$; UNLINK_ALL_SYMS(1); }

specification-inner:
   definition                 { $$ = tlb_new(parser_talloc_ctxt); $$ = tlb_append(parser_talloc_ctxt, $$, $1); UNLINK_ALL_SYMS(1); }
   | specification-inner definition { $$ = tlb_append(parser_talloc_ctxt, $1, $2); UNLINK_ALL_SYMS(2); }

/* Not part of RFC 4506 (RPCL specific) */

program-def:
   PROGRAM IDENT LBRACE version-list RBRACE EQ value SEMICOLON
   {
      $$ = tlb_finish(parser_talloc_ctxt, $4);
      $$ = make_prog(parser_talloc_ctxt, $2, $$, $7);
      UNLINK_ALL_SYMS(8);
   }

version-list:
   version SEMICOLON                { $$ = tlb_new(parser_talloc_ctxt); $$ = tlb_append(parser_talloc_ctxt, $$, $1); }
   | version-list version SEMICOLON { $$ = tlb_append(parser_talloc_ctxt, $1, $2); }

version:
   VERSION IDENT LBRACE procedure-list RBRACE EQ value
   {
      $$ = tlb_finish(parser_talloc_ctxt, $4);
      $$ = make_version(parser_talloc_ctxt, $2, $$, $6);
      UNLINK_ALL_SYMS(7);
   }

procedure-list:
   procedure SEMICOLON                    { $$ = tlb_new(parser_talloc_ctxt); $$ = tlb_append(parser_talloc_ctxt, $$, $1); UNLINK_ALL_SYMS(2); }
   | procedure-list procedure SEMICOLON   { $$ = tlb_append(parser_talloc_ctxt, $1, $2); UNLINK_ALL_SYMS(3); }

procedure:
   type-ident IDENT LPAREN type-ident RPAREN EQ value
   {
      $$ = make_proc(parser_talloc_ctxt, $2, $1, $4, $7);
      UNLINK_ALL_SYMS(7);
   }

type-ident:
   type-specifier
   | VOID

/* End Not part of RFC 4506 */
