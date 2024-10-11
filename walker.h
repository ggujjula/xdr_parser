/* Walks a token tree, applying rules on matched tokens to produce another tree */
/* If a walker doesn't want to modify the tree, it should return NULL from its
 * callbacks. */
#ifndef _WALKER_H
#define _WALKER_H

#include "token.h"
#include <stdint.h>

typedef struct walker walker;

typedef tok *(*walker_callback)(tok *, walker *, uint32_t);
typedef void(*walker_child_pre_callback)(tok *, walker *, uint32_t);
typedef void(*walker_child_post_callback)(tok *, walker *, uint32_t);
typedef void *(*walker_init_cb)(void *);
typedef void(*walker_deinit_cb)(void *);

typedef struct walker_callbacks {
        walker_callback cb[TOK_SIZE + 1];
        walker_init_cb init;
        walker_deinit_cb deinit;
} walker_callbacks;

walker *walker_init(struct walker_callbacks *cb, void *arg);
void walker_deinit(walker *w);
tok *walker_walk(walker *w, tok *tt, uint32_t walk_depth);

tok *walker_default(tok *, walker *, uint32_t);

void *walker_get_arg(walker *);

tok *walker_walk_all_children(walker *w, tok *tt, walker_child_pre_callback, walker_child_post_callback, uint32_t walk_depth);

#endif

