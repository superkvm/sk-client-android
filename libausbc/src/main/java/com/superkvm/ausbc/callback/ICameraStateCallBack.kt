
package com.superkvm.ausbc.callback

import com.superkvm.ausbc.MultiCameraClient


interface ICameraStateCallBack {
    fun onCameraState(self: MultiCameraClient.ICamera, code: State, msg: String? = null)

    enum class State {
        OPENED, CLOSED, ERROR
    }
}