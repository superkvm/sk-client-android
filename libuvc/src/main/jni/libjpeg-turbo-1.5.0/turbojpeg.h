

#ifndef __TURBOJPEG_H__
#define __TURBOJPEG_H__

#if defined(_WIN32) && defined(DLLDEFINE)
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif
#define DLLCALL






#define TJ_NUMSAMP 6


enum TJSAMP
{
  
  TJSAMP_444=0,
  
  TJSAMP_422,
  
  TJSAMP_420,
  
  TJSAMP_GRAY,
  
  TJSAMP_440,
  
  TJSAMP_411
};


static const int tjMCUWidth[TJ_NUMSAMP]  = {8, 16, 16, 8, 8, 32};


static const int tjMCUHeight[TJ_NUMSAMP] = {8, 8, 16, 8, 16, 8};



#define TJ_NUMPF 12


enum TJPF
{
  
  TJPF_RGB=0,
  
  TJPF_BGR,
  
  TJPF_RGBX,
  
  TJPF_BGRX,
  
  TJPF_XBGR,
  
  TJPF_XRGB,
  
  TJPF_GRAY,
  
  TJPF_RGBA,
  
  TJPF_BGRA,
  
  TJPF_ABGR,
  
  TJPF_ARGB,
  
  TJPF_CMYK
};



static const int tjRedOffset[TJ_NUMPF] = {0, 2, 0, 2, 3, 1, 0, 0, 2, 3, 1, -1};

static const int tjGreenOffset[TJ_NUMPF] = {1, 1, 1, 1, 2, 2, 0, 1, 1, 2, 2, -1};

static const int tjBlueOffset[TJ_NUMPF] = {2, 0, 2, 0, 1, 3, 0, 2, 0, 1, 3, -1};


static const int tjPixelSize[TJ_NUMPF] = {3, 3, 4, 4, 4, 4, 1, 4, 4, 4, 4, 4};



#define TJ_NUMCS 5


enum TJCS
{
  
  TJCS_RGB=0,
  
  TJCS_YCbCr,
  
  TJCS_GRAY,
  
  TJCS_CMYK,
  
  TJCS_YCCK
};



#define TJFLAG_BOTTOMUP        2

#define TJFLAG_FASTUPSAMPLE  256

#define TJFLAG_NOREALLOC     1024

#define TJFLAG_FASTDCT       2048

#define TJFLAG_ACCURATEDCT   4096



#define TJ_NUMXOP 8


enum TJXOP
{
  
  TJXOP_NONE=0,
  
  TJXOP_HFLIP,
  
  TJXOP_VFLIP,
  
  TJXOP_TRANSPOSE,
  
  TJXOP_TRANSVERSE,
  
  TJXOP_ROT90,
  
  TJXOP_ROT180,
  
  TJXOP_ROT270
};



#define TJXOPT_PERFECT  1

#define TJXOPT_TRIM     2

#define TJXOPT_CROP     4

#define TJXOPT_GRAY     8

#define TJXOPT_NOOUTPUT 16



typedef struct
{
  
  int num;
  
  int denom;
} tjscalingfactor;


typedef struct
{
  
  int x;
  
  int y;
  
  int w;
  
  int h;
} tjregion;


typedef struct tjtransform
{
  
  tjregion r;
  
  int op;
  
  int options;
  
  void *data;
  
  int (*customFilter)(short *coeffs, tjregion arrayRegion,
    tjregion planeRegion, int componentIndex, int transformIndex,
    struct tjtransform *transform);
} tjtransform;


typedef void* tjhandle;



#define TJPAD(width) (((width)+3)&(~3))


#define TJSCALED(dimension, scalingFactor) ((dimension * scalingFactor.num \
  + scalingFactor.denom - 1) / scalingFactor.denom)


