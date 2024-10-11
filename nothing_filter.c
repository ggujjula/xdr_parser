#include "nothing_filter.h"

#include <assert.h>

static tok *nothing_hook(tok *t, walker *w, uint32_t walk_depth)
{
   tok *child = tok_child(t);
   tok *next = tok_next(t);
   tok *ret = walker_default(t, w, walk_depth);
   assert(t == ret);
   assert(tok_child(t) == child);
   assert(tok_next(t) == next);
   return t;
}

walker_callbacks nothing_filter_callbacks = {
#define SDEF(token_type) \
        walker_default
        .cb = {
                TOKENTYPES
        },
#undef SDEF
};
