
package com.superkvm.ausbc.camera.bean

import androidx.annotation.Keep
import com.superkvm.ausbc.render.effect.AbstractEffect
import com.superkvm.ausbc.render.env.RotateType



@Keep
class CameraRequest private constructor() {
    var previewWidth: Int = DEFAULT_WIDTH
    var previewHeight: Int = DEFAULT_HEIGHT
    var renderMode: RenderMode = RenderMode.OPENGL
    var isAspectRatioShow: Boolean = true
    var isRawPreviewData: Boolean = false
    var isCaptureRawImage: Boolean = false
    var defaultEffect: AbstractEffect? = null
    var defaultRotateType: RotateType = RotateType.ANGLE_0
    var audioSource: AudioSource = AudioSource.SOURCE_AUTO
    var previewFormat: PreviewFormat = PreviewFormat.FORMAT_MJPEG

    @kotlin.Deprecated("Deprecated since version 3.3.0")
    var cameraId: String = ""

    @kotlin.Deprecated("Deprecated since version 3.3.0")
    var isFrontCamera: Boolean = false

    
    class Builder {
        private val mRequest by lazy {
            CameraRequest()
        }

        
        @kotlin.Deprecated("Deprecated since version 3.3.0")
        fun setFrontCamera(isFrontCamera: Boolean): Builder {
            mRequest.isFrontCamera = isFrontCamera
            return this
        }

        
        fun setPreviewWidth(width: Int): Builder {
            mRequest.previewWidth = width
            return this
        }

        
        fun setPreviewHeight(height: Int): Builder {
            mRequest.previewHeight = height
            return this
        }

        
        @kotlin.Deprecated("Deprecated since version 3.3.0")
        fun setCameraId(cameraId: String): Builder {
            mRequest.cameraId = cameraId
            return this
        }

        
        fun setRenderMode(renderMode: RenderMode): Builder {
            mRequest.renderMode = renderMode
            return this
        }

        
        fun setAspectRatioShow(isAspectRatioShow: Boolean): Builder {
            mRequest.isAspectRatioShow = isAspectRatioShow
            return this
        }

        
        fun setRawPreviewData(isRawPreviewData: Boolean): Builder {
            mRequest.isRawPreviewData = isRawPreviewData
            return this
        }

        
        fun setCaptureRawImage(isCaptureRawImage: Boolean): Builder {
            mRequest.isCaptureRawImage = isCaptureRawImage
            return this
        }

        
        fun setDefaultEffect(defaultEffect: AbstractEffect): Builder {
            mRequest.defaultEffect = defaultEffect
            return this
        }

        
        fun setDefaultRotateType(defaultRotateType: RotateType): Builder {
            mRequest.defaultRotateType = defaultRotateType
            return this
        }

        
        fun setAudioSource(source: AudioSource): Builder {
            mRequest.audioSource = source
            return this
        }

        
        fun setPreviewFormat(format: PreviewFormat): Builder {
            mRequest.previewFormat = format
            return this
        }

        
        fun create(): CameraRequest {
            return mRequest
        }
    }

    
    enum class RenderMode {
        NORMAL,
        OPENGL
    }

    
    enum class AudioSource {
        NONE,
        SOURCE_SYS_MIC,
        SOURCE_DEV_MIC,
        SOURCE_AUTO
    }

    
    enum class PreviewFormat {
        FORMAT_MJPEG,
        FORMAT_YUYV
    }

    companion object {
        private const val DEFAULT_WIDTH = 640
        private const val DEFAULT_HEIGHT = 480
    }
}