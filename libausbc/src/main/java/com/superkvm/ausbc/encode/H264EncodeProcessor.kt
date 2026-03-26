
package com.superkvm.ausbc.encode

import android.media.MediaCodec
import android.media.MediaCodecInfo
import android.media.MediaFormat
import android.view.Surface
import com.superkvm.ausbc.callback.IEncodeDataCallBack
import com.superkvm.ausbc.utils.Logger
import com.superkvm.natives.YUVUtils
import java.lang.Exception
import java.nio.ByteBuffer


class H264EncodeProcessor(
    val width: Int,
    val height: Int,
    private val gLESRender: Boolean = false,
    private val isPortrait: Boolean = true
) : AbstractProcessor(true) {
    private var mReadyListener: OnEncodeReadyListener? = null

    override fun getThreadName(): String = TAG

    override fun handleStartEncode() {
        try {
            val mediaFormat = MediaFormat.createVideoFormat(MIME, width, height)
            mediaFormat.setInteger(MediaFormat.KEY_FRAME_RATE, FRAME_RATE)
            mediaFormat.setInteger(
                MediaFormat.KEY_BIT_RATE,
                mBitRate ?: getEncodeBitrate(width, height)
            )
            mediaFormat.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, KEY_FRAME_INTERVAL)
            mediaFormat.setInteger(MediaFormat.KEY_COLOR_FORMAT, getSupportColorFormat())
            mMediaCodec = MediaCodec.createEncoderByType(MIME)
            mMediaCodec?.configure(mediaFormat, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE)
            if (gLESRender) {
                mReadyListener?.onReady(mMediaCodec?.createInputSurface())
            }
            mMediaCodec?.start()
            mEncodeState.set(true)
            doEncodeData()
            Logger.i(TAG, "init h264 media codec success, bit = ")

        } catch (e: Exception) {
            Logger.e(TAG, "start h264 media codec failed, err = ${e.localizedMessage}", e)
        }
    }

    override fun handleStopEncode() {
        try {
            mEncodeState.set(false)
            mMediaCodec?.stop()
            mMediaCodec?.release()
            Logger.i(TAG, "release h264 media codec success.")
        } catch (e: Exception) {
            Logger.e(TAG, "Stop mediaCodec failed, err = ${e.localizedMessage}", e)
        } finally {
            mRawDataQueue.clear()
            mMediaCodec = null
        }
    }

    override fun getPTSUs(bufferSize: Int): Long = System.nanoTime() / 1000L

    override fun processOutputData(
        encodeData: ByteBuffer,
        bufferInfo: MediaCodec.BufferInfo
    ): Pair<IEncodeDataCallBack.DataType, ByteBuffer> {
        val type = when (bufferInfo.flags) {
            MediaCodec.BUFFER_FLAG_CODEC_CONFIG -> {
                IEncodeDataCallBack.DataType.H264_SPS
            }
            MediaCodec.BUFFER_FLAG_KEY_FRAME -> {
                IEncodeDataCallBack.DataType.H264_KEY
            }
            else -> {
                IEncodeDataCallBack.DataType.H264
            }
        }
        return Pair(type, encodeData)
    }

    override fun processInputData(data: ByteArray): ByteArray? {
        return if (gLESRender) {
            null
        } else {
            data.apply {
                if (size != width * height * 3 /2) {
                    return null
                }
                if (isPortrait) {
                    YUVUtils.nativeRotateNV21(data,width, height, 90)
                }
                YUVUtils.nv21ToYuv420sp(data, width, height)
            }
        }
    }

    
    fun setOnEncodeReadyListener(listener: OnEncodeReadyListener) {
        this.mReadyListener = listener
    }

    private fun getSupportColorFormat(): Int {
        if (gLESRender) {
            return MediaCodecInfo.CodecCapabilities.COLOR_FormatSurface
        }
        return MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420SemiPlanar
    }

    private fun getEncodeBitrate(width: Int, height: Int): Int {
        var bitRate = width * height * 20 * 3 * 0.07F
        if (width >= 1920 || height >= 1920) {
            bitRate *= 0.75F
        } else if (width >= 1280 || height >= 1280) {
            bitRate *= 1.2F
        } else if (width >= 640 || height >= 640) {
            bitRate *= 1.4F
        }
        return bitRate.toInt()
    }

    
    fun getEncodeWidth() = width


    
    fun getEncodeHeight() = height

    
    interface OnEncodeReadyListener {
        
        fun onReady(surface: Surface?)
    }

    companion object {
        private const val TAG = "H264EncodeProcessor"
        private const val MIME = "video/avc"
        private const val FRAME_RATE = 30
        private const val KEY_FRAME_INTERVAL = 1
    }
}