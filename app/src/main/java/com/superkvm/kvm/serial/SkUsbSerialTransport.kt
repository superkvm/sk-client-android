package com.superkvm.kvm.serial

import android.annotation.SuppressLint
import android.app.PendingIntent
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.hardware.usb.UsbDevice
import android.hardware.usb.UsbManager
import android.os.Build
import android.os.Handler
import android.os.HandlerThread
import android.os.Looper
import android.util.Log
import com.hoho.android.usbserial.driver.UsbSerialPort
import com.hoho.android.usbserial.driver.UsbSerialProber
import java.io.IOException


class SkUsbSerialTransport(private val context: Context) {

    private val usbManager = context.getSystemService(Context.USB_SERVICE) as UsbManager
    private var connection: android.hardware.usb.UsbDeviceConnection? = null
    private var port: UsbSerialPort? = null
    private var writerThread: HandlerThread? = null
    private var writerHandler: Handler? = null

    var onConnectionState: ((connected: Boolean) -> Unit)? = null

    private val permissionIntent: PendingIntent by lazy {
        @SuppressLint("InlinedApi")
        var flags = PendingIntent.FLAG_UPDATE_CURRENT
        
        if (Build.VERSION.SDK_INT >= 31) {
            flags = flags or (1 shl 25)
        }
        PendingIntent.getBroadcast(
            context,
            0,
            Intent(ACTION_USB_PERMISSION).setPackage(context.packageName),
            flags
        )
    }

    private var permissionCallback: ((Boolean) -> Unit)? = null

    private val permissionReceiver = object : BroadcastReceiver() {
        override fun onReceive(ctx: Context, intent: Intent) {
            if (intent.action != ACTION_USB_PERMISSION) return
            @Suppress("DEPRECATION")
            val device: UsbDevice? = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE)
            val granted = intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)
            if (granted && device != null) {
                permissionCallback?.invoke(true)
            } else {
                permissionCallback?.invoke(false)
            }
            permissionCallback = null
            try {
                ctx.unregisterReceiver(this)
            } catch (_: Exception) {
            }
        }
    }

    fun isOpen(): Boolean = port != null

    private fun ensureWriter() {
        if (writerHandler != null) return
        val t = HandlerThread("sk-usb-serial").apply { start() }
        writerThread = t
        writerHandler = Handler(t.looper)
    }

    fun post(task: Runnable) {
        ensureWriter()
        writerHandler?.post(task) ?: task.run()
    }

    fun write(bytes: ByteArray) {
        ensureWriter()
        val h = writerHandler ?: return
        val job = Runnable {
            try {
                
                if (bytes.size >= 2 &&
                    (bytes[0].toInt() and 0xff) == 0x57 &&
                    (bytes[1].toInt() and 0xff) == 0xab
                ) {
                    val hex = bytes.joinToString(" ") { b -> (b.toInt() and 0xff).toString(16).padStart(2, '0') }
                    Log.d(TAG, "usb write ch9329 raw=[$hex] len=${bytes.size}")
                }
                port?.write(bytes, WRITE_TIMEOUT_MS)
            } catch (e: IOException) {
                Log.e(TAG, "usb write failed", e)
            }
        }
        if (Looper.myLooper() == h.looper) {
            job.run()
        } else {
            h.post(job)
        }
    }

    fun requestOpen(device: UsbDevice, onResult: (Boolean) -> Unit) {
        ensureWriter()
        if (usbManager.hasPermission(device)) {
            writerHandler?.post { openInternal(device, onResult) }
            return
        }
        permissionCallback = { granted ->
            if (granted) {
                writerHandler?.post { openInternal(device, onResult) }
            } else {
                onResult(false)
            }
        }
        val filter = IntentFilter(ACTION_USB_PERMISSION)
        context.registerReceiver(permissionReceiver, filter)
        usbManager.requestPermission(device, permissionIntent)
    }

    private fun openInternal(device: UsbDevice, onResult: (Boolean) -> Unit) {
        closeInternal()
        val driver = UsbSerialProber.getDefaultProber().findAllDrivers(usbManager)
            .firstOrNull { it.device.deviceId == device.deviceId }
        if (driver == null) {
            Log.w(TAG, "No UsbSerialDriver for ${device.deviceName}")
            onResult(false)
            return
        }
        val conn = usbManager.openDevice(device)
        if (conn == null) {
            Log.w(TAG, "openDevice failed")
            onResult(false)
            return
        }
        val p = driver.ports.firstOrNull()
        if (p == null) {
            conn.close()
            onResult(false)
            return
        }
        try {
            p.open(conn)
            p.setParameters(BAUD_RATE, 8, UsbSerialPort.STOPBITS_1, UsbSerialPort.PARITY_NONE)
            connection = conn
            port = p
            Log.i(TAG, "Serial open ${device.deviceName} @ $BAUD_RATE")
            onResult(true)
            onConnectionState?.invoke(true)
        } catch (e: IOException) {
            Log.e(TAG, "port open failed", e)
            try {
                p.close()
            } catch (_: Exception) {
            }
            conn.close()
            onResult(false)
        }
    }

    fun close() {
        writerHandler?.post { closeInternal() }
    }

    private fun closeInternal() {
        try {
            port?.close()
        } catch (_: Exception) {
        }
        port = null
        try {
            connection?.close()
        } catch (_: Exception) {
        }
        connection = null
        onConnectionState?.invoke(false)
    }

    fun shutdown() {
        closeInternal()
        writerThread?.quitSafely()
        writerThread = null
        writerHandler = null
    }

    companion object {
        private const val TAG = "superkvm"
        const val ACTION_USB_PERMISSION = "com.superkvm.kvm.USB_PERMISSION_SERIAL"
        private const val BAUD_RATE = 2_000_000
        private const val WRITE_TIMEOUT_MS = 500
    }
}
