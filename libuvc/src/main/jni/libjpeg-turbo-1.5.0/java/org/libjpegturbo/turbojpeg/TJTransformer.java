

package org.libjpegturbo.turbojpeg;


public class TJTransformer extends TJDecompressor {

  
  public TJTransformer() throws TJException {
    init();
  }

  
  public TJTransformer(byte[] jpegImage) throws TJException {
    init();
    setSourceImage(jpegImage, jpegImage.length);
  }

  
  public TJTransformer(byte[] jpegImage, int imageSize) throws TJException {
    init();
    setSourceImage(jpegImage, imageSize);
  }

  
  public void transform(byte[][] dstBufs, TJTransform[] transforms,
                        int flags) throws TJException {
    if (jpegBuf == null)
      throw new IllegalStateException("JPEG buffer not initialized");
    transformedSizes = transform(jpegBuf, jpegBufSize, dstBufs, transforms,
                                 flags);
  }

  
  public TJDecompressor[] transform(TJTransform[] transforms, int flags)
                                    throws TJException {
    byte[][] dstBufs = new byte[transforms.length][];
    if (jpegWidth < 1 || jpegHeight < 1)
      throw new IllegalStateException("JPEG buffer not initialized");
    for (int i = 0; i < transforms.length; i++) {
      int w = jpegWidth, h = jpegHeight;
      if ((transforms[i].options & TJTransform.OPT_CROP) != 0) {
        if (transforms[i].width != 0) w = transforms[i].width;
        if (transforms[i].height != 0) h = transforms[i].height;
      }
      dstBufs[i] = new byte[TJ.bufSize(w, h, jpegSubsamp)];
    }
    TJDecompressor[] tjd = new TJDecompressor[transforms.length];
    transform(dstBufs, transforms, flags);
    for (int i = 0; i < transforms.length; i++)
      tjd[i] = new TJDecompressor(dstBufs[i], transformedSizes[i]);
    return tjd;
  }

  
  public int[] getTransformedSizes() {
    if (transformedSizes == null)
      throw new IllegalStateException("No image has been transformed yet");
    return transformedSizes;
  }

  private native void init() throws TJException;

  private native int[] transform(byte[] srcBuf, int srcSize, byte[][] dstBufs,
    TJTransform[] transforms, int flags) throws TJException;

  static {
    TJLoader.load();
  }

  private int[] transformedSizes = null;
}
