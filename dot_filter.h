/* Print dot representation of token tree to a file. */
#ifndef _DOT_FILTER_H
#define _DOT_FILTER_H

#include "walker.h"

typedef struct dot_filter_params {
        char *filename;
} dot_filter_params;

extern walker_callbacks dot_filter_callbacks;

#endif
