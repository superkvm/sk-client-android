



#ifndef __YAJL_ALLOC_H__
#define __YAJL_ALLOC_H__

#include "api/yajl_common.h"

#define YA_MALLOC(afs, sz) (afs)->malloc((afs)->ctx, (sz))
#define YA_FREE(afs, ptr) (afs)->free((afs)->ctx, (ptr))
#define YA_REALLOC(afs, ptr, sz) (afs)->realloc((afs)->ctx, (ptr), (sz))

void yajl_set_default_alloc_funcs(yajl_alloc_funcs * yaf);

#endif
