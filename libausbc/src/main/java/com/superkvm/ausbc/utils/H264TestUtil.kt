
package com.superkvm.ausbc.utils

import android.os.Environment
import java.io.FileOutputStream
import java.io.IOException
import java.lang.Exception


object H264TestUtil {
    private var fos: FileOutputStream? = null

    fun createFile() {
        try {
            
            fos = FileOutputStream(Environment.getExternalStorageDirectory().path + "/file_" + System.currentTimeMillis() + ".264")
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    fun writeFile(data: ByteArray) {
        try {
            fos?.write(data)
            fos?.flush()
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    fun closeFile() {
        try {
            fos?.close()
            fos = null
        } catch (e: IOException) {
            e.printStackTrace()
        }
    }
}