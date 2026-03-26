

#define JPEG_CJPEG_DJPEG        
#define JPEG_INTERNAL_OPTIONS   
#include "jinclude.h"
#include "jpeglib.h"
#include "jerror.h"             
#include "cderror.h"            




typedef struct cjpeg_source_struct *cjpeg_source_ptr;

struct cjpeg_source_struct {
  void (*start_input) (j_compress_ptr cinfo, cjpeg_source_ptr sinfo);
  JDIMENSION (*get_pixel_rows) (j_compress_ptr cinfo, cjpeg_source_ptr sinfo);
  void (*finish_input) (j_compress_ptr cinfo, cjpeg_source_ptr sinfo);

  FILE *input_file;

  JSAMPARRAY buffer;
  JDIMENSION buffer_height;
};




typedef struct djpeg_dest_struct *djpeg_dest_ptr;

struct djpeg_dest_struct {
  
  void (*start_output) (j_decompress_ptr cinfo, djpeg_dest_ptr dinfo);
  
  void (*put_pixel_rows) (j_decompress_ptr cinfo, djpeg_dest_ptr dinfo,
                          JDIMENSION rows_supplied);
  
  void (*finish_output) (j_decompress_ptr cinfo, djpeg_dest_ptr dinfo);

  
  FILE *output_file;

  
  JSAMPARRAY buffer;
  JDIMENSION buffer_height;
};




struct cdjpeg_progress_mgr {
  struct jpeg_progress_mgr pub; 
  int completed_extra_passes;   
  int total_extra_passes;       
  
  int percent_done;
};

typedef struct cdjpeg_progress_mgr *cd_progress_ptr;




EXTERN(cjpeg_source_ptr) jinit_read_bmp (j_compress_ptr cinfo);
EXTERN(djpeg_dest_ptr) jinit_write_bmp (j_decompress_ptr cinfo,
                                        boolean is_os2);
EXTERN(cjpeg_source_ptr) jinit_read_gif (j_compress_ptr cinfo);
EXTERN(djpeg_dest_ptr) jinit_write_gif (j_decompress_ptr cinfo);
EXTERN(cjpeg_source_ptr) jinit_read_ppm (j_compress_ptr cinfo);
EXTERN(djpeg_dest_ptr) jinit_write_ppm (j_decompress_ptr cinfo);
EXTERN(cjpeg_source_ptr) jinit_read_rle (j_compress_ptr cinfo);
EXTERN(djpeg_dest_ptr) jinit_write_rle (j_decompress_ptr cinfo);
EXTERN(cjpeg_source_ptr) jinit_read_targa (j_compress_ptr cinfo);
EXTERN(djpeg_dest_ptr) jinit_write_targa (j_decompress_ptr cinfo);



EXTERN(boolean) read_quant_tables (j_compress_ptr cinfo, char *filename,
                                   boolean force_baseline);
EXTERN(boolean) read_scan_script (j_compress_ptr cinfo, char *filename);
EXTERN(boolean) set_quality_ratings (j_compress_ptr cinfo, char *arg,
                                     boolean force_baseline);
EXTERN(boolean) set_quant_slots (j_compress_ptr cinfo, char *arg);
EXTERN(boolean) set_sample_factors (j_compress_ptr cinfo, char *arg);



EXTERN(void) read_color_map (j_decompress_ptr cinfo, FILE *infile);



EXTERN(void) enable_signal_catcher (j_common_ptr cinfo);
EXTERN(void) start_progress_monitor (j_common_ptr cinfo,
                                     cd_progress_ptr progress);
EXTERN(void) end_progress_monitor (j_common_ptr cinfo);
EXTERN(boolean) keymatch (char *arg, const char *keyword, int minchars);
EXTERN(FILE *) read_stdin (void);
EXTERN(FILE *) write_stdout (void);



#ifdef DONT_USE_B_MODE          
#define READ_BINARY     "r"
#define WRITE_BINARY    "w"
#else
#define READ_BINARY     "rb"
#define WRITE_BINARY    "wb"
#endif

#ifndef EXIT_FAILURE            
#define EXIT_FAILURE  1
#endif
#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS  0
#endif
#ifndef EXIT_WARNING
#define EXIT_WARNING  2
#endif
