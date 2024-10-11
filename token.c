#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "token.h"
#include "parser.h"

#define SDEF(token_type) \
#token_type
char *tokentype_strs[TOK_SIZE+1] = {
   TOKENTYPES
};
#undef SDEF

struct token {
   tokentype type;
   int lval;
   char *name;
   union {
      char *str_val;
      int32_t int_val;
      uint32_t uint_val;
      int64_t hyper_val;
      uint64_t uhyper_val;
      float float_val;
      double double_val;
      long double quad_val; //TODO: support quad precision floating points
      long long const_val;
   } u;
   uint64_t size;
   tok *next;
   tok *child;
};

tok *tok_int_type;
tok *tok_uint_type;
tok *tok_hyp_type;
tok *tok_uhyp_type;
tok *tok_float_type;
tok *tok_double_type;
tok *tok_quad_type;
tok *tok_void_type;

void token_init(void)
{
   tok t = (tok){
      .type = TOK_INT,
      .size = 4,
   };
   tok_int_type = talloc_zero(NULL, tok);
   memcpy(tok_int_type, &t, sizeof(tok));
   t = (tok){
      .type = TOK_UINT,
      .size = 4,
   };
   tok_uint_type = talloc_zero(NULL, tok);
   memcpy(tok_uint_type, &t, sizeof(tok));
   t = (tok){
      .type = TOK_HYP,
      .size = 8,
   };
   tok_hyp_type = talloc_zero(NULL, tok);
   memcpy(tok_hyp_type, &t, sizeof(tok));
   t = (tok){
      .type = TOK_UHYP,
      .size = 8,
   };
   tok_uhyp_type = talloc_zero(NULL, tok);
   memcpy(tok_uhyp_type, &t, sizeof(tok));
   t = (tok){
      .type = TOK_FLOAT,
      .size = 4,
   };
   tok_float_type = talloc_zero(NULL, tok);
   memcpy(tok_float_type, &t, sizeof(tok));
   t = (tok){
      .type = TOK_DOUBLE,
      .size = 8,
   };
   tok_double_type = talloc_zero(NULL, tok);
   memcpy(tok_double_type, &t, sizeof(tok));
   t = (tok){
      .type = TOK_QUAD,
      .size = 16,
   };
   tok_quad_type = talloc_zero(NULL, tok);
   memcpy(tok_quad_type, &t, sizeof(tok));
   t = (tok){
      .type = TOK_VOID,
      .size = 0,
   };
   tok_void_type = talloc_zero(NULL, tok);
   memcpy(tok_void_type, &t, sizeof(tok));
}

/* Make a new token. Allocated as a top-level context. */
static tok *make_blank(TALLOC_CTX *ctx)
{
   tok *retval = talloc_zero(ctx, tok);
   //talloc_reference(ctx, retval);
   return retval;
}

static int token_destructor(tok *t)
{
   fprintf(stderr, "FREEING %p\n", t);
   tok_dump(t);
   return 0;
}

// Caller is responsible for freeing src_str.
tok *make_basic(TALLOC_CTX *ctx, int lval, char *src_str)
{
   tok *retval = make_blank(ctx);
   retval->type = TOK_BASIC;
   retval->lval = lval;
   retval->u.str_val = talloc_strdup(retval, src_str);
   talloc_set_destructor(retval, token_destructor);
   return retval;
}

tok *make_constant(TALLOC_CTX *ctx, tok *ident, tok *val)
{
   tok *t = make_blank(ctx);
   t->u.str_val = talloc_reference(t, ident->u.str_val);
   t->type = TOK_CONSTANT;
   t->child = talloc_reference(t, val);
   return t;
}

tok *make_decl(TALLOC_CTX *ctx, tok *ident, tok *type)
{
   ident = tok_set_next(ctx, ident, type);
   tok *t = make_blank(ctx);
   t->type = TOK_DECL;
   t->child = talloc_reference(t, ident);
   return t;
}

tok *make_typedef(TALLOC_CTX *ctx, tok *ident, tok *type)
{
   tok *t = make_blank(ctx);
   t->u.str_val = talloc_reference(t, ident->u.str_val);
   t->type = TOK_TYPEDEF;
   t->child = talloc_reference(t, type);
   return t;
}

tok *make_typeref(TALLOC_CTX *ctx, tok *ident, tok *type)
{
   tok *t = make_blank(ctx);
   t->u.str_val = talloc_reference(t, ident->u.str_val);
   t->type = TOK_TYPEREF;
   t->child = talloc_reference(t, type);
   return t;
}

