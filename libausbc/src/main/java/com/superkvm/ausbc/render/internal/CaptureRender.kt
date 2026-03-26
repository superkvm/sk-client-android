
package com.superkvm.ausbc.render.internal

import android.content.Context
import android.opengl.GLES20
import android.opengl.Matrix
import com.superkvm.ausbc.R
import kotlin.math.cos
import kotlin.math.sin


class CaptureRender(context: Context) : AbstractFboRender(context) {
    private var mMVPMatrixHandle: Int = -1
    private var mMVPMatrix = FloatArray(16)

    override fun init() {
        Matrix.setIdentityM(mMVPMatrix, 0)
        
        
        
        val radius = (180 * Math.PI / 180.0).toFloat()
        mMVPMatrix[5] *= cos(radius.toDouble()).toFloat()
        mMVPMatrix[6] += (-sin(radius.toDouble())).toFloat()
        mMVPMatrix[9] += sin(radius.toDouble()).toFloat()
        mMVPMatrix[10] *= cos(radius.toDouble()).toFloat()
        
        mMVPMatrixHandle = GLES20.glGetUniformLocation(mProgram, "uMVPMatrix")
    }

    override fun beforeDraw() {
        GLES20.glUniformMatrix4fv(mMVPMatrixHandle, 1, false, mMVPMatrix, 0)
    }

    override fun getVertexSourceId(): Int = R.raw.capture_vertex

    override fun getFragmentSourceId(): Int = R.raw.base_fragment
}