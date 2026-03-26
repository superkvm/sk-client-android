
package com.superkvm.ausbc.render.internal

import android.content.Context
import android.view.Surface
import com.superkvm.ausbc.R
import com.superkvm.ausbc.render.env.EGLEvn

class ScreenRender(context: Context) : AbstractRender(context) {
    private var mEgl: EGLEvn? = null

    fun initEGLEvn() {
        mEgl = EGLEvn()
        mEgl?.initEgl()
    }

    fun setupSurface(surface: Surface?, surfaceWidth: Int = 0, surfaceHeight: Int = 0) {
        mEgl?.setupSurface(surface, surfaceWidth, surfaceHeight)
        mEgl?.eglMakeCurrent()
        
        mEgl?.eglSwapInterval(0)
    }

    fun swapBuffers(timeStamp: Long) {
        mEgl?.setPresentationTime(timeStamp)
        mEgl?.swapBuffers()
    }

    fun getCurrentContext() = mEgl?.getEGLContext()

    override fun clear() {
        mEgl?.releaseElg()
        mEgl = null
    }

    override fun getVertexSourceId(): Int = R.raw.base_vertex

    override fun getFragmentSourceId(): Int = R.raw.base_fragment
}