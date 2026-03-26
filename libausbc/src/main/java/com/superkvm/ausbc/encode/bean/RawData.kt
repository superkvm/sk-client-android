
package com.superkvm.ausbc.encode.bean

import androidx.annotation.Keep


@Keep
data class RawData(val data: ByteArray, val size: Int) {
    override fun equals(other: Any?): Boolean {
        if (this === other) return true
        if (javaClass != other?.javaClass) return false

        other as RawData

        if (!data.contentEquals(other.data)) return false
        if (size != other.size) return false

        return true
    }

    override fun hashCode(): Int {
        var result = data.contentHashCode()
        result = 31 * result + size
        return result
    }
}
