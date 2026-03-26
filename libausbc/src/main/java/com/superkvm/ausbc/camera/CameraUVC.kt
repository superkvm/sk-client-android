
package com.superkvm.ausbc.camera

import android.content.ContentValues
import android.content.Context
import android.graphics.SurfaceTexture
import android.hardware.usb.UsbDevice
import android.provider.MediaStore
import android.view.Surface
import android.view.SurfaceView
import android.view.TextureView
import com.superkvm.ausbc.MultiCameraClient
import com.superkvm.ausbc.MultiCameraClient.Companion.CAPTURE_TIMES_OUT_SEC
import com.superkvm.ausbc.MultiCameraClient.Companion.MAX_NV21_DATA
import com.superkvm.ausbc.callback.ICameraStateCallBack
import com.superkvm.ausbc.callback.ICaptureCallBack
import com.superkvm.ausbc.callback.IPreviewDataCallBack
import com.superkvm.ausbc.camera.bean.CameraRequest
import com.superkvm.ausbc.camera.bean.PreviewSize
import com.superkvm.ausbc.utils.CameraUtils
import com.superkvm.ausbc.utils.Logger
import com.superkvm.ausbc.utils.MediaUtils
import com.superkvm.ausbc.utils.Utils
import com.superkvm.ausbc.widget.IAspectRatio
import com.superkvm.uvc.IFrameCallback
import com.superkvm.uvc.UVCCamera
import java.io.File
import java.util.concurrent.TimeUnit


class CameraUVC(ctx: Context, device: UsbDevice) : MultiCameraClient.ICamera(ctx, device) {
    private var mUvcCamera: UVCCamera? = null
    private val mCameraPreviewSize by lazy {
        arrayListOf<PreviewSize>()
    }

    private val frameCallBack = IFrameCallback { frame ->
        frame?.apply {
            frame.position(0)
            val data = ByteArray(capacity())
            get(data)
            mCameraRequest?.apply {
                if (data.size != previewWidth * previewHeight * 3 / 2) {
                    return@IFrameCallback
                }
                
                mPreviewDataCbList.forEach { cb ->
                    cb?.onPreviewData(data, previewWidth, previewHeight, IPreviewDataCallBack.DataFormat.NV21)
                }
                
                if (mNV21DataQueue.size >= MAX_NV21_DATA) {
                    mNV21DataQueue.removeLast()
                }
                mNV21DataQueue.offerFirst(data)
                
                
                putVideoData(data)
            }
        }
    }

