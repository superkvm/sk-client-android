
package com.superkvm.ausbc.camera

import android.Manifest
import android.content.Context
import android.content.pm.PackageManager
import android.graphics.SurfaceTexture
import android.os.*
import android.view.OrientationEventListener
import android.view.SurfaceHolder
import androidx.core.content.ContextCompat
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleEventObserver
import androidx.lifecycle.LifecycleOwner
import com.superkvm.ausbc.MultiCameraClient
import com.superkvm.ausbc.callback.ICaptureCallBack
import com.superkvm.ausbc.callback.IPreviewDataCallBack
import com.superkvm.ausbc.camera.bean.CameraInfo
import com.superkvm.ausbc.camera.bean.CameraRequest
import com.superkvm.ausbc.camera.bean.CameraStatus
import com.superkvm.ausbc.camera.bean.PreviewSize
import com.superkvm.ausbc.utils.bus.BusKey
import com.superkvm.ausbc.utils.bus.EventBus
import java.lang.Deprecated
import java.text.SimpleDateFormat
import java.util.*
import java.util.concurrent.CopyOnWriteArrayList
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors
import java.util.concurrent.atomic.AtomicBoolean


@kotlin.Deprecated("Deprecated since version 3.3.0")
abstract class ICameraStrategy(context: Context) : Handler.Callback {
    private var mThread: HandlerThread? = null
    private var mCameraHandler: Handler? = null
    private var mSurfaceTexture: SurfaceTexture? = null
    private var mSurfaceHolder: SurfaceHolder? = null
    private var mCameraRequest: CameraRequest? = null
    private var mContext: Context? = null
    protected var mPreviewDataCbList = CopyOnWriteArrayList<IPreviewDataCallBack>()
    protected var mCaptureDataCb: ICaptureCallBack? = null
    protected val mMainHandler: Handler = Handler(Looper.getMainLooper())
    protected val mSaveImageExecutor: ExecutorService = Executors.newSingleThreadExecutor()
    protected val mCameraInfoMap = hashMapOf<Int, CameraInfo>()
    protected var mIsCapturing: AtomicBoolean = AtomicBoolean(false)
    protected var mIsPreviewing: AtomicBoolean = AtomicBoolean(false)
    protected val mDateFormat by lazy {
        SimpleDateFormat("yyyyMMddHHmmssSSS", Locale.getDefault())
    }
    protected val mCameraDir by lazy {
        "${Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DCIM)}/Camera"
    }

    private val mDeviceOrientation = object : OrientationEventListener(context) {
        var orientation = 0
            private set

        override fun onOrientationChanged(orientation: Int) {
            this.orientation = orientation
        }
    }

    init {
        this.mContext = context.applicationContext
        addLifecycleObserver(context)
    }

    override fun handleMessage(msg: Message): Boolean {
        when(msg.what) {
            MSG_INIT -> {
                loadCameraInfo()
            }
            MSG_START_PREVIEW -> {
                msg.obj ?: return true
                (msg.obj as CameraRequest).apply {
                    if (mIsPreviewing.get()) {
                        mDeviceOrientation.disable()
                        stopPreviewInternal()
                    }
                    mDeviceOrientation.enable()
                    mCameraRequest = this
                    startPreviewInternal()
                }
            }
            MSG_STOP_PREVIEW -> {
                mCameraInfoMap.clear()
                mDeviceOrientation.disable()
                stopPreviewInternal()
            }
            MSG_CAPTURE_IMAGE -> {
                captureImageInternal(msg.obj as? String)
            }
            MSG_SWITCH_CAMERA -> {
                switchCameraInternal(msg.obj as? String)
            }
        }
        return true
    }

    
    @Synchronized
    fun <T> startPreview(request: CameraRequest?, renderSurface: T?) {
        if (mIsPreviewing.get() || mThread?.isAlive == true) {
            stopPreview()
        }
        if (mCameraRequest == null && request == null) {
            throw IllegalStateException("camera request can't be null")
        }
        if (mSurfaceHolder == null && mSurfaceTexture == null && renderSurface == null) {
            throw IllegalStateException("render surface can't be null")
        }
        when (renderSurface) {
            is SurfaceTexture -> {
                setSurfaceTexture(renderSurface)
            }
            is SurfaceHolder -> {
                setSurfaceHolder(renderSurface)
            }
            else -> {
            }
        }.also {
            val thread = HandlerThread(THREAD_NAME).apply {
                start()
            }.also {
                mCameraHandler = Handler(it.looper, this)
                mCameraHandler?.obtainMessage(MSG_INIT)?.sendToTarget()
                mCameraHandler?.obtainMessage(MSG_START_PREVIEW, request ?: mCameraRequest)?.sendToTarget()
            }
            this.mThread = thread
        }
    }

    
    @Synchronized
    fun stopPreview() {
        mThread ?: return
        mCameraHandler ?: return
        mCameraHandler?.obtainMessage(MSG_STOP_PREVIEW)?.sendToTarget()
        mThread?.quitSafely()
        mThread = null
        mCameraHandler = null
    }

    
    @Synchronized
    fun captureImage(callBack: ICaptureCallBack, savePath: String?) {
        this.mCaptureDataCb = callBack
        mCameraHandler?.obtainMessage(MSG_CAPTURE_IMAGE, savePath)?.sendToTarget()
    }

    
    @Synchronized
    fun switchCamera(cameraId: String? = null) {
        mCameraHandler?.obtainMessage(MSG_SWITCH_CAMERA, cameraId)?.sendToTarget()
    }

