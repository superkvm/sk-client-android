

package com.superkvm.uvc;

import android.graphics.SurfaceTexture;
import android.hardware.usb.UsbDevice;
import android.text.TextUtils;
import android.view.Surface;
import android.view.SurfaceHolder;

import com.superkvm.usb.USBMonitor;
import com.superkvm.usb.USBMonitor.UsbControlBlock;
import com.superkvm.utils.Size;
import com.superkvm.utils.XLogWrapper;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;

public class UVCCamera {
	public static boolean DEBUG = false;	
	private static final String TAG = UVCCamera.class.getSimpleName();
	private static final String DEFAULT_USBFS = "/dev/bus/usb";
	public static final int FRAME_FORMAT_YUYV = 0;
	public static final int FRAME_FORMAT_MJPEG = 1;

	public static final int DEFAULT_PREVIEW_MIN_FPS = 1;
	public static final int DEFAULT_PREVIEW_MAX_FPS = 31;
	public static final float DEFAULT_BANDWIDTH = 1.0f;
	public static final int DEFAULT_PREVIEW_MODE = FRAME_FORMAT_MJPEG;

	public static final int PIXEL_FORMAT_RAW = 0;
	public static final int PIXEL_FORMAT_YUV = 1;
	public static final int PIXEL_FORMAT_RGB565 = 2;
	public static final int PIXEL_FORMAT_RGBX = 3;
	public static final int PIXEL_FORMAT_YUV420SP = 4;	
	public static final int PIXEL_FORMAT_NV21 = 5;		

	
    public static final int	CTRL_SCANNING		= 0x00000001;	
    public static final int CTRL_AE				= 0x00000002;	
    public static final int CTRL_AE_PRIORITY	= 0x00000004;	
    public static final int CTRL_AE_ABS			= 0x00000008;	
    public static final int CTRL_AR_REL			= 0x00000010;	
    public static final int CTRL_FOCUS_ABS		= 0x00000020;	
    public static final int CTRL_FOCUS_REL		= 0x00000040;	
    public static final int CTRL_IRIS_ABS		= 0x00000080;	
    public static final int CTRL_IRIS_REL		= 0x00000100;	
    public static final int CTRL_ZOOM_ABS		= 0x00000200;	
    public static final int CTRL_ZOOM_REL		= 0x00000400;	
    public static final int CTRL_PANTILT_ABS	= 0x00000800;	
    public static final int CTRL_PANTILT_REL	= 0x00001000;	
    public static final int CTRL_ROLL_ABS		= 0x00002000;	
    public static final int CTRL_ROLL_REL		= 0x00004000;	
    public static final int CTRL_FOCUS_AUTO		= 0x00020000;	
    public static final int CTRL_PRIVACY		= 0x00040000;	
    public static final int CTRL_FOCUS_SIMPLE	= 0x00080000;	
    public static final int CTRL_WINDOW			= 0x00100000;	

    public static final int PU_BRIGHTNESS		= 0x80000001;	
    public static final int PU_CONTRAST			= 0x80000002;	
    public static final int PU_HUE				= 0x80000004;	
    public static final int PU_SATURATION		= 0x80000008;	
    public static final int PU_SHARPNESS		= 0x80000010;	
    public static final int PU_GAMMA			= 0x80000020;	
    public static final int PU_WB_TEMP			= 0x80000040;	
    public static final int PU_WB_COMPO			= 0x80000080;	
    public static final int PU_BACKLIGHT		= 0x80000100;	
    public static final int PU_GAIN				= 0x80000200;	
    public static final int PU_POWER_LF			= 0x80000400;	
    public static final int PU_HUE_AUTO			= 0x80000800;	
    public static final int PU_WB_TEMP_AUTO		= 0x80001000;	
    public static final int PU_WB_COMPO_AUTO	= 0x80002000;	
    public static final int PU_DIGITAL_MULT		= 0x80004000;	
    public static final int PU_DIGITAL_LIMIT	= 0x80008000;	
    public static final int PU_AVIDEO_STD		= 0x80010000;	
    public static final int PU_AVIDEO_LOCK		= 0x80020000;	
    public static final int PU_CONTRAST_AUTO	= 0x80040000;	

	
	public static final int STATUS_CLASS_CONTROL = 0x10;
	public static final int STATUS_CLASS_CONTROL_CAMERA = 0x11;
	public static final int STATUS_CLASS_CONTROL_PROCESSING = 0x12;

	
	public static final int STATUS_ATTRIBUTE_VALUE_CHANGE = 0x00;
	public static final int STATUS_ATTRIBUTE_INFO_CHANGE = 0x01;
	public static final int STATUS_ATTRIBUTE_FAILURE_CHANGE = 0x02;
	public static final int STATUS_ATTRIBUTE_UNKNOWN = 0xff;

	private static boolean isLoaded;
	static {
		if (!isLoaded) {
			System.loadLibrary("jpeg-turbo1500");
			System.loadLibrary("usb100");
			System.loadLibrary("uvc");
			System.loadLibrary("UVCCamera");
			isLoaded = true;
		}
	}

	private UsbControlBlock mCtrlBlock;
    protected long mControlSupports;			
    protected long mProcSupports;				
    protected int mCurrentFrameFormat = FRAME_FORMAT_MJPEG;
	protected int mCurrentWidth = 640, mCurrentHeight = 480;
	protected float mCurrentBandwidthFactor = DEFAULT_BANDWIDTH;
    protected String mSupportedSize;
    protected List<Size> mCurrentSizeList;
	