    override fun getAllPreviewSizes(aspectRatio: Double?): MutableList<PreviewSize> {
        val previewSizeList = arrayListOf<PreviewSize>()
        val isMjpegFormat = mCameraRequest?.previewFormat == CameraRequest.PreviewFormat.FORMAT_MJPEG
        if (isMjpegFormat && (mUvcCamera?.supportedSizeList?.isNotEmpty() == true)) {
            mUvcCamera?.supportedSizeList
        }  else {
            mUvcCamera?.getSupportedSizeList(UVCCamera.FRAME_FORMAT_YUYV)
        }?.let { sizeList ->
            if (sizeList.size > mCameraPreviewSize.size) {
                mCameraPreviewSize.clear()
                sizeList.forEach { size->
                    val width = size.width
                    val height = size.height
                    mCameraPreviewSize.add(PreviewSize(width, height))
                }
            }
            if (Utils.debugCamera) {
                Logger.i(TAG, "aspect ratio = $aspectRatio, supportedSizeList = $sizeList")
            }
            mCameraPreviewSize
        }?.onEach { size ->
            val width = size.width
            val height = size.height
            val ratio = width.toDouble() / height
            if (aspectRatio == null || aspectRatio == ratio) {
                previewSizeList.add(PreviewSize(width, height))
            }
        }
        return previewSizeList
    }

    
    private fun chooseBestUvcPreviewSize(uvc: UVCCamera, preferYuyv: Boolean): Triple<Int, Int, Int>? {
        val order = if (preferYuyv) {
            intArrayOf(UVCCamera.FRAME_FORMAT_YUYV, UVCCamera.FRAME_FORMAT_MJPEG)
        } else {
            intArrayOf(UVCCamera.FRAME_FORMAT_MJPEG, UVCCamera.FRAME_FORMAT_YUYV)
        }
        val loose = ArrayList<Pair<com.superkvm.utils.Size, Int>>()
        val tier30 = ArrayList<Pair<com.superkvm.utils.Size, Int>>()
        val tier24 = ArrayList<Pair<com.superkvm.utils.Size, Int>>()
        val tier18 = ArrayList<Pair<com.superkvm.utils.Size, Int>>()
        for (fmt in order) {
            val list = uvc.getSupportedSizeList(fmt) ?: continue
            for (sz in list) {
                val pair = sz to fmt
                loose.add(pair)
                val fpsMax = sz.fps?.maxOrNull() ?: continue
                if (fpsMax > PREFER_FPS_TIER_HIGH_EXCLUSIVE) {
                    tier30.add(pair)
                }
                if (fpsMax > PREFER_FPS_TIER_MID_EXCLUSIVE) {
                    tier24.add(pair)
                }
                if (fpsMax > MIN_PREVIEW_FPS_EXCLUSIVE) {
                    tier18.add(pair)
                }
            }
        }
        val noFpsMeta = tier30.isEmpty() && tier24.isEmpty() && tier18.isEmpty()
        val (pool0, tierTag0) = when {
            tier30.isNotEmpty() -> tier30 to "fps>29"
            tier24.isNotEmpty() -> tier24 to "fps>23"
            tier18.isNotEmpty() -> tier18 to "fps>18"
            else -> loose to "no-fps-filter"
        }
        if (pool0.isEmpty()) return null
        var pool: List<Pair<com.superkvm.utils.Size, Int>> = pool0
        var tierTag = tierTag0
        if (noFpsMeta) {
            val maxArea = if (preferYuyv) {
                HEURISTIC_MAX_AREA_YUYV_NO_FPS
            } else {
                HEURISTIC_MAX_AREA_MJPEG_NO_FPS
            }
            val capped = pool.filter { it.first.width * it.first.height <= maxArea }
            pool = if (capped.isNotEmpty()) {
                tierTag = "$tierTag, usb2-cap<=$maxArea"
                capped
            } else {
                val smallest = pool.minByOrNull { it.first.width * it.first.height }!!
                tierTag = "$tierTag, smallest=${smallest.first.width}x${smallest.first.height}"
                listOf(smallest)
            }
        }
        val maxArea = pool.maxOf { it.first.width * it.first.height }
        val tier = pool.filter { it.first.width * it.first.height == maxArea }
        val best = tier.minByOrNull { p ->
            val idx = order.indexOf(p.second)
            if (idx >= 0) idx else Int.MAX_VALUE
        }!!
        Logger.i(
            TAG,
            "chooseBestUvcPreviewSize: tier=$tierTag, pick ${best.first.width}x${best.first.height}, format=${best.second}"
        )
        return Triple(best.first.width, best.first.height, best.second)
    }

    private fun applyPreviewSizeToRequest(width: Int, height: Int): PreviewSize {
        mCameraRequest!!.previewWidth = width
        mCameraRequest!!.previewHeight = height
        return PreviewSize(width, height)
    }

