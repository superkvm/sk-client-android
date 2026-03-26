
package com.superkvm.kvm.ch9329

internal object Ch9329Packets {
    const val HEAD1: Int = 0x57
    const val HEAD2: Int = 0xab
    const val ADDR: Int = 0x00
    const val CMD_KB_GENERAL: Int = 0x02
    const val CMD_MS_ABS: Int = 0x04
    const val CMD_MS_REL: Int = 0x05

    fun intToByte(n: Int): Int {
        var v = n
        if (v > 127) v = 127
        if (v < -128) v = -128
        return (v + 256) and 0xff
    }

    private fun le16(n: Int): Pair<Int, Int> {
        val v = n and 0xffff
        return (v and 0xff) to ((v shr 8) and 0xff)
    }

    fun encode(addr: Int, cmd: Int, data: ByteArray): ByteArray {
        val a = addr and 0xff
        val c = cmd and 0xff
        val len = data.size and 0xff
        var sum = (HEAD1 + HEAD2 + a + c + len) and 0xff
        for (b in data) {
            sum = (sum + (b.toInt() and 0xff)) and 0xff
        }
        val out = ByteArray(5 + data.size + 1)
        out[0] = HEAD1.toByte()
        out[1] = HEAD2.toByte()
        out[2] = a.toByte()
        out[3] = c.toByte()
        out[4] = len.toByte()
        System.arraycopy(data, 0, out, 5, data.size)
        out[out.size - 1] = sum.toByte()
        return out
    }

    
    fun mouseAbs(buttons: Int, x: Int, y: Int, wheel: Int = 0): ByteArray {
        val xAbs = x.coerceIn(0, 4095)
        val yAbs = y.coerceIn(0, 4095)
        val xl = le16(xAbs)
        val yl = le16(yAbs)
        val data = byteArrayOf(
            0x02,
            (buttons and 0xff).toByte(),
            xl.first.toByte(),
            xl.second.toByte(),
            yl.first.toByte(),
            yl.second.toByte(),
            intToByte(wheel).toByte()
        )
        return encode(ADDR, CMD_MS_ABS, data)
    }

    
    fun mouseRel(buttons: Int, dx: Int, dy: Int, wheel: Int): ByteArray {
        val data = byteArrayOf(
            0x01,
            (buttons and 0xff).toByte(),
            intToByte(dx).toByte(),
            intToByte(dy).toByte(),
            intToByte(wheel).toByte()
        )
        return encode(ADDR, CMD_MS_REL, data)
    }

    
    fun keyboardReport(modifiers: Int, keyCodes: ByteArray): ByteArray {
        val k = ByteArray(6)
        val n = keyCodes.size.coerceAtMost(6)
        System.arraycopy(keyCodes, 0, k, 0, n)
        val data = ByteArray(8)
        data[0] = (modifiers and 0xff).toByte()
        data[1] = 0x00
        System.arraycopy(k, 0, data, 2, 6)
        return encode(ADDR, CMD_KB_GENERAL, data)
    }
}