    protected long mNativePtr;
    protected int mScanningModeMin, mScanningModeMax, mScanningModeDef;
    protected int mExposureModeMin, mExposureModeMax, mExposureModeDef;
    protected int mExposurePriorityMin, mExposurePriorityMax, mExposurePriorityDef;
    protected int mExposureMin, mExposureMax, mExposureDef;
    protected int mAutoFocusMin, mAutoFocusMax, mAutoFocusDef;
    protected int mFocusMin, mFocusMax, mFocusDef;
    protected int mFocusRelMin, mFocusRelMax, mFocusRelDef;
    protected int mFocusSimpleMin, mFocusSimpleMax, mFocusSimpleDef;
    protected int mIrisMin, mIrisMax, mIrisDef;
    protected int mIrisRelMin, mIrisRelMax, mIrisRelDef;
    protected int mPanMin, mPanMax, mPanDef;
    protected int mTiltMin, mTiltMax, mTiltDef;
    protected int mRollMin, mRollMax, mRollDef;
    protected int mPanRelMin, mPanRelMax, mPanRelDef;
    protected int mTiltRelMin, mTiltRelMax, mTiltRelDef;
    protected int mRollRelMin, mRollRelMax, mRollRelDef;
    protected int mPrivacyMin, mPrivacyMax, mPrivacyDef;
    protected int mAutoWhiteBlanceMin, mAutoWhiteBlanceMax, mAutoWhiteBlanceDef;
    protected int mAutoWhiteBlanceCompoMin, mAutoWhiteBlanceCompoMax, mAutoWhiteBlanceCompoDef;
    protected int mWhiteBlanceMin, mWhiteBlanceMax, mWhiteBlanceDef;
    protected int mWhiteBlanceCompoMin, mWhiteBlanceCompoMax, mWhiteBlanceCompoDef;
    protected int mWhiteBlanceRelMin, mWhiteBlanceRelMax, mWhiteBlanceRelDef;
    protected int mBacklightCompMin, mBacklightCompMax, mBacklightCompDef;
    protected int mBrightnessMin, mBrightnessMax, mBrightnessDef;
    protected int mContrastMin, mContrastMax, mContrastDef;
    protected int mSharpnessMin, mSharpnessMax, mSharpnessDef;
    protected int mGainMin, mGainMax, mGainDef;
    protected int mGammaMin, mGammaMax, mGammaDef;
    protected int mSaturationMin, mSaturationMax, mSaturationDef;
    protected int mHueMin, mHueMax, mHueDef;
    protected int mZoomMin, mZoomMax, mZoomDef;
    protected int mZoomRelMin, mZoomRelMax, mZoomRelDef;
    protected int mPowerlineFrequencyMin, mPowerlineFrequencyMax, mPowerlineFrequencyDef;
    protected int mMultiplierMin, mMultiplierMax, mMultiplierDef;
    protected int mMultiplierLimitMin, mMultiplierLimitMax, mMultiplierLimitDef;
    protected int mAnaXLogWrapperVideoStandardMin, mAnaXLogWrapperVideoStandardMax, mAnaXLogWrapperVideoStandardDef;
    protected int mAnaXLogWrapperVideoLockStateMin, mAnaXLogWrapperVideoLockStateMax, mAnaXLogWrapperVideoLockStateDef;
    
    
    public UVCCamera() {
    	mNativePtr = nativeCreate();
    	mSupportedSize = null;
	}

    
    public synchronized void open(final UsbControlBlock ctrlBlock) {
    	int result = -2;
		StringBuilder sb = new StringBuilder();
		close();
    	try {
			mCtrlBlock = ctrlBlock.clone();
			result = nativeConnect(mNativePtr,
				mCtrlBlock.getVenderId(), mCtrlBlock.getProductId(),
				mCtrlBlock.getFileDescriptor(),
				mCtrlBlock.getBusNum(),
				mCtrlBlock.getDevNum(),
				getUSBFSName(mCtrlBlock));
			sb.append("nativeConnect："+result);

		} catch (final Exception e) {
			XLogWrapper.w(TAG, e);
			for(int i = 0; i< e.getStackTrace().length; i++){
				sb.append(e.getStackTrace()[i].toString());
				sb.append("\n");
			}
			sb.append("core message ->"+e.getLocalizedMessage());
			result = -1;
		}

		if (result != 0) {
			throw new UnsupportedOperationException("open failed:result=" + result+"----->" +
					"id_camera="+mNativePtr+";venderId="+(mCtrlBlock==null ? "": mCtrlBlock.getVenderId())
					+";productId="+(mCtrlBlock==null ? "": mCtrlBlock.getProductId())+";fileDescriptor="+(mCtrlBlock==null ? "": mCtrlBlock.getFileDescriptor())
					+";busNum="+(mCtrlBlock==null ? "": mCtrlBlock.getBusNum())+";devAddr="+(mCtrlBlock==null ? "": mCtrlBlock.getDevNum())
					+";usbfs="+(mCtrlBlock==null ? "": getUSBFSName(mCtrlBlock))+"\n"+"Exception："+sb.toString());
		}
		mCurrentFrameFormat = FRAME_FORMAT_MJPEG;
    	if (mNativePtr != 0 && TextUtils.isEmpty(mSupportedSize)) {
    		mSupportedSize = nativeGetSupportedSize(mNativePtr);
    	}
    	if (USBMonitor.DEBUG) {
    		XLogWrapper.i(TAG, "open camera status: " + mNativePtr +", size: " + mSupportedSize);
		}
    	List<Size> supportedSizes = getSupportedSizeList();
		if (!supportedSizes.isEmpty()) {
			mCurrentWidth = supportedSizes.get(0).width;
			mCurrentHeight = supportedSizes.get(0).height;
		}
		nativeSetPreviewSize(mNativePtr, mCurrentWidth, mCurrentHeight,
			DEFAULT_PREVIEW_MIN_FPS, DEFAULT_PREVIEW_MAX_FPS, DEFAULT_PREVIEW_MODE, DEFAULT_BANDWIDTH);
    }

	
	public void setStatusCallback(final IStatusCallback callback) {
		if (mNativePtr != 0) {
			nativeSetStatusCallback(mNativePtr, callback);
		}
	}

	
	public void setButtonCallback(final IButtonCallback callback) {
		if (mNativePtr != 0) {
			nativeSetButtonCallback(mNativePtr, callback);
		}
	}

    
    public synchronized void close() {
    	stopPreview();
    	if (mNativePtr != 0) {
    		nativeRelease(mNativePtr);

    	}
    	if (mCtrlBlock != null) {
			mCtrlBlock.close();
   			mCtrlBlock = null;
		}
		mControlSupports = mProcSupports = 0;
		mCurrentFrameFormat = -1;
		mCurrentBandwidthFactor = 0;
		mSupportedSize = null;
		mCurrentSizeList = null;
    	if (DEBUG) XLogWrapper.v(TAG, "close:finished");
    }

