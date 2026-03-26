
package com.superkvm.kvm

import android.content.Context
import android.util.Log
import androidx.multidex.MultiDex
import com.superkvm.ausbc.base.BaseApplication


class MainApplication: BaseApplication() {

    override fun attachBaseContext(base: Context?) {
        super.attachBaseContext(base)
        MultiDex.install(this)
    }

    override fun onCreate() {
        super.onCreate()
        val marker = "superkvm BUILD_TYPE_MARK app=${BuildConfig.APPLICATION_ID}, type=${BuildConfig.BUILD_TYPE}"
        Log.e("superkvm", marker)
        System.err.println(marker)
    }
}
