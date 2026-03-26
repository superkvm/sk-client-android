
#include "proxy_yuv.h"

void yuv420spToNv21(JNIEnv *env, jobject instance, jbyteArray data, jint width, jint height) {
    if(! data || width == 0 || height == 0) {
        LOGE("Parameters error in nv21ToYuv420sp");
        return;
    }
    jbyte *srcData = env->GetByteArrayElements(data, JNI_FALSE);
    jsize srcLen = env->GetArrayLength(data);
    char *dest = (char *)malloc(srcLen);
    yuv420spToNv21Internal((char *)srcData,dest, width, height);
    env->SetByteArrayRegion(data,0,srcLen,(jbyte *)dest);
    env->ReleaseByteArrayElements(data, srcData, 0);
    free(dest);
}

void nv21ToYuv420sp(JNIEnv *env, jobject instance, jbyteArray data, jint width, jint height) {
    if(! data || width == 0 || height == 0) {
        LOGE("Parameters error in nv21ToYuv420sp");
        return;
    }
    jbyte *srcData = env->GetByteArrayElements(data, JNI_FALSE);
    jsize srcLen = env->GetArrayLength(data);
    char *dest = (char *)malloc(srcLen);
    nv21ToYuv420spInternal((char *)srcData,dest, width, height);
    env->SetByteArrayRegion(data,0,srcLen,(jbyte *)dest);
    env->ReleaseByteArrayElements(data, srcData, 0);
    free(dest);
}

void nv21ToYuv420spWithMirror(JNIEnv *env, jobject instance, jbyteArray data, jint width, jint height) {
    if(! data || width == 0 || height == 0) {
        LOGE("Parameters error in nv21ToYuv420spWithMirror");
        return;
    }
    jbyte *srcData = env->GetByteArrayElements(data, JNI_FALSE);
    jsize srcLen = env->GetArrayLength(data);
    char *dest = (char *)malloc(srcLen);
    nv21ToYuv420spWithMirrorInternal((char *)srcData,dest, width, height);
    env->SetByteArrayRegion(data,0,srcLen,(jbyte *)dest);
    env->ReleaseByteArrayElements(data, srcData, 0);
    free(dest);
}

void nv21ToYuv420p(JNIEnv *env, jobject instance, jbyteArray data, jint width, jint height) {
    if(! data || width == 0 || height == 0) {
        LOGE("Parameters error in nv21ToYuv420p");
        return;
    }
    jbyte *srcData = env->GetByteArrayElements(data, JNI_FALSE);
    jsize srcLen = env->GetArrayLength(data);
    char *dest = (char *)malloc(srcLen);
    nv21ToYuv420pInternal((char *)srcData,dest, width, height);
    env->SetByteArrayRegion(data,0,srcLen,(jbyte *)dest);
    env->ReleaseByteArrayElements(data, srcData, 0);
    free(dest);
}

void nv21ToYuv420pWithMirror(JNIEnv *env, jobject instance, jbyteArray data, jint width, jint height) {
    if(! data || width == 0 || height == 0) {
        LOGE("Parameters error in nv21ToYuv420pWithMirror");
        return;
    }
    jbyte *srcData = env->GetByteArrayElements(data, JNI_FALSE);
    jsize srcLen = env->GetArrayLength(data);
    char *dest = (char *)malloc(srcLen);
    nv21ToYuv420pWithMirrorInternal((char *)srcData,dest, width, height);
    env->SetByteArrayRegion(data,0,srcLen,(jbyte *)dest);
    env->ReleaseByteArrayElements(data, srcData, 0);
    free(dest);
}

void nativeRotateNV21(JNIEnv *env, jobject instance, jbyteArray j_srcArr, jint width, jint height, jint rotateDegree) {
    if(! j_srcArr || width == 0 || height == 0) {
        LOGE("Parameters error in nv21ToYuv420pWithMirror");
        return;
    }
    auto * c_srcArr = (jbyte*) env->GetByteArrayElements(j_srcArr, JNI_FALSE);
    jsize srcLen = env->GetArrayLength(j_srcArr);
    jint wh = width * height;
    jint frameSize =wh * 3 / 2;
    int yLength = wh;
    int uLength = yLength / 4;
    
    char *c_tmp = (char *)malloc(srcLen);

    int k = 0,i=0,j=0;
    if(rotateDegree == 90){
        
        for (i = 0; i < width; i++) {
            for (j = height - 1; j >= 0; j--) {
                c_tmp[k] = c_srcArr[width * j + i];
                k++;
            }
        }
        
        for (i = 0; i < width; i += 2) {
            for (j = height / 2 - 1; j >= 0; j--) {
                c_tmp[k] = c_srcArr[wh + width * j + i];
                c_tmp[k + 1] = c_srcArr[wh + width * j + i + 1];
                k += 2;
            }
        }
    }else if(rotateDegree == 180){
        
        for (i = wh - 1; i >= 0; i--) {
            c_tmp[k] = c_srcArr[i];
            k++;
        }
        
        for (j = wh * 3 / 2 - 1; j >= wh; j -= 2) {
            c_tmp[k] = c_srcArr[j - 1];
            c_tmp[k + 1] = c_srcArr[j];
            k += 2;
        }
    }else if(rotateDegree == 270){
        
        for(i=width-1 ; i>=0 ; i--){
            for(j=height-1 ; j>=0 ; j--){
                c_tmp[k] = c_srcArr[width*j + i];
                k++;
            }
        }
        
        for(i=width-1 ; i>=0 ; i-=2){
            for(j=height/2-1 ; j>=0 ; j--){
                c_tmp[k] = c_srcArr[wh + width*j + i-1];
                c_tmp[k+1] = c_srcArr[wh + width*j + i];
                k +=2;
            }
        }
    }

    
    env->SetByteArrayRegion(j_srcArr,0,srcLen,(jbyte *)c_tmp);
    
    env->ReleaseByteArrayElements(j_srcArr, c_srcArr, JNI_FALSE);
    
    free(c_tmp);
}
