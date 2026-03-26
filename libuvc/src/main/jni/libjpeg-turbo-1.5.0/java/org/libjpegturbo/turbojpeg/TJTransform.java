

package org.libjpegturbo.turbojpeg;

import java.awt.*;


public class TJTransform extends Rectangle {

  private static final long serialVersionUID = -127367705761430371L;

  
  public static final int NUMOP         = 8;
  
  public static final int OP_NONE       = 0;
  
  public static final int OP_HFLIP      = 1;
  
  public static final int OP_VFLIP      = 2;
  
  public static final int OP_TRANSPOSE  = 3;
  
  public static final int OP_TRANSVERSE = 4;
  
  public static final int OP_ROT90      = 5;
  
  public static final int OP_ROT180     = 6;
  
  public static final int OP_ROT270     = 7;


  
  public static final int OPT_PERFECT  = 1;
  
  public static final int OPT_TRIM     = 2;
  
  public static final int OPT_CROP     = 4;
  
  public static final int OPT_GRAY     = 8;
  
  public static final int OPT_NOOUTPUT = 16;


  
  public TJTransform() {
  }

  
  public TJTransform(int x, int y, int w, int h, int op, int options,
                     TJCustomFilter cf) {
    super(x, y, w, h);
    this.op = op;
    this.options = options;
    this.cf = cf;
  }

  
  public TJTransform(Rectangle r, int op, int options,
                     TJCustomFilter cf) {
    super(r);
    this.op = op;
    this.options = options;
    this.cf = cf;
  }

  
  public int op = 0;

  
  public int options = 0;

  
  public TJCustomFilter cf = null;
}
