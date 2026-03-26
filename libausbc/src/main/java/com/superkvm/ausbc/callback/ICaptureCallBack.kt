
package com.superkvm.ausbc.callback


interface ICaptureCallBack {
    fun onBegin()
    fun onError(error: String?)
    fun onComplete(path: String?)
}