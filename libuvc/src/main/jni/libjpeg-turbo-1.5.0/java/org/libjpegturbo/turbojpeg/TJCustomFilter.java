

package org.libjpegturbo.turbojpeg;

import java.awt.*;
import java.nio.*;


public interface TJCustomFilter {

  
  void customFilter(ShortBuffer coeffBuffer, Rectangle bufferRegion,
                    Rectangle planeRegion, int componentID, int transformID,
                    TJTransform transform)
    throws TJException;
}
