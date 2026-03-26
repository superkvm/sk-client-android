
package com.superkvm.ausbc.encode.audio

import android.media.AudioFormat
import com.superkvm.ausbc.encode.bean.RawData
import com.superkvm.ausbc.utils.Logger
import com.superkvm.ausbc.utils.Utils
import com.superkvm.uac.UACAudioCallBack
import com.superkvm.uac.UACAudioHandler
import com.superkvm.usb.USBMonitor
import java.util.concurrent.ConcurrentLinkedQueue


class AudioStrategyUAC(private val ctrlBlock: USBMonitor.UsbControlBlock): IAudioStrategy {
    private var mUacHandler: UACAudioHandler? = null
    private val mPcmDataQueue: ConcurrentLinkedQueue<ByteArray> = ConcurrentLinkedQueue()

    private val mCallback = UACAudioCallBack { data ->
        if (mPcmDataQueue.size >= MAX_QUEUE_SIZE) {
            mPcmDataQueue.poll()
        }
        mPcmDataQueue.offer(data)
    }

    override fun initAudioRecord() {
        mUacHandler = UACAudioHandler.createHandler(ctrlBlock)
        mUacHandler?.initAudioRecord()
        if (Utils.debugCamera) {
            Logger.i(TAG, "initAudioRecord")
        }
    }

    override fun startRecording() {
        mUacHandler?.addDataCallBack(mCallback)
        mUacHandler?.startRecording()
        if (Utils.debugCamera) {
            Logger.i(TAG, "startRecording:")
        }
    }

    override fun stopRecording() {
        mUacHandler?.stopRecording()
        mUacHandler?.removeDataCallBack(mCallback)
        if (Utils.debugCamera) {
            Logger.i(TAG, "stopRecording:")
        }
    }

    override fun releaseAudioRecord() {
        mUacHandler?.releaseAudioRecord()
        if (Utils.debugCamera) {
            Logger.i(TAG, "releaseAudioRecord:")
        }
    }

    override fun read(): RawData? {
        return mPcmDataQueue.poll()?.let {
            RawData(it, it.size)
        }
    }

    override fun isRecording(): Boolean = mUacHandler?.isRecording == true

    override fun getSampleRate(): Int {
        return mUacHandler?.sampleRate ?: SAMPLE_RATE
    }

    override fun getAudioFormat(): Int {
        return if (mUacHandler?.bitResolution == BIT_RESOLUTION) {
            AudioFormat.ENCODING_PCM_8BIT
        } else {
            AudioFormat.ENCODING_PCM_16BIT
        }
    }

    override fun getChannelCount(): Int {
        return mUacHandler?.channelCount ?: CHANNEL_COUNT
    }

    override fun getChannelConfig(): Int = if (getChannelCount() == CHANNEL_COUNT) {
        AudioFormat.CHANNEL_IN_MONO
    } else {
        AudioFormat.CHANNEL_IN_STEREO
    }

    companion object {
        private const val TAG = "AudioUac"
        private const val MAX_QUEUE_SIZE = 10
        private const val SAMPLE_RATE = 8000
        private const val BIT_RESOLUTION = 8
        private const val CHANNEL_COUNT = 1
    }
}