



#include "libuvc/libuvc.h"
#include "libuvc/libuvc_internal.h"


void *_uvc_handle_events(void *arg) {
  uvc_context_t *ctx = (uvc_context_t *) arg;

  while (!ctx->kill_handler_thread)
    libusb_handle_events(ctx->usb_ctx);
  return NULL;
}


uvc_error_t uvc_init(uvc_context_t **pctx, struct libusb_context *usb_ctx) {
  uvc_error_t ret = UVC_SUCCESS;
  uvc_context_t *ctx = calloc(1, sizeof(*ctx));

  if (usb_ctx == NULL) {
    ret = libusb_init(&ctx->usb_ctx);
    ctx->own_usb_ctx = 1;
    if (ret != UVC_SUCCESS) {
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


void uvc_exit(uvc_context_t *ctx) {
  uvc_device_handle_t *devh;

  DL_FOREACH(ctx->open_devices, devh) {
    uvc_close(devh);
  }

  if (ctx->own_usb_ctx)
    libusb_exit(ctx->usb_ctx);

  free(ctx);
}


void uvc_start_handler_thread(uvc_context_t *ctx) {
  if (ctx->own_usb_ctx)
    pthread_create(&ctx->handler_thread, NULL, _uvc_handle_events, (void*) ctx);
}