    override fun <T> openCameraInternal(cameraView: T) {
        if (Utils.isTargetSdkOverP(ctx) && !CameraUtils.hasCameraPermission(ctx)) {
            closeCamera()
            postStateEvent(ICameraStateCallBack.State.ERROR, "Has no CAMERA permission.")
            Logger.e(TAG,"open camera failed, need Manifest.permission.CAMERA permission when targetSdk>=28")
            return
        }
        if (mCtrlBlock == null) {
            closeCamera()
            postStateEvent(ICameraStateCallBack.State.ERROR, "Usb control block can not be null ")
            return
        }
        
        val request = mCameraRequest!!
        try {
            mUvcCamera = UVCCamera().apply {
                open(mCtrlBlock)
            }
        } catch (e: Exception) {
            closeCamera()
            postStateEvent(ICameraStateCallBack.State.ERROR, "open camera failed ${e.localizedMessage}")
            Logger.e(TAG, "open camera failed.", e)
            return
        }

        val uvc = mUvcCamera ?: return
        val preferYuyv = request.previewFormat == CameraRequest.PreviewFormat.FORMAT_YUYV
        val chosen = chooseBestUvcPreviewSize(uvc, preferYuyv)
        var previewSize: PreviewSize
        var previewFormat: Int
        if (chosen != null) {
            previewFormat = chosen.third
            previewSize = applyPreviewSizeToRequest(chosen.first, chosen.second)
        } else {
            Logger.w(TAG, "chooseBestUvcPreviewSize failed, fallback getSuitableSize")
            previewFormat = if (preferYuyv) UVCCamera.FRAME_FORMAT_YUYV else UVCCamera.FRAME_FORMAT_MJPEG
            previewSize = getSuitableSize(request.previewWidth, request.previewHeight).apply {
                mCameraRequest!!.previewWidth = width
                mCameraRequest!!.previewHeight = height
            }
        }

        fun setPreviewOrThrow(fmt: Int) {
            uvc.setPreviewSize(
                previewSize.width,
                previewSize.height,
                PREVIEW_FPS_NEGOTIATION_MIN,
                PREVIEW_FPS_NEGOTIATION_MAX,
                fmt,
                UVCCamera.DEFAULT_BANDWIDTH
            )
        }

        try {
            Logger.i(TAG, "setPreviewSize: $previewSize, format=$previewFormat")
            if (! isPreviewSizeSupported(previewSize)) {
                closeCamera()
                postStateEvent(ICameraStateCallBack.State.ERROR, "unsupported preview size")
                Logger.e(TAG, "open camera failed, preview size($previewSize) unsupported-> ${mUvcCamera?.supportedSizeList}")
                return
            }
            initEncodeProcessor(previewSize.width, previewSize.height)
            setPreviewOrThrow(previewFormat)
        } catch (e: Exception) {
            try {
                val altFormat = if (previewFormat == UVCCamera.FRAME_FORMAT_YUYV) {
                    UVCCamera.FRAME_FORMAT_MJPEG
                } else {
                    UVCCamera.FRAME_FORMAT_YUYV
                }
                Logger.e(TAG, " setPreviewSize failed(format is $previewFormat), try same size, format=$altFormat", e)
                previewFormat = altFormat
                setPreviewOrThrow(previewFormat)
            } catch (e2: Exception) {
                try {
                    previewSize = getSuitableSize(request.previewWidth, request.previewHeight).apply {
                        mCameraRequest!!.previewWidth = width
                        mCameraRequest!!.previewHeight = height
                    }
                    if (! isPreviewSizeSupported(previewSize)) {
                        postStateEvent(ICameraStateCallBack.State.ERROR, "unsupported preview size")
                        closeCamera()
                        Logger.e(TAG, "open camera failed, preview size($previewSize) unsupported-> ${mUvcCamera?.supportedSizeList}")
                        return
                    }
                    initEncodeProcessor(previewSize.width, previewSize.height)
                    val altFormat = if (previewFormat == UVCCamera.FRAME_FORMAT_YUYV) {
                        UVCCamera.FRAME_FORMAT_MJPEG
                    } else {
                        UVCCamera.FRAME_FORMAT_YUYV
                    }
                    Logger.e(TAG, " setPreviewSize failed, try getSuitableSize + other format...", e2)
                    setPreviewOrThrow(altFormat)
                } catch (e3: Exception) {
                    closeCamera()
                    postStateEvent(ICameraStateCallBack.State.ERROR, "err: ${e3.localizedMessage}")
                    Logger.e(TAG, " setPreviewSize failed, even using yuv format", e3)
                    return
                }
            }
        }
        
        
        
        
        val needCpuNv21Stream = if (isNeedGLESRender) {
            mCameraRequest!!.isRawPreviewData || mCameraRequest!!.isCaptureRawImage
        } else {
            mCameraRequest!!.isRawPreviewData
                || mCameraRequest!!.isCaptureRawImage
                || mPreviewDataCbList.isNotEmpty()
        }
        if (needCpuNv21Stream) {
            mUvcCamera?.setFrameCallback(frameCallBack, UVCCamera.PIXEL_FORMAT_YUV420SP)
        }
        
        when(cameraView) {
            is Surface -> {
                mUvcCamera?.setPreviewDisplay(cameraView)
            }
            is SurfaceTexture -> {
                mUvcCamera?.setPreviewTexture(cameraView)
            }
            is SurfaceView -> {
                mUvcCamera?.setPreviewDisplay(cameraView.holder)
            }
            is TextureView -> {
                mUvcCamera?.setPreviewTexture(cameraView.surfaceTexture)
            }
            else -> {
                throw IllegalStateException("Only support Surface or SurfaceTexture or SurfaceView or TextureView or GLSurfaceView--$cameraView")
            }
        }
        mUvcCamera?.autoFocus = true
        mUvcCamera?.autoWhiteBlance = true
        mUvcCamera?.startPreview()
        mUvcCamera?.updateCameraParams()
        isPreviewed = true

        mMainHandler.post {
            if (mCameraRequest?.isAspectRatioShow == true) {
                (mCameraView as? IAspectRatio)?.setAspectRatio(
                    mCameraRequest!!.previewWidth,
                    mCameraRequest!!.previewHeight
                )
            }
        }

        postStateEvent(ICameraStateCallBack.State.OPENED)
        if (Utils.debugCamera) {
            Logger.i(TAG, " start preview, name = ${device.deviceName}, preview=$previewSize")
        }
    }

