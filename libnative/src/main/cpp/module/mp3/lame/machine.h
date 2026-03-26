

#ifndef LAME_MACHINE_H
#define LAME_MACHINE_H

#include "version.h"

#if (LAME_RELEASE_VERSION == 0)
#undef NDEBUG
#endif

#include <stdio.h>
#include <assert.h>

#ifdef STDC_HEADERS
# include <stdlib.h>
# include <string.h>
#else









#endif

#if  defined(__riscos__)  &&  defined(FPA10)
# include "ymath.h"
#else
# include <math.h>
#endif
#include <limits.h>

#include <ctype.h>

#ifdef HAVE_ERRNO_H
# include <errno.h>
#endif
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

#if defined(macintosh)
# include <types.h>
# include <stat.h>
#else
# include <sys/types.h>
# include <sys/stat.h>
#endif

#ifdef HAVE_INTTYPES_H
# include <inttypes.h>
#else
# ifdef HAVE_STDINT_H
#  include <stdint.h>
# endif
#endif

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif



#define POW20(x) (assert(0 <= (x+Q_MAX2) && x < Q_MAX), pow20[x+Q_MAX2])



#define IPOW20(x)  (assert(0 <= x && x < Q_MAX), ipow20[x])




#ifndef inline
# define inline
#endif

#if defined(_MSC_VER)
# undef inline
# define inline _inline
#elif defined(__SASC) || defined(__GNUC__) || defined(__ICC) || defined(__ECC)

# undef inline
# define inline __inline
#endif

#if    defined(_MSC_VER)
# pragma warning( disable : 4244 )

#endif



#if ( defined(_MSC_VER) || defined(__BORLANDC__) || defined(__MINGW32__) )
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
# include <float.h>
# define FLOAT_MAX FLT_MAX
#else
# ifndef FLOAT
typedef float FLOAT;
#  ifdef FLT_MAX
#   define FLOAT_MAX FLT_MAX
#  else
#   define FLOAT_MAX 1e37 
#  endif
# endif
#endif

#ifndef FLOAT8
typedef double FLOAT8;
# ifdef DBL_MAX
#  define FLOAT8_MAX DBL_MAX
# else
#  define FLOAT8_MAX 1e99 
# endif
#else
# ifdef FLT_MAX
#  define FLOAT8_MAX FLT_MAX
# else
#  define FLOAT8_MAX 1e37 
# endif
#endif


typedef FLOAT sample_t;

#define dimension_of(array) (sizeof(array)/sizeof(array[0]))
#define beyond(array) (array+dimension_of(array))
#define compiletime_assert(expression) extern char static_assert_##FILE##_##LINE[expression?1:0]

#if 1
#define EQ(a,b) (\
(fabs(a) > fabs(b)) \
 ? (fabs((a)-(b)) <= (fabs(a) * 1e-6f)) \
 : (fabs((a)-(b)) <= (fabs(b) * 1e-6f)))
#else
#define EQ(a,b) (fabs((a)-(b))<1E-37)
#endif

#define NEQ(a,b) (!EQ(a,b))

#endif

#ifdef _MSC_VER
#  if _MSC_VER < 1400
#  define fabsf fabs
#  define powf pow
#  define log10f log10
#  endif
#endif



