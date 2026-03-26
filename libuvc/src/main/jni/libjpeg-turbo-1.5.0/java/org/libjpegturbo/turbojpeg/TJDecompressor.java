

package org.libjpegturbo.turbojpeg;

import java.awt.image.*;
import java.nio.*;
import java.io.*;


public class TJDecompressor implements Closeable {

  private static final String NO_ASSOC_ERROR =
    "No JPEG image is associated with this instance";

  
  public TJDecompressor() throws TJException {
    init();
  }

  
  public TJDecompressor(byte[] jpegImage) throws TJException {
    init();
    setSourceImage(jpegImage, jpegImage.length);
  }

  
  public TJDecompressor(byte[] jpegImage, int imageSize) throws TJException {
    init();
    setSourceImage(jpegImage, imageSize);
  }

  
  public TJDecompressor(YUVImage yuvImage) throws TJException {
    init();
    setSourceImage(yuvImage);
  }

  
  public void setSourceImage(byte[] jpegImage, int imageSize)
                             throws TJException {
    if (jpegImage == null || imageSize < 1)
      throw new IllegalArgumentException("Invalid argument in setSourceImage()");
    jpegBuf = jpegImage;
    jpegBufSize = imageSize;
    decompressHeader(jpegBuf, jpegBufSize);
    yuvImage = null;
  }

  
  @Deprecated
  public void setJPEGImage(byte[] jpegImage, int imageSize)
                           throws TJException {
    setSourceImage(jpegImage, imageSize);
  }

  
  public void setSourceImage(YUVImage srcImage) {
    if (srcImage == null)
      throw new IllegalArgumentException("Invalid argument in setSourceImage()");
    yuvImage = srcImage;
    jpegBuf = null;
    jpegBufSize = 0;
  }


  
  public int getWidth() {
    if (yuvImage != null)
      return yuvImage.getWidth();
    if (jpegWidth < 1)
      throw new IllegalStateException(NO_ASSOC_ERROR);
    return jpegWidth;
  }

  
  public int getHeight() {
    if (yuvImage != null)
      return yuvImage.getHeight();
    if (jpegHeight < 1)
      throw new IllegalStateException(NO_ASSOC_ERROR);
    return jpegHeight;
  }

  
  public int getSubsamp() {
    if (yuvImage != null)
      return yuvImage.getSubsamp();
    if (jpegSubsamp < 0)
      throw new IllegalStateException(NO_ASSOC_ERROR);
    if (jpegSubsamp >= TJ.NUMSAMP)
      throw new IllegalStateException("JPEG header information is invalid");
    return jpegSubsamp;
  }

  
  public int getColorspace() {
    if (yuvImage != null)
      return TJ.CS_YCbCr;
    if (jpegColorspace < 0)
      throw new IllegalStateException(NO_ASSOC_ERROR);
    if (jpegColorspace >= TJ.NUMCS)
      throw new IllegalStateException("JPEG header information is invalid");
    return jpegColorspace;
  }

  
  public byte[] getJPEGBuf() {
    if (jpegBuf == null)
      throw new IllegalStateException(NO_ASSOC_ERROR);
    return jpegBuf;
  }

  
  public int getJPEGSize() {
    if (jpegBufSize < 1)
      throw new IllegalStateException(NO_ASSOC_ERROR);
    return jpegBufSize;
  }

  
  public int getScaledWidth(int desiredWidth, int desiredHeight) {
    if (jpegWidth < 1 || jpegHeight < 1)
      throw new IllegalStateException(NO_ASSOC_ERROR);
    if (desiredWidth < 0 || desiredHeight < 0)
      throw new IllegalArgumentException("Invalid argument in getScaledWidth()");
    TJScalingFactor[] sf = TJ.getScalingFactors();
    if (desiredWidth == 0)
      desiredWidth = jpegWidth;
    if (desiredHeight == 0)
      desiredHeight = jpegHeight;
    int scaledWidth = jpegWidth, scaledHeight = jpegHeight;
    for (int i = 0; i < sf.length; i++) {
      scaledWidth = sf[i].getScaled(jpegWidth);
      scaledHeight = sf[i].getScaled(jpegHeight);
      if (scaledWidth <= desiredWidth && scaledHeight <= desiredHeight)
        break;
    }
    if (scaledWidth > desiredWidth || scaledHeight > desiredHeight)
      throw new IllegalArgumentException("Could not scale down to desired image dimensions");
    return scaledWidth;
  }

  
  public int getScaledHeight(int desiredWidth, int desiredHeight) {
    if (jpegWidth < 1 || jpegHeight < 1)
      throw new IllegalStateException(NO_ASSOC_ERROR);
    if (desiredWidth < 0 || desiredHeight < 0)
      throw new IllegalArgumentException("Invalid argument in getScaledHeight()");
    TJScalingFactor[] sf = TJ.getScalingFactors();
    if (desiredWidth == 0)
      desiredWidth = jpegWidth;
    if (desiredHeight == 0)
      desiredHeight = jpegHeight;
    int scaledWidth = jpegWidth, scaledHeight = jpegHeight;
    for (int i = 0; i < sf.length; i++) {
      scaledWidth = sf[i].getScaled(jpegWidth);
      scaledHeight = sf[i].getScaled(jpegHeight);
      if (scaledWidth <= desiredWidth && scaledHeight <= desiredHeight)
        break;
    }
    if (scaledWidth > desiredWidth || scaledHeight > desiredHeight)
      throw new IllegalArgumentException("Could not scale down to desired image dimensions");
    return scaledHeight;
  }

  
  public void decompress(byte[] dstBuf, int x, int y, int desiredWidth,
                         int pitch, int desiredHeight, int pixelFormat,
                         int flags) throws TJException {
    if (jpegBuf == null && yuvImage == null)
      throw new IllegalStateException(NO_ASSOC_ERROR);
    if (dstBuf == null || x < 0 || y < 0 || pitch < 0 ||
        (yuvImage != null && (desiredWidth < 0 || desiredHeight < 0)) ||
        pixelFormat < 0 || pixelFormat >= TJ.NUMPF || flags < 0)
      throw new IllegalArgumentException("Invalid argument in decompress()");
    if (yuvImage != null)
      decodeYUV(yuvImage.getPlanes(), yuvImage.getOffsets(),
                yuvImage.getStrides(), yuvImage.getSubsamp(), dstBuf, x, y,
                yuvImage.getWidth(), pitch, yuvImage.getHeight(), pixelFormat,
                flags);
    else {
      if (x > 0 || y > 0)
        decompress(jpegBuf, jpegBufSize, dstBuf, x, y, desiredWidth, pitch,
                   desiredHeight, pixelFormat, flags);
      else
        decompress(jpegBuf, jpegBufSize, dstBuf, desiredWidth, pitch,
                   desiredHeight, pixelFormat, flags);
    }
  }

  
  @Deprecated
  public void decompress(byte[] dstBuf, int desiredWidth, int pitch,
                         int desiredHeight, int pixelFormat, int flags)
                         throws TJException {
    decompress(dstBuf, 0, 0, desiredWidth, pitch, desiredHeight, pixelFormat,
               flags);
  }

  
  public byte[] decompress(int desiredWidth, int pitch, int desiredHeight,
                           int pixelFormat, int flags) throws TJException {
    if (pitch < 0 ||
        (yuvImage == null && (desiredWidth < 0 || desiredHeight < 0)) ||
        pixelFormat < 0 || pixelFormat >= TJ.NUMPF || flags < 0)
      throw new IllegalArgumentException("Invalid argument in decompress()");
    int pixelSize = TJ.getPixelSize(pixelFormat);
    int scaledWidth = getScaledWidth(desiredWidth, desiredHeight);
    int scaledHeight = getScaledHeight(desiredWidth, desiredHeight);
    if (pitch == 0)
      pitch = scaledWidth * pixelSize;
    byte[] buf = new byte[pitch * scaledHeight];
    decompress(buf, desiredWidth, pitch, desiredHeight, pixelFormat, flags);
    return buf;
  }

  
  public void decompressToYUV(YUVImage dstImage, int flags)
                              throws TJException {
    if (jpegBuf == null)
      throw new IllegalStateException(NO_ASSOC_ERROR);
    if (dstImage == null || flags < 0)
      throw new IllegalArgumentException("Invalid argument in decompressToYUV()");
    int scaledWidth = getScaledWidth(dstImage.getWidth(),
                                     dstImage.getHeight());
    int scaledHeight = getScaledHeight(dstImage.getWidth(),
                                       dstImage.getHeight());
    if (scaledWidth != dstImage.getWidth() ||
        scaledHeight != dstImage.getHeight())
      throw new IllegalArgumentException("YUVImage dimensions do not match one of the scaled image sizes that TurboJPEG is capable of generating.");
    if (jpegSubsamp != dstImage.getSubsamp())
      throw new IllegalArgumentException("YUVImage subsampling level does not match that of the JPEG image");

    decompressToYUV(jpegBuf, jpegBufSize, dstImage.getPlanes(),
                    dstImage.getOffsets(), dstImage.getWidth(),
                    dstImage.getStrides(), dstImage.getHeight(), flags);
  }

  
  @Deprecated
  public void decompressToYUV(byte[] dstBuf, int flags) throws TJException {
    YUVImage dstImage = new YUVImage(dstBuf, jpegWidth, 4, jpegHeight,
                                     jpegSubsamp);
    decompressToYUV(dstImage, flags);
  }

  
  public YUVImage decompressToYUV(int desiredWidth, int[] strides,
                                  int desiredHeight,
                                  int flags) throws TJException {
    if (flags < 0)
      throw new IllegalArgumentException("Invalid argument in decompressToYUV()");
    if (jpegWidth < 1 || jpegHeight < 1 || jpegSubsamp < 0)
      throw new IllegalStateException(NO_ASSOC_ERROR);
    if (jpegSubsamp >= TJ.NUMSAMP)
      throw new IllegalStateException("JPEG header information is invalid");
    if (yuvImage != null)
      throw new IllegalStateException("Source image is the wrong type");

    int scaledWidth = getScaledWidth(desiredWidth, desiredHeight);
    int scaledHeight = getScaledHeight(desiredWidth, desiredHeight);
    YUVImage yuvImage = new YUVImage(scaledWidth, null, scaledHeight,
                                     jpegSubsamp);
    decompressToYUV(yuvImage, flags);
    return yuvImage;
  }

  
  public YUVImage decompressToYUV(int desiredWidth, int pad, int desiredHeight,
                                  int flags) throws TJException {
    if (flags < 0)
      throw new IllegalArgumentException("Invalid argument in decompressToYUV()");
    if (jpegWidth < 1 || jpegHeight < 1 || jpegSubsamp < 0)
      throw new IllegalStateException(NO_ASSOC_ERROR);
    if (jpegSubsamp >= TJ.NUMSAMP)
      throw new IllegalStateException("JPEG header information is invalid");
    if (yuvImage != null)
      throw new IllegalStateException("Source image is the wrong type");

    int scaledWidth = getScaledWidth(desiredWidth, desiredHeight);
    int scaledHeight = getScaledHeight(desiredWidth, desiredHeight);
    YUVImage yuvImage = new YUVImage(scaledWidth, pad, scaledHeight,
                                     jpegSubsamp);
    decompressToYUV(yuvImage, flags);
    return yuvImage;
  }

  
  @Deprecated
  public byte[] decompressToYUV(int flags) throws TJException {
    YUVImage dstImage = new YUVImage(jpegWidth, 4, jpegHeight, jpegSubsamp);
    decompressToYUV(dstImage, flags);
    return dstImage.getBuf();
  }

  
  public void decompress(int[] dstBuf, int x, int y, int desiredWidth,
                         int stride, int desiredHeight, int pixelFormat,
                         int flags) throws TJException {
    if (jpegBuf == null && yuvImage == null)
      throw new IllegalStateException(NO_ASSOC_ERROR);
    if (dstBuf == null || x < 0 || y < 0 || stride < 0 ||
        (yuvImage != null && (desiredWidth < 0 || desiredHeight < 0)) ||
        pixelFormat < 0 || pixelFormat >= TJ.NUMPF || flags < 0)
      throw new IllegalArgumentException("Invalid argument in decompress()");
    if (yuvImage != null)
      decodeYUV(yuvImage.getPlanes(), yuvImage.getOffsets(),
                yuvImage.getStrides(), yuvImage.getSubsamp(), dstBuf, x, y,
                yuvImage.getWidth(), stride, yuvImage.getHeight(), pixelFormat,
                flags);
    else
      decompress(jpegBuf, jpegBufSize, dstBuf, x, y, desiredWidth, stride,
                 desiredHeight, pixelFormat, flags);
  }

  
  public void decompress(BufferedImage dstImage, int flags)
                         throws TJException {
    if (dstImage == null || flags < 0)
      throw new IllegalArgumentException("Invalid argument in decompress()");
    int desiredWidth = dstImage.getWidth();
    int desiredHeight = dstImage.getHeight();
    int scaledWidth, scaledHeight;

    if (yuvImage != null) {
      if (desiredWidth != yuvImage.getWidth() ||
          desiredHeight != yuvImage.getHeight())
        throw new IllegalArgumentException("BufferedImage dimensions do not match the dimensions of the source image.");
      scaledWidth = yuvImage.getWidth();
      scaledHeight = yuvImage.getHeight();
    } else {
      scaledWidth = getScaledWidth(desiredWidth, desiredHeight);
      scaledHeight = getScaledHeight(desiredWidth, desiredHeight);
      if (scaledWidth != desiredWidth || scaledHeight != desiredHeight)
        throw new IllegalArgumentException("BufferedImage dimensions do not match one of the scaled image sizes that TurboJPEG is capable of generating.");
    }
    int pixelFormat;  boolean intPixels = false;
    if (byteOrder == null)
      byteOrder = ByteOrder.nativeOrder();
    switch(dstImage.getType()) {
      case BufferedImage.TYPE_3BYTE_BGR:
        pixelFormat = TJ.PF_BGR;  break;
      case BufferedImage.TYPE_4BYTE_ABGR:
      case BufferedImage.TYPE_4BYTE_ABGR_PRE:
        pixelFormat = TJ.PF_XBGR;  break;
      case BufferedImage.TYPE_BYTE_GRAY:
        pixelFormat = TJ.PF_GRAY;  break;
      case BufferedImage.TYPE_INT_BGR:
        if (byteOrder == ByteOrder.BIG_ENDIAN)
          pixelFormat = TJ.PF_XBGR;
        else
          pixelFormat = TJ.PF_RGBX;
        intPixels = true;  break;
      case BufferedImage.TYPE_INT_RGB:
        if (byteOrder == ByteOrder.BIG_ENDIAN)
          pixelFormat = TJ.PF_XRGB;
        else
          pixelFormat = TJ.PF_BGRX;
        intPixels = true;  break;
      case BufferedImage.TYPE_INT_ARGB:
      case BufferedImage.TYPE_INT_ARGB_PRE:
        if (byteOrder == ByteOrder.BIG_ENDIAN)
          pixelFormat = TJ.PF_ARGB;
        else
          pixelFormat = TJ.PF_BGRA;
        intPixels = true;  break;
      default:
        throw new IllegalArgumentException("Unsupported BufferedImage format");
    }
    WritableRaster wr = dstImage.getRaster();
    if (intPixels) {
      SinglePixelPackedSampleModel sm =
        (SinglePixelPackedSampleModel)dstImage.getSampleModel();
      int stride = sm.getScanlineStride();
      DataBufferInt db = (DataBufferInt)wr.getDataBuffer();
      int[] buf = db.getData();
      if (yuvImage != null)
        decodeYUV(yuvImage.getPlanes(), yuvImage.getOffsets(),
                  yuvImage.getStrides(), yuvImage.getSubsamp(), buf, 0, 0,
                  yuvImage.getWidth(), stride, yuvImage.getHeight(),
                  pixelFormat, flags);
      else {
        if (jpegBuf == null)
          throw new IllegalStateException(NO_ASSOC_ERROR);
        decompress(jpegBuf, jpegBufSize, buf, 0, 0, scaledWidth, stride,
                   scaledHeight, pixelFormat, flags);
      }
    } else {
      ComponentSampleModel sm =
        (ComponentSampleModel)dstImage.getSampleModel();
      int pixelSize = sm.getPixelStride();
      if (pixelSize != TJ.getPixelSize(pixelFormat))
        throw new IllegalArgumentException("Inconsistency between pixel format and pixel size in BufferedImage");
      int pitch = sm.getScanlineStride();
      DataBufferByte db = (DataBufferByte)wr.getDataBuffer();
      byte[] buf = db.getData();
      decompress(buf, 0, 0, scaledWidth, pitch, scaledHeight, pixelFormat,
                 flags);
    }
  }

  
  public BufferedImage decompress(int desiredWidth, int desiredHeight,
                                  int bufferedImageType, int flags)
                                  throws TJException {
    if ((yuvImage == null && (desiredWidth < 0 || desiredHeight < 0)) ||
        flags < 0)
      throw new IllegalArgumentException("Invalid argument in decompress()");
    int scaledWidth = getScaledWidth(desiredWidth, desiredHeight);
    int scaledHeight = getScaledHeight(desiredWidth, desiredHeight);
    BufferedImage img = new BufferedImage(scaledWidth, scaledHeight,
                                          bufferedImageType);
    decompress(img, flags);
    return img;
  }

  
  @Override
  public void close() throws TJException {
    if (handle != 0)
      destroy();
  }

