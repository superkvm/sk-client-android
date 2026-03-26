
package com.superkvm.ausbc

import android.content.Context
import android.graphics.SurfaceTexture
import android.os.Handler
import android.os.Looper
import android.view.Surface
import androidx.annotation.MainThread
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleEventObserver
import androidx.lifecycle.LifecycleOwner
import com.superkvm.ausbc.callback.ICaptureCallBack
import com.superkvm.ausbc.callback.IEncodeDataCallBack
import com.superkvm.ausbc.callback.IPlayCallBack
import com.superkvm.ausbc.callback.IPreviewDataCallBack
import com.superkvm.ausbc.camera.*
import com.superkvm.ausbc.camera.bean.CameraRequest
import com.superkvm.ausbc.camera.bean.CameraStatus
import com.superkvm.ausbc.camera.bean.PreviewSize
import com.superkvm.ausbc.encode.AACEncodeProcessor
import com.superkvm.ausbc.encode.AbstractProcessor
import com.superkvm.ausbc.encode.H264EncodeProcessor
import com.superkvm.ausbc.encode.audio.AudioStrategySystem
import com.superkvm.ausbc.encode.bean.RawData
import com.superkvm.ausbc.encode.muxer.Mp4Muxer
import com.superkvm.ausbc.render.RenderManager
import com.superkvm.ausbc.render.effect.AbstractEffect
import com.superkvm.ausbc.render.env.RotateType
import com.superkvm.ausbc.utils.CameraUtils
import com.superkvm.ausbc.utils.Logger
import com.superkvm.ausbc.utils.Utils
import com.superkvm.ausbc.utils.bus.BusKey
import com.superkvm.ausbc.utils.bus.EventBus
import com.superkvm.ausbc.widget.AspectRatioSurfaceView
import com.superkvm.ausbc.widget.AspectRatioTextureView
import com.superkvm.ausbc.widget.IAspectRatio
import com.superkvm.usb.USBMonitor
import com.superkvm.uvc.UVCCamera
import kotlin.math.abs


@kotlin.Deprecated("Deprecated since version 3.3.0")
class CameraClient internal constructor(builder: Builder) : IPreviewDataCallBack {
    private val mCtx: Context? = builder.context
    private val isEnableGLEs: Boolean = builder.enableGLEs
    private val rawImage: Boolean = builder.rawImage
    private val mCamera: ICameraStrategy? = builder.camera
    private var mCameraView: IAspectRatio? = null
    private var mRequest: CameraRequest? = builder.cameraRequest
    private var mDefaultEffect: AbstractEffect? = builder.defaultEffect
    private val mDefaultRotateType: RotateType? = builder.defaultRotateType
    private var mAudioProcess: AbstractProcessor? = null
    private var mVideoProcess: AbstractProcessor? = null
    private var mMediaMuxer: Mp4Muxer? = null
    private val mMainHandler: Handler = Handler(Looper.getMainLooper())

    private val mRenderManager: RenderManager? by lazy {
        RenderManager(mCtx!!, mRequest!!.previewWidth, mRequest!!.previewHeight, null)
    }

    init {
        mRequest = mRequest ?: CameraRequest.Builder().create()
        mCtx?.let { context ->
            if (context !is LifecycleOwner) {
                throw IllegalArgumentException("context should be subclass of LifecycleOwner!")
            }
            addLifecycleObserver(context)
            
            EventBus.with<CameraStatus>(BusKey.KEY_CAMERA_STATUS).observe(context, { status ->
                when(status.code) {
                    CameraStatus.ERROR -> {
                        mCamera?.stopPreview()
                    }
                    CameraStatus.ERROR_PREVIEW_SIZE -> {
                        mRequest?.let { request ->
                            val oldPreviewWidth = request.previewWidth
                            val oldPreviewHeight = request.previewHeight
                            getSuitableSize(oldPreviewWidth, oldPreviewHeight).let {
                                it ?: return@observe
                            }.also {
                                Logger.i(TAG, "Automatically select the appropriate resolution (${it.width}x${it.height})")
                                updateResolution(it.width, it.height)
                            }
                        }
                    }
                    else -> { }
                }
            })
        }
        if (Utils.debugCamera) {
            Logger.i(TAG, "init camera client, camera = $mCamera")
        }
    }

