
package com.superkvm.ausbc.pusher
import android.content.Context
import com.superkvm.ausbc.pusher.aliyun.AliyunPusher
import com.superkvm.ausbc.pusher.callback.IStateCallback
import com.superkvm.ausbc.pusher.config.AusbcConfig


object AusbcPusher {
    private var mPusher: IPusher? =  null

    
    fun init(context: Context?, ausbcConfig: AusbcConfig, callback: IStateCallback?) {
        mPusher = ausbcConfig.getPusher() ?:  AliyunPusher()
        mPusher?. init(context, ausbcConfig, callback)
    }

    
    fun start(url: String?) {
        if (isPushing()) {
            mPusher?. stop()
        }
        mPusher?. start(url)
    }

    
    fun stop() {
        mPusher?. stop()
    }

    
    fun pause() {
        mPusher?. pause()
    }

    
    fun resume() {
        mPusher?. resume()
    }

    
    fun reconnect() {
        mPusher?. reconnect()
    }

    
    fun reconnectUrl(url: String?) {
        mPusher?. reconnectUrl(url)
    }

    
    fun destroy() {
        mPusher?. destroy()
    }

    
    fun pushStream(type: Int, data: ByteArray?, size: Int, pts: Long) {
        mPusher?. pushStream(type, data, size, pts)
    }

    
    fun isPushing(): Boolean {
        return mPusher?. isPushing() == true
    }
}