	public UsbDevice getDevice() {
		return mCtrlBlock != null ? mCtrlBlock.getDevice() : null;
	}

	public String getDeviceName(){
		return mCtrlBlock != null ? mCtrlBlock.getDeviceName() : null;
	}

	public UsbControlBlock getUsbControlBlock() {
		return mCtrlBlock;
	}

	public synchronized String getSupportedSize() {
    	return !TextUtils.isEmpty(mSupportedSize) ? mSupportedSize : (mSupportedSize = nativeGetSupportedSize(mNativePtr));
    }

	public Size getPreviewSize() {
		Size result = null;
		final List<Size> list = getSupportedSizeList();
		for (final Size sz: list) {
			if ((sz.width == mCurrentWidth)
				|| (sz.height == mCurrentHeight)) {
				result =sz;
				break;
			}
		}
		return result;
	}

	
	public void setPreviewSize(final int width, final int height) {
		setPreviewSize(width, height, DEFAULT_PREVIEW_MIN_FPS, DEFAULT_PREVIEW_MAX_FPS, mCurrentFrameFormat, mCurrentBandwidthFactor);
	}

	
	public void setPreviewSize(final int width, final int height, final int frameFormat) {
		setPreviewSize(width, height, DEFAULT_PREVIEW_MIN_FPS, DEFAULT_PREVIEW_MAX_FPS, frameFormat, mCurrentBandwidthFactor);
	}

	
	public void setPreviewSize(final int width, final int height, final int frameFormat, final float bandwidth) {
		setPreviewSize(width, height, DEFAULT_PREVIEW_MIN_FPS, DEFAULT_PREVIEW_MAX_FPS, frameFormat, bandwidth);
	}

	
	public void setPreviewSize(final int width, final int height, final int min_fps, final int max_fps, final int frameFormat, final float bandwidthFactor) {
		if ((width == 0) || (height == 0))
			throw new IllegalArgumentException("invalid preview size");
		if (mNativePtr != 0) {
			final int result = nativeSetPreviewSize(mNativePtr, width, height, min_fps, max_fps, frameFormat, bandwidthFactor);
			if (result != 0)
				throw new IllegalArgumentException("Failed to set preview size");
			mCurrentFrameFormat = frameFormat;
			mCurrentWidth = width;
			mCurrentHeight = height;
			mCurrentBandwidthFactor = bandwidthFactor;
		}
	}

	public List<Size> getSupportedSizeList() {
		if (mCurrentFrameFormat < 0) {
			mCurrentFrameFormat = FRAME_FORMAT_MJPEG;
		}
		return getSupportedSize((mCurrentFrameFormat > 0) ? 6 : 4, getSupportedSize());
	}

	public List<Size> getSupportedSizeList(int frameFormat) {
		return getSupportedSize((frameFormat > 0) ? 6 : 4, getSupportedSize());
	}

	public List<Size> getSupportedSize(final int type, final String supportedSize) {
		final List<Size> result = new ArrayList<Size>();
		if (!TextUtils.isEmpty(supportedSize))
		try {
			final JSONObject json = new JSONObject(supportedSize);
			final JSONArray formats = json.getJSONArray("formats");
			final int format_nums = formats.length();
			for (int i = 0; i < format_nums; i++) {
				final JSONObject format = formats.getJSONObject(i);
				if(format.has("type") && format.has("size")) {
					final int format_type = format.getInt("type");
					if ((format_type == type) || (type == -1)) {
						addSize(format, format_type, 0, result);
					}
				}
			}
		} catch (final JSONException e) {
			e.printStackTrace();
		}
		return result;
	}

	private static final void addSize(final JSONObject format, final int formatType, final int frameType, final List<Size> size_list) throws JSONException {
		final JSONArray size = format.getJSONArray("size");
		final int size_nums = size.length();
		for (int j = 0; j < size_nums; j++) {
			final String[] sz = size.getString(j).split("x");
			try {
				size_list.add(new Size(formatType, frameType, j, Integer.parseInt(sz[0]), Integer.parseInt(sz[1])));
			} catch (final Exception e) {
				break;
			}
		}
	}

    
    public synchronized void setPreviewDisplay(final SurfaceHolder holder) {
   		nativeSetPreviewDisplay(mNativePtr, holder.getSurface());
    }

    
    public synchronized void setPreviewTexture(final SurfaceTexture texture) {	
    	final Surface surface = new Surface(texture);	
    	nativeSetPreviewDisplay(mNativePtr, surface);
    }

    
    public synchronized void setPreviewDisplay(final Surface surface) {
    	nativeSetPreviewDisplay(mNativePtr, surface);
    }

    
    public void setFrameCallback(final IFrameCallback callback, final int pixelFormat) {
    	if (mNativePtr != 0) {
        	nativeSetFrameCallback(mNativePtr, callback, pixelFormat);
    	}
    }

    
    public synchronized void startPreview() {
    	if (mCtrlBlock != null) {
    		nativeStartPreview(mNativePtr);
    	}
    }

    
    public synchronized void stopPreview() {
    	setFrameCallback(null, 0);
    	if (mCtrlBlock != null) {
    		nativeStopPreview(mNativePtr);
    	}
    }

    
    public synchronized void destroy() {
    	close();
    	if (mNativePtr != 0) {
    		nativeDestroy(mNativePtr);
    		mNativePtr = 0;
    	}
    }

    
    
