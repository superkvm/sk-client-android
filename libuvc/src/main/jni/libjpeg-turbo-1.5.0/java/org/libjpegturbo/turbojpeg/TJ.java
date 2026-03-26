

package org.libjpegturbo.turbojpeg;


public final class TJ {


  
  public static final int NUMSAMP   = 6;
  
  public static final int SAMP_444  = 0;
  
  public static final int SAMP_422  = 1;
  
  public static final int SAMP_420  = 2;
  
  public static final int SAMP_GRAY = 3;
  
  public static final int SAMP_440  = 4;
  
  public static final int SAMP_411  = 5;


  
  public static int getMCUWidth(int subsamp) {
    checkSubsampling(subsamp);
    return mcuWidth[subsamp];
  }

  private static final int[] mcuWidth = {
    8, 16, 16, 8, 8, 32
  };


  
  public static int getMCUHeight(int subsamp) {
    checkSubsampling(subsamp);
    return mcuHeight[subsamp];
  }

  private static final int[] mcuHeight = {
    8, 8, 16, 8, 16, 8
  };


  
  public static final int NUMPF   = 12;
  
  public static final int PF_RGB  = 0;
  
  public static final int PF_BGR  = 1;
  
  public static final int PF_RGBX = 2;
  
  public static final int PF_BGRX = 3;
  
  public static final int PF_XBGR = 4;
  
  public static final int PF_XRGB = 5;
  
  public static final int PF_GRAY = 6;
  
  public static final int PF_RGBA = 7;
  
  public static final int PF_BGRA = 8;
  
  public static final int PF_ABGR = 9;
  
  public static final int PF_ARGB = 10;
  
  public static final int PF_CMYK = 11;


  
  public static int getPixelSize(int pixelFormat) {
    checkPixelFormat(pixelFormat);
    return pixelSize[pixelFormat];
  }

  private static final int[] pixelSize = {
    3, 3, 4, 4, 4, 4, 1, 4, 4, 4, 4, 4
  };


  
  public static int getRedOffset(int pixelFormat) {
    checkPixelFormat(pixelFormat);
    return redOffset[pixelFormat];
  }

  private static final int[] redOffset = {
    0, 2, 0, 2, 3, 1, 0, 0, 2, 3, 1, -1
  };


  
  public static int getGreenOffset(int pixelFormat) {
    checkPixelFormat(pixelFormat);
    return greenOffset[pixelFormat];
  }

  private static final int[] greenOffset = {
    1, 1, 1, 1, 2, 2, 0, 1, 1, 2, 2, -1
  };


  
  public static int getBlueOffset(int pixelFormat) {
    checkPixelFormat(pixelFormat);
    return blueOffset[pixelFormat];
  }

  private static final int[] blueOffset = {
    2, 0, 2, 0, 1, 3, 0, 2, 0, 1, 3, -1
  };


  
  public static final int NUMCS = 5;
  
  public static final int CS_RGB = 0;
  
  public static final int CS_YCbCr = 1;
  
  public static final int CS_GRAY = 2;
  
  public static final int CS_CMYK = 3;
  
  public static final int CS_YCCK = 4;


  
  public static final int FLAG_BOTTOMUP     = 2;

  @Deprecated
  public static final int FLAG_FORCEMMX     = 8;
  @Deprecated
  public static final int FLAG_FORCESSE     = 16;
  @Deprecated
  public static final int FLAG_FORCESSE2    = 32;
  @Deprecated
  public static final int FLAG_FORCESSE3    = 128;

  
  public static final int FLAG_FASTUPSAMPLE = 256;
  
  public static final int FLAG_FASTDCT      =  2048;
  
  public static final int FLAG_ACCURATEDCT  =  4096;


  
  public static native int bufSize(int width, int height, int jpegSubsamp);

  
  public static native int bufSizeYUV(int width, int pad, int height,
                                      int subsamp);

  
  @Deprecated
  public static native int bufSizeYUV(int width, int height, int subsamp);

  
  public static native int planeSizeYUV(int componentID, int width, int stride,
                                        int height, int subsamp);

  
  public static native int planeWidth(int componentID, int width, int subsamp);

  
  public static native int planeHeight(int componentID, int height,
                                       int subsamp);

  
  public static native TJScalingFactor[] getScalingFactors();

  static {
    TJLoader.load();
  }

  private static void checkPixelFormat(int pixelFormat) {
    if (pixelFormat < 0 || pixelFormat >= NUMPF)
      throw new IllegalArgumentException("Invalid pixel format");
  }

  private static void checkSubsampling(int subsamp) {
    if (subsamp < 0 || subsamp >= NUMSAMP)
      throw new IllegalArgumentException("Invalid subsampling type");
  }

}
