#include "codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include "token.h"

typedef struct codegen_state {
   FILE *file;
} codegen_state;

void *codegen_init(void *arg)
{
   codegen_params *p = (codegen_params *)(arg);
   codegen_state *s = calloc(1, sizeof(codegen_state));
   s->file = fopen(p->filename, "w");
   fprintf(s->file, "/* Generated using xdr_parser */\n\n");
   fprintf(s->file,
         "extern crate serde_xdr;\n"
         "extern crate serde_bytes;\n"
         "#[macro_use]\n"
         "extern crate serde_derive;\n"

         "use std::io::Cursor;\n"

         "use serde_bytes::ByteBuf;\n"
         "use serde_xdr::{from_reader, to_writer};\n\n");
   return s;
}

void codegen_deinit(void *arg)
{
   codegen_state *s = (codegen_state *)(arg);
   fclose(s->file);
   free(s);
}

static tok *const_pre_hook(tok *t, walker *w, uint32_t walk_depth)
{
   codegen_state *s = (codegen_state *)(walker_get_arg(w));
   fprintf(s->file, "macro_rules! %s { () => { %s ", tok_str(t), tok_str(tok_child(t)));
   walker_walk(w, tok_child(t), walk_depth);
   fprintf(s->file, "}; }\n");
   return NULL;
}

static tok *typedef_pre_hook(tok *t, walker *w, uint32_t walk_depth)
{
   codegen_state *s = (codegen_state *)(walker_get_arg(w));
   tok *new_t = tok_copy(NULL, t);
   new_t = tok_set_child(NULL, t, NULL);
   new_t = tok_set_next(NULL, t, NULL);
   switch (tok_type(tok_child(t))) {
      case TOK_INT:
         fprintf(s->file, "type %s = i32;\n", tok_str(t));
         break;
      case TOK_UINT:
         fprintf(s->file, "type %s = u32;\n", tok_str(t));
         break;
      case TOK_HYP:
         fprintf(s->file, "type %s = i64;\n", tok_str(t));
         break;
      case TOK_UHYP:
         fprintf(s->file, "type %s = u64;\n", tok_str(t));
         break;
      case TOK_FLOAT:
         fprintf(s->file, "type %s = f32;\n", tok_str(t));
         break;
      case TOK_DOUBLE:
         fprintf(s->file, "type %s = f64;\n", tok_str(t));
         break;
      case TOK_QUAD:
         fprintf(s->file, "type %s = f128;\n", tok_str(t));
         break;
      case TOK_BOOL:
         fprintf(s->file, "type %s = bool;\n", tok_str(t));
         break;
      case TOK_STRUCT:
         fprintf(s->file, "#[derive(Serialize, Deserialize)]\n");
         fprintf(s->file, "struct %s", tok_str(t));
         tok_set_child(NULL, new_t, walker_walk(w, tok_child(t), walk_depth));
         break;
      case TOK_UNION:
      case TOK_ENUM:
         fprintf(s->file, "#[derive(Serialize, Deserialize)]\n");
         fprintf(s->file, "enum %s", tok_str(t));
         tok_set_child(NULL, new_t, walker_walk(w, tok_child(t), walk_depth));
         break;
   }
   return new_t;
}

static tok *struct_pre_hook(tok *t, walker *w, uint32_t walk_depth)
{
   codegen_state *s = (codegen_state *)(walker_get_arg(w));
   fprintf(s->file, " {\n", tok_str(t));
   tok *new_t = tok_copy(NULL, t);
   new_t = tok_set_child(NULL, new_t, NULL);
   new_t = tok_set_next(NULL, new_t, NULL);
   tok *iter = tok_child(tok_child(t));
   while (iter != NULL) {
      walker_walk(w, iter, walk_depth);
      fprintf(s->file, ",\n");
      iter = tok_next(iter);
   }
   fprintf(s->file, "}\n", tok_str(t));
   return new_t;
}

static tok *decl_pre_hook(tok *t, walker *w, uint32_t walk_depth)
{
   codegen_state *s = (codegen_state *)(walker_get_arg(w));
   tok *decl_name_tok = tok_child(t);
   tok *decl_type_tok = tok_next(decl_name_tok);
   tokentype decl_type = tok_type(decl_type_tok);
   tok_dump(decl_type_tok);
   switch (decl_type) {
      case TOK_INT:
      case TOK_UINT:
      case TOK_HYP:
      case TOK_UHYP:
      case TOK_FLOAT:
      case TOK_DOUBLE:
      case TOK_QUAD:
      case TOK_BOOL:
         break;
      case TOK_STRING:
         fprintf(s->file, "%s: ", tok_str(tok_child(t)));
         fprintf(s->file, "String");
         break;
      case TOK_OPAQUE:
         fprintf(s->file, "%s: ", tok_str(tok_child(t)));
         fprintf(s->file, "ByteBuf");
         break;
      case TOK_DYNOPAQUE:
         fprintf(s->file, "%s: ", tok_str(tok_child(t)));
         fprintf(s->file, "ByteBuf");
         break;
      case TOK_TYPEREF:
         if (tok_type(tok_child(decl_type_tok)) != TOK_VOID){
            fprintf(s->file, "%s: ", tok_str(tok_child(t)));
            fprintf(s->file, "%s", tok_str(decl_type_tok));
         }
         break;
   }
   return NULL;
}

