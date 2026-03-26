
package com.superkvm.ausbc.base

import android.content.Context
import android.hardware.usb.UsbDevice
import com.superkvm.ausbc.MultiCameraClient
import com.superkvm.ausbc.callback.IDeviceConnectCallBack
import com.superkvm.usb.USBMonitor


abstract class MultiCameraFragment: BaseFragment() {
    private var mCameraClient: MultiCameraClient? = null
    private val mCameraMap = hashMapOf<Int, MultiCameraClient.ICamera>()

    override fun initData() {
        mCameraClient = MultiCameraClient(requireContext(), object : IDeviceConnectCallBack {
            override fun onAttachDev(device: UsbDevice?) {
                device ?: return
                context?.let {
                    if (mCameraMap.containsKey(device.deviceId)) {
                        return
                    }
                    generateCamera(it, device).apply {
                        mCameraMap[device.deviceId] = this
                        onCameraAttached(this)
                    }
                    
                    
                    
                    if (isAutoRequestPermission()) {
                        requestPermission(device)
                    }
                }
            }

            override fun onDetachDec(device: UsbDevice?) {
                mCameraMap.remove(device?.deviceId)?.apply {
                    setUsbControlBlock(null)
                    onCameraDetached(this)
                }
            }

            override fun onConnectDev(device: UsbDevice?, ctrlBlock: USBMonitor.UsbControlBlock?) {
                device ?: return
                ctrlBlock ?: return
                context ?: return
                mCameraMap[device.deviceId]?.apply {
                    setUsbControlBlock(ctrlBlock)
                    onCameraConnected(this)
                }
            }

            override fun onDisConnectDec(
                device: UsbDevice?,
                ctrlBlock: USBMonitor.UsbControlBlock?
            ) {
                mCameraMap[device?.deviceId]?.apply {
                    onCameraDisConnected(this)
                }
            }

            override fun onCancelDev(device: UsbDevice?) {
                mCameraMap[device?.deviceId]?.apply {
                    onCameraDisConnected(this)
                }
            }
        })
        mCameraClient?.register()
    }

    override fun clear() {
        mCameraMap.values.forEach {
            it.closeCamera()
        }
        mCameraMap.clear()
        mCameraClient?.unRegister()
        mCameraClient?.destroy()
        mCameraClient = null
    }


    
    abstract fun generateCamera(ctx: Context, device: UsbDevice): MultiCameraClient.ICamera

    
    protected abstract fun onCameraConnected(camera: MultiCameraClient.ICamera)

    
    protected abstract fun onCameraDisConnected(camera: MultiCameraClient.ICamera)

    
    protected abstract fun onCameraAttached(camera: MultiCameraClient.ICamera)

    
    protected abstract fun onCameraDetached(camera: MultiCameraClient.ICamera)

    
    protected fun getCameraMap() = mCameraMap

    
    protected fun getDeviceList() = mCameraClient?.getDeviceList()

    
    protected fun getCameraClient() = mCameraClient

    
    protected fun isAutoRequestPermission() = true

    
    protected fun requestPermission(device: UsbDevice?) {
        mCameraClient?.requestPermission(device)
    }

    
    protected fun hasPermission(device: UsbDevice?) = mCameraClient?.hasPermission(device) == true

    protected fun openDebug(debug: Boolean) {
        mCameraClient?.openDebug(debug)
    }

    protected fun isFragmentDetached() = !isAdded || isDetached
}