	public boolean checkSupportFlag(final long flag) {
    	updateCameraParams();
    	if ((flag & 0x80000000) == 0x80000000)
    		return ((mProcSupports & flag) == (flag & 0x7ffffffF));
    	else
    		return (mControlSupports & flag) == flag;
    }


	public synchronized void setAutoFocus(final boolean autoFocus) {
    	if (mNativePtr != 0) {
    		nativeSetAutoFocus(mNativePtr, autoFocus);
    	}
    }

	public synchronized boolean getAutoFocus() {
    	boolean result = true;
    	if (mNativePtr != 0) {
    		result = nativeGetAutoFocus(mNativePtr) > 0;
    	}
    	return result;
    }

    
	public synchronized void setFocus(final int focus) {
    	if (mNativePtr != 0) {
 		   final float range = Math.abs(mFocusMax - mFocusMin);
 		   if (range > 0)
 			   nativeSetFocus(mNativePtr, (int)(focus / 100.f * range) + mFocusMin);
    	}
    }

    
	public synchronized int getFocus(final int focus_abs) {
	   int result = 0;
	   if (mNativePtr != 0) {
		   nativeUpdateFocusLimit(mNativePtr);
		   final float range = Math.abs(mFocusMax - mFocusMin);
		   if (range > 0) {
			   result = (int)((focus_abs - mFocusMin) * 100.f / range);
		   }
	   }
	   return result;
	}

    
	public synchronized int getFocus() {
    	return getFocus(nativeGetFocus(mNativePtr));
    }

	public synchronized void resetFocus() {
    	if (mNativePtr != 0) {
    		nativeSetFocus(mNativePtr, mFocusDef);
    	}
    }


	public synchronized void setAutoWhiteBlance(final boolean autoWhiteBlance) {
    	if (mNativePtr != 0) {
    		nativeSetAutoWhiteBlance(mNativePtr, autoWhiteBlance);
    	}
    }

	public synchronized boolean getAutoWhiteBlance() {
    	boolean result = true;
    	if (mNativePtr != 0) {
    		result = nativeGetAutoWhiteBlance(mNativePtr) > 0;
    	}
    	return result;
    }


    
	public synchronized void setWhiteBlance(final int whiteBlance) {
    	if (mNativePtr != 0) {
 		   final float range = Math.abs(mWhiteBlanceMax - mWhiteBlanceMin);
 		   if (range > 0)
 			   nativeSetWhiteBlance(mNativePtr, (int)(whiteBlance / 100.f * range) + mWhiteBlanceMin);
    	}
    }

    
	public synchronized int getWhiteBlance(final int whiteBlance_abs) {
	   int result = 0;
	   if (mNativePtr != 0) {
		   nativeUpdateWhiteBlanceLimit(mNativePtr);
		   final float range = Math.abs(mWhiteBlanceMax - mWhiteBlanceMin);
		   if (range > 0) {
			   result = (int)((whiteBlance_abs - mWhiteBlanceMin) * 100.f / range);
		   }
	   }
	   return result;
	}

    
	public synchronized int getWhiteBlance() {
    	return getWhiteBlance(nativeGetWhiteBlance(mNativePtr));
    }

	public synchronized void resetWhiteBlance() {
    	if (mNativePtr != 0) {
    		nativeSetWhiteBlance(mNativePtr, mWhiteBlanceDef);
    	}
    }

    
	public synchronized void setBrightness(final int brightness) {
    	if (mNativePtr != 0) {
 		   final float range = Math.abs(mBrightnessMax - mBrightnessMin);
 		   if (range > 0)
 			   nativeSetBrightness(mNativePtr, (int)(brightness / 100.f * range) + mBrightnessMin);
    	}
    }

    
	public synchronized int getBrightness(final int brightness_abs) {
	   int result = 0;
	   if (mNativePtr != 0) {
		   nativeUpdateBrightnessLimit(mNativePtr);
		   final float range = Math.abs(mBrightnessMax - mBrightnessMin);
		   if (range > 0) {
			   result = (int)((brightness_abs - mBrightnessMin) * 100.f / range);
		   }
	   }
	   return result;
	}

    
	public synchronized int getBrightness() {
    	return getBrightness(nativeGetBrightness(mNativePtr));
    }

	public synchronized void resetBrightness() {
    	if (mNativePtr != 0) {
    		nativeSetBrightness(mNativePtr, mBrightnessDef);
    	}
    }

	public synchronized int getBrightnessMax() {
		return mBrightnessMax;
	}

	public synchronized int getBrightnessMin() {
		return mBrightnessMin;
	}


    
	public synchronized void setContrast(final int contrast) {
    	if (mNativePtr != 0) {
    		nativeUpdateContrastLimit(mNativePtr);
	    	final float range = Math.abs(mContrastMax - mContrastMin);
	    	if (range > 0)
	    		nativeSetContrast(mNativePtr, (int)(contrast / 100.f * range) + mContrastMin);
    	}
    }

    
	public synchronized int getContrast(final int contrast_abs) {
	   int result = 0;
	   if (mNativePtr != 0) {
		   final float range = Math.abs(mContrastMax - mContrastMin);
		   if (range > 0) {
			   result = (int)((contrast_abs - mContrastMin) * 100.f / range);
		   }
	   }
	   return result;
	}

    
	public synchronized int getContrast() {
    	return getContrast(nativeGetContrast(mNativePtr));
    }

