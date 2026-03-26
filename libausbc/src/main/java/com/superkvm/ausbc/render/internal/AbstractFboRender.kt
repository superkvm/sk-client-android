
package com.superkvm.ausbc.render.internal

import android.content.Context
import android.opengl.GLES20
import com.superkvm.ausbc.utils.Logger
import com.superkvm.ausbc.utils.OpenGLUtils


abstract class AbstractFboRender(context: Context) : AbstractRender(context) {
    private val mFrameBuffers by lazy {
        IntArray(1)
    }

    
    
    private val mFBOTextures by lazy {
        IntArray(1)
    }

    
    
    fun getFrameBufferId() = mFrameBuffers[0]

    fun getFrameBufferTexture() = mFBOTextures[0]

    override fun drawFrame(textureId: Int): Int {
        GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, mFrameBuffers[0])
        super.drawFrame(textureId)
        GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, 0)
        
        afterDrawFBO()
        return mFBOTextures[0]
    }

    override fun setSize(width: Int, height: Int) {
        super.setSize(width, height)
        loadFBO(width, height)
    }

    protected open fun afterDrawFBO() {}

    private fun loadFBO(width: Int, height: Int) {
        destroyFrameBuffers()
        
        GLES20.glGenFramebuffers(mFrameBuffers.size, mFrameBuffers, 0)
        
        createTexture(mFBOTextures)
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mFBOTextures[0])
        
        GLES20.glTexImage2D(
            GLES20.GL_TEXTURE_2D, 0, GLES20.GL_RGBA, width, height,
            0, GLES20.GL_RGBA, GLES20.GL_UNSIGNED_BYTE, null
        )
        GLES20.glBindFramebuffer(
            GLES20.GL_FRAMEBUFFER,
            mFrameBuffers[0]
        )
        OpenGLUtils.checkGlError("glBindFramebuffer")
        
        GLES20.glFramebufferTexture2D(
            GLES20.GL_FRAMEBUFFER, GLES20.GL_COLOR_ATTACHMENT0, GLES20.GL_TEXTURE_2D,
            mFBOTextures[0], 0
        )
        OpenGLUtils.checkGlError("glFramebufferTexture2D")
        
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, 0)
        GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, 0)
        Logger.i(TAG, "load fbo, textures: $mFBOTextures, buffers: $mFrameBuffers")
    }

    private fun destroyFrameBuffers() {
        GLES20.glDeleteTextures(1, mFBOTextures, 0)
        GLES20.glDeleteFramebuffers(1, mFrameBuffers, 0)
    }

    companion object {
        private const val TAG = "AbstractFboRender"
    }
}