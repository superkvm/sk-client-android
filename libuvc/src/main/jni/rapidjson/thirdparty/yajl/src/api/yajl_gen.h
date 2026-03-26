



#include <yajl/yajl_common.h>

#ifndef __YAJL_GEN_H__
#define __YAJL_GEN_H__

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif
    
    typedef enum {
        
        yajl_gen_status_ok = 0,
        
        yajl_gen_keys_must_be_strings,
        
        yajl_max_depth_exceeded,
        
        yajl_gen_in_error_state,
        
        yajl_gen_generation_complete,                
        
        yajl_gen_invalid_number,
        
        yajl_gen_no_buf,
        
        yajl_gen_invalid_string
    } yajl_gen_status;

    
    typedef struct yajl_gen_t * yajl_gen;

    
    typedef void (*yajl_print_t)(void * ctx,
                                 const char * str,
                                 size_t len);

    
    typedef enum {
        
        yajl_gen_beautify = 0x01,
        
        yajl_gen_indent_string = 0x02,
        
        yajl_gen_print_callback = 0x04,
        
        yajl_gen_validate_utf8 = 0x08,
        
        yajl_gen_escape_solidus = 0x10
    } yajl_gen_option;

    
    YAJL_API int yajl_gen_config(yajl_gen g, yajl_gen_option opt, ...);

    
    YAJL_API yajl_gen yajl_gen_alloc(const yajl_alloc_funcs * allocFuncs);

    
    YAJL_API void yajl_gen_free(yajl_gen handle);

    YAJL_API yajl_gen_status yajl_gen_integer(yajl_gen hand, long long int number);
    
    YAJL_API yajl_gen_status yajl_gen_double(yajl_gen hand, double number);
    YAJL_API yajl_gen_status yajl_gen_number(yajl_gen hand,
                                             const char * num,
                                             size_t len);
    YAJL_API yajl_gen_status yajl_gen_string(yajl_gen hand,
                                             const unsigned char * str,
                                             size_t len);
    YAJL_API yajl_gen_status yajl_gen_null(yajl_gen hand);
    YAJL_API yajl_gen_status yajl_gen_bool(yajl_gen hand, int boolean);
    YAJL_API yajl_gen_status yajl_gen_map_open(yajl_gen hand);
    YAJL_API yajl_gen_status yajl_gen_map_close(yajl_gen hand);
    YAJL_API yajl_gen_status yajl_gen_array_open(yajl_gen hand);
    YAJL_API yajl_gen_status yajl_gen_array_close(yajl_gen hand);

    
    YAJL_API yajl_gen_status yajl_gen_get_buf(yajl_gen hand,
                                              const unsigned char ** buf,
                                              size_t * len);

    
    YAJL_API void yajl_gen_clear(yajl_gen hand);

#ifdef __cplusplus
}
#endif    

#endif