	public synchronized void resetContrast() {
    	if (mNativePtr != 0) {
    		nativeSetContrast(mNativePtr, mContrastDef);
    	}
    }


    
	public synchronized void setSharpness(final int sharpness) {
    	if (mNativePtr != 0) {
 		   final float range = Math.abs(mSharpnessMax - mSharpnessMin);
 		   if (range > 0)
 			   nativeSetSharpness(mNativePtr, (int)(sharpness / 100.f * range) + mSharpnessMin);
    	}
    }

    
	public synchronized int getSharpness(final int sharpness_abs) {
	   int result = 0;
	   if (mNativePtr != 0) {
		   nativeUpdateSharpnessLimit(mNativePtr);
		   final float range = Math.abs(mSharpnessMax - mSharpnessMin);
		   if (range > 0) {
			   result = (int)((sharpness_abs - mSharpnessMin) * 100.f / range);
		   }
	   }
	   return result;
	}

    
	public synchronized int getSharpness() {
    	return getSharpness(nativeGetSharpness(mNativePtr));
    }

	public synchronized void resetSharpness() {
    	if (mNativePtr != 0) {
    		nativeSetSharpness(mNativePtr, mSharpnessDef);
    	}
    }

    
	public synchronized void setGain(final int gain) {
    	if (mNativePtr != 0) {
 		   final float range = Math.abs(mGainMax - mGainMin);
 		   if (range > 0)
 			   nativeSetGain(mNativePtr, (int)(gain / 100.f * range) + mGainMin);
    	}
    }

    
	public synchronized int getGain(final int gain_abs) {
	   int result = 0;
	   if (mNativePtr != 0) {
		   nativeUpdateGainLimit(mNativePtr);
		   final float range = Math.abs(mGainMax - mGainMin);
		   if (range > 0) {
			   result = (int)((gain_abs - mGainMin) * 100.f / range);
		   }
	   }
	   return result;
	}

    
	public synchronized int getGain() {
    	return getGain(nativeGetGain(mNativePtr));
    }

	public synchronized void resetGain() {
    	if (mNativePtr != 0) {
    		nativeSetGain(mNativePtr, mGainDef);
    	}
    }


    
	public synchronized void setGamma(final int gamma) {
    	if (mNativePtr != 0) {
 		   final float range = Math.abs(mGammaMax - mGammaMin);
 		   if (range > 0)
 			   nativeSetGamma(mNativePtr, (int)(gamma / 100.f * range) + mGammaMin);
    	}
    }

    
	public synchronized int getGamma(final int gamma_abs) {
	   int result = 0;
	   if (mNativePtr != 0) {
		   nativeUpdateGammaLimit(mNativePtr);
		   final float range = Math.abs(mGammaMax - mGammaMin);
		   if (range > 0) {
			   result = (int)((gamma_abs - mGammaMin) * 100.f / range);
		   }
	   }
	   return result;
	}

    
	public synchronized int getGamma() {
    	return getGamma(nativeGetGamma(mNativePtr));
    }

	public synchronized void resetGamma() {
    	if (mNativePtr != 0) {
    		nativeSetGamma(mNativePtr, mGammaDef);
    	}
    }


    
	public synchronized void setSaturation(final int saturation) {
    	if (mNativePtr != 0) {
 		   final float range = Math.abs(mSaturationMax - mSaturationMin);
 		   if (range > 0)
 			   nativeSetSaturation(mNativePtr, (int)(saturation / 100.f * range) + mSaturationMin);
    	}
    }

    
	public synchronized int getSaturation(final int saturation_abs) {
	   int result = 0;
	   if (mNativePtr != 0) {
		   nativeUpdateSaturationLimit(mNativePtr);
		   final float range = Math.abs(mSaturationMax - mSaturationMin);
		   if (range > 0) {
			   result = (int)((saturation_abs - mSaturationMin) * 100.f / range);
		   }
	   }
	   return result;
	}

    
	public synchronized int getSaturation() {
    	return getSaturation(nativeGetSaturation(mNativePtr));
    }

	public synchronized void resetSaturation() {
    	if (mNativePtr != 0) {
    		nativeSetSaturation(mNativePtr, mSaturationDef);
    	}
    }

    
	public synchronized void setHue(final int hue) {
    	if (mNativePtr != 0) {
 		   final float range = Math.abs(mHueMax - mHueMin);
 		   if (range > 0)
 			   nativeSetHue(mNativePtr, (int)(hue / 100.f * range) + mHueMin);
    	}
    }

    
	public synchronized int getHue(final int hue_abs) {
	   int result = 0;
	   if (mNativePtr != 0) {
		   nativeUpdateHueLimit(mNativePtr);
		   final float range = Math.abs(mHueMax - mHueMin);
		   if (range > 0) {
			   result = (int)((hue_abs - mHueMin) * 100.f / range);
		   }
	   }
	   return result;
	}

    
	public synchronized int getHue() {
    	return getHue(nativeGetHue(mNativePtr));
    }

	public synchronized void resetHue() {
    	if (mNativePtr != 0) {
    		nativeSetHue(mNativePtr, mSaturationDef);
    	}
    }


	public void setPowerlineFrequency(final int frequency) {
    	if (mNativePtr != 0)
    		nativeSetPowerlineFrequency(mNativePtr, frequency);
    }

	public int getPowerlineFrequency() {
    	return nativeGetPowerlineFrequency(mNativePtr);
    }


    
	public synchronized void setZoom(final int zoom) {
    	if (mNativePtr != 0) {
 		   final float range = Math.abs(mZoomMax - mZoomMin);
 		   if (range > 0) {
 			   final int z = (int)(zoom / 100.f * range) + mZoomMin;

 			   nativeSetZoom(mNativePtr, z);
 		   }
    	}
    }

    
	public synchronized int getZoom(final int zoom_abs) {
	   int result = 0;
	   if (mNativePtr != 0) {
		   nativeUpdateZoomLimit(mNativePtr);
		   final float range = Math.abs(mZoomMax - mZoomMin);
		   if (range > 0) {
			   result = (int)((zoom_abs - mZoomMin) * 100.f / range);
		   }
	   }
	   return result;
	}

    
	public synchronized int getZoom() {
    	return getZoom(nativeGetZoom(mNativePtr));
    }