tok *make_version(TALLOC_CTX *ctx, tok *ident, tok *proc_list, tok *value) {
   proc_list = tok_set_next(ctx, proc_list, value);
   tok *t = make_blank(ctx);
   t->type = TOK_VERSION;
   t->u.str_val = talloc_reference(t, ident->u.str_val);
   t->child = talloc_reference(t, proc_list);
   return t;
}

tok *make_proc(TALLOC_CTX *ctx, tok *ident, tok *rettype, tok *argtype, tok *value) {
   argtype = tok_set_next(ctx, argtype, value);
   rettype = tok_set_next(ctx, rettype, argtype);
   tok *t = make_blank(ctx);
   t->type = TOK_PROC;
   t->u.str_val = talloc_reference(t, ident->u.str_val);
   t->child = talloc_reference(t, rettype);
   return t;
}

tok *make_prog(TALLOC_CTX *ctx, tok *ident, tok *version_list, tok *value) {
   version_list = tok_set_next(ctx, version_list, value);
   tok *t = make_blank(ctx);
   t->type = TOK_PROG;
   t->u.str_val = talloc_reference(t, ident->u.str_val);
   t->child = talloc_reference(t, version_list);
   talloc_free(ident);
   return t;
}

tok *make_case(TALLOC_CTX *ctx, tok *list, tok *decl) {
   list = tok_set_next(ctx, list, decl);
   tok *t = make_blank(ctx);
   t->type = TOK_CASE;
   t->child = talloc_reference(t, list);
   return t;
}

static char get_discrim_type(tok *discrim)
{
   assert(discrim != NULL);
   tok_dump(discrim);
   tokentype type = tok_type(tok_next(tok_child(discrim)));
   if(type == TOK_TYPEREF) {
      type = tok_type(tok_child(tok_next(tok_child(discrim))));
   }
   if(type == TOK_INT || type == TOK_UINT || type == TOK_ENUM)
   {
      return type;
   }
   return TOK_SIZE;
}


tok *make_union(TALLOC_CTX *ctx, tok *discrim, tok *body, tok *def) {
   tokentype type = get_discrim_type(discrim);
   if(type == TOK_SIZE) {
      return NULL;
   }
   if (type == TOK_ENUM) {
      type = TOK_INT;
   }
   tok *case_spec_iter = tok_child(body);
   tok *case_spec_tlb = tlb_new(ctx);
   while(case_spec_iter != NULL) {
      fprintf(stderr, "Case spec: \n");
      tok_dump(case_spec_iter);
      tok *value_iter = tok_child(tok_child(case_spec_iter));
      tok *value_tlb = tlb_new(ctx);
      while(value_iter != NULL) {
         fprintf(stderr, "Value: \n");
         tok_dump(value_iter);
         tok *value_to_add = make_number(ctx, type, value_iter);
         assert(value_to_add != NULL);
         value_tlb = tlb_append(ctx, value_tlb, value_to_add);
         value_iter = tok_next(value_iter);
      }
      tok *new_inner = tlb_finish(ctx, value_tlb);
      tok *new_case_spec = make_case(ctx, new_inner, tok_next(tok_child(case_spec_iter)));
      case_spec_tlb = tlb_append(ctx, case_spec_tlb, new_case_spec);
      case_spec_iter = tok_next(case_spec_iter);
   }
   body = tlb_finish(ctx, case_spec_tlb);
   body = tok_set_next(ctx, body, def);
   discrim = tok_set_next(ctx, discrim, body);
   tok *t = make_blank(ctx);
   t->type = TOK_UNION;
   t->child = talloc_reference(t, discrim);
   return t;
}

tok *make_struct(TALLOC_CTX *ctx, tok *decls) {
   tok *t = make_blank(ctx);
   t->type = TOK_STRUCT;
   t->child = talloc_reference(t, decls);
   return t;
}

tok *make_enum_item(TALLOC_CTX *ctx, tok *ident, tok *value) {
   ident = tok_set_next(ctx, ident, value);
   tok *t = make_blank(ctx);
   t->type = TOK_ENUM_ITEM;
   t->child = talloc_reference(t, ident);
   return t;
}

tok *make_enum(TALLOC_CTX *ctx, tok *decls) {
   tok *t = make_blank(ctx);
   t->type = TOK_ENUM;
   t->child = talloc_reference(t, decls);
   return t;
}

