
package com.superkvm.ausbc.pusher.aliyun

import android.content.Context
import com.superkvm.ausbc.pusher.IPusher
import com.superkvm.ausbc.pusher.callback.IStateCallback
import com.superkvm.ausbc.pusher.config.AusbcConfig


class AliyunPusher: IPusher {
    override fun init(context: Context?, ausbcConfig: AusbcConfig?, callback: IStateCallback?) {
        TODO("Not yet implemented")
    }

    override fun start(url: String?) {
        TODO("Not yet implemented")
    }

    override fun stop() {
        TODO("Not yet implemented")
    }

    override fun pause() {
        TODO("Not yet implemented")
    }

    override fun resume() {
        TODO("Not yet implemented")
    }

    override fun reconnect() {
        TODO("Not yet implemented")
    }

    override fun reconnectUrl(url: String?) {
        TODO("Not yet implemented")
    }

    override fun pushStream(type: Int, data: ByteArray?, size: Int, pts: Long) {
        TODO("Not yet implemented")
    }

    override fun destroy() {
        TODO("Not yet implemented")
    }

    override fun isPushing(): Boolean {
        TODO("Not yet implemented")
    }
}