    private fun setSurfaceTexture(surfaceTexture: SurfaceTexture) {
        this.mSurfaceTexture = surfaceTexture
    }

    private fun setSurfaceHolder(holder: SurfaceHolder) {
        this.mSurfaceHolder = holder
    }

    
    abstract fun getAllPreviewSizes(aspectRatio: Double? = null): MutableList<PreviewSize>?

    
    fun getSurfaceTexture(): SurfaceTexture? = mSurfaceTexture

    
    fun getSurfaceHolder(): SurfaceHolder? = mSurfaceHolder

    
    protected fun getContext(): Context? = mContext

    
    protected fun getRequest(): CameraRequest? = mCameraRequest

    
    protected fun getCameraHandler(): Handler? = mCameraHandler

    
    protected fun getDeviceOrientation(): Int = mDeviceOrientation.orientation

    
    protected fun postCameraStatus(status: CameraStatus) {
        EventBus.with<CameraStatus>(BusKey.KEY_CAMERA_STATUS).postMessage(status)
    }

    
    open fun register() {}

    
    open fun unRegister() {}

    
    protected abstract fun loadCameraInfo()

    
    protected abstract fun startPreviewInternal()

    
    protected abstract fun stopPreviewInternal()

    
    protected abstract fun captureImageInternal(savePath: String?)

    
    protected abstract fun switchCameraInternal(cameraId: String?)

    
    protected abstract fun updateResolutionInternal(width: Int, height: Int)

    
    protected fun hasCameraPermission(): Boolean{
        getContext() ?: return false
        val locPermission = ContextCompat.checkSelfPermission(getContext()!!, Manifest.permission.CAMERA)
        return locPermission == PackageManager.PERMISSION_GRANTED
    }

    
    protected fun hasStoragePermission(): Boolean {
        getContext() ?: return false
        val locPermission = ContextCompat.checkSelfPermission(getContext()!!, Manifest.permission.WRITE_EXTERNAL_STORAGE)
        return locPermission == PackageManager.PERMISSION_GRANTED
    }

    private fun addLifecycleObserver(context: Context) {
        if (context !is LifecycleOwner) return
        context.lifecycle.addObserver(object : LifecycleEventObserver {
            override fun onStateChanged(source: LifecycleOwner, event: Lifecycle.Event) {
                when (event) {
                    Lifecycle.Event.ON_CREATE -> {
                        register()
                    }
                    Lifecycle.Event.ON_DESTROY -> {
                        stopPreview()
                        unRegister()
                    }
                    else -> {}
                }
            }
        })
    }

    
    fun addPreviewDataCallBack(callBack: IPreviewDataCallBack) {
        if (mPreviewDataCbList.contains(callBack)) {
            return
        }
        mPreviewDataCbList.add(callBack)
    }

    
    fun removePreviewDataCallBack(callBack: IPreviewDataCallBack) {
        if (! mPreviewDataCbList.contains(callBack)) {
            return
        }
        mPreviewDataCbList.remove(callBack)
    }

    
    fun isCameraOpened() = mIsPreviewing.get()

    companion object {
        private const val TAG = "ICameraStrategy"
        private const val THREAD_NAME = "camera_manager"
        private const val MSG_INIT = 0x00
        private const val MSG_START_PREVIEW = 0x01
        private const val MSG_STOP_PREVIEW = 0x02
        private const val MSG_CAPTURE_IMAGE = 0x03
        private const val MSG_SWITCH_CAMERA = 0x04

        internal const val TYPE_FRONT = 0
        internal const val TYPE_BACK = 1
        internal const val TYPE_OTHER = 2
    }
}