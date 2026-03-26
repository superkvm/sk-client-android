

#include "cdjpeg.h"             

#ifdef RLE_SUPPORTED



#include <rle.h>



#if BITS_IN_JSAMPLE != 8
  Sorry, this code only copes with 8-bit JSAMPLEs. 
#endif



typedef enum
  { GRAYSCALE, MAPPEDGRAY, PSEUDOCOLOR, TRUECOLOR, DIRECTCOLOR } rle_kind;




typedef struct _rle_source_struct *rle_source_ptr;

typedef struct _rle_source_struct {
  struct cjpeg_source_struct pub; 

  rle_kind visual;              
  jvirt_sarray_ptr image;       
  JDIMENSION row;               
  rle_hdr header;               
  rle_pixel **rle_row;          

} rle_source_struct;




METHODDEF(void)
start_input_rle (j_compress_ptr cinfo, cjpeg_source_ptr sinfo)
{
  rle_source_ptr source = (rle_source_ptr) sinfo;
  JDIMENSION width, height;
#ifdef PROGRESS_REPORT
  cd_progress_ptr progress = (cd_progress_ptr) cinfo->progress;
#endif

  
  source->header = *rle_hdr_init(NULL);
  source->header.rle_file = source->pub.input_file;
  switch (rle_get_setup(&(source->header))) {
  case RLE_SUCCESS:
    
    break;
  case RLE_NOT_RLE:
    ERREXIT(cinfo, JERR_RLE_NOT);
    break;
  case RLE_NO_SPACE:
    ERREXIT(cinfo, JERR_RLE_MEM);
    break;
  case RLE_EMPTY:
    ERREXIT(cinfo, JERR_RLE_EMPTY);
    break;
  case RLE_EOF:
    ERREXIT(cinfo, JERR_RLE_EOF);
    break;
  default:
    ERREXIT(cinfo, JERR_RLE_BADERROR);
    break;
  }

  

  width  = source->header.xmax - source->header.xmin + 1;
  height = source->header.ymax - source->header.ymin + 1;
  source->header.xmin = 0;              
  source->header.xmax = width-1;

  cinfo->image_width      = width;
  cinfo->image_height     = height;
  cinfo->data_precision   = 8;  

  if (source->header.ncolors == 1 && source->header.ncmap == 0) {
    source->visual     = GRAYSCALE;
    TRACEMS2(cinfo, 1, JTRC_RLE_GRAY, width, height);
  } else if (source->header.ncolors == 1 && source->header.ncmap == 1) {
    source->visual     = MAPPEDGRAY;
    TRACEMS3(cinfo, 1, JTRC_RLE_MAPGRAY, width, height,
             1 << source->header.cmaplen);
  } else if (source->header.ncolors == 1 && source->header.ncmap == 3) {
    source->visual     = PSEUDOCOLOR;
    TRACEMS3(cinfo, 1, JTRC_RLE_MAPPED, width, height,
             1 << source->header.cmaplen);
  } else if (source->header.ncolors == 3 && source->header.ncmap == 3) {
    source->visual     = TRUECOLOR;
    TRACEMS3(cinfo, 1, JTRC_RLE_FULLMAP, width, height,
             1 << source->header.cmaplen);
  } else if (source->header.ncolors == 3 && source->header.ncmap == 0) {
    source->visual     = DIRECTCOLOR;
    TRACEMS2(cinfo, 1, JTRC_RLE, width, height);
  } else
    ERREXIT(cinfo, JERR_RLE_UNSUPPORTED);

  if (source->visual == GRAYSCALE || source->visual == MAPPEDGRAY) {
    cinfo->in_color_space   = JCS_GRAYSCALE;
    cinfo->input_components = 1;
  } else {
    cinfo->in_color_space   = JCS_RGB;
    cinfo->input_components = 3;
  }

  
  if (source->visual != GRAYSCALE) {
    source->rle_row = (rle_pixel**) (*cinfo->mem->alloc_sarray)
      ((j_common_ptr) cinfo, JPOOL_IMAGE,
       (JDIMENSION) width, (JDIMENSION) cinfo->input_components);
  }

  
  source->image = (*cinfo->mem->request_virt_sarray)
    ((j_common_ptr) cinfo, JPOOL_IMAGE, FALSE,
     (JDIMENSION) (width * source->header.ncolors),
     (JDIMENSION) height, (JDIMENSION) 1);

#ifdef PROGRESS_REPORT
  if (progress != NULL) {
    
    progress->total_extra_passes++;
  }
#endif

  source->pub.buffer_height = 1;
}




METHODDEF(JDIMENSION)
get_rle_row (j_compress_ptr cinfo, cjpeg_source_ptr sinfo)
{
  rle_source_ptr source = (rle_source_ptr) sinfo;

  source->row--;
  source->pub.buffer = (*cinfo->mem->access_virt_sarray)
    ((j_common_ptr) cinfo, source->image, source->row, (JDIMENSION) 1, FALSE);

  return 1;
}



