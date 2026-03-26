
package com.superkvm.ausbc.base

import android.content.Context
import android.graphics.SurfaceTexture
import android.hardware.usb.UsbDevice
import android.view.*
import android.widget.FrameLayout
import android.widget.LinearLayout
import android.widget.RelativeLayout
import com.superkvm.ausbc.MultiCameraClient
import com.superkvm.ausbc.camera.bean.CameraRequest
import com.superkvm.ausbc.camera.bean.PreviewSize
import com.superkvm.ausbc.callback.*
import com.superkvm.ausbc.camera.CameraUVC
import com.superkvm.ausbc.render.effect.AbstractEffect
import com.superkvm.ausbc.render.effect.EffectBlackWhite
import com.superkvm.ausbc.render.env.RotateType
import com.superkvm.ausbc.utils.Logger
import com.superkvm.ausbc.utils.SettableFuture
import com.superkvm.ausbc.widget.IAspectRatio
import com.superkvm.usb.USBMonitor
import java.lang.IllegalArgumentException
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicBoolean



abstract class CameraActivity: BaseActivity(), ICameraStateCallBack {
    private var mCameraView: IAspectRatio? = null
    private var mCameraClient: MultiCameraClient? = null
    private val mCameraMap = hashMapOf<Int, MultiCameraClient.ICamera>()
    private var mCurrentCamera: SettableFuture<MultiCameraClient.ICamera>? = null

    private val mRequestPermission: AtomicBoolean by lazy {
        AtomicBoolean(false)
    }

    override fun initView() {
        when (val cameraView = getCameraView()) {
            is TextureView -> {
                handleTextureView(cameraView)
                cameraView
            }
            is SurfaceView -> {
                handleSurfaceView(cameraView)
                cameraView
            }
            else -> {
                null
            }
        }.apply {
            mCameraView = this
            
            if (this == null) {
                registerMultiCamera()
                return
            }
        }?.also { view->
            getCameraViewContainer()?.apply {
                removeAllViews()
                addView(view, getViewLayoutParams(this))
            }
        }
    }

    override fun clear() {
        unRegisterMultiCamera()
    }

    protected fun registerMultiCamera() {
        mCameraClient = MultiCameraClient(this, object : IDeviceConnectCallBack {
            override fun onAttachDev(device: UsbDevice?) {
                device ?: return
                if (mCameraMap.containsKey(device.deviceId)) {
                    return
                }
                generateCamera(this@CameraActivity, device).apply {
                    mCameraMap[device.deviceId] = this
                }
                
                
                if (mRequestPermission.get()) {
                    return
                }
                getDefaultCamera()?.apply {
                    if (vendorId == device.vendorId && productId == device.productId) {
                        Logger.i(TAG, "default camera pid: $productId, vid: $vendorId")
                        requestPermission(device)
                    }
                    return
                }
                requestPermission(device)
            }

            override fun onDetachDec(device: UsbDevice?) {
                mCameraMap.remove(device?.deviceId)?.apply {
                    setUsbControlBlock(null)
                }
                mRequestPermission.set(false)
                try {
                    mCurrentCamera?.cancel(true)
                    mCurrentCamera = null
                } catch (e: Exception) {
                    e.printStackTrace()
                }
            }

            override fun onConnectDev(device: UsbDevice?, ctrlBlock: USBMonitor.UsbControlBlock?) {
                device ?: return
                ctrlBlock ?: return
                mCameraMap[device.deviceId]?.apply {
                    setUsbControlBlock(ctrlBlock)
                }?.also { camera ->
                    try {
                        mCurrentCamera?.cancel(true)
                        mCurrentCamera = null
                    } catch (e: Exception) {
                        e.printStackTrace()
                    }
                    mCurrentCamera = SettableFuture()
                    mCurrentCamera?.set(camera)
                    openCamera(mCameraView)
                    Logger.i(TAG, "camera connection. pid: ${device.productId}, vid: ${device.vendorId}")
                }
            }

            override fun onDisConnectDec(device: UsbDevice?, ctrlBlock: USBMonitor.UsbControlBlock?) {
                closeCamera()
                mRequestPermission.set(false)
            }

            override fun onCancelDev(device: UsbDevice?) {
                mRequestPermission.set(false)
                try {
                    mCurrentCamera?.cancel(true)
                    mCurrentCamera = null
                } catch (e: Exception) {
                    e.printStackTrace()
                }
            }
        })
        mCameraClient?.register()
    }

    protected fun unRegisterMultiCamera() {
        mCameraMap.values.forEach {
            it.closeCamera()
        }
        mCameraMap.clear()
        mCameraClient?.unRegister()
        mCameraClient?.destroy()
        mCameraClient = null
    }