#ifdef __cplusplus
extern "C" {
#endif



DLLEXPORT tjhandle DLLCALL tjInitCompress(void);



DLLEXPORT int DLLCALL tjCompress2(tjhandle handle, const unsigned char *srcBuf,
  int width, int pitch, int height, int pixelFormat, unsigned char **jpegBuf,
  unsigned long *jpegSize, int jpegSubsamp, int jpegQual, int flags);



DLLEXPORT int DLLCALL tjCompressFromYUV(tjhandle handle,
  const unsigned char *srcBuf, int width, int pad, int height, int subsamp,
  unsigned char **jpegBuf, unsigned long *jpegSize, int jpegQual, int flags);



DLLEXPORT int DLLCALL tjCompressFromYUVPlanes(tjhandle handle,
  const unsigned char **srcPlanes, int width, const int *strides, int height,
  int subsamp, unsigned char **jpegBuf, unsigned long *jpegSize, int jpegQual,
  int flags);



DLLEXPORT unsigned long DLLCALL tjBufSize(int width, int height,
  int jpegSubsamp);



DLLEXPORT unsigned long DLLCALL tjBufSizeYUV2(int width, int pad, int height,
  int subsamp);



DLLEXPORT unsigned long DLLCALL tjPlaneSizeYUV(int componentID, int width,
  int stride, int height, int subsamp);



DLLEXPORT int tjPlaneWidth(int componentID, int width, int subsamp);



DLLEXPORT int tjPlaneHeight(int componentID, int height, int subsamp);



DLLEXPORT int DLLCALL tjEncodeYUV3(tjhandle handle,
  const unsigned char *srcBuf, int width, int pitch, int height,
  int pixelFormat, unsigned char *dstBuf, int pad, int subsamp, int flags);



DLLEXPORT int DLLCALL tjEncodeYUVPlanes(tjhandle handle,
  const unsigned char *srcBuf, int width, int pitch, int height,
  int pixelFormat, unsigned char **dstPlanes, int *strides, int subsamp,
  int flags);



DLLEXPORT tjhandle DLLCALL tjInitDecompress(void);



DLLEXPORT int DLLCALL tjDecompressHeader3(tjhandle handle,
  const unsigned char *jpegBuf, unsigned long jpegSize, int *width,
  int *height, int *jpegSubsamp, int *jpegColorspace);



DLLEXPORT tjscalingfactor* DLLCALL tjGetScalingFactors(int *numscalingfactors);



DLLEXPORT int DLLCALL tjDecompress2(tjhandle handle,
  const unsigned char *jpegBuf, unsigned long jpegSize, unsigned char *dstBuf,
  int width, int pitch, int height, int pixelFormat, int flags);



DLLEXPORT int DLLCALL tjDecompressToYUV2(tjhandle handle,
  const unsigned char *jpegBuf, unsigned long jpegSize, unsigned char *dstBuf,
  int width, int pad, int height, int flags);



DLLEXPORT int DLLCALL tjDecompressToYUVPlanes(tjhandle handle,
  const unsigned char *jpegBuf, unsigned long jpegSize,
  unsigned char **dstPlanes, int width, int *strides, int height, int flags);



DLLEXPORT int DLLCALL tjDecodeYUV(tjhandle handle, const unsigned char *srcBuf,
  int pad, int subsamp, unsigned char *dstBuf, int width, int pitch,
  int height, int pixelFormat, int flags);



DLLEXPORT int DLLCALL tjDecodeYUVPlanes(tjhandle handle,
  const unsigned char **srcPlanes, const int *strides, int subsamp,
  unsigned char *dstBuf, int width, int pitch, int height, int pixelFormat,
  int flags);



DLLEXPORT tjhandle DLLCALL tjInitTransform(void);



DLLEXPORT int DLLCALL tjTransform(tjhandle handle,
  const unsigned char *jpegBuf, unsigned long jpegSize, int n,
  unsigned char **dstBufs, unsigned long *dstSizes, tjtransform *transforms,
  int flags);



DLLEXPORT int DLLCALL tjDestroy(tjhandle handle);



DLLEXPORT unsigned char* DLLCALL tjAlloc(int bytes);



DLLEXPORT void DLLCALL tjFree(unsigned char *buffer);



DLLEXPORT char* DLLCALL tjGetErrorStr(void);



#define TJFLAG_FORCEMMX        8
#define TJFLAG_FORCESSE       16
#define TJFLAG_FORCESSE2      32
#define TJFLAG_FORCESSE3     128



#define NUMSUBOPT TJ_NUMSAMP
#define TJ_444 TJSAMP_444
#define TJ_422 TJSAMP_422
#define TJ_420 TJSAMP_420
#define TJ_411 TJSAMP_420
#define TJ_GRAYSCALE TJSAMP_GRAY

#define TJ_BGR 1
#define TJ_BOTTOMUP TJFLAG_BOTTOMUP
#define TJ_FORCEMMX TJFLAG_FORCEMMX
#define TJ_FORCESSE TJFLAG_FORCESSE
#define TJ_FORCESSE2 TJFLAG_FORCESSE2
#define TJ_ALPHAFIRST 64
#define TJ_FORCESSE3 TJFLAG_FORCESSE3
#define TJ_FASTUPSAMPLE TJFLAG_FASTUPSAMPLE
#define TJ_YUV 512

DLLEXPORT unsigned long DLLCALL TJBUFSIZE(int width, int height);

DLLEXPORT unsigned long DLLCALL TJBUFSIZEYUV(int width, int height,
  int jpegSubsamp);

DLLEXPORT unsigned long DLLCALL tjBufSizeYUV(int width, int height,
  int subsamp);

DLLEXPORT int DLLCALL tjCompress(tjhandle handle, unsigned char *srcBuf,
  int width, int pitch, int height, int pixelSize, unsigned char *dstBuf,
  unsigned long *compressedSize, int jpegSubsamp, int jpegQual, int flags);

DLLEXPORT int DLLCALL tjEncodeYUV(tjhandle handle,
  unsigned char *srcBuf, int width, int pitch, int height, int pixelSize,
  unsigned char *dstBuf, int subsamp, int flags);

DLLEXPORT int DLLCALL tjEncodeYUV2(tjhandle handle,
  unsigned char *srcBuf, int width, int pitch, int height, int pixelFormat,
  unsigned char *dstBuf, int subsamp, int flags);

DLLEXPORT int DLLCALL tjDecompressHeader(tjhandle handle,
  unsigned char *jpegBuf, unsigned long jpegSize, int *width, int *height);

DLLEXPORT int DLLCALL tjDecompressHeader2(tjhandle handle,
  unsigned char *jpegBuf, unsigned long jpegSize, int *width, int *height,
  int *jpegSubsamp);

DLLEXPORT int DLLCALL tjDecompress(tjhandle handle,
  unsigned char *jpegBuf, unsigned long jpegSize, unsigned char *dstBuf,
  int width, int pitch, int height, int pixelSize, int flags);

DLLEXPORT int DLLCALL tjDecompressToYUV(tjhandle handle,
  unsigned char *jpegBuf, unsigned long jpegSize, unsigned char *dstBuf,
  int flags);




#ifdef __cplusplus
}
#endif

#endif
