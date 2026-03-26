#include "libuvc/libuvc.h"
#include <stdio.h>


void cb(uvc_frame_t *frame, void *ptr) {
  uvc_frame_t *bgr;
  uvc_error_t ret;

  
  bgr = uvc_allocate_frame(frame->width * frame->height * 3);
  if (!bgr) {
    printf("unable to allocate bgr frame!");
    return;
  }

  
  ret = uvc_any2bgr(frame, bgr);
  if (ret) {
    uvc_perror(ret, "uvc_any2bgr");
    uvc_free_frame(bgr);
    return;
  }

  

  

  

  uvc_free_frame(bgr);
}

int main(int argc, char **argv) {
  uvc_context_t *ctx;
  uvc_device_t *dev;
  uvc_device_handle_t *devh;
  uvc_stream_ctrl_t ctrl;
  uvc_error_t res;

  
  res = uvc_init(&ctx, NULL);

  if (res < 0) {
    uvc_perror(res, "uvc_init");
    return res;
  }

  puts("UVC initialized");

  
  res = uvc_find_device(
      ctx, &dev,
      0, 0, NULL); 

  if (res < 0) {
    uvc_perror(res, "uvc_find_device"); 
  } else {
    puts("Device found");

    
    res = uvc_open(dev, &devh);

    if (res < 0) {
      uvc_perror(res, "uvc_open"); 
    } else {
      puts("Device opened");

      
      uvc_print_diag(devh, stderr);

      
      res = uvc_get_stream_ctrl_format_size(
          devh, &ctrl, 
          UVC_FRAME_FORMAT_YUYV, 
          640, 480, 30 
      );

      
      uvc_print_stream_ctrl(&ctrl, stderr);

      if (res < 0) {
        uvc_perror(res, "get_mode"); 
      } else {
        
        res = uvc_start_iso_streaming(devh, &ctrl, cb, 12345);

        if (res < 0) {
          uvc_perror(res, "start_streaming"); 
        } else {
          puts("Streaming...");

          uvc_set_ae_mode(devh, 1); 

          sleep(10); 

          
          uvc_stop_streaming(devh);
          puts("Done streaming.");
        }
      }

      
      uvc_close(devh);
      puts("Device closed");
    }

    
    uvc_unref_device(dev);
  }

  
  uvc_exit(ctx);
  puts("UVC exited");

  return 0;
}

