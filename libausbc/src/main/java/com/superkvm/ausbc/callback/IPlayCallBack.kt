
package com.superkvm.ausbc.callback


interface IPlayCallBack {
    fun onBegin()
    fun onError(error: String)
    fun onComplete()
}