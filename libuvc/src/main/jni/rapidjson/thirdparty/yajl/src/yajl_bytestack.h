



#ifndef __YAJL_BYTESTACK_H__
#define __YAJL_BYTESTACK_H__

#include "api/yajl_common.h"

#define YAJL_BS_INC 128

typedef struct yajl_bytestack_t
{
    unsigned char * stack;
    size_t size;
    size_t used;
    yajl_alloc_funcs * yaf;
} yajl_bytestack;


#define yajl_bs_init(obs, _yaf) {               \
        (obs).stack = NULL;                     \
        (obs).size = 0;                         \
        (obs).used = 0;                         \
        (obs).yaf = (_yaf);                     \
    }                                           \



#define yajl_bs_free(obs)                 \
    if ((obs).stack) (obs).yaf->free((obs).yaf->ctx, (obs).stack);

#define yajl_bs_current(obs)               \
    (assert((obs).used > 0), (obs).stack[(obs).used - 1])

#define yajl_bs_push(obs, byte) {                       \
    if (((obs).size - (obs).used) == 0) {               \
        (obs).size += YAJL_BS_INC;                      \
        (obs).stack = (obs).yaf->realloc((obs).yaf->ctx,\
                                         (void *) (obs).stack, (obs).size);\
    }                                                   \
    (obs).stack[((obs).used)++] = (byte);               \
}


#define yajl_bs_pop(obs) { ((obs).used)--; }

#define yajl_bs_set(obs, byte)                          \
    (obs).stack[((obs).used) - 1] = (byte);


#endif
