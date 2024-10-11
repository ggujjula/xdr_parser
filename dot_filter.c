#include "dot_filter.h"
#include <stdio.h>
#include <stdlib.h>
#include "token.h"

typedef struct dot_filter_state {
        FILE *file;
} dot_filter_state;

void *dot_init(void *arg)
{
        dot_filter_params *p = (dot_filter_params *)(arg);
        dot_filter_state *s = calloc(1, sizeof(dot_filter_state));
        s->file = fopen(p->filename, "w");
        fprintf(s->file, "strict digraph token_tree {\n");
        return s;
}

void dot_deinit(void *arg)
{
        dot_filter_state *s = (dot_filter_state *)(arg);
        fprintf(s->file, "}");
        fclose(s->file);
        free(s);
}

static tok *dot_hook(tok *t, walker *w, uint32_t walk_depth)
{
        for(int i = 0; i < walk_depth; i++){
                printf("| ");
        }
        printf("%s [%p] ", tokentype_strs[tok_type(t)], t);
        printf("%s", tok_str(t));
        printf("\n");
        dot_filter_state *s = (dot_filter_state *)(walker_get_arg(w));
        //fprintf(s->file, "node \"%p\" [type=%s]\n", t, tokentype_strs[tok_type(t)]);
        //fprintf(s->file, "node \"%p\"\n", t);
        if (t != NULL) {
           fprintf(s->file, "\"%p\" [label=\"%s ", t, tokentype_strs[tok_type(t)]);
           switch(tok_type(t)) {
              case TOK_BASIC:
              case TOK_CONSTANT:
              case TOK_TYPEDEF:
              case TOK_TYPEREF:
                 fprintf(s->file, "%s", tok_str(t));
                 break;
              case TOK_DECL:
              case TOK_INT:
              case TOK_UINT:
              case TOK_HYP:
              case TOK_UHYP:
              case TOK_FLOAT:
              case TOK_DOUBLE:
              case TOK_QUAD:
              case TOK_BOOL:
              case TOK_STRUCT:
              case TOK_UNION:
              case TOK_ENUM:
              case TOK_ENUM_ITEM:
              case TOK_ARRAY:
              case TOK_DYNARRAY:
              case TOK_OPAQUE:
              case TOK_DYNOPAQUE:
              case TOK_STRING:
              case TOK_OPTIONAL:
              case TOK_VOID:
              case TOK_LIST:
              case TOK_VERSION:
              case TOK_PROC:
              case TOK_CASE:
              case TOK_PROG:
              case TOK_NUMBER:
              case TOK_SIZE:
                 break;
           }
           fprintf(s->file, "\"]\n");
        }
        if(tok_child(t) != NULL){
           fprintf(s->file, "\"%p\" -> \"%p\"\n", t, tok_child(t));
        }
        if(tok_next(t) != NULL){
           fprintf(s->file, "\"%p\" -> \"%p\"\n", t, tok_next(t));
        }
      walker_walk_all_children(w, t, NULL, NULL, walk_depth);
      return NULL;
}

walker_callbacks dot_filter_callbacks = {
        .init = dot_init,
        .deinit = dot_deinit,
#define SDEF(token_type) \
        dot_hook
        
        .cb = {
                TOKENTYPES
        },
#undef SDEF
//#define SDEF(token_type) \
//        dot_hook
//        
//        .pre_cb = {
//                TOKENTYPES
//        },
//#undef SDEF
//#define SDEF(token_type) \
//        walker_post_default
//        
//        .post_cb = {
//                TOKENTYPES
//        }
//#undef SDEF
};
