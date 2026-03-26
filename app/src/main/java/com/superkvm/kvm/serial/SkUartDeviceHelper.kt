package com.superkvm.kvm.serial

import android.content.Context
import android.hardware.usb.UsbConstants
import android.hardware.usb.UsbDevice
import android.hardware.usb.UsbManager
import com.hoho.android.usbserial.driver.UsbSerialProber

object SkUartDeviceHelper {

    fun isUartInterface(device: UsbDevice): Boolean {
        if (device.deviceClass == UsbConstants.USB_CLASS_COMM ||
            device.deviceClass == UsbConstants.USB_CLASS_CDC_DATA
        ) {
            return true
        }
        for (i in 0 until device.interfaceCount) {
            val cls = device.getInterface(i).interfaceClass
            if (cls == UsbConstants.USB_CLASS_COMM || cls == UsbConstants.USB_CLASS_CDC_DATA) {
                return true
            }
        }
        return false
    }

    fun buildDisplayName(device: UsbDevice): String {
        val product = device.productName?.takeIf { it.isNotBlank() }
        val manufacturer = device.manufacturerName?.takeIf { it.isNotBlank() }
        val deviceName = device.deviceName.takeIf { it.isNotBlank() } ?: "unknown"
        return when {
            manufacturer != null && product != null -> "$manufacturer $product"
            product != null -> product
            manufacturer != null -> manufacturer
            else -> deviceName
        }
    }

    fun nameContainsSk(device: UsbDevice): Boolean {
        return buildDisplayName(device).contains("SK", ignoreCase = true)
    }

    
    fun hasSerialDriver(usbManager: UsbManager, device: UsbDevice): Boolean {
        return UsbSerialProber.getDefaultProber().findAllDrivers(usbManager)
            .any { it.device.deviceId == device.deviceId }
    }

    fun listSkUartCandidates(context: Context): List<UsbDevice> {
        val usbManager = context.getSystemService(Context.USB_SERVICE) as? UsbManager ?: return emptyList()
        return usbManager.deviceList.values
            .filter { isUartInterface(it) && nameContainsSk(it) && hasSerialDriver(usbManager, it) }
            .sortedBy { it.deviceName ?: "" }
    }

    fun pickDevice(context: Context, preferredKey: String?): UsbDevice? {
        val list = listSkUartCandidates(context)
        if (list.isEmpty()) return null
        if (!preferredKey.isNullOrBlank()) {
            list.find { UartPreferences.deviceKey(it) == preferredKey }?.let { return it }
        }
        return list.first()
    }
}
