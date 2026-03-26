package com.superkvm.kvm.serial

import android.content.Context
import android.hardware.usb.UsbDevice

object UartPreferences {
    private const val PREF_NAME = "sk_kvm_uart"
    private const val KEY_DEVICE = "preferred_uart_device_key"

    fun deviceKey(device: UsbDevice): String {
        return "${device.deviceName}|${device.vendorId}|${device.productId}"
    }

    fun getPreferredKey(context: Context): String? {
        val sp = context.getSharedPreferences(PREF_NAME, Context.MODE_PRIVATE)
        return sp.getString(KEY_DEVICE, null)?.takeIf { it.isNotBlank() }
    }

    fun setPreferredKey(context: Context, device: UsbDevice) {
        context.getSharedPreferences(PREF_NAME, Context.MODE_PRIVATE)
            .edit()
            .putString(KEY_DEVICE, deviceKey(device))
            .apply()
    }

    fun clear(context: Context) {
        context.getSharedPreferences(PREF_NAME, Context.MODE_PRIVATE)
            .edit()
            .remove(KEY_DEVICE)
            .apply()
    }
}
