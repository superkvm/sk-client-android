



#include <yajl/yajl_common.h>

#ifndef __YAJL_PARSE_H__
#define __YAJL_PARSE_H__

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif
    
    typedef enum {
        
        yajl_status_ok,
        
        yajl_status_client_canceled,
        
        yajl_status_error
    } yajl_status;

    
    YAJL_API const char * yajl_status_to_string(yajl_status code);

    
    typedef struct yajl_handle_t * yajl_handle;

    
    typedef struct {
        int (* yajl_null)(void * ctx);
        int (* yajl_boolean)(void * ctx, int boolVal);
        int (* yajl_integer)(void * ctx, long long integerVal);
        int (* yajl_double)(void * ctx, double doubleVal);
        
        int (* yajl_number)(void * ctx, const char * numberVal,
                            size_t numberLen);

        
        int (* yajl_string)(void * ctx, const unsigned char * stringVal,
                            size_t stringLen);

        int (* yajl_start_map)(void * ctx);
        int (* yajl_map_key)(void * ctx, const unsigned char * key,
                             size_t stringLen);
        int (* yajl_end_map)(void * ctx);

        int (* yajl_start_array)(void * ctx);
        int (* yajl_end_array)(void * ctx);
    } yajl_callbacks;

    
    YAJL_API yajl_handle yajl_alloc(const yajl_callbacks * callbacks,
                                    yajl_alloc_funcs * afs,
                                    void * ctx);


    
    typedef enum {
        
        yajl_allow_comments = 0x01,
        
        yajl_dont_validate_strings     = 0x02,
        
        yajl_allow_trailing_garbage = 0x04,
        
        yajl_allow_multiple_values = 0x08,
        
        yajl_allow_partial_values = 0x10
    } yajl_option;

    
    YAJL_API int yajl_config(yajl_handle h, yajl_option opt, ...);

    
    YAJL_API void yajl_free(yajl_handle handle);

    
    YAJL_API yajl_status yajl_parse(yajl_handle hand,
                                    const unsigned char * jsonText,
                                    size_t jsonTextLength);

    
    YAJL_API yajl_status yajl_complete_parse(yajl_handle hand);

    
    YAJL_API unsigned char * yajl_get_error(yajl_handle hand, int verbose,
                                            const unsigned char * jsonText,
                                            size_t jsonTextLength);

    
    YAJL_API size_t yajl_get_bytes_consumed(yajl_handle hand);

    
    YAJL_API void yajl_free_error(yajl_handle hand, unsigned char * str);

#ifdef __cplusplus
}
#endif

#endif