  @Override
  protected void finalize() throws Throwable {
    try {
      close();
    } catch(TJException e) {
    } finally {
      super.finalize();
    }
  };

  private native void init() throws TJException;

  private native void destroy() throws TJException;

  private native void decompressHeader(byte[] srcBuf, int size)
    throws TJException;

  @Deprecated
  private native void decompress(byte[] srcBuf, int size, byte[] dstBuf,
    int desiredWidth, int pitch, int desiredHeight, int pixelFormat, int flags)
    throws TJException;

  private native void decompress(byte[] srcBuf, int size, byte[] dstBuf, int x,
    int y, int desiredWidth, int pitch, int desiredHeight, int pixelFormat,
    int flags) throws TJException;

  @Deprecated
  private native void decompress(byte[] srcBuf, int size, int[] dstBuf,
    int desiredWidth, int stride, int desiredHeight, int pixelFormat,
    int flags) throws TJException;

  private native void decompress(byte[] srcBuf, int size, int[] dstBuf, int x,
    int y, int desiredWidth, int stride, int desiredHeight, int pixelFormat,
    int flags) throws TJException;

  @Deprecated
  private native void decompressToYUV(byte[] srcBuf, int size, byte[] dstBuf,
    int flags) throws TJException;

  private native void decompressToYUV(byte[] srcBuf, int size,
    byte[][] dstPlanes, int[] dstOffsets, int desiredWidth, int[] dstStrides,
    int desiredheight, int flags) throws TJException;

  private native void decodeYUV(byte[][] srcPlanes, int[] srcOffsets,
    int[] srcStrides, int subsamp, byte[] dstBuf, int x, int y, int width,
    int pitch, int height, int pixelFormat, int flags) throws TJException;

  private native void decodeYUV(byte[][] srcPlanes, int[] srcOffsets,
    int[] srcStrides, int subsamp, int[] dstBuf, int x, int y, int width,
    int stride, int height, int pixelFormat, int flags) throws TJException;

  static {
    TJLoader.load();
  }

  protected long handle = 0;
  protected byte[] jpegBuf = null;
  protected int jpegBufSize = 0;
  protected YUVImage yuvImage = null;
  protected int jpegWidth = 0;
  protected int jpegHeight = 0;
  protected int jpegSubsamp = -1;
  protected int jpegColorspace = -1;
  private ByteOrder byteOrder = null;
}
