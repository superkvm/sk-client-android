



#ifndef __ULTRAJSON_H__
#define __ULTRAJSON_H__

#include <stdio.h>
#include <wchar.h>




#define JSON_NO_EXTRA_WHITESPACE


#ifndef JSON_DOUBLE_MAX_DECIMALS
#define JSON_DOUBLE_MAX_DECIMALS 9
#endif


#ifndef JSON_MAX_RECURSION_DEPTH
#define JSON_MAX_RECURSION_DEPTH 256
#endif


#ifndef JSON_MAX_STACK_BUFFER_SIZE
#define JSON_MAX_STACK_BUFFER_SIZE 131072
#endif

#ifdef _WIN32

typedef __int64 JSINT64;
typedef unsigned __int64 JSUINT64;

typedef unsigned __int32 uint32_t;
typedef __int32 JSINT32;
typedef uint32_t JSUINT32;
typedef unsigned __int8 JSUINT8;
typedef unsigned __int16 JSUTF16;
typedef unsigned __int32 JSUTF32;
typedef __int64 JSLONG;

#define EXPORTFUNCTION __declspec(dllexport)

#define FASTCALL_MSVC __fastcall
#define FASTCALL_ATTR 
#define INLINE_PREFIX __inline

#else

#include <sys/types.h>
typedef int64_t JSINT64;
typedef u_int64_t JSUINT64;

typedef int32_t JSINT32;
typedef u_int32_t JSUINT32;

#define FASTCALL_MSVC 
#define FASTCALL_ATTR __attribute__((fastcall))
#define INLINE_PREFIX inline

typedef u_int32_t uint32_t;

typedef u_int8_t JSUINT8;
typedef u_int16_t JSUTF16;
typedef u_int32_t JSUTF32;

typedef int64_t JSLONG;

#define EXPORTFUNCTION
#endif

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define __LITTLE_ENDIAN__
#else

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define __BIG_ENDIAN__
#endif

#endif

#if !defined(__LITTLE_ENDIAN__) && !defined(__BIG_ENDIAN__)
#error "Endianess not supported"
#endif

enum JSTYPES
{
	JT_NULL,		
	JT_TRUE,		
	JT_FALSE,		
	JT_INT,			
	JT_LONG,		
	JT_DOUBLE,	
	JT_UTF8,		
	JT_ARRAY,		
	JT_OBJECT,	
	JT_INVALID,	
};

typedef void * JSOBJ;
typedef void * JSITER;

typedef struct __JSONTypeContext
{
	int type;
	void *prv[32];
} JSONTypeContext;


typedef void (*JSPFN_ITERBEGIN)(JSOBJ obj, JSONTypeContext *tc);
typedef int (*JSPFN_ITERNEXT)(JSOBJ obj, JSONTypeContext *tc);
typedef void (*JSPFN_ITEREND)(JSOBJ obj, JSONTypeContext *tc);
typedef JSOBJ (*JSPFN_ITERGETVALUE)(JSOBJ obj, JSONTypeContext *tc);
typedef char *(*JSPFN_ITERGETNAME)(JSOBJ obj, JSONTypeContext *tc, size_t *outLen);
typedef	void *(*JSPFN_MALLOC)(size_t size);
typedef void (*JSPFN_FREE)(void *pptr);
typedef void *(*JSPFN_REALLOC)(void *base, size_t size);

typedef struct __JSONObjectEncoder
{
	void (*beginTypeContext)(JSOBJ obj, JSONTypeContext *tc);
	void (*endTypeContext)(JSOBJ obj, JSONTypeContext *tc);
	const char *(*getStringValue)(JSOBJ obj, JSONTypeContext *tc, size_t *_outLen);
	JSINT64 (*getLongValue)(JSOBJ obj, JSONTypeContext *tc);
	JSINT32 (*getIntValue)(JSOBJ obj, JSONTypeContext *tc);
	double (*getDoubleValue)(JSOBJ obj, JSONTypeContext *tc);

	
	JSPFN_ITERBEGIN iterBegin;

	
	JSPFN_ITERNEXT iterNext;

	
	JSPFN_ITEREND iterEnd;

	
	JSPFN_ITERGETVALUE iterGetValue;
	
	
	JSPFN_ITERGETNAME iterGetName;
	
	
	void (*releaseObject)(JSOBJ obj);

	
	JSPFN_MALLOC malloc;
	JSPFN_REALLOC realloc;
	JSPFN_FREE free;

	
	int recursionMax;

	
	int doublePrecision;

	
	int forceASCII;


	
	const char *errorMsg;
	JSOBJ errorObj;

	
	char *start;
	char *offset;
	char *end;
	int heap;
	int level;

} JSONObjectEncoder;



EXPORTFUNCTION char *JSON_EncodeObject(JSOBJ obj, JSONObjectEncoder *enc, char *buffer, size_t cbBuffer);



typedef struct __JSONObjectDecoder
{
	JSOBJ (*newString)(wchar_t *start, wchar_t *end);
	void (*objectAddKey)(JSOBJ obj, JSOBJ name, JSOBJ value);
	void (*arrayAddItem)(JSOBJ obj, JSOBJ value);
	JSOBJ (*newTrue)(void);
	JSOBJ (*newFalse)(void);
	JSOBJ (*newNull)(void);
	JSOBJ (*newObject)(void);
	JSOBJ (*newArray)(void);
	JSOBJ (*newInt)(JSINT32 value);
	JSOBJ (*newLong)(JSINT64 value);
	JSOBJ (*newDouble)(double value);
	void (*releaseObject)(JSOBJ obj);
	JSPFN_MALLOC malloc;
	JSPFN_FREE free;
	JSPFN_REALLOC realloc;

	char *errorStr;
	char *errorOffset;



} JSONObjectDecoder;

EXPORTFUNCTION JSOBJ JSON_DecodeObject(JSONObjectDecoder *dec, const char *buffer, size_t cbBuffer);

#endif