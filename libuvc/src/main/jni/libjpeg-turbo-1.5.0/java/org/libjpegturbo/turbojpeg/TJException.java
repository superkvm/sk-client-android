

package org.libjpegturbo.turbojpeg;

import java.io.IOException;

public class TJException extends IOException {

  private static final long serialVersionUID = 1L;

  public TJException() {
    super();
  }

  public TJException(String message, Throwable cause) {
    super(message, cause);
  }

  public TJException(String message) {
    super(message);
  }

  public TJException(Throwable cause) {
    super(cause);
  }

}
