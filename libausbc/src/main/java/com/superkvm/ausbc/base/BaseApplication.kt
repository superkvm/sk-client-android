
package com.superkvm.ausbc.base

import android.app.Application
import com.superkvm.ausbc.utils.ToastUtils


open class BaseApplication: Application() {

    override fun onCreate() {
        super.onCreate()
        ToastUtils.init(this)
    }
}