    override fun onPreviewData(data: ByteArray?, width: Int, height: Int, format: IPreviewDataCallBack.DataFormat) {
        data?.let {
            
            if (data.size != width * height * 3 /2) {
                return
            }
            mVideoProcess?.putRawData(RawData(it, it.size))
        }
    }

    
    fun openCamera(cameraView: IAspectRatio?, isReboot: Boolean = false) {
        if (mCtx != null && Utils.isTargetSdkOverP(mCtx) && !CameraUtils.hasCameraPermission(mCtx)) {
            Logger.e(TAG,"open camera failed, need Manifest.permission.CAMERA permission")
            return
        }
        initEncodeProcessor()
        Logger.i(TAG, "start open camera request = $mRequest, gl = $isEnableGLEs")
        val previewWidth = mRequest!!.previewWidth
        val previewHeight = mRequest!!.previewHeight
        when (cameraView) {
            is AspectRatioSurfaceView -> {
                if (! isEnableGLEs) {
                    cameraView.postUITask {
                        mCamera?.startPreview(mRequest!!, cameraView.holder)
                        mCamera?.addPreviewDataCallBack(this)
                    }
                }
                cameraView.setAspectRatio(previewWidth, previewHeight)
                cameraView
            }
            is AspectRatioTextureView -> {
                if (! isEnableGLEs) {
                    cameraView.postUITask {
                        mCamera?.startPreview(mRequest!!, cameraView.surfaceTexture)
                        mCamera?.addPreviewDataCallBack(this)
                    }
                }
                cameraView.setAspectRatio(previewWidth, previewHeight)
                cameraView
            }
            else -> {
                cameraView
            }
        }.also { view->
            
            
            mCameraView = view ?: mCameraView

            
            
            if (! isEnableGLEs) return
            view.apply {
                val listener = object : RenderManager.CameraSurfaceTextureListener {
                    override fun onSurfaceTextureAvailable(surfaceTexture: SurfaceTexture?) {
                        surfaceTexture?.let {
                            mCamera?.startPreview(mRequest!!, it)
                            mCamera?.addPreviewDataCallBack(this@CameraClient)
                        }
                    }
                }
                if (this == null) {
                    Logger.i(TAG, "Offscreen render, width=$previewWidth, height=$previewHeight")
                    mRenderManager?.startRenderScreen(previewWidth, previewHeight, null, listener)
                    if (isReboot) {
                        mRenderManager?.getCacheEffectList()?.forEach { effect ->
                            mRenderManager?.addRenderEffect(effect)
                        }
                        return@apply
                    }
                    mRenderManager?.addRenderEffect(mDefaultEffect)
                    return@apply
                }
                postUITask {
                    val surfaceWidth = getSurfaceWidth()
                    val surfaceHeight = getSurfaceHeight()
                    val surface = getSurface()
                    mRenderManager?.startRenderScreen(surfaceWidth, surfaceHeight, surface, listener)
                    mRenderManager?.setRotateType(mDefaultRotateType)
                    if (isReboot) {
                        mRenderManager?.getCacheEffectList()?.forEach { effect ->
                            mRenderManager?.addRenderEffect(effect)
                        }
                        return@postUITask
                    }
                    mRenderManager?.addRenderEffect(mDefaultEffect)
                    Logger.i(TAG, "Display render, width=$surfaceWidth, height=$surfaceHeight")
                }
            }
        }
    }

    
    fun closeCamera() {
        if (Utils.debugCamera) {
            Logger.i(TAG, "closeCamera...")
        }
        releaseEncodeProcessor()
        if (isEnableGLEs) {
            mRenderManager?.stopRenderScreen()
        }
        mCamera?.stopPreview()
    }

    
    fun setRotateType(type: RotateType?) {
        mRenderManager?.setRotateType(type)
    }

    
    fun setRenderSize(width: Int, height: Int) {
        mRenderManager?.setRenderSize(width, height)
    }

    
    fun addRenderEffect(effect: AbstractEffect) {
        mRenderManager?.addRenderEffect(effect)
    }

    
    fun removeRenderEffect(effect: AbstractEffect) {
        mRenderManager?.removeRenderEffect(effect)
    }

    
    fun updateRenderEffect(classifyId: Int, effect: AbstractEffect?) {
        mRenderManager?.getCacheEffectList()?.find {
            it.getClassifyId() == classifyId
        }?.also {
            removeRenderEffect(it)
        }
        effect ?: return
        addRenderEffect(effect)
    }

    
    fun switchCamera(cameraId: String? = null) {
        if (Utils.debugCamera) {
            Logger.i(TAG, "switchCamera, id = $cameraId")
        }
        mCamera?.switchCamera(cameraId)
    }

    
    fun captureImage(callBack: ICaptureCallBack, path: String? = null) {
        if (Utils.debugCamera) {
            Logger.i(TAG, "captureImage...")
        }
        if (isEnableGLEs && ! rawImage) {
            mRenderManager?.saveImage(callBack, path)
            return
        }
        mCamera?.captureImage(callBack, path)
    }

    
    fun isCameraOpened() = mCamera?.isCameraOpened()

    
    fun startPlayMic(callBack: IPlayCallBack?) {
        (mAudioProcess as? AACEncodeProcessor)?.playAudioStart(callBack)
    }

    
    fun stopPlayMic() {
        (mAudioProcess as? AACEncodeProcessor)?.playAudioStop()
    }

    
    fun startPush() {
        mVideoProcess?.startEncode()
        mAudioProcess?.startEncode()
    }

    
    fun captureAudioStart(callBack: ICaptureCallBack, mp3Path: String?=null) {
        val path = if (mp3Path.isNullOrEmpty()) {
            "${mCtx?.getExternalFilesDir(null)?.path}/${System.currentTimeMillis()}.mp3"
        } else {
            mp3Path
        }
        (mAudioProcess as? AACEncodeProcessor)?.recordMp3Start(path, callBack)
    }

    
    fun captureAudioStop() {
        (mAudioProcess as? AACEncodeProcessor)?.recordMp3Stop()
    }

    
    fun stopPush() {
        mVideoProcess?.stopEncode()
        mAudioProcess?.stopEncode()
    }

    
    fun addEncodeDataCallBack(callBack: IEncodeDataCallBack) {
        mVideoProcess?.setEncodeDataCallBack(callBack)
        mAudioProcess?.setEncodeDataCallBack(callBack)
    }

    
    fun addPreviewDataCallBack(callBack: IPreviewDataCallBack) {
        mCamera?.addPreviewDataCallBack(callBack)
    }

    
    fun removePreviewDataCallBack(callBack: IPreviewDataCallBack) {
        mCamera?.removePreviewDataCallBack(callBack)
    }

    
    fun captureVideoStart(callBack: ICaptureCallBack, path: String ?= null, durationInSec: Long = 0L) {
        mMediaMuxer = Mp4Muxer(mCtx, callBack,  path, durationInSec)
        (mVideoProcess as? H264EncodeProcessor)?.apply {
            startEncode()
            setMp4Muxer(mMediaMuxer!!)
            setOnEncodeReadyListener(object : H264EncodeProcessor.OnEncodeReadyListener {
                override fun onReady(surface: Surface?) {
                    if (! isEnableGLEs) {
                        return
                    }
                    if (surface == null) {
                        Logger.e(TAG, "Input surface can't be null.")
                        return
                    }
                    mRenderManager?.startRenderCodec(surface, width, height)
                }
            })
        }
        (mAudioProcess as? AACEncodeProcessor)?.apply {
            startEncode()
            setMp4Muxer(mMediaMuxer!!)
        }
    }

    
    fun captureVideoStop() {
        mRenderManager?.stopRenderCodec()
        mMediaMuxer?.release()
        mVideoProcess?.stopEncode()
        mAudioProcess?.stopEncode()
        mMediaMuxer = null
    }

    
    @MainThread
    fun updateResolution(width: Int, height: Int): Boolean {
        if (Utils.debugCamera) {
            Logger.i(TAG, "updateResolution size = ${width}x${height}")
        }
        getCameraRequest().apply {
            if (this == null) {
                Logger.e(TAG, "updateResolution failed, camera request is null.")
                return false
            }
            if (mVideoProcess?.isEncoding() == true) {
                Logger.e(TAG, "updateResolution failed, video recording...")
                return false
            }
            previewWidth = width
            previewHeight = height
            closeCamera()
            mMainHandler.postDelayed({
                openCamera(mCameraView, true)
            }, 500)
        }
        return true
    }


    
    fun getAllPreviewSizes(aspectRatio: Double? = null): MutableList<PreviewSize>? {
        return mCamera?.getAllPreviewSizes(aspectRatio)
    }

    
    fun getCameraRequest() = mRequest

    
    fun getCameraStrategy() = mCamera

    
    fun getDefaultEffect() = mDefaultEffect

    
    fun sendCameraCommand(command: Int): Int? {
        if (mCamera !is CameraUvcStrategy) {
            return null
        }
        return mCamera.sendCameraCommand(command)
    }

