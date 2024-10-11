/* token2.h: A better token iterface
 *
 * The original token interface is not great. The distinction between
 * mutable and immutable interfaces is not clear. As a result, it is
 * difficult to use the iterface in a way that can manipulate the tokens
 * safely; this makes code harder to reason about as a result.
 *
 * This interface should be simpler and safer than the original, which
 * will make using the interface in AST generation and filtering easier.
 *
 * Tokens are immutable. All fields cannot be changed in a given token
 * allocation after it has been created. "Created" here means that the
 * token allocation was returned to a caller external to the token code.
 *
 * This immutability creates a problem when trying to create chains of
 * tokens (for struct member lists, union case lists, etc.). Since we
 * can't modify a token to have it's next point to a new token, an append
 * must create a new token with a new next pointer value. But now if this
 * token was already part of a list, then the token that was pointing to it
 * must be updated. This is a double-edged sword: token trees cannot be changed,
 * but this necessarily means that a tree must be rebuilt up from a desired change point
 * up to the top of the tree if we want to modify something.
 *
 * The parentage of tokens becomes more complex with the involvement of the symbol table.
 * Every token was the only parent of it's child and next, with the exception of TYPEREFs
 * and VALUEs. This two types may have held a reference to a token, but it was not the
 * only one. There were also the TYPEDEF and CONSTANT references. Since these tokens are
 * also in the symbol table, symtab has additional refs to them. As a result, all tokens
 * that were in the symbol table can have multiple parents.
 *
 * To avoid problems with reference counting, we need to adhere to these rules:
 * Tokens that were added to symtab should never be talloc_free()ed.
 *    (Technically we can free if we are sure there is max one parent, but idk a use case.)
 * Tokens that were added to symtab should be referenced with talloc_reference/unlink(),
 * not talloc_steal().
 *
 * The problem is again made worse when we consider walking/filtering the tree. When
 * producing a new tree, we may want to reuse subtrees from the original graph.
 * This creates another multiple parent issue, this time between two different token
 * trees pointing to the same token.
 * This issues means that we can't talloc_steal() when walking either, since the old
 * tree still needs to be valid. At this point, using talloc_steal() when creating the
 * original AST seems like an edge case.
 * I'll try using talloc_reference/unlink() in the token make_* functions, and leaving it
 * to the parser to free the NULL parent for tokens it reduces.
 *
 */
#ifndef _TOKEN2_H
#define _TOKEN2_H

#include <stdint.h>
#include "external/include/talloc.h"

// Do not change order. Append new types before TOK_SIZE.
#define TOKENTYPES \
        SDEF(TOK_BASIC), \
        SDEF(TOK_CONSTANT), \
        SDEF(TOK_DECL), \
        SDEF(TOK_TYPEDEF), \
        SDEF(TOK_INT), \
        SDEF(TOK_UINT), \
        SDEF(TOK_HYP), \
        SDEF(TOK_UHYP), \
        SDEF(TOK_FLOAT), \
        SDEF(TOK_DOUBLE), \
        SDEF(TOK_QUAD), \
        SDEF(TOK_BOOL), \
        SDEF(TOK_STRUCT), \
        SDEF(TOK_UNION), \
        SDEF(TOK_ENUM), \
        SDEF(TOK_ENUM_ITEM), \
        SDEF(TOK_ARRAY), \
        SDEF(TOK_DYNARRAY), \
        SDEF(TOK_OPAQUE), \
        SDEF(TOK_DYNOPAQUE), \
        SDEF(TOK_STRING), \
        SDEF(TOK_OPTIONAL), \
        SDEF(TOK_VOID), \
        SDEF(TOK_LIST), \
        SDEF(TOK_VERSION), \
        SDEF(TOK_PROC), \
        SDEF(TOK_TYPEREF), \
        SDEF(TOK_CASE), \
        SDEF(TOK_PROG), \
        SDEF(TOK_NUMBER), \
        SDEF(TOK_VALUE), \
        SDEF(TOK_LIST_BUILDER), \
        SDEF(TOK_VALREF), \
        SDEF(TOK_SIZE),

#define SDEF(tt) \
        tt
typedef enum tokentype {
        TOKENTYPES
} tokentype;
#undef SDEF

