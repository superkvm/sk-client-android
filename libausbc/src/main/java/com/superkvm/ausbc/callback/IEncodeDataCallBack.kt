
package com.superkvm.ausbc.callback

import java.nio.ByteBuffer


interface IEncodeDataCallBack {
    fun onEncodeData(type: DataType, buffer:ByteBuffer, offset: Int, size: Int, timestamp: Long)
    enum class DataType {
        AAC,       
                   
        H264_KEY,  
        H264_SPS,  
        H264       
    }
}