

#ifndef __YAJL_PARSER_H__
#define __YAJL_PARSER_H__

#include "api/yajl_parse.h"
#include "yajl_bytestack.h"
#include "yajl_buf.h"
#include "yajl_lex.h"


typedef enum {
    yajl_state_start = 0,
    yajl_state_parse_complete,
    yajl_state_parse_error,
    yajl_state_lexical_error,
    yajl_state_map_start,
    yajl_state_map_sep,
    yajl_state_map_need_val,
    yajl_state_map_got_val,
    yajl_state_map_need_key,
    yajl_state_array_start,
    yajl_state_array_got_val,
    yajl_state_array_need_val,
    yajl_state_got_value,
} yajl_state;

struct yajl_handle_t {
    const yajl_callbacks * callbacks;
    void * ctx;
    yajl_lexer lexer;
    const char * parseError;
    
    size_t bytesConsumed;
    
    yajl_buf decodeBuf;
    
    yajl_bytestack stateStack;
    
    yajl_alloc_funcs alloc;
    
    unsigned int flags;
};

yajl_status
yajl_do_parse(yajl_handle handle, const unsigned char * jsonText,
              size_t jsonTextLen);

yajl_status
yajl_do_finish(yajl_handle handle);

unsigned char *
yajl_render_error_string(yajl_handle hand, const unsigned char * jsonText,
                         size_t jsonTextLen, int verbose);


long long
yajl_parse_integer(const unsigned char *number, unsigned int length);


#endif
