

package org.libjpegturbo.turbojpeg;

import java.awt.image.*;
import java.nio.*;
import java.io.*;


public class TJCompressor implements Closeable {

  private static final String NO_ASSOC_ERROR =
    "No source image is associated with this instance";

  
  public TJCompressor() throws TJException {
    init();
  }

  
  public TJCompressor(byte[] srcImage, int x, int y, int width, int pitch,
                      int height, int pixelFormat) throws TJException {
    setSourceImage(srcImage, x, y, width, pitch, height, pixelFormat);
  }

  
  @Deprecated
  public TJCompressor(byte[] srcImage, int width, int pitch, int height,
                      int pixelFormat) throws TJException {
    setSourceImage(srcImage, width, pitch, height, pixelFormat);
  }

  
  public TJCompressor(BufferedImage srcImage, int x, int y, int width,
                      int height) throws TJException {
    setSourceImage(srcImage, x, y, width, height);
  }

  
  public void setSourceImage(byte[] srcImage, int x, int y, int width,
                             int pitch, int height, int pixelFormat)
                             throws TJException {
    if (handle == 0) init();
    if (srcImage == null || x < 0 || y < 0 || width < 1 || height < 1 ||
        pitch < 0 || pixelFormat < 0 || pixelFormat >= TJ.NUMPF)
      throw new IllegalArgumentException("Invalid argument in setSourceImage()");
    srcBuf = srcImage;
    srcWidth = width;
    if (pitch == 0)
      srcPitch = width * TJ.getPixelSize(pixelFormat);
    else
      srcPitch = pitch;
    srcHeight = height;
    srcPixelFormat = pixelFormat;
    srcX = x;
    srcY = y;
    srcBufInt = null;
    srcYUVImage = null;
  }

  
  @Deprecated
  public void setSourceImage(byte[] srcImage, int width, int pitch,
                             int height, int pixelFormat) throws TJException {
    setSourceImage(srcImage, 0, 0, width, pitch, height, pixelFormat);
    srcX = srcY = -1;
  }

  
  public void setSourceImage(BufferedImage srcImage, int x, int y, int width,
                             int height) throws TJException {
    if (handle == 0) init();
    if (srcImage == null || x < 0 || y < 0 || width < 0 || height < 0)
      throw new IllegalArgumentException("Invalid argument in setSourceImage()");
    srcX = x;
    srcY = y;
    srcWidth = (width == 0) ? srcImage.getWidth(): width;
    srcHeight = (height == 0) ? srcImage.getHeight() : height;
    if (x + width > srcImage.getWidth() || y + height > srcImage.getHeight())
      throw new IllegalArgumentException("Compression region exceeds the bounds of the source image");

    int pixelFormat;
    boolean intPixels = false;
    if (byteOrder == null)
      byteOrder = ByteOrder.nativeOrder();
    switch(srcImage.getType()) {
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
      case BufferedImage.TYPE_INT_ARGB:
      case BufferedImage.TYPE_INT_ARGB_PRE:
        if (byteOrder == ByteOrder.BIG_ENDIAN)
          pixelFormat = TJ.PF_XRGB;
        else
          pixelFormat = TJ.PF_BGRX;
        intPixels = true;  break;
      default:
        throw new IllegalArgumentException("Unsupported BufferedImage format");
    }
    srcPixelFormat = pixelFormat;

    WritableRaster wr = srcImage.getRaster();
    if (intPixels) {
      SinglePixelPackedSampleModel sm =
        (SinglePixelPackedSampleModel)srcImage.getSampleModel();
      srcStride = sm.getScanlineStride();
      DataBufferInt db = (DataBufferInt)wr.getDataBuffer();
      srcBufInt = db.getData();
      srcBuf = null;
    } else {
      ComponentSampleModel sm =
        (ComponentSampleModel)srcImage.getSampleModel();
      int pixelSize = sm.getPixelStride();
      if (pixelSize != TJ.getPixelSize(pixelFormat))
        throw new IllegalArgumentException("Inconsistency between pixel format and pixel size in BufferedImage");
      srcPitch = sm.getScanlineStride();
      DataBufferByte db = (DataBufferByte)wr.getDataBuffer();
      srcBuf = db.getData();
      srcBufInt = null;
    }
    srcYUVImage = null;
  }

  
  public void setSourceImage(YUVImage srcImage) throws TJException {
    if (handle == 0) init();
    if (srcImage == null)
      throw new IllegalArgumentException("Invalid argument in setSourceImage()");
    srcYUVImage = srcImage;
    srcBuf = null;
    srcBufInt = null;
  }

  
  public void setSubsamp(int newSubsamp) {
    if (newSubsamp < 0 || newSubsamp >= TJ.NUMSAMP)
      throw new IllegalArgumentException("Invalid argument in setSubsamp()");
    subsamp = newSubsamp;
  }

  
  public void setJPEGQuality(int quality) {
    if (quality < 1 || quality > 100)
      throw new IllegalArgumentException("Invalid argument in setJPEGQuality()");
    jpegQuality = quality;
  }

  
  public void compress(byte[] dstBuf, int flags) throws TJException {
    if (dstBuf == null || flags < 0)
      throw new IllegalArgumentException("Invalid argument in compress()");
    if (srcBuf == null && srcBufInt == null && srcYUVImage == null)
      throw new IllegalStateException(NO_ASSOC_ERROR);
    if (jpegQuality < 0)
      throw new IllegalStateException("JPEG Quality not set");
    if (subsamp < 0 && srcYUVImage == null)
      throw new IllegalStateException("Subsampling level not set");

    if (srcYUVImage != null)
      compressedSize = compressFromYUV(srcYUVImage.getPlanes(),
                                       srcYUVImage.getOffsets(),
                                       srcYUVImage.getWidth(),
                                       srcYUVImage.getStrides(),
                                       srcYUVImage.getHeight(),
                                       srcYUVImage.getSubsamp(),
                                       dstBuf, jpegQuality, flags);
    else if (srcBuf != null) {
      if (srcX >= 0 && srcY >= 0)
        compressedSize = compress(srcBuf, srcX, srcY, srcWidth, srcPitch,
                                  srcHeight, srcPixelFormat, dstBuf, subsamp,
                                  jpegQuality, flags);
      else
        compressedSize = compress(srcBuf, srcWidth, srcPitch, srcHeight,
                                  srcPixelFormat, dstBuf, subsamp, jpegQuality,
                                  flags);
    } else if (srcBufInt != null) {
      if (srcX >= 0 && srcY >= 0)
        compressedSize = compress(srcBufInt, srcX, srcY, srcWidth, srcStride,
                                  srcHeight, srcPixelFormat, dstBuf, subsamp,
                                  jpegQuality, flags);
      else
        compressedSize = compress(srcBufInt, srcWidth, srcStride, srcHeight,
                                  srcPixelFormat, dstBuf, subsamp, jpegQuality,
                                  flags);
    }
  }

  
  public byte[] compress(int flags) throws TJException {
    checkSourceImage();
    byte[] buf = new byte[TJ.bufSize(srcWidth, srcHeight, subsamp)];
    compress(buf, flags);
    return buf;
  }

  
  @Deprecated
  public void compress(BufferedImage srcImage, byte[] dstBuf, int flags)
                       throws TJException {
    setSourceImage(srcImage, 0, 0, 0, 0);
    compress(dstBuf, flags);
  }

  
  @Deprecated
  public byte[] compress(BufferedImage srcImage, int flags)
                         throws TJException {
    setSourceImage(srcImage, 0, 0, 0, 0);
    return compress(flags);
  }

  
  public void encodeYUV(YUVImage dstImage, int flags) throws TJException {
    if (dstImage == null || flags < 0)
      throw new IllegalArgumentException("Invalid argument in encodeYUV()");
    if (srcBuf == null && srcBufInt == null)
      throw new IllegalStateException(NO_ASSOC_ERROR);
    if (srcYUVImage != null)
      throw new IllegalStateException("Source image is not correct type");
    checkSubsampling();
    if (srcWidth != dstImage.getWidth() || srcHeight != dstImage.getHeight())
      throw new IllegalStateException("Destination image is the wrong size");

    if (srcBufInt != null) {
      encodeYUV(srcBufInt, srcX, srcY, srcWidth, srcStride, srcHeight,
                srcPixelFormat, dstImage.getPlanes(), dstImage.getOffsets(),
                dstImage.getStrides(), dstImage.getSubsamp(), flags);
    } else {
      encodeYUV(srcBuf, srcX, srcY, srcWidth, srcPitch, srcHeight,
                srcPixelFormat, dstImage.getPlanes(), dstImage.getOffsets(),
                dstImage.getStrides(), dstImage.getSubsamp(), flags);
    }
    compressedSize = 0;
  }

  
  @Deprecated
  public void encodeYUV(byte[] dstBuf, int flags) throws TJException {
    if(dstBuf == null)
      throw new IllegalArgumentException("Invalid argument in encodeYUV()");
    checkSourceImage();
    checkSubsampling();
    YUVImage yuvImage = new YUVImage(dstBuf, srcWidth, 4, srcHeight, subsamp);
    encodeYUV(yuvImage, flags);
  }

  
  public YUVImage encodeYUV(int pad, int flags) throws TJException {
    checkSourceImage();
    checkSubsampling();
    if(pad < 1 || ((pad & (pad - 1)) != 0))
      throw new IllegalStateException("Invalid argument in encodeYUV()");
    YUVImage yuvImage = new YUVImage(srcWidth, pad, srcHeight, subsamp);
    encodeYUV(yuvImage, flags);
    return yuvImage;
  }

  
  public YUVImage encodeYUV(int[] strides, int flags) throws TJException {
    checkSourceImage();
    checkSubsampling();
    YUVImage yuvImage = new YUVImage(srcWidth, strides, srcHeight, subsamp);
    encodeYUV(yuvImage, flags);
    return yuvImage;
  }

  
  @Deprecated
  public byte[] encodeYUV(int flags) throws TJException {
    checkSourceImage();
    checkSubsampling();
    YUVImage yuvImage = new YUVImage(srcWidth, 4, srcHeight, subsamp);
    encodeYUV(yuvImage, flags);
    return yuvImage.getBuf();
  }

  
  @Deprecated
  public void encodeYUV(BufferedImage srcImage, byte[] dstBuf, int flags)
                        throws TJException {
    setSourceImage(srcImage, 0, 0, 0, 0);
    encodeYUV(dstBuf, flags);
  }

  
  @Deprecated
  public byte[] encodeYUV(BufferedImage srcImage, int flags)
                          throws TJException {
    setSourceImage(srcImage, 0, 0, 0, 0);
    return encodeYUV(flags);
  }

  
  public int getCompressedSize() {
    return compressedSize;
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

  
  @Deprecated
  private native int compress(byte[] srcBuf, int width, int pitch,
    int height, int pixelFormat, byte[] dstBuf, int jpegSubsamp, int jpegQual,
    int flags) throws TJException;

  private native int compress(byte[] srcBuf, int x, int y, int width,
    int pitch, int height, int pixelFormat, byte[] dstBuf, int jpegSubsamp,
    int jpegQual, int flags) throws TJException;

  @Deprecated
  private native int compress(int[] srcBuf, int width, int stride,
    int height, int pixelFormat, byte[] dstBuf, int jpegSubsamp, int jpegQual,
    int flags) throws TJException;

  private native int compress(int[] srcBuf, int x, int y, int width,
    int stride, int height, int pixelFormat, byte[] dstBuf, int jpegSubsamp,
    int jpegQual, int flags) throws TJException;

  private native int compressFromYUV(byte[][] srcPlanes, int[] srcOffsets,
    int width, int[] srcStrides, int height, int subsamp, byte[] dstBuf,
    int jpegQual, int flags)
    throws TJException;

  @Deprecated
  private native void encodeYUV(byte[] srcBuf, int width, int pitch,
    int height, int pixelFormat, byte[] dstBuf, int subsamp, int flags)
    throws TJException;

  private native void encodeYUV(byte[] srcBuf, int x, int y, int width,
    int pitch, int height, int pixelFormat, byte[][] dstPlanes,
    int[] dstOffsets, int[] dstStrides, int subsamp, int flags)
    throws TJException;

  @Deprecated
  private native void encodeYUV(int[] srcBuf, int width, int stride,
    int height, int pixelFormat, byte[] dstBuf, int subsamp, int flags)
    throws TJException;

  private native void encodeYUV(int[] srcBuf, int x, int y, int width,
    int srcStride, int height, int pixelFormat, byte[][] dstPlanes,
    int[] dstOffsets, int[] dstStrides, int subsamp, int flags)
    throws TJException;

  static {
    TJLoader.load();
  }

  private void checkSourceImage() {
    if (srcWidth < 1 || srcHeight < 1)
      throw new IllegalStateException(NO_ASSOC_ERROR);
  }

  private void checkSubsampling() {
    if (subsamp < 0)
      throw new IllegalStateException("Subsampling level not set");
  }

  private long handle = 0;
  private byte[] srcBuf = null;
  private int[] srcBufInt = null;
  private int srcWidth = 0;
  private int srcHeight = 0;
  private int srcX = -1;
  private int srcY = -1;
  private int srcPitch = 0;
  private int srcStride = 0;
  private int srcPixelFormat = -1;
  private YUVImage srcYUVImage = null;
  private int subsamp = -1;
  private int jpegQuality = -1;
  private int compressedSize = 0;
  private int yuvPad = 4;
  private ByteOrder byteOrder = null;
}
