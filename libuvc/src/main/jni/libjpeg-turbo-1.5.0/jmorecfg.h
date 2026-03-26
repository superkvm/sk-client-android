




#define MAX_COMPONENTS  10      






#if BITS_IN_JSAMPLE == 8


#ifdef HAVE_UNSIGNED_CHAR

typedef unsigned char JSAMPLE;
#define GETJSAMPLE(value)  ((int) (value))

#else 

typedef char JSAMPLE;
#ifdef __CHAR_UNSIGNED__
#define GETJSAMPLE(value)  ((int) (value))
#else
#define GETJSAMPLE(value)  ((int) (value) & 0xFF)
#endif 

#endif 

#define MAXJSAMPLE      255
#define CENTERJSAMPLE   128

#endif 


#if BITS_IN_JSAMPLE == 12


typedef short JSAMPLE;
#define GETJSAMPLE(value)  ((int) (value))

#define MAXJSAMPLE      4095
#define CENTERJSAMPLE   2048

#endif 




typedef short JCOEF;




#ifdef HAVE_UNSIGNED_CHAR

typedef unsigned char JOCTET;
#define GETJOCTET(value)  (value)

#else 

typedef char JOCTET;
#ifdef __CHAR_UNSIGNED__
#define GETJOCTET(value)  (value)
#else
#define GETJOCTET(value)  ((value) & 0xFF)
#endif 

#endif 






#ifdef HAVE_UNSIGNED_CHAR
typedef unsigned char UINT8;
#else 
#ifdef __CHAR_UNSIGNED__
typedef char UINT8;
#else 
typedef short UINT8;
#endif 
#endif 



#ifdef HAVE_UNSIGNED_SHORT
typedef unsigned short UINT16;
#else 
typedef unsigned int UINT16;
#endif 



#ifndef XMD_H                   
typedef short INT16;
#endif



#ifndef XMD_H                   
#ifndef _BASETSD_H_		
#ifndef _BASETSD_H		
#ifndef QGLOBAL_H		
typedef long INT32;
#endif
#endif
#endif
#endif



typedef unsigned int JDIMENSION;

#define JPEG_MAX_DIMENSION  65500L  





#define METHODDEF(type)         static type

#define LOCAL(type)             static type

#define GLOBAL(type)            type

#define EXTERN(type)            extern type




#define JMETHOD(type,methodname,arglist)  type (*methodname) arglist




#undef FAR
#define FAR




#ifndef HAVE_BOOLEAN
typedef int boolean;
#endif
#ifndef FALSE                   
#define FALSE   0               
#endif
#ifndef TRUE
#define TRUE    1
#endif




#ifdef JPEG_INTERNALS
#define JPEG_INTERNAL_OPTIONS
#endif

#ifdef JPEG_INTERNAL_OPTIONS






#define DCT_ISLOW_SUPPORTED     
#define DCT_IFAST_SUPPORTED     
#define DCT_FLOAT_SUPPORTED     



#define C_MULTISCAN_FILES_SUPPORTED 
#define C_PROGRESSIVE_SUPPORTED     
#define ENTROPY_OPT_SUPPORTED       

#define INPUT_SMOOTHING_SUPPORTED   



#define D_MULTISCAN_FILES_SUPPORTED 
#define D_PROGRESSIVE_SUPPORTED     
#define SAVE_MARKERS_SUPPORTED      
#define BLOCK_SMOOTHING_SUPPORTED   
#define IDCT_SCALING_SUPPORTED      
#undef  UPSAMPLE_SCALING_SUPPORTED  
#define UPSAMPLE_MERGING_SUPPORTED  
#define QUANT_1PASS_SUPPORTED       
#define QUANT_2PASS_SUPPORTED       






#define RGB_RED         0       
#define RGB_GREEN       1       
#define RGB_BLUE        2       
#define RGB_PIXELSIZE   3       

#define JPEG_NUMCS 17

#define EXT_RGB_RED        0
#define EXT_RGB_GREEN      1
#define EXT_RGB_BLUE       2
#define EXT_RGB_PIXELSIZE  3

#define EXT_RGBX_RED       0
#define EXT_RGBX_GREEN     1
#define EXT_RGBX_BLUE      2
#define EXT_RGBX_PIXELSIZE 4

#define EXT_BGR_RED        2
#define EXT_BGR_GREEN      1
#define EXT_BGR_BLUE       0
#define EXT_BGR_PIXELSIZE  3

#define EXT_BGRX_RED       2
#define EXT_BGRX_GREEN     1
#define EXT_BGRX_BLUE      0
#define EXT_BGRX_PIXELSIZE 4

#define EXT_XBGR_RED       3
#define EXT_XBGR_GREEN     2
#define EXT_XBGR_BLUE      1
#define EXT_XBGR_PIXELSIZE 4

#define EXT_XRGB_RED       1
#define EXT_XRGB_GREEN     2
#define EXT_XRGB_BLUE      3
#define EXT_XRGB_PIXELSIZE 4

static const int rgb_red[JPEG_NUMCS] = {
  -1, -1, RGB_RED, -1, -1, -1, EXT_RGB_RED, EXT_RGBX_RED,
  EXT_BGR_RED, EXT_BGRX_RED, EXT_XBGR_RED, EXT_XRGB_RED,
  EXT_RGBX_RED, EXT_BGRX_RED, EXT_XBGR_RED, EXT_XRGB_RED,
  -1
};

static const int rgb_green[JPEG_NUMCS] = {
  -1, -1, RGB_GREEN, -1, -1, -1, EXT_RGB_GREEN, EXT_RGBX_GREEN,
  EXT_BGR_GREEN, EXT_BGRX_GREEN, EXT_XBGR_GREEN, EXT_XRGB_GREEN,
  EXT_RGBX_GREEN, EXT_BGRX_GREEN, EXT_XBGR_GREEN, EXT_XRGB_GREEN,
  -1
};

static const int rgb_blue[JPEG_NUMCS] = {
  -1, -1, RGB_BLUE, -1, -1, -1, EXT_RGB_BLUE, EXT_RGBX_BLUE,
  EXT_BGR_BLUE, EXT_BGRX_BLUE, EXT_XBGR_BLUE, EXT_XRGB_BLUE,
  EXT_RGBX_BLUE, EXT_BGRX_BLUE, EXT_XBGR_BLUE, EXT_XRGB_BLUE,
  -1
};

static const int rgb_pixelsize[JPEG_NUMCS] = {
  -1, -1, RGB_PIXELSIZE, -1, -1, -1, EXT_RGB_PIXELSIZE, EXT_RGBX_PIXELSIZE,
  EXT_BGR_PIXELSIZE, EXT_BGRX_PIXELSIZE, EXT_XBGR_PIXELSIZE, EXT_XRGB_PIXELSIZE,
  EXT_RGBX_PIXELSIZE, EXT_BGRX_PIXELSIZE, EXT_XBGR_PIXELSIZE, EXT_XRGB_PIXELSIZE,
  -1
};





#ifndef MULTIPLIER
#ifndef WITH_SIMD
#define MULTIPLIER  int         
#else
#define MULTIPLIER short  
#endif
#endif




#ifndef FAST_FLOAT
#define FAST_FLOAT  float
#endif

#endif 
