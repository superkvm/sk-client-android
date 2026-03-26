

package com.superkvm.usb;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbInterface;
import android.hardware.usb.UsbManager;
import android.os.Build;
import android.os.Handler;
import android.text.TextUtils;
import android.util.SparseArray;

import androidx.annotation.RequiresApi;

import com.superkvm.utils.BuildCheck;
import com.superkvm.utils.HandlerThreadHandler;
import com.superkvm.utils.XLogWrapper;

import java.io.UnsupportedEncodingException;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Locale;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;

public final class USBMonitor {

	public static boolean DEBUG = true;	
	private static final String TAG = "USBMonitor";

	private static final String ACTION_USB_PERMISSION_BASE = "com.serenegiant.USB_PERMISSION.";
	private final String ACTION_USB_PERMISSION = ACTION_USB_PERMISSION_BASE + hashCode();

	public static final String ACTION_USB_DEVICE_ATTACHED = "android.hardware.usb.action.USB_DEVICE_ATTACHED";

	
	private final ConcurrentHashMap<UsbDevice, UsbControlBlock> mCtrlBlocks = new ConcurrentHashMap<UsbDevice, UsbControlBlock>();
	private final SparseArray<WeakReference<UsbDevice>> mHasPermissions = new SparseArray<WeakReference<UsbDevice>>();

	private final WeakReference<Context> mWeakContext;
	private final UsbManager mUsbManager;
	private final OnDeviceConnectListener mOnDeviceConnectListener;
	private PendingIntent mPermissionIntent = null;
	private List<DeviceFilter> mDeviceFilters = new ArrayList<DeviceFilter>();

	
	private final Handler mAsyncHandler;
	private volatile boolean destroyed;
	
	public interface OnDeviceConnectListener {
		
		public void onAttach(UsbDevice device);
		
		public void onDetach(UsbDevice device);
		
		public void onConnect(UsbDevice device, UsbControlBlock ctrlBlock, boolean createNew);
		
		public void onDisconnect(UsbDevice device, UsbControlBlock ctrlBlock);
		
		public void onCancel(UsbDevice device);
	}

