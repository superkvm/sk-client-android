



#include "yajl_alloc.h"
#include <stdlib.h>

static void * yajl_internal_malloc(void *ctx, size_t sz)
{
    return malloc(sz);
}

static void * yajl_internal_realloc(void *ctx, void * previous,
                                    size_t sz)
{
    return realloc(previous, sz);
}

static void yajl_internal_free(void *ctx, void * ptr)
{
    free(ptr);
}

void yajl_set_default_alloc_funcs(yajl_alloc_funcs * yaf)
{
    yaf->malloc = yajl_internal_malloc;
    yaf->free = yajl_internal_free;
    yaf->realloc = yajl_internal_realloc;
    yaf->ctx = NULL;
}

