package com.superkvm.ausbc.utils

import android.Manifest
import android.content.Context
import android.content.pm.PackageManager
import android.hardware.usb.UsbConstants
import android.hardware.usb.UsbDevice
import android.media.Image
import androidx.core.content.ContextCompat
import com.superkvm.ausbc.R
import com.superkvm.usb.DeviceFilter


object CameraUtils {

    fun transferYUV420ToNV21(image: Image, width: Int, height: Int): ByteArray {
        val nv21 = ByteArray(width * height * 3 / 2)
        val planes = image.planes
        
        val yBuffer = planes[0].buffer
        val yLen = width * height
        yBuffer.get(nv21, 0, yLen)
        
        val vBuffer = planes[2].buffer
        val vPixelStride = planes[2].pixelStride
        for ((index, i) in (0 until vBuffer.remaining() step vPixelStride).withIndex()) {
            val vIndex = yLen + 2 * index
            if (vIndex >= nv21.size) {
                break
            }
            nv21[vIndex] = vBuffer.get(i)
        }
        
        val uBuffer = planes[1].buffer
        val uPixelStride = planes[1].pixelStride
        for ((index, i) in (0 until uBuffer.remaining() step uPixelStride).withIndex()) {
            val uIndex = yLen + (2 * index + 1)
            if (uIndex >= nv21.size) {
                break
            }
            nv21[yLen + (2 * index + 1)] = uBuffer.get(i)
        }
        return nv21
    }

    
    fun isUsbCamera(device: UsbDevice?): Boolean {
        return when (device?.deviceClass) {
            UsbConstants.USB_CLASS_VIDEO -> {
                true
            }
            UsbConstants.USB_CLASS_MISC -> {
                var isVideo = false
                for (i in 0 until device.interfaceCount) {
                    val cls = device.getInterface(i).interfaceClass
                    if (cls == UsbConstants.USB_CLASS_VIDEO) {
                        isVideo = true
                        break
                    }
                }
                isVideo
            }
            else -> {
                false
            }
        }
    }

    
    fun isCameraContainsMic(device: UsbDevice?): Boolean {
        device ?: return false
        var hasMic = false
        for (i in 0 until device.interfaceCount) {
            val cls = device.getInterface(i).interfaceClass
            if (cls == UsbConstants.USB_CLASS_AUDIO) {
                hasMic = true
                break
            }
        }
        return hasMic
    }

    
    fun isFilterDevice(context: Context?, usbDevice: UsbDevice?): Boolean {
        return DeviceFilter.getDeviceFilters(context, R.xml.default_device_filter)
            .find { devFilter ->
                devFilter.mProductId == usbDevice?.productId && devFilter.mVendorId == usbDevice.vendorId
            }.let { dev ->
                dev != null
            }
    }

    fun hasAudioPermission(ctx: Context): Boolean{
        val locPermission = ContextCompat.checkSelfPermission(ctx, Manifest.permission.RECORD_AUDIO)
        return locPermission == PackageManager.PERMISSION_GRANTED
    }

    fun hasStoragePermission(ctx: Context): Boolean{
        val locPermission = ContextCompat.checkSelfPermission(ctx, Manifest.permission.WRITE_EXTERNAL_STORAGE)
        return locPermission == PackageManager.PERMISSION_GRANTED
    }

    fun hasCameraPermission(ctx: Context): Boolean{
        val locPermission = ContextCompat.checkSelfPermission(ctx, Manifest.permission.CAMERA)
        return locPermission == PackageManager.PERMISSION_GRANTED
    }
}