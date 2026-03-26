package com.superkvm.kvm.hid

import com.superkvm.kvm.ch9329.Ch9329Packets
import com.superkvm.kvm.serial.SkUsbSerialTransport
import android.util.Log


class KvmHidBridge(private val transport: SkUsbSerialTransport) {

    private var mouseButtons: Int = 0
    private var modifiers: Int = 0
    private val pressedKeys = ArrayList<Int>(6)
    private var lastAbsX4095: Int = 2047
    private var lastAbsY4095: Int = 2047

    fun isReady(): Boolean = transport.isOpen()

    private inline fun hid(crossinline block: () -> Unit) {
        transport.post { block() }
    }

    private fun keysSix(): ByteArray {
        val keys = ByteArray(6)
        val n = pressedKeys.size.coerceAtMost(6)
        for (i in 0 until n) {
            keys[i] = (pressedKeys[i] and 0xff).toByte()
        }
        return keys
    }

    private fun sendKeyboardReport() {
        transport.write(Ch9329Packets.keyboardReport(modifiers, keysSix()))
    }

    
    fun tapKey(modifierMask: Int, hid: Int) = hid {
        if (hid == 0 || !transport.isOpen()) return@hid
        val keys = ByteArray(6)
        keys[0] = (hid and 0xff).toByte()
        transport.write(Ch9329Packets.keyboardReport(modifierMask, keys))
        Thread.sleep(25)
        transport.write(Ch9329Packets.keyboardReport(0, ByteArray(6)))
    }

    
    fun pasteText(text: String, charGapMs: Long = 5L) = hid {
        if (!transport.isOpen()) return@hid
        text.forEach { raw ->
            val c = if (raw == '\r') '\n' else raw
            val mapped = KeyboardHidMapper.charToHidWithShift(c)
            if (mapped.hid == 0) return@forEach
            val keys = ByteArray(6)
            keys[0] = (mapped.hid and 0xff).toByte()
            val modifier = if (mapped.needsShift) (1 shl 1) else 0
            transport.write(Ch9329Packets.keyboardReport(modifier, keys))
            Thread.sleep(charGapMs)
            transport.write(Ch9329Packets.keyboardReport(0, ByteArray(6)))
            Thread.sleep(charGapMs)
        }
    }

    fun moveAbs(x4095: Int, y4095: Int) = hid {
        if (!transport.isOpen()) return@hid
        lastAbsX4095 = x4095.coerceIn(0, 4095)
        lastAbsY4095 = y4095.coerceIn(0, 4095)
        transport.write(Ch9329Packets.mouseAbs(mouseButtons, lastAbsX4095, lastAbsY4095))
    }

    
    fun moveAbsNoButtons(x4095: Int, y4095: Int) = hid {
        if (!transport.isOpen()) return@hid
        lastAbsX4095 = x4095.coerceIn(0, 4095)
        lastAbsY4095 = y4095.coerceIn(0, 4095)
        transport.write(Ch9329Packets.mouseAbs(0, lastAbsX4095, lastAbsY4095, 0))
    }

    
    fun scroll(delta: Int) = hid {
        val open = transport.isOpen()
        Log.d("superkvm", "scroll() open=$open delta=$delta mouseButtons=$mouseButtons")
        if (!open) return@hid
        
        val xFixed = lastAbsX4095
        val yFixed = lastAbsY4095

        val pktWheel = Ch9329Packets.mouseAbs(mouseButtons, xFixed, yFixed, delta)
        val pktWheelZero = Ch9329Packets.mouseAbs(mouseButtons, xFixed, yFixed, 0)

        val hexWheel = pktWheel.joinToString(" ") { b -> (b.toInt() and 0xff).toString(16).padStart(2, '0') }
        val hexZero = pktWheelZero.joinToString(" ") { b -> (b.toInt() and 0xff).toString(16).padStart(2, '0') }
        Log.d("superkvm", "wheel(abs) pktWheel=[$hexWheel]")
        Log.d("superkvm", "wheel(abs) pktWheelZero=[$hexZero]")

        for (i in 0 until 3) {
            transport.write(pktWheel)
            Thread.sleep(5)
            transport.write(pktWheelZero)
            if (i < 2) Thread.sleep(50)
        }
    }

    private fun syncMouseAbsNoMove() {
        
        transport.write(Ch9329Packets.mouseAbs(mouseButtons, lastAbsX4095, lastAbsY4095))
    }

    fun setLeftDown(down: Boolean) = hid {
        if (!transport.isOpen()) return@hid
        mouseButtons = if (down) mouseButtons or 0x1 else mouseButtons and 0x1.inv()
        syncMouseAbsNoMove()
    }

    fun setRightDown(down: Boolean) = hid {
        if (!transport.isOpen()) return@hid
        mouseButtons = if (down) mouseButtons or 0x2 else mouseButtons and 0x2.inv()
        syncMouseAbsNoMove()
    }

    
    fun leftPressOnlyAt(x4095: Int, y4095: Int) = hid {
        if (!transport.isOpen()) return@hid
        lastAbsX4095 = x4095.coerceIn(0, 4095)
        lastAbsY4095 = y4095.coerceIn(0, 4095)
        val b = mouseButtons or 0x1
        transport.write(Ch9329Packets.mouseAbs(b, lastAbsX4095, lastAbsY4095, 0))
    }