typedef struct token tok;

extern char *tokentype_strs[TOK_SIZE+1];

void token_init(void);

/* Make functions:
 * The main way to create tokens.
 * The parser and filters should use these functions to create tokens.
 *
 * The parser is responsible for cleaning up NULL parents to tokens
 * it reduces. Walkers should not have this issue (calling talloc_free()
 * on an old tree after it has been filtered should take care of cleanup).
 *
 */
tok *make_basic(TALLOC_CTX *ctx, int lval, char *src_str);
tok *make_constant(TALLOC_CTX* ctx, tok *ident, tok *val);
tok *make_decl(TALLOC_CTX* ctx, tok *ident, tok *type);
tok *make_typedef(TALLOC_CTX* ctx, tok *ident, tok *type);
tok *make_struct(TALLOC_CTX* ctx, tok *decls);
tok *make_union(TALLOC_CTX* ctx, tok *discrim, tok *body, tok *def);
tok *make_enum(TALLOC_CTX* ctx, tok *decls);
tok *make_enum_item(TALLOC_CTX* ctx, tok *ident, tok *value);
tok *make_array(TALLOC_CTX* ctx, tok *type, tok *len); 
tok *make_dynarray(TALLOC_CTX* ctx, tok *type, tok *len);
tok *make_opaque(TALLOC_CTX* ctx, tok *len);
tok *make_dynopaque(TALLOC_CTX* ctx, tok *len);
tok *make_string(TALLOC_CTX* ctx, tok *len);
tok *make_optional(TALLOC_CTX* ctx, tok *type);
tok *make_version(TALLOC_CTX* ctx, tok *ident, tok *proc_list, tok *value);
tok *make_proc(TALLOC_CTX* ctx, tok *ident, tok *rettype, tok *argtype, tok *value);
tok *make_typeref(TALLOC_CTX* ctx, tok *ident, tok *type);
tok *make_case(TALLOC_CTX* ctx, tok *list, tok *decl);
tok *make_prog(TALLOC_CTX* ctx, tok *ident, tok *version_list, tok *value);
tok *make_number(TALLOC_CTX* ctx, tokentype type, tok *t);
tok *make_value(TALLOC_CTX* ctx, tok *);
tok *make_valref(TALLOC_CTX *ctx, tok *ident, tok *val);

/* List builder
 * Since appending to a list would require
 * rebuilding it, we use a list builder to
 * make construction easier. The opacity
 * of the builder is not 100%; the caller
 * can still view the contents of the builder
 * with tok_child/next(), but we need to trust
 * the caller a little if we want to use yyval
 * (which is a tok *) to pass around the builder
 * in the parser.
 */
tok *tlb_new(TALLOC_CTX* ctx);
tok *tlb_append(TALLOC_CTX* ctx, tok *, tok *);
tok *tlb_finish(TALLOC_CTX* ctx, tok *);

/* Basic types
 * Basic types are preallocated on token_init().
 * All pointers in all trees point to these
 * allocations.
 */
tok *tok_get_int(void);
tok *tok_get_uint(void);
tok *tok_get_hyp(void);
tok *tok_get_uhyp(void);
tok *tok_get_float(void);
tok *tok_get_double(void);
tok *tok_get_quad(void);
tok *tok_get_bool(void);
tok *tok_get_void(void);

/* Getters for token struct */
tokentype tok_type(tok *t);
char *tok_str(tok *t);
uint64_t tok_uint64(tok *t);
int32_t tok_int32(tok *t);
tok *tok_child(tok *t);
tok *tok_next(tok *t);
void tok_dump(tok *t);

/* Modifiers
 * These functions create a shallow copy of a given token,
 * modify their pointers, and handle ref counting correctly.
 */
tok *tok_set_child(TALLOC_CTX* ctx, tok *t, tok *child);
tok *tok_set_next(TALLOC_CTX* ctx, tok *t, tok *next);

/* Copies */
tok *tok_copy(TALLOC_CTX* ctx, tok *t);
tok *tok_deep_copy(TALLOC_CTX* ctx, tok *t);

//Not sure
int case_is_in_enum(tok *case_tok, tok *enum_tok);

#endif
