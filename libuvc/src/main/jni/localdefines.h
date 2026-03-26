

#ifndef LOCALDEFINES_H_
#define LOCALDEFINES_H_

#include <jni.h>

#ifndef LOG_TAG
#define LOG_TAG "libUVCCamera"
#endif

#define LIBUVC_HAS_JPEG


#define	ARRAYELEMENTS_COPYBACK_AND_RELEASE 0

#define	ARRAYELEMENTS_COPYBACK_ONLY JNI_COMMIT

#define ARRAYELEMENTS_ABORT_AND_RELEASE JNI_ABORT

#define THREAD_PRIORITY_DEFAULT			0
#define THREAD_PRIORITY_LOWEST			19
#define THREAD_PRIORITY_BACKGROUND		10
#define THREAD_PRIORITY_FOREGROUND		-2
#define THREAD_PRIORITY_DISPLAY			-4
#define THREAD_PRIORITY_URGENT_DISPLAY	-8
#define THREAD_PRIORITY_AUDIO			-16
#define THREAD_PRIORITY_URGENT_AUDIO	-19

#define USE_LOGALL	


#define USE_LOGI
#define USE_LOGW
#define USE_LOGE
#define USE_LOGF

#ifdef NDEBUG
#undef USE_LOGALL
#endif

#ifdef LOG_NDEBUG
#undef USE_LOGALL
#endif



#define		JTYPE_SYSTEM				"Ljava/lang/System;"
#define		JTYPE_UVCCAMERA				"Lcom/serenegiant/usb/UVCCamera;"

typedef		jlong						ID_TYPE;

#endif 
