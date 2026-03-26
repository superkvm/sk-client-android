

package org.libjpegturbo.turbojpeg;


public class TJScalingFactor {

  public TJScalingFactor(int num, int denom) {
    if (num < 1 || denom < 1)
      throw new IllegalArgumentException("Numerator and denominator must be >= 1");
    this.num = num;
    this.denom = denom;
  }

  
  public int getNum() {
    return num;
  }

  
  public int getDenom() {
    return denom;
  }

  
  public int getScaled(int dimension) {
    return (dimension * num + denom - 1) / denom;
  }

  
  public boolean equals(TJScalingFactor other) {
    return this.num == other.num && this.denom == other.denom;
  }

  
  public boolean isOne() {
    return num == 1 && denom == 1;
  }

  
  private int num = 1;

  
  private int denom = 1;
}