    protected fun getDeviceList() = mCameraClient?.getDeviceList()

    private fun handleTextureView(textureView: TextureView) {
        textureView.surfaceTextureListener = object : TextureView.SurfaceTextureListener {
            override fun onSurfaceTextureAvailable(
                surface: SurfaceTexture?,
                width: Int,
                height: Int
            ) {
                registerMultiCamera()
            }

            override fun onSurfaceTextureSizeChanged(
                surface: SurfaceTexture?,
                width: Int,
                height: Int
            ) {
                surfaceSizeChanged(width, height)
            }

            override fun onSurfaceTextureDestroyed(surface: SurfaceTexture?): Boolean {
                unRegisterMultiCamera()
                return false
            }

            override fun onSurfaceTextureUpdated(surface: SurfaceTexture?) {
            }
        }
    }

    private fun handleSurfaceView(surfaceView: SurfaceView) {
        surfaceView.holder.addCallback(object : SurfaceHolder.Callback {
            override fun surfaceCreated(holder: SurfaceHolder?) {
                registerMultiCamera()
            }

            override fun surfaceChanged(
                holder: SurfaceHolder?,
                format: Int,
                width: Int,
                height: Int
            ) {
                surfaceSizeChanged(width, height)
            }

            override fun surfaceDestroyed(holder: SurfaceHolder?) {
                unRegisterMultiCamera()
            }
        })
    }

    
    protected fun getCurrentCamera(): MultiCameraClient.ICamera? {
        return try {
            mCurrentCamera?.get(2, TimeUnit.SECONDS)
        } catch (e: Exception) {
            e.printStackTrace()
            null
        }
    }

    
    protected fun requestPermission(device: UsbDevice?) {
        mRequestPermission.set(true)
        mCameraClient?.requestPermission(device)
    }

    
    protected open fun generateCamera(ctx: Context, device: UsbDevice): MultiCameraClient.ICamera {
        return CameraUVC(ctx, device)
    }

    
    protected open fun getDefaultCamera(): UsbDevice? = null

    
    protected fun captureImage(callBack: ICaptureCallBack, savePath: String? = null) {
        getCurrentCamera()?.captureImage(callBack, savePath)
    }


    
    protected fun getDefaultEffect() = getCurrentCamera()?.getDefaultEffect()

    
    protected fun switchCamera(usbDevice: UsbDevice) {
        getCurrentCamera()?.closeCamera()
        try {
            Thread.sleep(500)
        } catch (e: Exception) {
            e.printStackTrace()
        }
        requestPermission(usbDevice)
    }

    
    protected fun isCameraOpened() = getCurrentCamera()?.isCameraOpened()  ?: false

    
    protected fun updateResolution(width: Int, height: Int) {
        getCurrentCamera()?.updateResolution(width, height)
    }

    
    protected fun getAllPreviewSizes(aspectRatio: Double? = null) = getCurrentCamera()?.getAllPreviewSizes(aspectRatio)

    
    protected fun addRenderEffect(effect: AbstractEffect) {
        getCurrentCamera()?.addRenderEffect(effect)
    }

    
    protected fun removeRenderEffect(effect: AbstractEffect) {
        getCurrentCamera()?.removeRenderEffect(effect)
    }

    
    protected fun updateRenderEffect(classifyId: Int, effect: AbstractEffect?) {
        getCurrentCamera()?.updateRenderEffect(classifyId, effect)
    }

    
    protected fun captureStreamStart() {
        getCurrentCamera()?.captureStreamStart()
    }

    
    protected fun captureStreamStop() {
        getCurrentCamera()?.captureStreamStop()
    }

    
    protected fun setEncodeDataCallBack(callBack: IEncodeDataCallBack) {
        getCurrentCamera()?.setEncodeDataCallBack(callBack)
    }

    
    protected fun addPreviewDataCallBack(callBack: IPreviewDataCallBack) {
        getCurrentCamera()?.addPreviewDataCallBack(callBack)
    }

    
    fun removePreviewDataCallBack(callBack: IPreviewDataCallBack) {
        getCurrentCamera()?.removePreviewDataCallBack(callBack)
    }

    
    protected fun captureVideoStart(callBack: ICaptureCallBack, path: String ?= null, durationInSec: Long = 0L) {
        getCurrentCamera()?.captureVideoStart(callBack, path, durationInSec)
    }

    
    protected fun captureVideoStop() {
        getCurrentCamera()?.captureVideoStop()
    }

    
    protected fun captureAudioStart(callBack: ICaptureCallBack, path: String ?= null) {
        getCurrentCamera()?.captureAudioStart(callBack, path)
    }

    
    protected fun captureAudioStop() {
        getCurrentCamera()?.captureAudioStop()
    }

    
    protected fun startPlayMic(callBack: IPlayCallBack? = null) {
        getCurrentCamera()?.startPlayMic(callBack)
    }

    
    protected fun stopPlayMic() {
        getCurrentCamera()?.stopPlayMic()
    }

    
    protected fun getCurrentPreviewSize(): PreviewSize? {
        return getCurrentCamera()?.getCameraRequest()?.let {
            PreviewSize(it.previewWidth, it.previewHeight)
        }
    }

    
    protected fun setRotateType(type: RotateType) {
        getCurrentCamera()?.setRotateType(type)
    }

    
    
    
    protected fun sendCameraCommand(command: Int) {
        getCurrentCamera()?.let { camera ->
            if (camera !is CameraUVC) {
                return
            }
            camera.sendCameraCommand(command)
        }
    }

    
    protected fun setAutoFocus(focus: Boolean) {
        getCurrentCamera()?.let { camera ->
            if (camera !is CameraUVC) {
                return
            }
            camera.setAutoFocus(focus)
        }
    }

    
    protected fun getAutoFocus(): Boolean? {
        return getCurrentCamera()?.let { camera ->
            if (camera !is CameraUVC) {
                return@let false
            }
            camera.getAutoFocus()
        }
    }

    
    protected fun resetAutoFocus() {
        getCurrentCamera()?.let { camera ->
            if (camera !is CameraUVC) {
                return
            }
            camera.resetAutoFocus()
        }
    }



    
    protected fun setBrightness(brightness: Int) {
        getCurrentCamera()?.let { camera ->
            if (camera !is CameraUVC) {
                return
            }
            camera.setBrightness(brightness)
        }
    }

    
    protected fun getBrightness(): Int? {
        return getCurrentCamera()?.let { camera ->
            if (camera !is CameraUVC) {
                return@let null
            }
            camera.getBrightness()
        }
    }

    
    protected fun resetBrightness() {
        getCurrentCamera()?.let { camera ->
            if (camera !is CameraUVC) {
                return
            }
            camera.resetBrightness()
        }
    }

    
    protected fun setContrast(contrast: Int) {
        getCurrentCamera()?.let { camera ->
            if (camera !is CameraUVC) {
                return
            }
            camera.setContrast(contrast)
        }
    }

    
    protected fun getContrast(): Int? {
        return getCurrentCamera()?.let { camera ->
            if (camera !is CameraUVC) {
                return@let null
            }
            camera.getContrast()
        }
    }

    
    protected fun resetContrast() {
        getCurrentCamera()?.let { camera ->
            if (camera !is CameraUVC) {
                return
            }
            camera.resetContrast()
        }
    }

    
    protected fun setGain(gain: Int) {
        getCurrentCamera()?.let { camera ->
            if (camera !is CameraUVC) {
                return
            }
            camera.setGain(gain)
        }
    }

    
    protected fun getGain(): Int? {
        return getCurrentCamera()?.let { camera ->
            if (camera !is CameraUVC) {
                return@let null
            }
            camera.getGain()
        }
    }

    
    protected fun resetGain() {
        getCurrentCamera()?.let { camera ->
            if (camera !is CameraUVC) {
                return
            }
            camera.resetGain()
        }
    }

    
    protected fun setGamma(gamma: Int) {
        getCurrentCamera()?.let { camera ->
            if (camera !is CameraUVC) {
                return
            }
            camera.setGamma(gamma)
        }
    }

    
    protected fun getGamma(): Int? {
        return getCurrentCamera()?.let { camera ->
            if (camera !is CameraUVC) {
                return@let null
            }
            camera.getGamma()
        }
    }

    
    protected fun resetGamma() {
        getCurrentCamera()?.let { camera ->
            if (camera !is CameraUVC) {
                return
            }
            camera.resetGamma()
        }
    }

    
    protected fun setHue(hue: Int) {
        getCurrentCamera()?.let { camera ->
            if (camera !is CameraUVC) {
                return
            }
            camera.setHue(hue)
        }
    }

    
    protected fun getHue(): Int? {
        return getCurrentCamera()?.let { camera ->
            if (camera !is CameraUVC) {
                return@let null
            }
            camera.getHue()
        }
    }

    
    protected fun resetHue() {
        getCurrentCamera()?.let { camera ->
            if (camera !is CameraUVC) {
                return
            }
            camera.resetHue()
        }
    }

    
    protected fun setZoom(zoom: Int) {
        getCurrentCamera()?.let { camera ->
            if (camera !is CameraUVC) {
                return
            }
            camera.setZoom(zoom)
        }
    }

    
    protected fun getZoom(): Int? {
        return getCurrentCamera()?.let { camera ->
            if (camera !is CameraUVC) {
                return@let null
            }
            camera.getZoom()
        }
    }

    
    protected fun resetZoom() {
        getCurrentCamera()?.let { camera ->
            if (camera !is CameraUVC) {
                return
            }
            camera.resetZoom()
        }
    }

    
    protected fun setSharpness(sharpness: Int) {
        getCurrentCamera()?.let { camera ->
            if (camera !is CameraUVC) {
                return
            }
            camera.setSharpness(sharpness)
        }
    }

    
    protected fun getSharpness(): Int? {
        return getCurrentCamera()?.let { camera ->
            if (camera !is CameraUVC) {
                return@let null
            }
            camera.getSharpness()
        }
    }

    
    protected fun resetSharpness() {
        getCurrentCamera()?.let { camera ->
            if (camera !is CameraUVC) {
                return
            }
            camera.resetSharpness()
        }
    }

    
    protected fun setSaturation(saturation: Int) {
        getCurrentCamera()?.let { camera ->
            if (camera !is CameraUVC) {
                return
            }
            camera.setSaturation(saturation)
        }
    }

    
    protected fun getSaturation(): Int? {
        return getCurrentCamera()?.let { camera ->
            if (camera !is CameraUVC) {
                return@let null
            }
            camera.getSaturation()
        }
    }

    
    protected fun resetSaturation() {
        getCurrentCamera()?.let { camera ->
            if (camera !is CameraUVC) {
                return
            }
            camera.resetSaturation()
        }
    }