    override fun closeCameraInternal() {
        postStateEvent(ICameraStateCallBack.State.CLOSED)
        isPreviewed = false
        releaseEncodeProcessor()
        mUvcCamera?.destroy()
        mUvcCamera = null
        if (Utils.debugCamera) {
            Logger.i(TAG, " stop preview, name = ${device.deviceName}")
        }
    }

    override fun captureImageInternal(savePath: String?, callback: ICaptureCallBack) {
        mSaveImageExecutor.submit {
            if (! CameraUtils.hasStoragePermission(ctx)) {
                mMainHandler.post {
                    callback.onError("have no storage permission")
                }
                Logger.e(TAG,"open camera failed, have no storage permission")
                return@submit
            }
            if (! isPreviewed) {
                mMainHandler.post {
                    callback.onError("camera not previewing")
                }
                Logger.i(TAG, "captureImageInternal failed, camera not previewing")
                return@submit
            }
            val data = mNV21DataQueue.pollFirst(CAPTURE_TIMES_OUT_SEC, TimeUnit.SECONDS)
            if (data == null) {
                mMainHandler.post {
                    callback.onError("Times out")
                }
                Logger.i(TAG, "captureImageInternal failed, times out.")
                return@submit
            }
            mMainHandler.post {
                callback.onBegin()
            }
            val date = mDateFormat.format(System.currentTimeMillis())
            val title = savePath ?: "IMG_SuperKVM_$date"
            val displayName = savePath ?: "$title.jpg"
            val path = savePath ?: "$mCameraDir/$displayName"
            val location = Utils.getGpsLocation(ctx)
            val width = mCameraRequest!!.previewWidth
            val height = mCameraRequest!!.previewHeight
            val ret = MediaUtils.saveYuv2Jpeg(path, data, width, height)
            if (! ret) {
                val file = File(path)
                if (file.exists()) {
                    file.delete()
                }
                mMainHandler.post {
                    callback.onError("save yuv to jpeg failed.")
                }
                Logger.w(TAG, "save yuv to jpeg failed.")
                return@submit
            }
            val values = ContentValues()
            values.put(MediaStore.Images.ImageColumns.TITLE, title)
            values.put(MediaStore.Images.ImageColumns.DISPLAY_NAME, displayName)
            values.put(MediaStore.Images.ImageColumns.DATA, path)
            values.put(MediaStore.Images.ImageColumns.DATE_TAKEN, date)
            values.put(MediaStore.Images.ImageColumns.LONGITUDE, location?.longitude)
            values.put(MediaStore.Images.ImageColumns.LATITUDE, location?.latitude)
            ctx.contentResolver?.insert(MediaStore.Images.Media.EXTERNAL_CONTENT_URI, values)
            mMainHandler.post {
                callback.onComplete(path)
            }
            if (Utils.debugCamera) { Logger.i(TAG, "captureImageInternal save path = $path") }
        }
    }

    
    fun isMicSupported() = CameraUtils.isCameraContainsMic(this.device)

    
    fun sendCameraCommand(command: Int) {
        mCameraHandler?.post {
            mUvcCamera?.sendCommand(command)
        }
    }

    
    fun setAutoFocus(enable: Boolean) {
        mUvcCamera?.autoFocus = enable
    }

    
    fun getAutoFocus() = mUvcCamera?.autoFocus

    
    fun resetAutoFocus() {
        mUvcCamera?.resetFocus()
    }

    
    fun setAutoWhiteBalance(autoWhiteBalance: Boolean) {
        mUvcCamera?.autoWhiteBlance = autoWhiteBalance
    }

    
    fun getAutoWhiteBalance() = mUvcCamera?.autoWhiteBlance

    
    fun setZoom(zoom: Int) {
        mUvcCamera?.zoom = zoom
    }

    
    fun getZoom() = mUvcCamera?.zoom

    
    fun resetZoom() {
        mUvcCamera?.resetZoom()
    }

    
    fun setGain(gain: Int) {
        mUvcCamera?.gain = gain
    }

    
    fun getGain() = mUvcCamera?.gain

    
    fun resetGain() {
        mUvcCamera?.resetGain()
    }

    
    fun setGamma(gamma: Int) {
        mUvcCamera?.gamma = gamma
    }

    
    fun getGamma() = mUvcCamera?.gamma

    
    fun resetGamma() {
        mUvcCamera?.resetGamma()
    }

    
    fun setBrightness(brightness: Int) {
        mUvcCamera?.brightness = brightness
    }

    
    fun getBrightness() = mUvcCamera?.brightness
    
