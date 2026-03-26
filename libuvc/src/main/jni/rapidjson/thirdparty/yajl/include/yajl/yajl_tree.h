



#ifndef YAJL_TREE_H
#define YAJL_TREE_H 1

#include <yajl/yajl_common.h>


typedef enum {
    yajl_t_string = 1,
    yajl_t_number = 2,
    yajl_t_object = 3,
    yajl_t_array = 4,
    yajl_t_true = 5,
    yajl_t_false = 6,
    yajl_t_null = 7,
    
    yajl_t_any = 8
} yajl_type;

#define YAJL_NUMBER_INT_VALID    0x01
#define YAJL_NUMBER_DOUBLE_VALID 0x02


typedef struct yajl_val_s * yajl_val;


struct yajl_val_s
{
    
    yajl_type type;
    
    union
    {
        char * string;
        struct {
            long long i; 
            double  d;   
            
            char   *r;   
            unsigned int flags;
        } number;
        struct {
            const char **keys; 
            yajl_val *values; 
            size_t len; 
        } object;
        struct {
            yajl_val *values; 
            size_t len; 
        } array;
    } u;
};


YAJL_API yajl_val yajl_tree_parse (const char *input,
                                   char *error_buffer, size_t error_buffer_size);


YAJL_API void yajl_tree_free (yajl_val v);


YAJL_API yajl_val yajl_tree_get(yajl_val parent, const char ** path, yajl_type type);


#define YAJL_IS_STRING(v) (((v) != NULL) && ((v)->type == yajl_t_string))
#define YAJL_IS_NUMBER(v) (((v) != NULL) && ((v)->type == yajl_t_number))
#define YAJL_IS_INTEGER(v) (YAJL_IS_NUMBER(v) && ((v)->u.flags & YAJL_NUMBER_INT_VALID))
#define YAJL_IS_DOUBLE(v) (YAJL_IS_NUMBER(v) && ((v)->u.flags & YAJL_NUMBER_DOUBLE_VALID))
#define YAJL_IS_OBJECT(v) (((v) != NULL) && ((v)->type == yajl_t_object))
#define YAJL_IS_ARRAY(v)  (((v) != NULL) && ((v)->type == yajl_t_array ))
#define YAJL_IS_TRUE(v)   (((v) != NULL) && ((v)->type == yajl_t_true  ))
#define YAJL_IS_FALSE(v)  (((v) != NULL) && ((v)->type == yajl_t_false ))
#define YAJL_IS_NULL(v)   (((v) != NULL) && ((v)->type == yajl_t_null  ))


#define YAJL_GET_STRING(v) (YAJL_IS_STRING(v) ? (v)->u.string : NULL)


#define YAJL_GET_NUMBER(v) ((v)->u.number.r)


#define YAJL_GET_DOUBLE(v) ((v)->u.number.d)


#define YAJL_GET_INTEGER(v) ((v)->u.number.i)


#define YAJL_GET_OBJECT(v) (YAJL_IS_OBJECT(v) ? &(v)->u.object : NULL)


#define YAJL_GET_ARRAY(v)  (YAJL_IS_ARRAY(v)  ? &(v)->u.array  : NULL)

#endif 
