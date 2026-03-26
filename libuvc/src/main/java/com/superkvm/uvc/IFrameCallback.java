

package com.superkvm.uvc;

import java.nio.ByteBuffer;


public interface IFrameCallback {
	
	public void onFrame(ByteBuffer frame);
}
