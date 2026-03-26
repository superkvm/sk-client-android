

#ifndef __YAJL_ENCODE_H__
#define __YAJL_ENCODE_H__

#include "yajl_buf.h"
#include "api/yajl_gen.h"

void yajl_string_encode(const yajl_print_t printer,
                        void * ctx,
                        const unsigned char * str,
                        size_t length,
                        int escape_solidus);

void yajl_string_decode(yajl_buf buf, const unsigned char * str,
                        size_t length);

int yajl_string_validate_utf8(const unsigned char * s, size_t len);

#endif
