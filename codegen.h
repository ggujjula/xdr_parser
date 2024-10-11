#ifndef _CODEGEN_H
#define _CODEGEN_H

#include "walker.h"

typedef struct codegen_params {
   char *filename;
} codegen_params;

extern walker_callbacks codegen_callbacks;

#endif
