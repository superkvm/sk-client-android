
package com.superkvm.ausbc.camera.bean


@kotlin.Deprecated("Deprecated since version 3.3.0")
class CameraUvcInfo(override val cameraId: String) : CameraInfo(cameraId) {
    var cameraName: String = ""
    var cameraProductName: String? = null
    var cameraManufacturerName: String? = null
    var cameraProtocol: Int = 0
    var cameraClass: Int = 0
    var cameraSubClass: Int = 0

    override fun toString(): String {
        return "CameraUvcInfo(cameraId='$cameraId', " +
                "cameraName='$cameraName', " +
                "cameraProductName='$cameraProductName', " +
                "cameraManufacturerName='$cameraManufacturerName', " +
                "cameraProtocol=$cameraProtocol, " +
                "cameraClass=$cameraClass, " +
                "cameraSubClass=$cameraSubClass, " +
                "cameraVid=$cameraVid, " +
                "cameraPid=$cameraPid, " +
                "cameraPreviewSizes=$cameraPreviewSizes)"

    }
}
