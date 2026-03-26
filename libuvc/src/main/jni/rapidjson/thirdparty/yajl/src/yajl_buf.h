

#ifndef __YAJL_BUF_H__
#define __YAJL_BUF_H__

#include "api/yajl_common.h"
#include "yajl_alloc.h"




typedef struct yajl_buf_t * yajl_buf;


yajl_buf yajl_buf_alloc(yajl_alloc_funcs * alloc);


void yajl_buf_free(yajl_buf buf);


void yajl_buf_append(yajl_buf buf, const void * data, size_t len);


void yajl_buf_clear(yajl_buf buf);


const unsigned char * yajl_buf_data(yajl_buf buf);


size_t yajl_buf_len(yajl_buf buf);


void yajl_buf_truncate(yajl_buf buf, size_t len);

#endif