    private fun initEncodeProcessor() {
        releaseEncodeProcessor()
        val  encodeWidth = if (isEnableGLEs) {
            mRequest!!.previewHeight
        } else {
            mRequest!!.previewWidth
        }
        val encodeHeight = if (isEnableGLEs) {
            mRequest!!.previewWidth
        } else {
            mRequest!!.previewHeight
        }
        mAudioProcess = AACEncodeProcessor(AudioStrategySystem())
        mVideoProcess = H264EncodeProcessor(encodeWidth, encodeHeight, isEnableGLEs)
    }

    private fun releaseEncodeProcessor() {
        (mAudioProcess as? AACEncodeProcessor)?.playAudioStop()
        mVideoProcess?.stopEncode()
        mAudioProcess?.stopEncode()
        mVideoProcess = null
        mAudioProcess = null
    }

    private fun addLifecycleObserver(context: Context) {
        (context as LifecycleOwner).lifecycle.addObserver(object : LifecycleEventObserver {
            override fun onStateChanged(source: LifecycleOwner, event: Lifecycle.Event) {
                when (event) {
                    Lifecycle.Event.ON_DESTROY -> {
                        captureVideoStop()
                        closeCamera()
                    }
                    else -> {}
                }
            }
        })
    }

    private fun getSuitableSize(
        maxWidth: Int,
        maxHeight: Int
    ): PreviewSize? {
        val sizeList = getAllPreviewSizes()
        
        sizeList?.find {
            it.width == maxWidth && it.height == maxHeight
        }.also { size ->
            size ?: return@also
            return size
        }
        
        val aspectRatio = maxWidth.toFloat() / maxHeight
        sizeList?.find {
            val w = it.width
            val h = it.height
            val ratio = w.toFloat() / h
            ratio == aspectRatio && w <= maxWidth && h <= maxHeight
        }.also { size ->
            size ?: return@also
            return size
        }
        
        var minDistance: Int = maxWidth
        var closetSize: PreviewSize? = null
        sizeList?.forEach { size ->
            if (minDistance >= abs((maxWidth - size.width))) {
                minDistance = abs(maxWidth - size.width)
                closetSize = size
            }
        }
        return closetSize
    }