tok *make_array(TALLOC_CTX *ctx, tok *type, tok *len)
{
   if (len != NULL) {
      type = tok_set_next(ctx, type, len);
   }
   tok *t = make_blank(ctx);
   t->type = TOK_ARRAY;
   t->child = talloc_reference(t, type);
   /* TODO size */
   return t;
}

tok *make_dynarray(TALLOC_CTX *ctx, tok *type, tok *len)
{
   if (len != NULL) {
      type = tok_set_next(ctx, type, len);
   }
   tok *t = make_blank(ctx);
   t->type = TOK_DYNARRAY;
   t->child = talloc_reference(t, type);
   /* TODO size */
   return t;
}

tok *make_opaque(TALLOC_CTX *ctx, tok *len)
{
   tok *t = make_blank(ctx);
   t->type = TOK_OPAQUE;
   t->child = talloc_reference(t, len);
   /* TODO size */
   return t;
}

tok *make_dynopaque(TALLOC_CTX *ctx, tok *len)
{
   tok *t = make_blank(ctx);
   t->type = TOK_DYNOPAQUE;
   t->child = talloc_reference(t, len);
   /* TODO size */
   return t;
}

tok *make_string(TALLOC_CTX *ctx, tok *len)
{
   tok *t = make_blank(ctx);
   t->type = TOK_STRING;
   t->child = talloc_reference(t, len);
   /* TODO size */
   return t;
}

tok *make_optional(TALLOC_CTX *ctx, tok *type)
{
   tok *t = make_blank(ctx);
   t->type = TOK_OPTIONAL;
   t->child = talloc_reference(t, type);
   /* TODO size */
   return t;
}

tok *tlb_new(TALLOC_CTX *ctx)
{
   tok *head = talloc_zero(NULL, tok);
   tok *tail = talloc_zero(NULL, tok);
   head->type = TOK_LIST_BUILDER;
   tail->type = TOK_LIST_BUILDER;
   head->next = talloc_reference(head, tail);
   return head;
}

tok *tlb_append(TALLOC_CTX *ctx, tok *head, tok *t)
{
   assert(head != NULL);
   tok *tail = head->next;
   assert(tail != NULL);
   tok *new_head = tlb_new(ctx);
   tok *new_tail = new_head->next;
   if (head->child == NULL) {
      new_head->child = talloc_reference(new_head, t);
      new_tail->child = talloc_reference(new_tail, t);
   } else {
      tail->child->next = talloc_reference(tail->child, t);
      new_head->child = talloc_reference(new_head, head->child);
      new_tail->child = talloc_reference(new_tail, t);
   }
   return new_head;
}

tok *tlb_finish(TALLOC_CTX *ctx, tok *head){
   tok *list = make_blank(ctx);
   list->type = TOK_LIST;
   list->child = talloc_reference(list, head->child);
   return list;
}

static int lto32t_checked(long long val, int32_t *retval)
{
   if(val < INT32_MIN  || val > INT32_MAX) {
      return 1;
   }
   *retval = (int32_t)(val);
   return 0;
}

static int ultou32t_checked(long long val, uint32_t *retval)
{
   if(val < 0  || val > UINT32_MAX) {
      return 1;
   }
   *retval = (uint32_t)(val);
   return 0;
}

static int llto64t_checked(long long val, int64_t *retval)
{
   if(val < INT64_MIN  || val > INT64_MAX) {
      return 1;
   }
   *retval = (int64_t)(val);
   return 0;
}

static int ulltou64t_checked(long long val, int64_t *retval)
{
   if(val < 0  || val > UINT64_MAX) {
      return 1;
   }
   *retval = (uint64_t)(val);
   return 0;
}

tok *make_number(TALLOC_CTX *ctx, tokentype type, tok *ct)
{
   tok *t;
   if (tok_type(ct) == TOK_VALREF) {
      t = tok_child(ct);
   } else {
      t = ct;
   }
   tok *num = make_blank(ctx);
   char *num_str = tok_str(tok_child(t));
   char *end_ptr = NULL;
   tok *type_tok = NULL;
   errno = 0;
   switch(type) {
      case TOK_INT:
         num->u.int_val = strtol(num_str, &end_ptr, 0);
         type_tok = tok_int_type;
         break;
      case TOK_UINT:
         num->u.uint_val = strtoul(num_str, &end_ptr, 0);
         type_tok = tok_uint_type;
         break;
      case TOK_HYP:
         num->u.hyper_val = strtoll(num_str, &end_ptr, 0);
         type_tok = tok_hyp_type;
         break;
      case TOK_UHYP:
         num->u.uhyper_val = strtoull(num_str, &end_ptr, 0);
         type_tok = tok_uhyp_type;
         break;
      case TOK_FLOAT:
         num->u.float_val = strtof(num_str, &end_ptr);
         type_tok = tok_float_type;
         break;
      case TOK_DOUBLE:
         num->u.double_val = strtod(num_str, &end_ptr);
         type_tok = tok_double_type;
         break;
      default:
         return NULL;
   }
   if (errno != 0) {
      return NULL;
   }
   num->type = TOK_NUMBER;
   ct = tok_set_next(ctx, ct, type_tok);
   num->child = talloc_reference(num, ct);
   return num;
}

