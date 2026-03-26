

#define JPEG_INTERNALS
#include "../jinclude.h"
#include "../jpeglib.h"
#include "../jsimd.h"
#include "../jdct.h"
#include "../jsimddct.h"
#include "jsimd.h"
#include <altivec.h>




#define __4X(a) a, a, a, a
#define __4X2(a, b) a, b, a, b, a, b, a, b
#define __8X(a) __4X(a), __4X(a)
#define __16X(a) __8X(a), __8X(a)

#define TRANSPOSE(row, col)  \
{  \
  __vector short row04l, row04h, row15l, row15h,  \
                 row26l, row26h, row37l, row37h;  \
  __vector short col01e, col01o, col23e, col23o,  \
                 col45e, col45o, col67e, col67o;  \
  \
                                        \
  row04l = vec_mergeh(row##0, row##4);  \
  row04h = vec_mergel(row##0, row##4);  \
  row15l = vec_mergeh(row##1, row##5);  \
  row15h = vec_mergel(row##1, row##5);  \
  row26l = vec_mergeh(row##2, row##6);  \
  row26h = vec_mergel(row##2, row##6);  \
  row37l = vec_mergeh(row##3, row##7);  \
  row37h = vec_mergel(row##3, row##7);  \
  \
                                        \
  col01e = vec_mergeh(row04l, row26l);  \
  col23e = vec_mergel(row04l, row26l);  \
  col45e = vec_mergeh(row04h, row26h);  \
  col67e = vec_mergel(row04h, row26h);  \
  col01o = vec_mergeh(row15l, row37l);  \
  col23o = vec_mergel(row15l, row37l);  \
  col45o = vec_mergeh(row15h, row37h);  \
  col67o = vec_mergel(row15h, row37h);  \
  \
                                        \
  col##0 = vec_mergeh(col01e, col01o);    \
  col##1 = vec_mergel(col01e, col01o);    \
  col##2 = vec_mergeh(col23e, col23o);    \
  col##3 = vec_mergel(col23e, col23o);    \
  col##4 = vec_mergeh(col45e, col45o);    \
  col##5 = vec_mergel(col45e, col45o);    \
  col##6 = vec_mergeh(col67e, col67o);    \
  col##7 = vec_mergel(col67e, col67o);    \
}

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif




#if __BIG_ENDIAN__

#define VEC_LD(a, b) vec_ld(a, b)
#define VEC_ST(a, b, c) vec_st(a, b, c)
#define VEC_UNPACKHU(a) vec_mergeh(pb_zero, a)
#define VEC_UNPACKLU(a) vec_mergel(pb_zero, a)

#else

#define VEC_LD(a, b) vec_vsx_ld(a, b)
#define VEC_ST(a, b, c) vec_vsx_st(a, b, c)
#define VEC_UNPACKHU(a) vec_mergeh(a, pb_zero)
#define VEC_UNPACKLU(a) vec_mergel(a, pb_zero)

#endif