    companion object {
        private const val TAG = "CameraClient"

        @JvmStatic
        fun newBuilder(ctx: Context) = Builder(ctx)
    }

    class Builder constructor() {
        internal var context: Context? = null
        internal var cameraRequest: CameraRequest? = null
        internal var enableGLEs: Boolean = true
        internal var rawImage: Boolean = true
        internal var camera: ICameraStrategy? = null
        internal var defaultEffect: AbstractEffect? = null
        internal var videoEncodeBitRate: Int? = null
        internal var videoEncodeFrameRate: Int? = null
        internal var defaultRotateType: RotateType? = null

        constructor(context: Context) : this() {
            this.context = context
        }

        
        fun setCameraStrategy(camera: ICameraStrategy?): Builder {
            this.camera = camera
            return this
        }

        
        fun setCameraRequest(request: CameraRequest): Builder {
            this.cameraRequest = request
            return this
        }

        
        fun setEnableGLES(enable: Boolean): Builder {
            this.enableGLEs = enable
            return this
        }

        
        fun setRawImage(rawImage: Boolean): Builder {
            this.rawImage = rawImage
            return this
        }

        
        fun setDefaultEffect(effect: AbstractEffect): Builder {
            this.defaultEffect = effect
            return this
        }

        
        @Deprecated("Not realized")
        fun setVideoEncodeBitRate(bitRate: Int): Builder {
            this.videoEncodeBitRate = bitRate
            return this
        }

        
        @Deprecated("Not realized")
        fun setVideoEncodeFrameRate(frameRate: Int): Builder {
            this.videoEncodeFrameRate = frameRate
            return this
        }

        
        fun openDebug(debug: Boolean): Builder {
            UVCCamera.DEBUG = debug
            USBMonitor.DEBUG = debug
            Utils.debugCamera = debug
            return this
        }

        
        fun setDefaultRotateType(type: RotateType?): Builder {
            this.defaultRotateType = type
            return this
        }

        override fun toString(): String {
            return "Builder(context=$context, cameraType=$camera, " +
                    "cameraRequest=$cameraRequest, glEsVersion=$enableGLEs)"
        }

        
        fun build() = CameraClient(this)
    }
}

