

#ifndef __YAJL_COMMON_H__
#define __YAJL_COMMON_H__

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif    

#define YAJL_MAX_DEPTH 128


#if defined(WIN32) && defined(YAJL_SHARED)
#  ifdef YAJL_BUILD
#    define YAJL_API __declspec(dllexport)
#  else
#    define YAJL_API __declspec(dllimport)
#  endif
#else
#  if defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__) >= 303
#    define YAJL_API __attribute__ ((visibility("default")))
#  else
#    define YAJL_API
#  endif
#endif 


typedef void * (*yajl_malloc_func)(void *ctx, size_t sz);


typedef void (*yajl_free_func)(void *ctx, void * ptr);


typedef void * (*yajl_realloc_func)(void *ctx, void * ptr, size_t sz);


typedef struct
{
    
    yajl_malloc_func malloc;
    
    yajl_realloc_func realloc;
    
    yajl_free_func free;
    
    void * ctx;
} yajl_alloc_funcs;

#ifdef __cplusplus
}
#endif

#endif