    protected fun openCamera(st: IAspectRatio? = null) {
        when (st) {
            is TextureView, is SurfaceView -> {
                st
            }
            else -> {
                null
            }
        }.apply {
            getCurrentCamera()?.openCamera(this, getCameraRequest())
            getCurrentCamera()?.setCameraStateCallBack(this@CameraActivity)
        }
    }

    protected fun closeCamera() {
        getCurrentCamera()?.closeCamera()
    }

    private fun surfaceSizeChanged(surfaceWidth: Int, surfaceHeight: Int) {
        getCurrentCamera()?.setRenderSize(surfaceWidth, surfaceHeight)
    }

    private fun getViewLayoutParams(viewGroup: ViewGroup): ViewGroup.LayoutParams {
        return when(viewGroup) {
            is FrameLayout -> {
                FrameLayout.LayoutParams(
                    FrameLayout.LayoutParams.MATCH_PARENT,
                    FrameLayout.LayoutParams.MATCH_PARENT,
                    getGravity()
                )
            }
            is LinearLayout -> {
                LinearLayout.LayoutParams(
                    LinearLayout.LayoutParams.MATCH_PARENT,
                    LinearLayout.LayoutParams.MATCH_PARENT
                ).apply {
                    gravity = getGravity()
                }
            }
            is RelativeLayout -> {
                RelativeLayout.LayoutParams(
                    RelativeLayout.LayoutParams.MATCH_PARENT,
                    RelativeLayout.LayoutParams.MATCH_PARENT
                ).apply{
                    when(getGravity()) {
                        Gravity.TOP -> {
                            addRule(RelativeLayout.ALIGN_PARENT_TOP, RelativeLayout.TRUE)
                        }
                        Gravity.BOTTOM -> {
                            addRule(RelativeLayout.ALIGN_PARENT_BOTTOM, RelativeLayout.TRUE)
                        }
                        else -> {
                            addRule(RelativeLayout.CENTER_HORIZONTAL, RelativeLayout.TRUE)
                            addRule(RelativeLayout.CENTER_VERTICAL, RelativeLayout.TRUE)
                        }
                    }
                }
            }
            else -> throw IllegalArgumentException("Unsupported container view, " +
                    "you can use FrameLayout or LinearLayout or RelativeLayout")
        }
    }

    
    protected abstract fun getCameraView(): IAspectRatio?

    
    protected abstract fun getCameraViewContainer(): ViewGroup?

    
    protected open fun getGravity() = Gravity.CENTER

    protected open fun getCameraRequest(): CameraRequest {
        return CameraRequest.Builder()
            .setPreviewWidth(640)
            .setPreviewHeight(480)
            .setRenderMode(CameraRequest.RenderMode.OPENGL)
            .setDefaultRotateType(RotateType.ANGLE_0)
            .setAudioSource(CameraRequest.AudioSource.SOURCE_SYS_MIC)
            .setPreviewFormat(CameraRequest.PreviewFormat.FORMAT_MJPEG)
            .setAspectRatioShow(true)
            .setCaptureRawImage(false)
            .setRawPreviewData(false)
            .create()
    }

    companion object {
        private const val TAG = "CameraFragment"
    }
}