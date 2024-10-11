#include "walker.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

struct walker {
        walker_callbacks *cb;
        void *arg;
};

walker *walker_init(struct walker_callbacks *cb, void *arg)
{
        walker *w = calloc(1, sizeof(walker));
        w->cb = cb;
        if(cb->init != NULL) {
                w->arg = cb->init(arg);
        }
        return w;
}

void walker_deinit(walker *w)
{
        if(w->cb->deinit != NULL) {
                w->cb->deinit(w->arg);
        }
        free(w);
}

/* Walk across the token tree */
tok *walker_walk(walker *w, tok *tt, uint32_t walk_depth)
{
   //fprintf(stderr, "Walking over %p\n", tt);
   if(w == NULL || tt == NULL) {
      return NULL;
   }
   assert(w->cb->cb[tok_type(tt)] != NULL);
   //fprintf(stderr, "Calling hook: %p with token %p\n", w->cb->cb[tok_type(tt)], tt);
   tok *retval = w->cb->cb[tok_type(tt)](tt, w, walk_depth);
   return retval != NULL ? retval : tt;
   //return w->cb->cb[tok_type(tt)](tt, w, w->arg);
}

/* Default walks across the children, makes a copy of this token,
 * assigns the new children to the copy, and return the copy. */
tok *walker_default(tok *tt, walker *w, uint32_t walk_depth)
{
   tok *new_child = walker_walk_all_children(w, tt, NULL, NULL, walk_depth);
   tok *ret = tok_copy(NULL, tt);
   ret = tok_set_next(NULL, ret, NULL);
   ret = tok_set_child(NULL, ret, new_child);
   return ret;
}

void *walker_get_arg(walker *w)
{
   return w->arg;
}

tok *walker_walk_all_children(walker *w, tok *tt, walker_child_pre_callback pre_cb, walker_child_post_callback post_cb, uint32_t walk_depth)
{
   tok *placeholder = tlb_new(NULL);
   if(tok_child(tt) != NULL) {
       walk_depth += 1;
       tok *ct = tok_child(tt);
       while(ct != NULL) {
               //fprintf(stderr, "Recurse into %p\n", ct);
               if (pre_cb != NULL) {
                  pre_cb(ct, w, walk_depth);
               }
               tok *new_ct = walker_walk(w, ct, walk_depth);
               if (post_cb != NULL) {
                  post_cb(ct, w, walk_depth);
               }
               //fprintf(stderr, "Got new ct %p\n", new_ct);
               if(new_ct != NULL){
                       tlb_append(NULL, placeholder, new_ct);
               }
               ct = tok_next(ct);
               //fprintf(stderr, "ct now %p\n", ct);
       }
       walk_depth -= 1;
   }
   return tok_child(tlb_finish(NULL, placeholder));
}