	public synchronized void resetZoom() {
    	if (mNativePtr != 0) {
    		nativeSetZoom(mNativePtr, mZoomDef);
    	}
    }



	
	public synchronized int sendCommand(int command) {
		if (mNativePtr != 0) {
			return nativeSendCommand(mNativePtr, command);
		}
		return (int) mNativePtr;
	}


	public synchronized void updateCameraParams() {
    	if (mNativePtr != 0) {
    		if ((mControlSupports == 0) || (mProcSupports == 0)) {
        		
    			if (mControlSupports == 0)
    				mControlSupports = nativeGetCtrlSupports(mNativePtr);
    			if (mProcSupports == 0)
    				mProcSupports = nativeGetProcSupports(mNativePtr);
    	    	
    	    	if ((mControlSupports != 0) && (mProcSupports != 0)) {
	    	    	nativeUpdateBrightnessLimit(mNativePtr);
	    	    	nativeUpdateContrastLimit(mNativePtr);
	    	    	nativeUpdateSharpnessLimit(mNativePtr);
	    	    	nativeUpdateGainLimit(mNativePtr);
	    	    	nativeUpdateGammaLimit(mNativePtr);
	    	    	nativeUpdateSaturationLimit(mNativePtr);
	    	    	nativeUpdateHueLimit(mNativePtr);
	    	    	nativeUpdateZoomLimit(mNativePtr);
	    	    	nativeUpdateWhiteBlanceLimit(mNativePtr);
	    	    	nativeUpdateFocusLimit(mNativePtr);
    	    	}
    	    	if (false) {
					dumpControls(mControlSupports);
					dumpProc(mProcSupports);
					XLogWrapper.v(TAG, String.format("Brightness:min=%d,max=%d,def=%d", mBrightnessMin, mBrightnessMax, mBrightnessDef));
					XLogWrapper.v(TAG, String.format("Contrast:min=%d,max=%d,def=%d", mContrastMin, mContrastMax, mContrastDef));
					XLogWrapper.v(TAG, String.format("Sharpness:min=%d,max=%d,def=%d", mSharpnessMin, mSharpnessMax, mSharpnessDef));
					XLogWrapper.v(TAG, String.format("Gain:min=%d,max=%d,def=%d", mGainMin, mGainMax, mGainDef));
					XLogWrapper.v(TAG, String.format("Gamma:min=%d,max=%d,def=%d", mGammaMin, mGammaMax, mGammaDef));
					XLogWrapper.v(TAG, String.format("Saturation:min=%d,max=%d,def=%d", mSaturationMin, mSaturationMax, mSaturationDef));
					XLogWrapper.v(TAG, String.format("Hue:min=%d,max=%d,def=%d", mHueMin, mHueMax, mHueDef));
					XLogWrapper.v(TAG, String.format("Zoom:min=%d,max=%d,def=%d", mZoomMin, mZoomMax, mZoomDef));
					XLogWrapper.v(TAG, String.format("WhiteBlance:min=%d,max=%d,def=%d", mWhiteBlanceMin, mWhiteBlanceMax, mWhiteBlanceDef));
					XLogWrapper.v(TAG, String.format("Focus:min=%d,max=%d,def=%d", mFocusMin, mFocusMax, mFocusDef));
				}
			}
    	} else {
    		mControlSupports = mProcSupports = 0;
    	}
    }

    private static final String[] SUPPORTS_CTRL = {
    	"D0:  Scanning Mode",
    	"D1:  Auto-Exposure Mode",
    	"D2:  Auto-Exposure Priority",
    	"D3:  Exposure Time (Absolute)",
    	"D4:  Exposure Time (Relative)",
    	"D5:  Focus (Absolute)",
    	"D6:  Focus (Relative)",
    	"D7:  Iris (Absolute)",
    	"D8:  Iris (Relative)",
    	"D9:  Zoom (Absolute)",
    	"D10: Zoom (Relative)",
    	"D11: PanTilt (Absolute)",
    	"D12: PanTilt (Relative)",
    	"D13: Roll (Absolute)",
    	"D14: Roll (Relative)",
		"D15: Reserved",
		"D16: Reserved",
		"D17: Focus, Auto",
		"D18: Privacy",
		"D19: Focus, Simple",
		"D20: Window",
		"D21: Region of Interest",
		"D22: Reserved, set to zero",
		"D23: Reserved, set to zero",
    };

    private static final String[] SUPPORTS_PROC = {
		"D0: Brightness",
		"D1: Contrast",
		"D2: Hue",
		"D3: Saturation",
		"D4: Sharpness",
		"D5: Gamma",
		"D6: White Balance Temperature",
		"D7: White Balance Component",
		"D8: Backlight Compensation",
		"D9: Gain",
		"D10: Power Line Frequency",
		"D11: Hue, Auto",
		"D12: White Balance Temperature, Auto",
		"D13: White Balance Component, Auto",
		"D14: Digital Multiplier",
		"D15: Digital Multiplier Limit",
		"D16: AnaXLogWrapper Video Standard",
		"D17: AnaXLogWrapper Video Lock Status",
		"D18: Contrast, Auto",
		"D19: Reserved. Set to zero",
		"D20: Reserved. Set to zero",
		"D21: Reserved. Set to zero",
		"D22: Reserved. Set to zero",
		"D23: Reserved. Set to zero",
	};