    fun getBrightnessMax() = mUvcCamera?.brightnessMax

    fun getBrightnessMin() = mUvcCamera?.brightnessMin
    
    
    fun resetBrightness() {
        mUvcCamera?.resetBrightness()
    }

    
    fun setContrast(contrast: Int) {
        mUvcCamera?.contrast = contrast
    }

    
    fun getContrast() = mUvcCamera?.contrast

    
    fun resetContrast() {
        mUvcCamera?.resetContrast()
    }

    
    fun setSharpness(sharpness: Int) {
        mUvcCamera?.sharpness = sharpness
    }

    
    fun getSharpness() = mUvcCamera?.sharpness

    
    fun resetSharpness() {
        mUvcCamera?.resetSharpness()
    }

    
    fun setSaturation(saturation: Int) {
        mUvcCamera?.saturation = saturation
    }

    
    fun getSaturation() = mUvcCamera?.saturation

    
    fun resetSaturation() {
        mUvcCamera?.resetSaturation()
    }

    
    fun setHue(hue: Int) {
        mUvcCamera?.hue = hue
    }

    
    fun getHue() = mUvcCamera?.hue

    
    fun resetHue() {
        mUvcCamera?.resetHue()
    }

    companion object {
        private const val TAG = "CameraUVC"
        
        private const val PREFER_FPS_TIER_HIGH_EXCLUSIVE = 29f
        
        private const val PREFER_FPS_TIER_MID_EXCLUSIVE = 23f
        
        private const val MIN_PREVIEW_FPS_EXCLUSIVE = 18f
        
        private const val HEURISTIC_MAX_AREA_MJPEG_NO_FPS = 1920 * 1080
        
        private const val HEURISTIC_MAX_AREA_YUYV_NO_FPS = 1280 * 720
        
        private const val PREVIEW_FPS_NEGOTIATION_MIN = 19
        private const val PREVIEW_FPS_NEGOTIATION_MAX = 61
    }
}
