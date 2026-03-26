package com.superkvm.ausbc.decode

import android.media.MediaCodec
import android.media.MediaCodecInfo
import android.media.MediaFormat
import android.os.Build
import android.util.Log
import android.view.Surface

class MjpegHardwareDecoder {

    interface Callback {
        fun onDecodeError(error: String)
        fun onDecodeStarted()
        fun onFrameDecoded()
    }

    private var mediaCodec: MediaCodec? = null
    private var surface: Surface? = null
    private var width = 0
    private var height = 0
    private var running = false
    private var callback: Callback? = null
    private val bufferInfo = MediaCodec.BufferInfo()

    fun init(surface: Surface, width: Int, height: Int, callback: Callback?): Boolean {
        this.surface = surface
        this.width = width
        this.height = height
        this.callback = callback
        return try {
            val format = MediaFormat.createVideoFormat("image/jpeg", width, height)
            format.setInteger(MediaFormat.KEY_COLOR_FORMAT, MediaCodecInfo.CodecCapabilities.COLOR_FormatSurface)
            mediaCodec = MediaCodec.createDecoderByType("image/jpeg")
            mediaCodec?.configure(format, surface, null, 0)
            mediaCodec?.start()
            running = true
            callback?.onDecodeStarted()
            true
        } catch (e: Exception) {
            Log.e(TAG, "init failed: ${e.message}")
            callback?.onDecodeError(e.message ?: "init failed")
            release()
            false
        }
    }

    fun decode(data: ByteArray, offset: Int, size: Int): Boolean {
        if (!running || mediaCodec == null) return false

        return try {
            val inputBufferIndex = mediaCodec!!.dequeueInputBuffer(10000)
            if (inputBufferIndex >= 0) {
                val inputBuffer = mediaCodec!!.getInputBuffer(inputBufferIndex)
                inputBuffer?.clear()
                inputBuffer?.put(data, offset, size)
                mediaCodec!!.queueInputBuffer(
                    inputBufferIndex,
                    0,
                    size,
                    0,
                    0
                )
            }

            var outputBufferIndex = mediaCodec!!.dequeueOutputBuffer(bufferInfo, 10000)
            while (outputBufferIndex >= 0) {
                mediaCodec!!.releaseOutputBuffer(outputBufferIndex, true)
                callback?.onFrameDecoded()
                outputBufferIndex = mediaCodec!!.dequeueOutputBuffer(bufferInfo, 0)
            }
            true
        } catch (e: Exception) {
            Log.e(TAG, "decode failed: ${e.message}")
            callback?.onDecodeError(e.message ?: "decode failed")
            false
        }
    }

    fun decodeDirect(buffer: java.nio.Buffer, offset: Int, size: Int): Boolean {
        if (!running || mediaCodec == null) return false

        return try {
            val inputBufferIndex = mediaCodec!!.dequeueInputBuffer(10000)
            if (inputBufferIndex >= 0) {
                val inputBuffer = mediaCodec!!.getInputBuffer(inputBufferIndex)
                inputBuffer?.clear()
                if (buffer is java.nio.ByteBuffer) {
                    buffer.position(offset)
                    inputBuffer?.put(buffer)
                }
                mediaCodec!!.queueInputBuffer(
                    inputBufferIndex,
                    0,
                    size,
                    0,
                    0
                )
            }

            var outputBufferIndex = mediaCodec!!.dequeueOutputBuffer(bufferInfo, 10000)
            while (outputBufferIndex >= 0) {
                mediaCodec!!.releaseOutputBuffer(outputBufferIndex, true)
                callback?.onFrameDecoded()
                outputBufferIndex = mediaCodec!!.dequeueOutputBuffer(bufferInfo, 0)
            }
            true
        } catch (e: Exception) {
            Log.e(TAG, "decodeDirect failed: ${e.message}")
            callback?.onDecodeError(e.message ?: "decodeDirect failed")
            false
        }
    }

    fun resize(newWidth: Int, newHeight: Int) {
        if (width == newWidth && height == newHeight) return
        release()
        surface?.let { init(it, newWidth, newHeight, callback) }
    }

    fun release() {
        running = false
        try {
            mediaCodec?.stop()
            mediaCodec?.release()
        } catch (e: Exception) {
            Log.e(TAG, "release error: ${e.message}")
        }
        mediaCodec = null
    }

    fun isRunning(): Boolean = running

    companion object {
        private const val TAG = "MjpegHardwareDecoder"

        fun isHardwareDecoderSupported(): Boolean {
            return try {
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                    
                    true
                } else {
                    
                    true
                }
            } catch (e: Exception) {
                false
            }
        }
    }
}