tok *make_value(TALLOC_CTX *ctx, tok *ct)
{
   tok *t = make_blank(ctx);
   t->type = TOK_VALUE;
   t->child = talloc_reference(t, ct);
   return t;
}

tok *make_valref(TALLOC_CTX *ctx, tok *ident, tok *val)
{
   val = tok_set_next(ctx, val, ident);
   tok *t = make_blank(ctx);
   t->type = TOK_VALREF;
   t->child = talloc_reference(t, val);
   return t;
}

tok *tok_get_int(void) {
   return tok_int_type;
}

tok *tok_get_uint(void)
{
   return tok_uint_type;
}

tok *tok_get_hyp(void)
{
   return tok_hyp_type;
}

tok *tok_get_uhyp(void)
{
   return tok_uhyp_type;
}

tok *tok_get_float(void)
{
   return tok_float_type;
}

tok *tok_get_double(void)
{
   return tok_double_type;
}

tok *tok_get_quad(void)
{
   return tok_quad_type;
}

tok *tok_get_bool(void)
{
   return NULL;
}

tok *tok_get_void(void)
{
   return tok_void_type;
}

tokentype tok_type(tok *t)
{
   return t->type;
}

char *tok_str(tok *t)
{
   if (t == NULL ||
         !(t->type == TOK_BASIC ||
            t->type == TOK_CONSTANT ||
            t->type == TOK_TYPEDEF ||
            t->type == TOK_TYPEREF)) {
      return NULL;
   }
   return t->u.str_val;
}

uint64_t tok_uint64(tok *t)
{
   return t->u.uhyper_val;
}

int32_t tok_int32(tok *t)
{
   return t->u.int_val;
}

tok *tok_child(tok *t)
{
   if (t == NULL) {
      return NULL;
   }
   return t->child;
}

tok *tok_next(tok *t)
{
   if (t == NULL) {
      return NULL;
   }
   return t->next;
}

void tok_dump(tok *t) {
   if (t == NULL) {
      return;
   }
   fprintf(stderr, "Token: %s (%p) %s\n", tokentype_strs[tok_type(t)], t, tok_str(t));
}

tok *tok_set_child(TALLOC_CTX *ctx, tok *t, tok *child)
{
   tok *retval = tok_copy(ctx, t);
   retval->child = talloc_reference(retval, child);
   return retval;
}

tok *tok_set_next(TALLOC_CTX *ctx, tok *t, tok *next)
{
   tok *retval = tok_copy(ctx, t);
   retval->next = talloc_reference(retval, next);
   return retval;
}

tok *tok_copy(TALLOC_CTX *ctx, tok *src)
{
   if(src == NULL) {
      return NULL;
   }
   tok *t = make_blank(ctx);
   if(t == NULL) {
      return NULL;
   }
   memcpy(t, src, sizeof(tok));
   if (tok_str(t) != NULL) {
      t->u.str_val = talloc_reference(t, tok_str(t));
   }
   t->child = talloc_reference(t, tok_child(src));
   t->next = talloc_reference(t, tok_next(src));
   return t;
}

tok *tok_deep_copy(TALLOC_CTX *ctx, tok *t)
{
   assert(0);
   return NULL;
}

int case_is_in_enum(tok *case_tok, tok *enum_tok)
{
   char *target_str = tok_str(case_tok);
   assert(case_tok != NULL && enum_tok != NULL);
   tok *item_iter = tok_child(tok_child(enum_tok));
   while(item_iter != NULL) {
      tok *case_in_list = tok_child(item_iter);
      char *case_str = tok_str(case_in_list);
      fprintf(stderr, "Compare %s to %s", target_str, case_str);
      if (strcmp(target_str, case_str) == 0) {
         fprintf(stderr, "Case match", target_str, case_str);
         return 0;
      }
      item_iter = tok_next(item_iter);
   }
   return 1;
}
