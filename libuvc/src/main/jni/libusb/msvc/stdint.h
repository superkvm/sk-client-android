



#ifndef _MSC_VER
#error This header should only be used with Microsoft compilers
#endif

#ifndef _STDINT_H
#define _STDINT_H

#ifndef _INTPTR_T_DEFINED
#define _INTPTR_T_DEFINED
#ifndef __intptr_t_defined
#define __intptr_t_defined
#undef intptr_t
#ifdef _WIN64
  typedef __int64 intptr_t;
#else
  typedef int intptr_t;
#endif 
#endif 
#endif 

#ifndef _UINTPTR_T_DEFINED
#define _UINTPTR_T_DEFINED
#ifndef __uintptr_t_defined
#define __uintptr_t_defined
#undef uintptr_t
#ifdef _WIN64
  typedef unsigned __int64 uintptr_t;
#else
  typedef unsigned int uintptr_t;
#endif 
#endif 
#endif 

#ifndef _PTRDIFF_T_DEFINED
#define _PTRDIFF_T_DEFINED
#ifndef _PTRDIFF_T_
#define _PTRDIFF_T_
#undef ptrdiff_t
#ifdef _WIN64
  typedef __int64 ptrdiff_t;
#else
  typedef int ptrdiff_t;
#endif 
#endif 
#endif 

#ifndef _WCHAR_T_DEFINED
#define _WCHAR_T_DEFINED
#ifndef __cplusplus
  typedef unsigned short wchar_t;
#endif 
#endif 

#ifndef _WCTYPE_T_DEFINED
#define _WCTYPE_T_DEFINED
#ifndef _WINT_T
#define _WINT_T
  typedef unsigned short wint_t;
  typedef unsigned short wctype_t;
#endif 
#endif 


typedef __int8 int8_t;
typedef unsigned __int8   uint8_t;
typedef __int16  int16_t;
typedef unsigned __int16  uint16_t;
typedef __int32  int32_t;
typedef unsigned __int32  uint32_t;
typedef __int64  int64_t;
typedef unsigned __int64   uint64_t;


typedef signed char int_least8_t;
typedef unsigned char   uint_least8_t;
typedef short  int_least16_t;
typedef unsigned short  uint_least16_t;
typedef int  int_least32_t;
typedef unsigned   uint_least32_t;
typedef __int64  int_least64_t;
typedef unsigned __int64   uint_least64_t;


typedef __int8 int_fast8_t;
typedef unsigned __int8 uint_fast8_t;
typedef __int16  int_fast16_t;
typedef unsigned __int16  uint_fast16_t;
typedef __int32  int_fast32_t;
typedef unsigned  __int32  uint_fast32_t;
typedef __int64  int_fast64_t;
typedef unsigned __int64   uint_fast64_t;


typedef __int64  intmax_t;
typedef unsigned __int64   uintmax_t;




#define INT8_MIN (-128)
#define INT16_MIN (-32768)
#define INT32_MIN (-2147483647 - 1)
#define INT64_MIN  (-9223372036854775807LL - 1)

#define INT8_MAX 127
#define INT16_MAX 32767
#define INT32_MAX 2147483647
#define INT64_MAX 9223372036854775807LL

#define UINT8_MAX 255
#define UINT16_MAX 65535
#define UINT32_MAX 0xffffffffU  
#define UINT64_MAX 0xffffffffffffffffULL 


#define INT_LEAST8_MIN INT8_MIN
#define INT_LEAST16_MIN INT16_MIN
#define INT_LEAST32_MIN INT32_MIN
#define INT_LEAST64_MIN INT64_MIN

#define INT_LEAST8_MAX INT8_MAX
#define INT_LEAST16_MAX INT16_MAX
#define INT_LEAST32_MAX INT32_MAX
#define INT_LEAST64_MAX INT64_MAX

#define UINT_LEAST8_MAX UINT8_MAX
#define UINT_LEAST16_MAX UINT16_MAX
#define UINT_LEAST32_MAX UINT32_MAX
#define UINT_LEAST64_MAX UINT64_MAX


#define INT_FAST8_MIN INT8_MIN
#define INT_FAST16_MIN INT16_MIN
#define INT_FAST32_MIN INT32_MIN
#define INT_FAST64_MIN INT64_MIN

#define INT_FAST8_MAX INT8_MAX
#define INT_FAST16_MAX INT16_MAX
#define INT_FAST32_MAX INT32_MAX
#define INT_FAST64_MAX INT64_MAX

#define UINT_FAST8_MAX UINT8_MAX
#define UINT_FAST16_MAX UINT16_MAX
#define UINT_FAST32_MAX UINT32_MAX
#define UINT_FAST64_MAX UINT64_MAX


#ifdef _WIN64
#define INTPTR_MIN INT64_MIN
#define INTPTR_MAX INT64_MAX
#define UINTPTR_MAX UINT64_MAX
#else
#define INTPTR_MIN INT32_MIN
#define INTPTR_MAX INT32_MAX
#define UINTPTR_MAX UINT32_MAX
#endif


#define INTMAX_MIN INT64_MIN
#define INTMAX_MAX INT64_MAX
#define UINTMAX_MAX UINT64_MAX


#ifdef _WIN64
#define PTRDIFF_MIN INT64_MIN
#define PTRDIFF_MAX INT64_MAX
#else
#define PTRDIFF_MIN INT32_MIN
#define PTRDIFF_MAX INT32_MAX
#endif

#define SIG_ATOMIC_MIN INT32_MIN
#define SIG_ATOMIC_MAX INT32_MAX

#ifndef SIZE_MAX
#ifdef _WIN64
#define SIZE_MAX UINT64_MAX
#else
#define SIZE_MAX UINT32_MAX
#endif
#endif

#ifndef WCHAR_MIN  
#define WCHAR_MIN 0U
#define WCHAR_MAX 0xffffU
#endif


#define WINT_MIN 0U
#define WINT_MAX 0xffffU






#define INT8_C(val) (INT_LEAST8_MAX-INT_LEAST8_MAX+(val))
#define INT16_C(val) (INT_LEAST16_MAX-INT_LEAST16_MAX+(val))
#define INT32_C(val) (INT_LEAST32_MAX-INT_LEAST32_MAX+(val))

#define INT64_C(val) val##i64

#define UINT8_C(val) (val)
#define UINT16_C(val) (val)
#define UINT32_C(val) (val##i32)
#define UINT64_C(val) val##ui64


#define INTMAX_C(val) val##i64
#define UINTMAX_C(val) val##ui64

#endif