	public USBMonitor(final Context context, final OnDeviceConnectListener listener) {
		if (DEBUG) XLogWrapper.v(TAG, "USBMonitor:Constructor");
		if (listener == null)
			throw new IllegalArgumentException("OnDeviceConnectListener should not null.");
		mWeakContext = new WeakReference<Context>(context);
		mUsbManager = (UsbManager)context.getSystemService(Context.USB_SERVICE);
		mOnDeviceConnectListener = listener;
		mAsyncHandler = HandlerThreadHandler.createHandler(TAG);
		destroyed = false;
		if (DEBUG) XLogWrapper.v(TAG, "USBMonitor:mUsbManager=" + mUsbManager);
	}

	
	public void destroy() {
		if (DEBUG) XLogWrapper.i(TAG, "destroy:");
		unregister();
		if (!destroyed) {
			destroyed = true;
			
			final Set<UsbDevice> keys = mCtrlBlocks.keySet();
			if (keys != null) {
				UsbControlBlock ctrlBlock;
				try {
					for (final UsbDevice key: keys) {
						ctrlBlock = mCtrlBlocks.remove(key);
						if (ctrlBlock != null) {
							ctrlBlock.close();
						}
					}
				} catch (final Exception e) {
					XLogWrapper.e(TAG, "destroy:", e);
				}
			}
			mCtrlBlocks.clear();
			try {
				mAsyncHandler.getLooper().quit();
			} catch (final Exception e) {
				XLogWrapper.e(TAG, "destroy:", e);
			}
		}
	}

	
	@SuppressLint({"UnspecifiedImmutableFlag", "WrongConstant"})
	public synchronized void register() throws IllegalStateException {
		if (destroyed) throw new IllegalStateException("already destroyed");
		if (mPermissionIntent == null) {
			if (DEBUG) XLogWrapper.i(TAG, "register:");
			final Context context = mWeakContext.get();
			if (context != null) {
        if (Build.VERSION.SDK_INT >= 34) {
          mPermissionIntent = PendingIntent.getBroadcast(context, 0, new Intent(ACTION_USB_PERMISSION), PendingIntent.FLAG_IMMUTABLE);
        }
				else if (Build.VERSION.SDK_INT >= 31) {
					
					
					
					int PENDING_FLAG_IMMUTABLE = 1<<25;
					mPermissionIntent = PendingIntent.getBroadcast(context, 0, new Intent(ACTION_USB_PERMISSION), PENDING_FLAG_IMMUTABLE);
				} else {
					mPermissionIntent = PendingIntent.getBroadcast(context, 0, new Intent(ACTION_USB_PERMISSION), 0);
				}
				final IntentFilter filter = new IntentFilter(ACTION_USB_PERMISSION);
				
				filter.addAction(ACTION_USB_DEVICE_ATTACHED);
				filter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);
				if (Build.VERSION.SDK_INT >= 34) {
					
					int RECEIVER_NOT_EXPORTED = 4;
					context.registerReceiver(mUsbReceiver, filter, RECEIVER_NOT_EXPORTED);
				} else {
					context.registerReceiver(mUsbReceiver, filter);
				}
			}
			
			mDeviceCounts = 0;
			mAsyncHandler.postDelayed(mDeviceCheckRunnable, 1000);
		}
	}

	
	public synchronized void unregister() throws IllegalStateException {
		
		mDeviceCounts = 0;
		if (!destroyed) {
			mAsyncHandler.removeCallbacks(mDeviceCheckRunnable);
		}
		if (mPermissionIntent != null) {

			final Context context = mWeakContext.get();
			try {
				if (context != null) {
					context.unregisterReceiver(mUsbReceiver);
				}
			} catch (final Exception e) {
				XLogWrapper.w(TAG, e);
			}
			mPermissionIntent = null;
		}
	}

	public synchronized boolean isRegistered() {
		return !destroyed && (mPermissionIntent != null);
	}

	
	public void setDeviceFilter(final DeviceFilter filter) throws IllegalStateException {
		if (destroyed) throw new IllegalStateException("already destroyed");
		mDeviceFilters.clear();
		mDeviceFilters.add(filter);
	}

	
	public void addDeviceFilter(final DeviceFilter filter) throws IllegalStateException {
		if (destroyed) throw new IllegalStateException("already destroyed");
		mDeviceFilters.add(filter);
	}

	
	public void removeDeviceFilter(final DeviceFilter filter) throws IllegalStateException {
		if (destroyed) throw new IllegalStateException("already destroyed");
		mDeviceFilters.remove(filter);
	}

	
	public void setDeviceFilter(final List<DeviceFilter> filters) throws IllegalStateException {
		if (destroyed) throw new IllegalStateException("already destroyed");
		mDeviceFilters.clear();
		mDeviceFilters.addAll(filters);
	}

	
	public void addDeviceFilter(final List<DeviceFilter> filters) throws IllegalStateException {
		if (destroyed) throw new IllegalStateException("already destroyed");
		mDeviceFilters.addAll(filters);
	}

	
	public void removeDeviceFilter(final List<DeviceFilter> filters) throws IllegalStateException {
		if (destroyed) throw new IllegalStateException("already destroyed");
		mDeviceFilters.removeAll(filters);
	}

	
	public int getDeviceCount() throws IllegalStateException {
		if (destroyed) throw new IllegalStateException("already destroyed");
		return getDeviceList().size();
	}

	
	public List<UsbDevice> getDeviceList() throws IllegalStateException {
		if (destroyed) throw new IllegalStateException("already destroyed");
		return getDeviceList(mDeviceFilters);
	}

	
	public List<UsbDevice> getDeviceList(final List<DeviceFilter> filters) throws IllegalStateException {
		if (destroyed) throw new IllegalStateException("already destroyed");
		
		final HashMap<String, UsbDevice> deviceList = mUsbManager.getDeviceList();
		final List<UsbDevice> result = new ArrayList<UsbDevice>();
		if (deviceList != null) {
			if ((filters == null) || filters.isEmpty()) {
				result.addAll(deviceList.values());
			} else {
				for (final UsbDevice device: deviceList.values() ) {
					
					for (final DeviceFilter filter: filters) {
						if ((filter != null) && filter.matches(device) || (filter != null && filter.mSubclass == device.getDeviceSubclass())) {
							
							if (!filter.isExclude) {
								result.add(device);
							}
							break;
						}
					}
				}
			}
		}
		return result;
	}

	
	public List<UsbDevice> getDeviceList(final DeviceFilter filter) throws IllegalStateException {
		if (destroyed) throw new IllegalStateException("already destroyed");
		final HashMap<String, UsbDevice> deviceList = mUsbManager.getDeviceList();
		final List<UsbDevice> result = new ArrayList<UsbDevice>();
		if (deviceList != null) {
			for (final UsbDevice device: deviceList.values() ) {
				if ((filter == null) || (filter.matches(device) && !filter.isExclude)) {
					result.add(device);
				}
			}
		}
		return result;
	}

	
	public Iterator<UsbDevice> getDevices() throws IllegalStateException {
		if (destroyed) throw new IllegalStateException("already destroyed");
		Iterator<UsbDevice> iterator = null;
		final HashMap<String, UsbDevice> list = mUsbManager.getDeviceList();
		if (list != null)
			iterator = list.values().iterator();
		return iterator;
	}

	
	public final void dumpDevices() {
		final HashMap<String, UsbDevice> list = mUsbManager.getDeviceList();
		if (list != null) {
			final Set<String> keys = list.keySet();
			if (keys != null && keys.size() > 0) {
				final StringBuilder sb = new StringBuilder();
				for (final String key: keys) {
					final UsbDevice device = list.get(key);
					final int num_interface = device != null ? device.getInterfaceCount() : 0;
					sb.setLength(0);
					for (int i = 0; i < num_interface; i++) {
						sb.append(String.format(Locale.US, "interface%d:%s", i, device.getInterface(i).toString()));
					}
					XLogWrapper.i(TAG, "key=" + key + ":" + device + ":" + sb.toString());
				}
			} else {
				XLogWrapper.i(TAG, "no device");
			}
		} else {
			XLogWrapper.i(TAG, "no device");
		}
	}

	
	public final boolean hasPermission(final UsbDevice device) throws IllegalStateException {
		if (destroyed) {
			XLogWrapper.w(TAG, "hasPermission failed, camera destroyed!");
			return false;
		}
		return updatePermission(device, device != null && mUsbManager.hasPermission(device));
	}

	
	private boolean updatePermission(final UsbDevice device, final boolean hasPermission) {
		
		try {
			final int deviceKey = getDeviceKey(device, true);
			synchronized (mHasPermissions) {
				if (hasPermission) {
					if (mHasPermissions.get(deviceKey) == null) {
						mHasPermissions.put(deviceKey, new WeakReference<UsbDevice>(device));
					}
				} else {
					mHasPermissions.remove(deviceKey);
				}
			}
		} catch (SecurityException e) {
			XLogWrapper.w("superkvm", e.getLocalizedMessage());
		}

		return hasPermission;
	}

	
	public synchronized boolean requestPermission(final UsbDevice device) {

		boolean result = false;
		if (isRegistered()) {
			if (device != null) {
				if (DEBUG) XLogWrapper.i(TAG,"request permission, has permission: " + mUsbManager.hasPermission(device));
				if (mUsbManager.hasPermission(device)) {
					
					processConnect(device);
				} else {
					try {
						
						if (DEBUG) XLogWrapper.i(TAG, "start request permission...");
						mUsbManager.requestPermission(device, mPermissionIntent);
					} catch (final Exception e) {
						
						XLogWrapper.w(TAG,"request permission failed, e = " + e.getLocalizedMessage() ,e);
						processCancel(device);
						result = true;
					}
				}
			} else {
				if (DEBUG)
					XLogWrapper.w(TAG,"request permission failed, device is null?");
				processCancel(device);
				result = true;
			}
		} else {
			if (DEBUG)
				XLogWrapper.w(TAG,"request permission failed, not registered?");
			processCancel(device);
			result = true;
		}
		return result;
	}

	
	public UsbControlBlock openDevice(final UsbDevice device) throws SecurityException {
		if (hasPermission(device)) {
			UsbControlBlock result = mCtrlBlocks.get(device);
			if (result == null) {
				result = new UsbControlBlock(USBMonitor.this, device);    
				mCtrlBlocks.put(device, result);
			}
			return result;
		} else {
			throw new SecurityException("has no permission");
		}
	}

	
	private final BroadcastReceiver mUsbReceiver = new BroadcastReceiver() {

		@Override
		public void onReceive(final Context context, final Intent intent) {
			if (destroyed) return;
			final String action = intent.getAction();
			if (ACTION_USB_PERMISSION.equals(action)) {
				
				synchronized (USBMonitor.this) {
					final UsbDevice device = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
					if (intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)) {
						if (device != null) {
							
							if (DEBUG)
								XLogWrapper.w(TAG, "get permission success in mUsbReceiver");
							processConnect(device);
						}
					} else {
						
						if (DEBUG)
							XLogWrapper.w(TAG, "get permission failed in mUsbReceiver");
						processCancel(device);
					}
				}
			} else if (UsbManager.ACTION_USB_DEVICE_ATTACHED.equals(action)) {
				final UsbDevice device = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
				updatePermission(device, hasPermission(device));
				processAttach(device);
			} else if (UsbManager.ACTION_USB_DEVICE_DETACHED.equals(action)) {
				
				final UsbDevice device = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
				if (device != null) {
					UsbControlBlock ctrlBlock = mCtrlBlocks.remove(device);
					if (ctrlBlock != null) {
						
						ctrlBlock.close();
					}
					mDeviceCounts = 0;
					processDettach(device);
				}
			}
		}
	};

	
	private volatile int mDeviceCounts = 0;
	
	private final Runnable mDeviceCheckRunnable = new Runnable() {
		@Override
		public void run() {
			if (destroyed) return;
			final List<UsbDevice> devices = getDeviceList();
			final int n = devices.size();
			final int hasPermissionCounts;
			final int m;
			synchronized (mHasPermissions) {
				hasPermissionCounts = mHasPermissions.size();
				mHasPermissions.clear();
				for (final UsbDevice device: devices) {
					hasPermission(device);
				}
				m = mHasPermissions.size();
			}
			if ((n > mDeviceCounts) || (m > hasPermissionCounts)) {
				mDeviceCounts = n;
				if (mOnDeviceConnectListener != null) {
					for (int i = 0; i < n; i++) {
						final UsbDevice device = devices.get(i);
						mAsyncHandler.post(new Runnable() {
							@Override
							public void run() {
								mOnDeviceConnectListener.onAttach(device);
							}
						});
					}
				}
			}
			mAsyncHandler.postDelayed(this, 2000);	
		}
	};

	
	private void processConnect(final UsbDevice device) {
		if (destroyed) return;
		updatePermission(device, true);
		mAsyncHandler.post(() -> {
			if (DEBUG) XLogWrapper.v(TAG, "processConnect:device=" + device.getDeviceName());
			UsbControlBlock ctrlBlock;
			final boolean createNew;
			ctrlBlock = mCtrlBlocks.get(device);
			if (ctrlBlock == null) {
				ctrlBlock = new UsbControlBlock(USBMonitor.this, device);
				mCtrlBlocks.put(device, ctrlBlock);
				createNew = true;
			} else {
				createNew = false;
			}
			if (mOnDeviceConnectListener != null) {
				if (ctrlBlock.getConnection() == null) {
					XLogWrapper.e(TAG, "processConnect: Open device failed");
					mOnDeviceConnectListener.onCancel(device);
					return;
				}
				mOnDeviceConnectListener.onConnect(device, ctrlBlock, createNew);
			}
		});
	}

	private void processCancel(final UsbDevice device) {
		if (destroyed) return;
		if (DEBUG) XLogWrapper.v(TAG, "processCancel:");
		updatePermission(device, false);
		if (mOnDeviceConnectListener != null) {
			mAsyncHandler.post(new Runnable() {
				@Override
				public void run() {
					mOnDeviceConnectListener.onCancel(device);
				}
			});
		}
	}

	private void processAttach(final UsbDevice device) {
		if (destroyed) return;
		if (DEBUG) XLogWrapper.v(TAG, "processAttach:");
		if (mOnDeviceConnectListener != null) {
			mAsyncHandler.post(new Runnable() {
				@Override
				public void run() {
					mOnDeviceConnectListener.onAttach(device);
				}
			});
		}
	}

	private void processDettach(final UsbDevice device) {
		if (destroyed) return;
		if (DEBUG) XLogWrapper.v(TAG, "processDettach:");
		if (mOnDeviceConnectListener != null) {
			mAsyncHandler.post(new Runnable() {
				@Override
				public void run() {
					mOnDeviceConnectListener.onDetach(device);
				}
			});
		}
	}

	
	public static final String getDeviceKeyName(final UsbDevice device) {
		return getDeviceKeyName(device, null, false);
	}

	
	public static final String getDeviceKeyName(final UsbDevice device, final boolean useNewAPI) {
		return getDeviceKeyName(device, null, useNewAPI);
	}
	
	@SuppressLint("NewApi")
	public static final String getDeviceKeyName(final UsbDevice device, final String serial, final boolean useNewAPI) {
		if (device == null) return "";
		final StringBuilder sb = new StringBuilder();
		sb.append(device.getVendorId());			sb.append("#");	
		sb.append(device.getProductId());			sb.append("#");	
		sb.append(device.getDeviceClass());			sb.append("#");	
		sb.append(device.getDeviceSubclass());		sb.append("#");	
		sb.append(device.getDeviceProtocol());						
		if (!TextUtils.isEmpty(serial)) {
			sb.append("#");	sb.append(serial);
		}
		if (useNewAPI && BuildCheck.isAndroid5()) {
			sb.append("#");
			if (TextUtils.isEmpty(serial)) {
				try { sb.append(device.getSerialNumber());	sb.append("#");	} 
				catch(SecurityException ignore) {}
			}
			sb.append(device.getManufacturerName());	sb.append("#");	
			sb.append(device.getConfigurationCount());	sb.append("#");	
			if (BuildCheck.isMarshmallow()) {
				sb.append(device.getVersion());			sb.append("#");	
			}
		}

		return sb.toString();
	}

	
	public static final int getDeviceKey(final UsbDevice device) {
		return device != null ? getDeviceKeyName(device, null, false).hashCode() : 0;
	}

	
	public static final int getDeviceKey(final UsbDevice device, final boolean useNewAPI) {
		return device != null ? getDeviceKeyName(device, null, useNewAPI).hashCode() : 0;
	}

	
	public static final int getDeviceKey(final UsbDevice device, final String serial, final boolean useNewAPI) {
		return device != null ? getDeviceKeyName(device, serial, useNewAPI).hashCode() : 0;
	}

	public static class UsbDeviceInfo {
		public String usb_version;
		public String manufacturer;
		public String product;
		public String version;
		public String serial;

		private void clear() {
			usb_version = manufacturer = product = version = serial = null;
		}

		@Override
		public String toString() {
			return String.format("UsbDevice:usb_version=%s,manufacturer=%s,product=%s,version=%s,serial=%s",
				usb_version != null ? usb_version : "",
				manufacturer != null ? manufacturer : "",
				product != null ? product : "",
				version != null ? version : "",
				serial != null ? serial : "");
		}
	}

	private static final int USB_DIR_OUT = 0;
	private static final int USB_DIR_IN = 0x80;
	private static final int USB_TYPE_MASK = (0x03 << 5);
	private static final int USB_TYPE_STANDARD = (0x00 << 5);
	private static final int USB_TYPE_CLASS = (0x01 << 5);
	private static final int USB_TYPE_VENDOR = (0x02 << 5);
	private static final int USB_TYPE_RESERVED = (0x03 << 5);
	private static final int USB_RECIP_MASK = 0x1f;
	private static final int USB_RECIP_DEVICE = 0x00;
	private static final int USB_RECIP_INTERFACE = 0x01;
	private static final int USB_RECIP_ENDPOINT = 0x02;
	private static final int USB_RECIP_OTHER = 0x03;
	private static final int USB_RECIP_PORT = 0x04;
	private static final int USB_RECIP_RPIPE = 0x05;
	private static final int USB_REQ_GET_STATUS = 0x00;
	private static final int USB_REQ_CLEAR_FEATURE = 0x01;
	private static final int USB_REQ_SET_FEATURE = 0x03;
	private static final int USB_REQ_SET_ADDRESS = 0x05;
	private static final int USB_REQ_GET_DESCRIPTOR = 0x06;
	private static final int USB_REQ_SET_DESCRIPTOR = 0x07;
	private static final int USB_REQ_GET_CONFIGURATION = 0x08;
	private static final int USB_REQ_SET_CONFIGURATION = 0x09;
	private static final int USB_REQ_GET_INTERFACE = 0x0A;
	private static final int USB_REQ_SET_INTERFACE = 0x0B;
	private static final int USB_REQ_SYNCH_FRAME = 0x0C;
	private static final int USB_REQ_SET_SEL = 0x30;
	private static final int USB_REQ_SET_ISOCH_DELAY = 0x31;
	private static final int USB_REQ_SET_ENCRYPTION = 0x0D;
	private static final int USB_REQ_GET_ENCRYPTION = 0x0E;
	private static final int USB_REQ_RPIPE_ABORT = 0x0E;
	private static final int USB_REQ_SET_HANDSHAKE = 0x0F;
	private static final int USB_REQ_RPIPE_RESET = 0x0F;
	private static final int USB_REQ_GET_HANDSHAKE = 0x10;
	private static final int USB_REQ_SET_CONNECTION = 0x11;
	private static final int USB_REQ_SET_SECURITY_DATA = 0x12;
	private static final int USB_REQ_GET_SECURITY_DATA = 0x13;
	private static final int USB_REQ_SET_WUSB_DATA = 0x14;
	private static final int USB_REQ_LOOPBACK_DATA_WRITE = 0x15;
	private static final int USB_REQ_LOOPBACK_DATA_READ = 0x16;
	private static final int USB_REQ_SET_INTERFACE_DS = 0x17;

	private static final int USB_REQ_STANDARD_DEVICE_SET = (USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_DEVICE);		
	private static final int USB_REQ_STANDARD_DEVICE_GET = (USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_DEVICE);			
	private static final int USB_REQ_STANDARD_INTERFACE_SET = (USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_INTERFACE);	
	private static final int USB_REQ_STANDARD_INTERFACE_GET = (USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_INTERFACE);	
	private static final int USB_REQ_STANDARD_ENDPOINT_SET = (USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_ENDPOINT);	
	private static final int USB_REQ_STANDARD_ENDPOINT_GET = (USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_ENDPOINT);		

	private static final int USB_REQ_CS_DEVICE_SET  = (USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_DEVICE);				
	private static final int USB_REQ_CS_DEVICE_GET = (USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_DEVICE);					
	private static final int USB_REQ_CS_INTERFACE_SET = (USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE);			
	private static final int USB_REQ_CS_INTERFACE_GET = (USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE);			
	private static final int USB_REQ_CS_ENDPOINT_SET = (USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_ENDPOINT);				
	private static final int USB_REQ_CS_ENDPOINT_GET = (USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_ENDPOINT);				

	private static final int USB_REQ_VENDER_DEVICE_SET = (USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_DEVICE);				
	private static final int USB_REQ_VENDER_DEVICE_GET = (USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_DEVICE);				
	private static final int USB_REQ_VENDER_INTERFACE_SET = (USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE);		
	private static final int USB_REQ_VENDER_INTERFACE_GET = (USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE);		
	private static final int USB_REQ_VENDER_ENDPOINT_SET = (USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_ENDPOINT);			
	private static final int USB_REQ_VENDER_ENDPOINT_GET = (USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_ENDPOINT);			

	private static final int USB_DT_DEVICE = 0x01;
	private static final int USB_DT_CONFIG = 0x02;
	private static final int USB_DT_STRING = 0x03;
	private static final int USB_DT_INTERFACE = 0x04;
	private static final int USB_DT_ENDPOINT = 0x05;
	private static final int USB_DT_DEVICE_QUALIFIER = 0x06;
	private static final int USB_DT_OTHER_SPEED_CONFIG = 0x07;
	private static final int USB_DT_INTERFACE_POWER = 0x08;
	private static final int USB_DT_OTG = 0x09;
	private static final int USB_DT_DEBUG = 0x0a;
	private static final int USB_DT_INTERFACE_ASSOCIATION = 0x0b;
	private static final int USB_DT_SECURITY = 0x0c;
	private static final int USB_DT_KEY = 0x0d;
	private static final int USB_DT_ENCRYPTION_TYPE = 0x0e;
	private static final int USB_DT_BOS = 0x0f;
	private static final int USB_DT_DEVICE_CAPABILITY = 0x10;
	private static final int USB_DT_WIRELESS_ENDPOINT_COMP = 0x11;
	private static final int USB_DT_WIRE_ADAPTER = 0x21;
	private static final int USB_DT_RPIPE = 0x22;
	private static final int USB_DT_CS_RADIO_CONTROL = 0x23;
	private static final int USB_DT_PIPE_USAGE = 0x24;
	private static final int USB_DT_SS_ENDPOINT_COMP = 0x30;
	private static final int USB_DT_CS_DEVICE = (USB_TYPE_CLASS | USB_DT_DEVICE);
	private static final int USB_DT_CS_CONFIG = (USB_TYPE_CLASS | USB_DT_CONFIG);
	private static final int USB_DT_CS_STRING = (USB_TYPE_CLASS | USB_DT_STRING);
	private static final int USB_DT_CS_INTERFACE = (USB_TYPE_CLASS | USB_DT_INTERFACE);
	private static final int USB_DT_CS_ENDPOINT = (USB_TYPE_CLASS | USB_DT_ENDPOINT);
	private static final int USB_DT_DEVICE_SIZE = 18;

	
	private static String getString(final UsbDeviceConnection connection, final int id, final int languageCount, final byte[] languages) {
		final byte[] work = new byte[256];
		String result = null;
		for (int i = 1; i <= languageCount; i++) {
			int ret = connection.controlTransfer(
				USB_REQ_STANDARD_DEVICE_GET, 
				USB_REQ_GET_DESCRIPTOR,
				(USB_DT_STRING << 8) | id, languages[i], work, 256, 0);
			if ((ret > 2) && (work[0] == ret) && (work[1] == USB_DT_STRING)) {
				
				try {
					result = new String(work, 2, ret - 2, "UTF-16LE");
					if (!"Љ".equals(result)) {	
						break;
					} else {
						result = null;
					}
				} catch (final UnsupportedEncodingException e) {
					
				}
			}
		}
		return result;
	}

	
	public UsbDeviceInfo getDeviceInfo(final UsbDevice device) {
		return updateDeviceInfo(mUsbManager, device, null);
	}

	
	public static UsbDeviceInfo getDeviceInfo(final Context context, final UsbDevice device) {
		return updateDeviceInfo((UsbManager)context.getSystemService(Context.USB_SERVICE), device, new UsbDeviceInfo());
	}

	
	@TargetApi(Build.VERSION_CODES.M)
	public static UsbDeviceInfo updateDeviceInfo(final UsbManager manager, final UsbDevice device, final UsbDeviceInfo _info) {
		final UsbDeviceInfo info = _info != null ? _info : new UsbDeviceInfo();
		info.clear();

		if (device != null) {
			if (BuildCheck.isLollipop()) {
				info.manufacturer = device.getManufacturerName();
				info.product = device.getProductName();
				info.serial = device.getSerialNumber();
			}
			if (BuildCheck.isMarshmallow()) {
				info.usb_version = device.getVersion();
			}
			if ((manager != null) && manager.hasPermission(device)) {
				final UsbDeviceConnection connection = manager.openDevice(device);
				if(connection == null) {
					return null;
				}
				final byte[] desc = connection.getRawDescriptors();

				if (TextUtils.isEmpty(info.usb_version)) {
					info.usb_version = String.format("%x.%02x", ((int)desc[3] & 0xff), ((int)desc[2] & 0xff));
				}
				if (TextUtils.isEmpty(info.version)) {
					info.version = String.format("%x.%02x", ((int)desc[13] & 0xff), ((int)desc[12] & 0xff));
				}
				if (TextUtils.isEmpty(info.serial)) {
					info.serial = connection.getSerial();
				}

				final byte[] languages = new byte[256];
				int languageCount = 0;
				
				try {
					int result = connection.controlTransfer(
						USB_REQ_STANDARD_DEVICE_GET, 
	    				USB_REQ_GET_DESCRIPTOR,
	    				(USB_DT_STRING << 8) | 0, 0, languages, 256, 0);
					if (result > 0) {
	        			languageCount = (result - 2) / 2;
					}
					if (languageCount > 0) {
						if (TextUtils.isEmpty(info.manufacturer)) {
							info.manufacturer = getString(connection, desc[14], languageCount, languages);
						}
						if (TextUtils.isEmpty(info.product)) {
							info.product = getString(connection, desc[15], languageCount, languages);
						}
						if (TextUtils.isEmpty(info.serial)) {
							info.serial = getString(connection, desc[16], languageCount, languages);
						}
					}
				} finally {
					connection.close();
				}
			}
			if (TextUtils.isEmpty(info.manufacturer)) {
				info.manufacturer = String.format("%04x", device.getVendorId());
			}
			if (TextUtils.isEmpty(info.product)) {
				info.product = String.format("%04x", device.getProductId());
			}
		}
		return info;
	}

	
	public static final class UsbControlBlock implements Cloneable {
		private final WeakReference<USBMonitor> mWeakMonitor;
		private final WeakReference<UsbDevice> mWeakDevice;
		protected UsbDeviceConnection mConnection;
		protected final UsbDeviceInfo mInfo;
		private final int mBusNum;
		private final int mDevNum;
		private final SparseArray<SparseArray<UsbInterface>> mInterfaces = new SparseArray<SparseArray<UsbInterface>>();

		
		private UsbControlBlock(final USBMonitor monitor, final UsbDevice device) {
			if (DEBUG) XLogWrapper.i(TAG, "UsbControlBlock:constructor");
			mWeakMonitor = new WeakReference<USBMonitor>(monitor);
			mWeakDevice = new WeakReference<UsbDevice>(device);
			mConnection = monitor.mUsbManager.openDevice(device);
			if (mConnection == null) {
				XLogWrapper.w(TAG, "openDevice failed in UsbControlBlock11, wait and try again");
				try {
					Thread.sleep(500);
				} catch (InterruptedException e) {
					e.printStackTrace();
				}
				mConnection = monitor.mUsbManager.openDevice(device);
			}
			mInfo = updateDeviceInfo(monitor.mUsbManager, device, null);
			final String name = device.getDeviceName();
			final String[] v = !TextUtils.isEmpty(name) ? name.split("/") : null;
			int busnum = 0;
			int devnum = 0;
			if (v != null) {
				busnum = Integer.parseInt(v[v.length-2]);
				devnum = Integer.parseInt(v[v.length-1]);
			}
			mBusNum = busnum;
			mDevNum = devnum;
			if (DEBUG) {
				if (mConnection != null) {
					final int desc = mConnection.getFileDescriptor();
					final byte[] rawDesc = mConnection.getRawDescriptors();
					XLogWrapper.i(TAG, String.format(Locale.US, "name=%s,desc=%d,busnum=%d,devnum=%d,rawDesc=", name, desc, busnum, devnum));
				} else {
					XLogWrapper.e(TAG, "could not connect to device(mConnection=null) " + name);
				}
			}
		}

		
		private UsbControlBlock(final UsbControlBlock src) throws IllegalStateException {
			final USBMonitor monitor = src.getUSBMonitor();
			final UsbDevice device = src.getDevice();
			if (device == null) {
				throw new IllegalStateException("device may already be removed");
			}
			mConnection = monitor.mUsbManager.openDevice(device);
			if (mConnection == null) {
				XLogWrapper.w(TAG, "openDevice failed in UsbControlBlock, wait and try again");
				try {
					Thread.sleep(500);
				} catch (InterruptedException e) {
					e.printStackTrace();
				}
				mConnection = monitor.mUsbManager.openDevice(device);
				if (mConnection == null) {
					throw new IllegalStateException("openDevice failed. device may already be removed or have no permission, dev = " + device);
				}
			}
			mInfo = updateDeviceInfo(monitor.mUsbManager, device, null);
			mWeakMonitor = new WeakReference<USBMonitor>(monitor);
			mWeakDevice = new WeakReference<UsbDevice>(device);
			mBusNum = src.mBusNum;
			mDevNum = src.mDevNum;
			
		}

		
		@Override
		public UsbControlBlock clone() throws CloneNotSupportedException {
			final UsbControlBlock ctrlblock;
			try {
				ctrlblock = new UsbControlBlock(this);
			} catch (final IllegalStateException e) {
				throw new CloneNotSupportedException(e.getMessage());
			}
			return ctrlblock;
		}

		public USBMonitor getUSBMonitor() {
			return mWeakMonitor.get();
		}

		public final UsbDevice getDevice() {
			return mWeakDevice.get();
		}

		
		public String getDeviceName() {
			final UsbDevice device = mWeakDevice.get();
			return device != null ? device.getDeviceName() : "";
		}

		
		public int getDeviceId() {
			final UsbDevice device = mWeakDevice.get();
			return device != null ? device.getDeviceId() : 0;
		}

		
		public String getDeviceKeyName() {
			return USBMonitor.getDeviceKeyName(mWeakDevice.get());
		}

		
		public String getDeviceKeyName(final boolean useNewAPI) throws IllegalStateException {
			if (useNewAPI) checkConnection();
			return USBMonitor.getDeviceKeyName(mWeakDevice.get(), mInfo.serial, useNewAPI);
		}

		
		public int getDeviceKey() throws IllegalStateException {
			checkConnection();
			return USBMonitor.getDeviceKey(mWeakDevice.get());
		}

		
		public int getDeviceKey(final boolean useNewAPI) throws IllegalStateException {
			if (useNewAPI) checkConnection();
			return USBMonitor.getDeviceKey(mWeakDevice.get(), mInfo.serial, useNewAPI);
		}

		
		public String getDeviceKeyNameWithSerial() {
			return USBMonitor.getDeviceKeyName(mWeakDevice.get(), mInfo.serial, false);
		}

		
		public int getDeviceKeyWithSerial() {
			return getDeviceKeyNameWithSerial().hashCode();
		}

		
		public synchronized UsbDeviceConnection getConnection() {
			return mConnection;
		}

		
		public synchronized int getFileDescriptor() throws IllegalStateException {
			checkConnection();
			return mConnection.getFileDescriptor();
		}

		
		public synchronized byte[] getRawDescriptors() throws IllegalStateException {
			checkConnection();
			return mConnection.getRawDescriptors();
		}

		
		public int getVenderId() {
			final UsbDevice device = mWeakDevice.get();
			return device != null ? device.getVendorId() : 0;
		}

		
		public int getProductId() {
			final UsbDevice device = mWeakDevice.get();
			return device != null ? device.getProductId() : 0;
		}

		
		public String getUsbVersion() {
			return mInfo.usb_version;
		}

		
		public String getManufacture() {
			return mInfo.manufacturer;
		}

		
		public String getProductName() {
			return mInfo.product;
		}

		
		public String getVersion() {
			return mInfo.version;
		}

		
		public String getSerial() {
			return mInfo.serial;
		}

		public int getBusNum() {
			return mBusNum;
		}

		public int getDevNum() {
			return mDevNum;
		}

		
		public synchronized UsbInterface getInterface(final int interface_id) throws IllegalStateException {
			return getInterface(interface_id, 0);
		}

		
		@RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
		public synchronized UsbInterface getInterface(final int interface_id, final int altsetting) throws IllegalStateException {
			checkConnection();
			SparseArray<UsbInterface> intfs = mInterfaces.get(interface_id);
			if (intfs == null) {
				intfs = new SparseArray<UsbInterface>();
				mInterfaces.put(interface_id, intfs);
			}
			UsbInterface intf = intfs.get(altsetting);
			if (intf == null) {
				final UsbDevice device = mWeakDevice.get();
				final int n = device.getInterfaceCount();
				for (int i = 0; i < n; i++) {
					final UsbInterface temp = device.getInterface(i);
					if ((temp.getId() == interface_id) && (temp.getAlternateSetting() == altsetting)) {
						intf = temp;
						break;
					}
				}
				if (intf != null) {
					intfs.append(altsetting, intf);
				}
			}
			return intf;
		}

		
		public synchronized void claimInterface(final UsbInterface intf) {
			claimInterface(intf, true);
		}

		public synchronized void claimInterface(final UsbInterface intf, final boolean force) {
			checkConnection();
			mConnection.claimInterface(intf, force);
		}

		
		public synchronized void releaseInterface(final UsbInterface intf) throws IllegalStateException {
			checkConnection();
			final SparseArray<UsbInterface> intfs = mInterfaces.get(intf.getId());
			if (intfs != null) {
				final int index = intfs.indexOfValue(intf);
				intfs.removeAt(index);
				if (intfs.size() == 0) {
					mInterfaces.remove(intf.getId());
				}
			}
			mConnection.releaseInterface(intf);
		}

		
		public synchronized void close() {
			if (DEBUG) XLogWrapper.i(TAG, "UsbControlBlock#close:");

			if (mConnection != null) {
				final int n = mInterfaces.size();
				for (int i = 0; i < n; i++) {
					final SparseArray<UsbInterface> intfs = mInterfaces.valueAt(i);
					if (intfs != null) {
						final int m = intfs.size();
						for (int j = 0; j < m; j++) {
							final UsbInterface intf = intfs.valueAt(j);
							mConnection.releaseInterface(intf);
						}
						intfs.clear();
					}
				}
				mInterfaces.clear();
				mConnection.close();
				mConnection = null;
				final USBMonitor monitor = mWeakMonitor.get();
				if (monitor != null) {
					if (monitor.mOnDeviceConnectListener != null) {
						monitor.mOnDeviceConnectListener.onDisconnect(mWeakDevice.get(), UsbControlBlock.this);
					}
					monitor.mCtrlBlocks.remove(getDevice());
				}
			}
		}

		@Override
		public boolean equals(final Object o) {
			if (o == null) return false;
			if (o instanceof UsbControlBlock) {
				final UsbDevice device = ((UsbControlBlock) o).getDevice();
				return device == null ? mWeakDevice.get() == null
						: device.equals(mWeakDevice.get());
			} else if (o instanceof UsbDevice) {
				return o.equals(mWeakDevice.get());
			}
			return super.equals(o);
		}







		private synchronized void checkConnection() throws IllegalStateException {
			if (mConnection == null) {
				throw new IllegalStateException("already closed");
			}
		}
	}

}
