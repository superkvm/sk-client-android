

package org.libjpegturbo.turbojpeg;


public class YUVImage {

  private static final String NO_ASSOC_ERROR =
    "No image data is associated with this instance";

  
  public YUVImage(int width, int[] strides, int height, int subsamp) {
    setBuf(null, null, width, strides, height, subsamp, true);
  }

  
  public YUVImage(int width, int pad, int height, int subsamp) {
    setBuf(new byte[TJ.bufSizeYUV(width, pad, height, subsamp)], width, pad,
           height, subsamp);
  }

  
  public YUVImage(byte[][] planes, int[] offsets, int width, int[] strides,
                  int height, int subsamp) {
    setBuf(planes, offsets, width, strides, height, subsamp, false);
  }

  
  public YUVImage(byte[] yuvImage, int width, int pad, int height,
                  int subsamp) {
    setBuf(yuvImage, width, pad, height, subsamp);
  }

  
  public void setBuf(byte[][] planes, int[] offsets, int width, int strides[],
                     int height, int subsamp) {
    setBuf(planes, offsets, width, strides, height, subsamp, false);
  }

  private void setBuf(byte[][] planes, int[] offsets, int width, int strides[],
                     int height, int subsamp, boolean alloc) {
    if ((planes == null && !alloc) || width < 1 || height < 1 || subsamp < 0 ||
        subsamp >= TJ.NUMSAMP)
      throw new IllegalArgumentException("Invalid argument in YUVImage::setBuf()");

    int nc = (subsamp == TJ.SAMP_GRAY ? 1 : 3);
    if (planes.length != nc || (offsets != null && offsets.length != nc) ||
        (strides != null && strides.length != nc))
      throw new IllegalArgumentException("YUVImage::setBuf(): planes, offsets, or strides array is the wrong size");

    if (offsets == null)
      offsets = new int[nc];
    if (strides == null)
      strides = new int[nc];

    for (int i = 0; i < nc; i++) {
      int pw = TJ.planeWidth(i, width, subsamp);
      int ph = TJ.planeHeight(i, height, subsamp);
      int planeSize = TJ.planeSizeYUV(i, width, strides[i], height, subsamp);

      if (strides[i] == 0)
        strides[i] = pw;
      if (alloc) {
        if (strides[i] < pw)
          throw new IllegalArgumentException("Stride must be >= plane width when allocating a new YUV image");
        planes[i] = new byte[strides[i] * ph];
      }
      if (planes[i] == null || offsets[i] < 0)
        throw new IllegalArgumentException("Invalid argument in YUVImage::setBuf()");
      if (strides[i] < 0 && offsets[i] - planeSize + pw < 0)
        throw new IllegalArgumentException("Stride for plane " + i + " would cause memory to be accessed below plane boundary");
      if (planes[i].length < offsets[i] + planeSize)
        throw new IllegalArgumentException("Image plane " + i + " is not large enough");
    }

    yuvPlanes = planes;
    yuvOffsets = offsets;
    yuvWidth = width;
    yuvStrides = strides;
    yuvHeight = height;
    yuvSubsamp = subsamp;
  }

  
  public void setBuf(byte[] yuvImage, int width, int pad, int height,
                     int subsamp) {
    if (yuvImage == null || width < 1 || pad < 1 || ((pad & (pad - 1)) != 0) ||
        height < 1 || subsamp < 0 || subsamp >= TJ.NUMSAMP)
      throw new IllegalArgumentException("Invalid argument in YUVImage::setBuf()");
    if (yuvImage.length < TJ.bufSizeYUV(width, pad, height, subsamp))
      throw new IllegalArgumentException("YUV image buffer is not large enough");

    int nc = (subsamp == TJ.SAMP_GRAY ? 1 : 3);
    byte[][] planes = new byte[nc][];
    int[] strides = new int[nc];
    int[] offsets = new int[nc];

    planes[0] = yuvImage;
    strides[0] = PAD(TJ.planeWidth(0, width, subsamp), pad);
    if (subsamp != TJ.SAMP_GRAY) {
      strides[1] = strides[2] = PAD(TJ.planeWidth(1, width, subsamp), pad);
      planes[1] = planes[2] = yuvImage;
      offsets[1] = offsets[0] +
        strides[0] * TJ.planeHeight(0, height, subsamp);
      offsets[2] = offsets[1] +
        strides[1] * TJ.planeHeight(1, height, subsamp);
    }

    yuvPad = pad;
    setBuf(planes, offsets, width, strides, height, subsamp);
  }

  
  public int getWidth() {
    if (yuvWidth < 1)
      throw new IllegalStateException(NO_ASSOC_ERROR);
    return yuvWidth;
  }

  
  public int getHeight() {
    if (yuvHeight < 1)
      throw new IllegalStateException(NO_ASSOC_ERROR);
    return yuvHeight;
  }

  
  public int getPad() {
    if (yuvPlanes == null)
      throw new IllegalStateException(NO_ASSOC_ERROR);
    if (yuvPad < 1 || ((yuvPad & (yuvPad - 1)) != 0))
      throw new IllegalStateException("Image is not stored in a unified buffer");
    return yuvPad;
  }

  
  public int[] getStrides() {
    if (yuvStrides == null)
      throw new IllegalStateException(NO_ASSOC_ERROR);
    return yuvStrides;
  }

  
  public int[] getOffsets() {
    if (yuvOffsets == null)
      throw new IllegalStateException(NO_ASSOC_ERROR);
    return yuvOffsets;
  }

  
  public int getSubsamp() {
    if (yuvSubsamp < 0 || yuvSubsamp >= TJ.NUMSAMP)
      throw new IllegalStateException(NO_ASSOC_ERROR);
    return yuvSubsamp;
  }

  
  public byte[][] getPlanes() {
    if (yuvPlanes == null)
      throw new IllegalStateException(NO_ASSOC_ERROR);
    return yuvPlanes;
  }

  
  public byte[] getBuf() {
    if (yuvPlanes == null || yuvSubsamp < 0 || yuvSubsamp >= TJ.NUMSAMP)
      throw new IllegalStateException(NO_ASSOC_ERROR);
    int nc = (yuvSubsamp == TJ.SAMP_GRAY ? 1 : 3);
    for (int i = 1; i < nc; i++) {
      if (yuvPlanes[i] != yuvPlanes[0])
        throw new IllegalStateException("Image is not stored in a unified buffer");
    }
    return yuvPlanes[0];
  }

  
  public int getSize() {
    if (yuvPlanes == null || yuvSubsamp < 0 || yuvSubsamp >= TJ.NUMSAMP)
      throw new IllegalStateException(NO_ASSOC_ERROR);
    int nc = (yuvSubsamp == TJ.SAMP_GRAY ? 1 : 3);
    if (yuvPad < 1)
      throw new IllegalStateException("Image is not stored in a unified buffer");
    for (int i = 1; i < nc; i++) {
      if (yuvPlanes[i] != yuvPlanes[0])
        throw new IllegalStateException("Image is not stored in a unified buffer");
    }
    return TJ.bufSizeYUV(yuvWidth, yuvPad, yuvHeight, yuvSubsamp);
  }

  private static final int PAD(int v, int p) {
    return (v + p - 1) & (~(p - 1));
  }

  protected long handle = 0;
  protected byte[][] yuvPlanes = null;
  protected int[] yuvOffsets = null;
  protected int[] yuvStrides = null;
  protected int yuvPad = 0;
  protected int yuvWidth = 0;
  protected int yuvHeight = 0;
  protected int yuvSubsamp = -1;
}