static tok *process_union_case(tok *t, tok *discrim_type, walker *w, uint32_t walk_depth)
{
   codegen_state *s = (codegen_state *)(walker_get_arg(w));
   tok *iter = tok_child(tok_child(t));
   tok *decl = tok_next(tok_child(t));
   while (iter != NULL) {
      if (tok_type(discrim_type) == TOK_TYPEREF) {
         fprintf(s->file, "%s::%s", tok_str(discrim_type), tok_str(tok_next(tok_child(tok_child(iter)))));
      }
      else {
         walker_walk(w, iter, walk_depth);
      }
      fprintf(s->file, "{", tok_str(discrim_type));
      walker_walk(w, decl, walk_depth);
      fprintf(s->file, "},\n", tok_str(discrim_type));
      iter = tok_next(iter);
   }
   return NULL;
}

static tok *union_pre_hook(tok *t, walker *w, uint32_t walk_depth)
{
   codegen_state *s = (codegen_state *)(walker_get_arg(w));
   tok *discrim_type = tok_next(tok_child(tok_child(t)));
   fprintf(s->file, " {\n", tok_str(t));
   tok *case_iter = tok_child(tok_next(tok_child(t)));
   while (case_iter != NULL) {
      process_union_case(case_iter, discrim_type, w, walk_depth);
      case_iter = tok_next(case_iter);
   }
   fprintf(s->file, "}\n", tok_str(t));
   return NULL;
}

static tok *enum_pre_hook(tok *t, walker *w, uint32_t walk_depth)
{
   codegen_state *s = (codegen_state *)(walker_get_arg(w));
   fprintf(s->file, " {\n", tok_str(t));
   walker_walk_all_children(w, t, NULL, NULL, walk_depth);
   fprintf(s->file, "}\n", tok_str(t));
   return NULL;
}

static tok *enum_item_pre_hook(tok *t, walker *w, uint32_t walk_depth)
{
   codegen_state *s = (codegen_state *)(walker_get_arg(w));
   fprintf(s->file, "%s = ", tok_str(tok_child(t)));
   tok *num_tok = make_number(NULL, TOK_INT, tok_next(tok_child(t)));
   walker_walk(w, num_tok, walk_depth);
   //walker_walk(w, tok_next(tok_child(t)), walk_depth);
   fprintf(s->file, ",\n", tok_str(tok_child(t)));
   return NULL;
}

static tok *number_pre_hook(tok *t, walker *w, uint32_t walk_depth)
{
   codegen_state *s = (codegen_state *)(walker_get_arg(w));
   fprintf(s->file, "0x%x", tok_int32(t));
   return NULL;
}

static tok *typeref_pre_hook(tok *t, walker *w, uint32_t walk_depth)
{
   codegen_state *s = (codegen_state *)(walker_get_arg(w));
   fprintf(s->file, "%s", tok_str(tok_child(t)));
   return NULL;
}

walker_callbacks codegen_callbacks = {
   .init = codegen_init,
   .deinit = codegen_deinit,
   .cb = {
      walker_default, //SDEF(TOK_BASIC), 
      const_pre_hook, //SDEF(TOK_CONSTANT), 
      decl_pre_hook, //SDEF(TOK_DECL), 
      typedef_pre_hook, //SDEF(TOK_TYPEDEF), 
      walker_default, //SDEF(TOK_INT), 
      walker_default, //SDEF(TOK_UINT), 
      walker_default, //SDEF(TOK_HYP), 
      walker_default, //SDEF(TOK_UHYP), 
      walker_default, //SDEF(TOK_FLOAT), 
      walker_default, //SDEF(TOK_DOUBLE), 
      walker_default, //SDEF(TOK_QUAD), 
      walker_default, //SDEF(TOK_BOOL), 
      struct_pre_hook, //SDEF(TOK_STRUCT), 
      union_pre_hook, //SDEF(TOK_UNION), 
      enum_pre_hook, //SDEF(TOK_ENUM), 
      enum_item_pre_hook, //SDEF(TOK_ENUM_ITEM), 
      walker_default, //SDEF(TOK_ARRAY), 
      walker_default, //SDEF(TOK_DYNARRAY), 
      walker_default, //SDEF(TOK_OPAQUE), 
      walker_default, //SDEF(TOK_DYNOPAQUE), 
      walker_default, //SDEF(TOK_STRING), 
      walker_default, //SDEF(TOK_OPTIONAL), 
      walker_default, //SDEF(TOK_VOID), 
      walker_default, //SDEF(TOK_LIST), 
      walker_default, //SDEF(TOK_VERSION), 
      walker_default, //SDEF(TOK_PROC), 
      typeref_pre_hook, //SDEF(TOK_TYPEREF), 
      walker_default, //SDEF(TOK_CASE), 
      walker_default, //SDEF(TOK_PROG), 
      number_pre_hook, //SDEF(TOK_NUMBER), 
      walker_default,  //SDEF(TOK_VALUE),
      NULL,             //SDEF(TOK_LIST_BUILDER),
      walker_default,  //SDEF(TOK_VALREF),
      walker_default, //SDEF(TOK_SIZE),
   },
};

