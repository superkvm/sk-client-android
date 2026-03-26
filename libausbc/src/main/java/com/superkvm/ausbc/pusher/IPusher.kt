
package com.superkvm.ausbc.pusher
import android.content.Context
import com.superkvm.ausbc.pusher.callback.IStateCallback
import com.superkvm.ausbc.pusher.config.AusbcConfig


interface IPusher {
    
    
    fun init(context: Context?, ausbcConfig: AusbcConfig?, callback: IStateCallback?)
    
    
    fun start(url: String?)
    
    
    fun stop()

    
    fun pause()

    
    fun resume()

    
    fun reconnect()

    
    fun reconnectUrl(url: String?)

    
    fun pushStream(type: Int, data: ByteArray?, size: Int, pts: Long)

    
    fun destroy()

    
    fun isPushing(): Boolean
}