    private static final void dumpControls(final long controlSupports) {
    	XLogWrapper.i(TAG, String.format("controlSupports=%x", controlSupports));
    	for (int i = 0; i < SUPPORTS_CTRL.length; i++) {
    		XLogWrapper.i(TAG, SUPPORTS_CTRL[i] + ((controlSupports & (0x1 << i)) != 0 ? "=enabled" : "=disabled"));
    	}
    }

	private static final void dumpProc(final long procSupports) {
    	XLogWrapper.i(TAG, String.format("procSupports=%x", procSupports));
    	for (int i = 0; i < SUPPORTS_PROC.length; i++) {
    		XLogWrapper.i(TAG, SUPPORTS_PROC[i] + ((procSupports & (0x1 << i)) != 0 ? "=enabled" : "=disabled"));
    	}
    }

	private final String getUSBFSName(final UsbControlBlock ctrlBlock) {
		String result = null;
		final String name = ctrlBlock.getDeviceName();
		final String[] v = !TextUtils.isEmpty(name) ? name.split("/") : null;
		if ((v != null) && (v.length > 2)) {
			final StringBuilder sb = new StringBuilder(v[0]);
			for (int i = 1; i < v.length - 2; i++)
				sb.append("/").append(v[i]);
			result = sb.toString();
		}
		if (TextUtils.isEmpty(result)) {
			XLogWrapper.w(TAG, "failed to get USBFS path, try to use default path:" + name);
			result = DEFAULT_USBFS;
		}
		return result;
	}

	
	private final native long nativeCreate();
	private final native void nativeDestroy(final long id_camera);

	private final native int nativeConnect(long id_camera, int venderId, int productId, int fileDescriptor, int busNum, int devAddr, String usbfs);
	private static final native int nativeRelease(final long id_camera);

	private static final native int nativeSetStatusCallback(final long mNativePtr, final IStatusCallback callback);
	private static final native int nativeSetButtonCallback(final long mNativePtr, final IButtonCallback callback);

	private static final native int nativeSetPreviewSize(final long id_camera, final int width, final int height, final int min_fps, final int max_fps, final int mode, final float bandwidth);
	private static final native String nativeGetSupportedSize(final long id_camera);
	private static final native int nativeStartPreview(final long id_camera);
	private static final native int nativeStopPreview(final long id_camera);
	private static final native int nativeSetPreviewDisplay(final long id_camera, final Surface surface);
	private static final native int nativeSetFrameCallback(final long mNativePtr, final IFrameCallback callback, final int pixelFormat);


	
	public void startCapture(final Surface surface) {
		if (mCtrlBlock != null && surface != null) {
			nativeSetCaptureDisplay(mNativePtr, surface);
		} else
			throw new NullPointerException("startCapture");
	}

	
	public void stopCapture() {
		if (mCtrlBlock != null) {
			nativeSetCaptureDisplay(mNativePtr, null);
		}
	}
	private static final native int nativeSetCaptureDisplay(final long id_camera, final Surface surface);

	private static final native long nativeGetCtrlSupports(final long id_camera);
	private static final native long nativeGetProcSupports(final long id_camera);

	private final native int nativeUpdateScanningModeLimit(final long id_camera);
	private static final native int nativeSetScanningMode(final long id_camera, final int scanning_mode);
	private static final native int nativeGetScanningMode(final long id_camera);

	private final native int nativeUpdateExposureModeLimit(final long id_camera);
	private static final native int nativeSetExposureMode(final long id_camera, final int exposureMode);
	private static final native int nativeGetExposureMode(final long id_camera);

	private final native int nativeUpdateExposurePriorityLimit(final long id_camera);
	private static final native int nativeSetExposurePriority(final long id_camera, final int priority);
	private static final native int nativeGetExposurePriority(final long id_camera);

	private final native int nativeUpdateExposureLimit(final long id_camera);
	private static final native int nativeSetExposure(final long id_camera, final int exposure);
	private static final native int nativeGetExposure(final long id_camera);

	private final native int nativeUpdateExposureRelLimit(final long id_camera);
	private static final native int nativeSetExposureRel(final long id_camera, final int exposure_rel);
	private static final native int nativeGetExposureRel(final long id_camera);

	private final native int nativeUpdateAutoFocusLimit(final long id_camera);
	private static final native int nativeSetAutoFocus(final long id_camera, final boolean autofocus);
	private static final native int nativeGetAutoFocus(final long id_camera);

	private final native int nativeUpdateFocusLimit(final long id_camera);
	private static final native int nativeSetFocus(final long id_camera, final int focus);
	private static final native int nativeGetFocus(final long id_camera);

	private final native int nativeUpdateFocusRelLimit(final long id_camera);
	private static final native int nativeSetFocusRel(final long id_camera, final int focus_rel);
	private static final native int nativeGetFocusRel(final long id_camera);

	private final native int nativeUpdateIrisLimit(final long id_camera);
	private static final native int nativeSetIris(final long id_camera, final int iris);
	private static final native int nativeGetIris(final long id_camera);

	private final native int nativeUpdateIrisRelLimit(final long id_camera);
	private static final native int nativeSetIrisRel(final long id_camera, final int iris_rel);
	private static final native int nativeGetIrisRel(final long id_camera);

	private final native int nativeUpdatePanLimit(final long id_camera);
	private static final native int nativeSetPan(final long id_camera, final int pan);
	private static final native int nativeGetPan(final long id_camera);

	private final native int nativeUpdatePanRelLimit(final long id_camera);
	private static final native int nativeSetPanRel(final long id_camera, final int pan_rel);
	private static final native int nativeGetPanRel(final long id_camera);

	private final native int nativeUpdateTiltLimit(final long id_camera);
	private static final native int nativeSetTilt(final long id_camera, final int tilt);
	private static final native int nativeGetTilt(final long id_camera);

