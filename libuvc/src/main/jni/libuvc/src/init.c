




#include "libuvc/libuvc.h"
#include "libuvc/libuvc_internal.h"
#if defined(__ANDROID__)
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#endif	


void *_uvc_handle_events(void *arg) {
	uvc_context_t *ctx = (uvc_context_t *) arg;

#if defined(__ANDROID__)
	
	int prio = getpriority(PRIO_PROCESS, 0);
	nice(-18);
	if (UNLIKELY(getpriority(PRIO_PROCESS, 0) >= prio)) {
		LOGW("could not change thread priority");
	}
#endif
	for (; !ctx->kill_handler_thread ;)
		libusb_handle_events(ctx->usb_ctx);
	return NULL;
}


uvc_error_t uvc_init2(uvc_context_t **pctx, struct libusb_context *usb_ctx, const char *usbfs) {
	uvc_error_t ret = UVC_SUCCESS;
	uvc_context_t *ctx = calloc(1, sizeof(*ctx));

	if (usb_ctx == NULL) {
		if (usbfs && strlen(usbfs) > 0) {
			LOGD("call #libusb_init2");
			ret = libusb_init2(&ctx->usb_ctx, usbfs);
		} else {
			LOGD("call #libusb_init");
			ret = libusb_init(&ctx->usb_ctx);
		}
		ctx->own_usb_ctx = 1;
		if (UNLIKELY(ret != UVC_SUCCESS)) {
			LOGW("failed:err=%d", ret);
			free(ctx);
			ctx = NULL;
		}
	} else {
		ctx->own_usb_ctx = 0;
		ctx->usb_ctx = usb_ctx;
	}

	if (ctx != NULL)
		*pctx = ctx;

	return ret;
}

uvc_error_t uvc_init(uvc_context_t **pctx, struct libusb_context *usb_ctx) {
	return uvc_init2(pctx, usb_ctx, NULL);
#if 0
	uvc_error_t ret = UVC_SUCCESS;
	uvc_context_t *ctx = calloc(1, sizeof(*ctx));

	if (usb_ctx == NULL) {
		ret = libusb_init(&ctx->usb_ctx);
		ctx->own_usb_ctx = 1;
		if (UNLIKELY(ret != UVC_SUCCESS)) {
			free(ctx);
			ctx = NULL;
		}
	} else {
		ctx->own_usb_ctx = 0;
		ctx->usb_ctx = usb_ctx;
	}

	if (ctx != NULL)
		*pctx = ctx;

	return ret;
#endif
}


void uvc_exit(uvc_context_t *ctx) {
	uvc_device_handle_t *devh;

	DL_FOREACH(ctx->open_devices, devh)
	{
		uvc_close(devh);
	}

	if (ctx->own_usb_ctx)
		libusb_exit(ctx->usb_ctx);

	free(ctx);
}


void uvc_start_handler_thread(uvc_context_t *ctx) {
	if (ctx->own_usb_ctx) {
		pthread_create(&ctx->handler_thread, NULL, _uvc_handle_events, (void*) ctx);
	}
}