    fun leftClickAt(x4095: Int, y4095: Int) = hid {
        if (!transport.isOpen()) return@hid
        lastAbsX4095 = x4095.coerceIn(0, 4095)
        lastAbsY4095 = y4095.coerceIn(0, 4095)
        mouseButtons = mouseButtons or 0x1
        transport.write(Ch9329Packets.mouseAbs(mouseButtons, x4095, y4095, 0))
        Thread.sleep(50)
        mouseButtons = mouseButtons and 0x1.inv()
        transport.write(Ch9329Packets.mouseAbs(mouseButtons, x4095, y4095, 0))
    }

    
    fun leftDoubleClickBurstAt(x4095: Int, y4095: Int) = hid {
        if (!transport.isOpen()) return@hid
        val x = x4095.coerceIn(0, 4095)
        val y = y4095.coerceIn(0, 4095)
        lastAbsX4095 = x
        lastAbsY4095 = y
        val base = mouseButtons
        val down = base or 0x1
        val up = base and 0x1.inv()
        val gapMs = 10L
        transport.write(Ch9329Packets.mouseAbs(down, x, y, 0))
        Thread.sleep(gapMs)
        transport.write(Ch9329Packets.mouseAbs(up, x, y, 0))
        Thread.sleep(gapMs)
        transport.write(Ch9329Packets.mouseAbs(down, x, y, 0))
        Thread.sleep(gapMs)
        transport.write(Ch9329Packets.mouseAbs(up, x, y, 0))
        mouseButtons = up
    }

    fun rightClickAt(x4095: Int, y4095: Int) = hid {
        if (!transport.isOpen()) return@hid
        lastAbsX4095 = x4095.coerceIn(0, 4095)
        lastAbsY4095 = y4095.coerceIn(0, 4095)
        mouseButtons = mouseButtons or 0x2
        transport.write(Ch9329Packets.mouseAbs(mouseButtons, x4095, y4095, 0))
        Thread.sleep(50)
        mouseButtons = mouseButtons and 0x2.inv()
        transport.write(Ch9329Packets.mouseAbs(mouseButtons, x4095, y4095, 0))
    }

    fun middleClickAt(x4095: Int, y4095: Int) = hid {
        if (!transport.isOpen()) return@hid
        lastAbsX4095 = x4095.coerceIn(0, 4095)
        lastAbsY4095 = y4095.coerceIn(0, 4095)
        mouseButtons = mouseButtons or 0x4
        transport.write(Ch9329Packets.mouseAbs(mouseButtons, x4095, y4095, 0))
        Thread.sleep(50)
        mouseButtons = mouseButtons and 0x4.inv()
        transport.write(Ch9329Packets.mouseAbs(mouseButtons, x4095, y4095, 0))
    }

    
    fun microJitter() = hid {
        if (!transport.isOpen()) return@hid
        transport.write(Ch9329Packets.mouseRel(mouseButtons, 1, 0, 0))
        Thread.sleep(5)
        transport.write(Ch9329Packets.mouseRel(mouseButtons, -1, 0, 0))
    }

    
    fun setMouseAutoMoveEnabled(enabled: Boolean) = hid {
        if (!transport.isOpen()) return@hid
        val data = byteArrayOf(if (enabled) 1 else 0)
        transport.write(buildDeviceProtocolPacket(channel = 0x01, type = 0x0B, sn = 0, data = data))
    }

    private fun buildDeviceProtocolPacket(channel: Int, type: Int, sn: Int, data: ByteArray): ByteArray {
        val dataLen = data.size
        val totalLen = 2 + 4 + dataLen
        val packetSize = 2 + 1 + 4 + 4 + 2 + 4 + dataLen + 4
        val out = ByteArray(packetSize)
        var o = 0

        out[o++] = 0x57.toByte()
        out[o++] = 0xAB.toByte()
        out[o++] = (channel and 0xff).toByte()
        putLe32(out, o, sn); o += 4
        putLe32(out, o, totalLen); o += 4
        putLe16(out, o, type); o += 2
        putLe32(out, o, dataLen); o += 4
        if (dataLen > 0) {
            System.arraycopy(data, 0, out, o, dataLen)
            o += dataLen
        }
        val crc = calcMcAddCrc(out, o)
        putLe32(out, o, crc); o += 4
        return if (o == out.size) out else out.copyOf(o)
    }

    private fun putLe16(buf: ByteArray, offset: Int, value: Int) {
        buf[offset] = (value and 0xff).toByte()
        buf[offset + 1] = ((value ushr 8) and 0xff).toByte()
    }

    private fun putLe32(buf: ByteArray, offset: Int, value: Int) {
        buf[offset] = (value and 0xff).toByte()
        buf[offset + 1] = ((value ushr 8) and 0xff).toByte()
        buf[offset + 2] = ((value ushr 16) and 0xff).toByte()
        buf[offset + 3] = ((value ushr 24) and 0xff).toByte()
    }

    
    private fun calcMcAddCrc(bytes: ByteArray, length: Int): Int {
        var sum = 0L
        for (i in 0 until length) {
            val b = bytes[i].toInt() and 0xff
            sum += b + ((b and 0x07) * (i and 0x1f))
        }
        return (sum and 0xffffffffL).toInt()
    }

    fun keyDown(hid: Int) = hid {
        if (hid == 0) return@hid
        if (!pressedKeys.contains(hid) && pressedKeys.size < 6) {
            pressedKeys.add(hid)
        }
        sendKeyboardReport()
    }

    fun keyUp(hid: Int) = hid {
        pressedKeys.removeAll { it == hid }
        sendKeyboardReport()
    }

    fun modifierDown(bit: Int) = hid {
        modifiers = modifiers or (1 shl bit)
        sendKeyboardReport()
    }

    fun modifierUp(bit: Int) = hid {
        modifiers = modifiers and (1 shl bit).inv()
        sendKeyboardReport()
    }

    fun resetKeyboard() = hid {
        modifiers = 0
        pressedKeys.clear()
        sendKeyboardReport()
    }
}