	private final native int nativeUpdateTiltRelLimit(final long id_camera);
	private static final native int nativeSetTiltRel(final long id_camera, final int tilt_rel);
	private static final native int nativeGetTiltRel(final long id_camera);

	private final native int nativeUpdateRollLimit(final long id_camera);
	private static final native int nativeSetRoll(final long id_camera, final int roll);
	private static final native int nativeGetRoll(final long id_camera);

	private final native int nativeUpdateRollRelLimit(final long id_camera);
	private static final native int nativeSetRollRel(final long id_camera, final int roll_rel);
	private static final native int nativeGetRollRel(final long id_camera);

	private final native int nativeUpdateAutoWhiteBlanceLimit(final long id_camera);
	private static final native int nativeSetAutoWhiteBlance(final long id_camera, final boolean autoWhiteBlance);
	private static final native int nativeGetAutoWhiteBlance(final long id_camera);

	private final native int nativeUpdateAutoWhiteBlanceCompoLimit(final long id_camera);
	private static final native int nativeSetAutoWhiteBlanceCompo(final long id_camera, final boolean autoWhiteBlanceCompo);
	private static final native int nativeGetAutoWhiteBlanceCompo(final long id_camera);

	private final native int nativeUpdateWhiteBlanceLimit(final long id_camera);
	private static final native int nativeSetWhiteBlance(final long id_camera, final int whiteBlance);
	private static final native int nativeGetWhiteBlance(final long id_camera);

	private final native int nativeUpdateWhiteBlanceCompoLimit(final long id_camera);
	private static final native int nativeSetWhiteBlanceCompo(final long id_camera, final int whiteBlance_compo);
	private static final native int nativeGetWhiteBlanceCompo(final long id_camera);

	private final native int nativeUpdateBacklightCompLimit(final long id_camera);
	private static final native int nativeSetBacklightComp(final long id_camera, final int backlight_comp);
	private static final native int nativeGetBacklightComp(final long id_camera);

	private final native int nativeUpdateBrightnessLimit(final long id_camera);
	private static final native int nativeSetBrightness(final long id_camera, final int brightness);
	private static final native int nativeGetBrightness(final long id_camera);

	private final native int nativeUpdateContrastLimit(final long id_camera);
	private static final native int nativeSetContrast(final long id_camera, final int contrast);
	private static final native int nativeGetContrast(final long id_camera);

	private final native int nativeUpdateAutoContrastLimit(final long id_camera);
	private static final native int nativeSetAutoContrast(final long id_camera, final boolean autocontrast);
	private static final native int nativeGetAutoContrast(final long id_camera);

	private final native int nativeUpdateSharpnessLimit(final long id_camera);
	private static final native int nativeSetSharpness(final long id_camera, final int sharpness);
	private static final native int nativeGetSharpness(final long id_camera);

	private final native int nativeUpdateGainLimit(final long id_camera);
	private static final native int nativeSetGain(final long id_camera, final int gain);
	private static final native int nativeGetGain(final long id_camera);

	private final native int nativeUpdateGammaLimit(final long id_camera);
	private static final native int nativeSetGamma(final long id_camera, final int gamma);
	private static final native int nativeGetGamma(final long id_camera);

	private final native int nativeUpdateSaturationLimit(final long id_camera);
	private static final native int nativeSetSaturation(final long id_camera, final int saturation);
	private static final native int nativeGetSaturation(final long id_camera);

	private final native int nativeUpdateHueLimit(final long id_camera);
	private static final native int nativeSetHue(final long id_camera, final int hue);
	private static final native int nativeGetHue(final long id_camera);

	private final native int nativeUpdateAutoHueLimit(final long id_camera);
	private static final native int nativeSetAutoHue(final long id_camera, final boolean autohue);
	private static final native int nativeGetAutoHue(final long id_camera);

	private final native int nativeUpdatePowerlineFrequencyLimit(final long id_camera);
	private static final native int nativeSetPowerlineFrequency(final long id_camera, final int frequency);
	private static final native int nativeGetPowerlineFrequency(final long id_camera);

	private static final native int nativeSendCommand(final long id_camera, final int command);

	private final native int nativeUpdateZoomLimit(final long id_camera);
	private static final native int nativeSetZoom(final long id_camera, final int zoom);
	private static final native int nativeGetZoom(final long id_camera);

	private final native int nativeUpdateZoomRelLimit(final long id_camera);
	private static final native int nativeSetZoomRel(final long id_camera, final int zoom_rel);
	private static final native int nativeGetZoomRel(final long id_camera);

	private final native int nativeUpdateDigitalMultiplierLimit(final long id_camera);
	private static final native int nativeSetDigitalMultiplier(final long id_camera, final int multiplier);
	private static final native int nativeGetDigitalMultiplier(final long id_camera);

	private final native int nativeUpdateDigitalMultiplierLimitLimit(final long id_camera);
	private static final native int nativeSetDigitalMultiplierLimit(final long id_camera, final int multiplier_limit);
	private static final native int nativeGetDigitalMultiplierLimit(final long id_camera);

	private final native int nativeUpdateAnalogVideoStandardLimit(final long id_camera);
	private static final native int nativeSetAnalogVideoStandard(final long id_camera, final int standard);
	private static final native int nativeGetAnalogVideoStandard(final long id_camera);

	private final native int nativeUpdateAnalogVideoLockStateLimit(final long id_camera);
	private static final native int nativeSetAnalogVideoLoackState(final long id_camera, final int state);
	private static final native int nativeGetAnalogVideoLoackState(final long id_camera);

	private final native int nativeUpdatePrivacyLimit(final long id_camera);
	private static final native int nativeSetPrivacy(final long id_camera, final boolean privacy);
	private static final native int nativeGetPrivacy(final long id_camera);
}