METHODDEF(JDIMENSION)
get_pseudocolor_row (j_compress_ptr cinfo, cjpeg_source_ptr sinfo)
{
  rle_source_ptr source = (rle_source_ptr) sinfo;
  JSAMPROW src_row, dest_row;
  JDIMENSION col;
  rle_map *colormap;
  int val;

  colormap = source->header.cmap;
  dest_row = source->pub.buffer[0];
  source->row--;
  src_row = *(*cinfo->mem->access_virt_sarray)
    ((j_common_ptr) cinfo, source->image, source->row, (JDIMENSION) 1, FALSE);

  for (col = cinfo->image_width; col > 0; col--) {
    val = GETJSAMPLE(*src_row++);
    *dest_row++ = (JSAMPLE) (colormap[val      ] >> 8);
    *dest_row++ = (JSAMPLE) (colormap[val + 256] >> 8);
    *dest_row++ = (JSAMPLE) (colormap[val + 512] >> 8);
  }

  return 1;
}




METHODDEF(JDIMENSION)
load_image (j_compress_ptr cinfo, cjpeg_source_ptr sinfo)
{
  rle_source_ptr source = (rle_source_ptr) sinfo;
  JDIMENSION row, col;
  JSAMPROW  scanline, red_ptr, green_ptr, blue_ptr;
  rle_pixel **rle_row;
  rle_map *colormap;
  char channel;
#ifdef PROGRESS_REPORT
  cd_progress_ptr progress = (cd_progress_ptr) cinfo->progress;
#endif

  colormap = source->header.cmap;
  rle_row = source->rle_row;

  
  RLE_CLR_BIT(source->header, RLE_ALPHA); 

#ifdef PROGRESS_REPORT
  if (progress != NULL) {
    progress->pub.pass_limit = cinfo->image_height;
    progress->pub.pass_counter = 0;
    (*progress->pub.progress_monitor) ((j_common_ptr) cinfo);
  }
#endif

  switch (source->visual) {

  case GRAYSCALE:
  case PSEUDOCOLOR:
    for (row = 0; row < cinfo->image_height; row++) {
      rle_row = (rle_pixel **) (*cinfo->mem->access_virt_sarray)
         ((j_common_ptr) cinfo, source->image, row, (JDIMENSION) 1, TRUE);
      rle_getrow(&source->header, rle_row);
#ifdef PROGRESS_REPORT
      if (progress != NULL) {
        progress->pub.pass_counter++;
        (*progress->pub.progress_monitor) ((j_common_ptr) cinfo);
      }
#endif
    }
    break;

  case MAPPEDGRAY:
  case TRUECOLOR:
    for (row = 0; row < cinfo->image_height; row++) {
      scanline = *(*cinfo->mem->access_virt_sarray)
        ((j_common_ptr) cinfo, source->image, row, (JDIMENSION) 1, TRUE);
      rle_row = source->rle_row;
      rle_getrow(&source->header, rle_row);

      for (col = 0; col < cinfo->image_width; col++) {
        for (channel = 0; channel < source->header.ncolors; channel++) {
          *scanline++ = (JSAMPLE)
            (colormap[GETJSAMPLE(rle_row[channel][col]) + 256 * channel] >> 8);
        }
      }

#ifdef PROGRESS_REPORT
      if (progress != NULL) {
        progress->pub.pass_counter++;
        (*progress->pub.progress_monitor) ((j_common_ptr) cinfo);
      }
#endif
    }
    break;

  case DIRECTCOLOR:
    for (row = 0; row < cinfo->image_height; row++) {
      scanline = *(*cinfo->mem->access_virt_sarray)
        ((j_common_ptr) cinfo, source->image, row, (JDIMENSION) 1, TRUE);
      rle_getrow(&source->header, rle_row);

      red_ptr   = rle_row[0];
      green_ptr = rle_row[1];
      blue_ptr  = rle_row[2];

      for (col = cinfo->image_width; col > 0; col--) {
        *scanline++ = *red_ptr++;
        *scanline++ = *green_ptr++;
        *scanline++ = *blue_ptr++;
      }

#ifdef PROGRESS_REPORT
      if (progress != NULL) {
        progress->pub.pass_counter++;
        (*progress->pub.progress_monitor) ((j_common_ptr) cinfo);
      }
#endif
    }
  }

#ifdef PROGRESS_REPORT
  if (progress != NULL)
    progress->completed_extra_passes++;
#endif

  
  if (source->visual == PSEUDOCOLOR) {
    source->pub.buffer = source->rle_row;
    source->pub.get_pixel_rows = get_pseudocolor_row;
  } else {
    source->pub.get_pixel_rows = get_rle_row;
  }
  source->row = cinfo->image_height;

  
  return (*source->pub.get_pixel_rows) (cinfo, sinfo);
}




METHODDEF(void)
finish_input_rle (j_compress_ptr cinfo, cjpeg_source_ptr sinfo)
{
  
}




GLOBAL(cjpeg_source_ptr)
jinit_read_rle (j_compress_ptr cinfo)
{
  rle_source_ptr source;

  
  source = (rle_source_ptr)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
                                  sizeof(rle_source_struct));
  
  source->pub.start_input = start_input_rle;
  source->pub.finish_input = finish_input_rle;
  source->pub.get_pixel_rows = load_image;

  return (cjpeg_source_ptr) source;
}

#endif 
