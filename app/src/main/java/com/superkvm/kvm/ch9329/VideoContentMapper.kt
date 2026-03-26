
package com.superkvm.kvm.ch9329

object VideoContentMapper {

    data class Result(val x4095: Int, val y4095: Int, val insideContent: Boolean)

    
    fun touchTo4095(
        viewW: Int,
        viewH: Int,
        videoW: Int,
        videoH: Int,
        touchX: Float,
        touchY: Float
    ): Result {
        if (viewW <= 0 || viewH <= 0 || videoW <= 0 || videoH <= 0) {
            return Result(2047, 2047, false)
        }
        val containerRatio = viewW.toDouble() / viewH
        val videoRatio = videoW.toDouble() / videoH
        val displayW: Double
        val displayH: Double
        val displayX: Double
        val displayY: Double
        if (videoRatio > containerRatio) {
            displayW = viewW.toDouble()
            displayH = displayW / videoRatio
            displayX = 0.0
            displayY = (viewH - displayH) / 2.0
        } else {
            displayH = viewH.toDouble()
            displayW = displayH * videoRatio
            displayY = 0.0
            displayX = (viewW - displayW) / 2.0
        }
        val x = (touchX - displayX).coerceIn(0.0, displayW)
        val y = (touchY - displayY).coerceIn(0.0, displayH)
        val inside = touchX >= displayX && touchX <= displayX + displayW &&
            touchY >= displayY && touchY <= displayY + displayH
        val x4095 = kotlin.math.floor((x / displayW) * 4095.0).toInt().coerceIn(0, 4095)
        val y4095 = kotlin.math.floor((y / displayH) * 4095.0).toInt().coerceIn(0, 4095)
        return Result(x4095, y4095, inside)
    }
}
