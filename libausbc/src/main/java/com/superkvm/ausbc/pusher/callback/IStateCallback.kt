
package com.superkvm.ausbc.pusher.callback


interface IStateCallback {
    fun onPushState(code: Int, msg: String?)
}