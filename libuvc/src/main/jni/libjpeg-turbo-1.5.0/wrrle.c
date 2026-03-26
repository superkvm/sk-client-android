

#include "cdjpeg.h"             

#ifdef RLE_SUPPORTED



#include <rle.h>



#if BITS_IN_JSAMPLE != 8
  Sorry, this code only copes with 8-bit JSAMPLEs. 
#endif







#define CMAPBITS        8
#define CMAPLENGTH      (1<<(CMAPBITS))

typedef struct {
  struct djpeg_dest_struct pub; 

  jvirt_sarray_ptr image;       
  rle_map *colormap;            
  rle_pixel **rle_row;          

} rle_dest_struct;

typedef rle_dest_struct *rle_dest_ptr;


METHODDEF(void) rle_put_pixel_rows
        (j_decompress_ptr cinfo, djpeg_dest_ptr dinfo,
         JDIMENSION rows_supplied);




METHODDEF(void)
start_output_rle (j_decompress_ptr cinfo, djpeg_dest_ptr dinfo)
{
  rle_dest_ptr dest = (rle_dest_ptr) dinfo;
  size_t cmapsize;
  int i, ci;
#ifdef PROGRESS_REPORT
  cd_progress_ptr progress = (cd_progress_ptr) cinfo->progress;
#endif

  

  if (cinfo->output_width > 32767 || cinfo->output_height > 32767)
    ERREXIT2(cinfo, JERR_RLE_DIMENSIONS, cinfo->output_width,
             cinfo->output_height);

  if (cinfo->out_color_space != JCS_GRAYSCALE &&
      cinfo->out_color_space != JCS_RGB)
    ERREXIT(cinfo, JERR_RLE_COLORSPACE);

  if (cinfo->output_components != 1 && cinfo->output_components != 3)
    ERREXIT1(cinfo, JERR_RLE_TOOMANYCHANNELS, cinfo->num_components);

  

  dest->colormap = NULL;

  if (cinfo->quantize_colors) {
    
    cmapsize = cinfo->out_color_components * CMAPLENGTH * sizeof(rle_map);
    dest->colormap = (rle_map *) (*cinfo->mem->alloc_small)
      ((j_common_ptr) cinfo, JPOOL_IMAGE, cmapsize);
    MEMZERO(dest->colormap, cmapsize);

    
    
    for (ci = 0; ci < cinfo->out_color_components; ci++) {
      for (i = 0; i < cinfo->actual_number_of_colors; i++) {
        dest->colormap[ci * CMAPLENGTH + i] =
          GETJSAMPLE(cinfo->colormap[ci][i]) << 8;
      }
    }
  }

  
  dest->pub.buffer = (*cinfo->mem->access_virt_sarray)
    ((j_common_ptr) cinfo, dest->image, (JDIMENSION) 0, (JDIMENSION) 1, TRUE);
  dest->pub.buffer_height = 1;

  dest->pub.put_pixel_rows = rle_put_pixel_rows;

#ifdef PROGRESS_REPORT
  if (progress != NULL) {
    progress->total_extra_passes++;  
  }
#endif
}




METHODDEF(void)
rle_put_pixel_rows (j_decompress_ptr cinfo, djpeg_dest_ptr dinfo,
                    JDIMENSION rows_supplied)
{
  rle_dest_ptr dest = (rle_dest_ptr) dinfo;

  if (cinfo->output_scanline < cinfo->output_height) {
    dest->pub.buffer = (*cinfo->mem->access_virt_sarray)
      ((j_common_ptr) cinfo, dest->image,
       cinfo->output_scanline, (JDIMENSION) 1, TRUE);
  }
}



METHODDEF(void)
finish_output_rle (j_decompress_ptr cinfo, djpeg_dest_ptr dinfo)
{
  rle_dest_ptr dest = (rle_dest_ptr) dinfo;
  rle_hdr header;               
  rle_pixel **rle_row, *red, *green, *blue;
  JSAMPROW output_row;
  char cmapcomment[80];
  int row, col;
  int ci;
#ifdef PROGRESS_REPORT
  cd_progress_ptr progress = (cd_progress_ptr) cinfo->progress;
#endif

  
  header = *rle_hdr_init(NULL);
  header.rle_file = dest->pub.output_file;
  header.xmin     = 0;
  header.xmax     = cinfo->output_width  - 1;
  header.ymin     = 0;
  header.ymax     = cinfo->output_height - 1;
  header.alpha    = 0;
  header.ncolors  = cinfo->output_components;
  for (ci = 0; ci < cinfo->output_components; ci++) {
    RLE_SET_BIT(header, ci);
  }
  if (cinfo->quantize_colors) {
    header.ncmap   = cinfo->out_color_components;
    header.cmaplen = CMAPBITS;
    header.cmap    = dest->colormap;
    
    sprintf(cmapcomment, "color_map_length=%d", cinfo->actual_number_of_colors);
    rle_putcom(cmapcomment, &header);
  }

  
  rle_put_setup(&header);

  

#ifdef PROGRESS_REPORT
  if (progress != NULL) {
    progress->pub.pass_limit = cinfo->output_height;
    progress->pub.pass_counter = 0;
    (*progress->pub.progress_monitor) ((j_common_ptr) cinfo);
  }
#endif

  if (cinfo->output_components == 1) {
    for (row = cinfo->output_height-1; row >= 0; row--) {
      rle_row = (rle_pixel **) (*cinfo->mem->access_virt_sarray)
        ((j_common_ptr) cinfo, dest->image,
         (JDIMENSION) row, (JDIMENSION) 1, FALSE);
      rle_putrow(rle_row, (int) cinfo->output_width, &header);
#ifdef PROGRESS_REPORT
      if (progress != NULL) {
        progress->pub.pass_counter++;
        (*progress->pub.progress_monitor) ((j_common_ptr) cinfo);
      }
#endif
    }
  } else {
    for (row = cinfo->output_height-1; row >= 0; row--) {
      rle_row = (rle_pixel **) dest->rle_row;
      output_row = *(*cinfo->mem->access_virt_sarray)
        ((j_common_ptr) cinfo, dest->image,
         (JDIMENSION) row, (JDIMENSION) 1, FALSE);
      red = rle_row[0];
      green = rle_row[1];
      blue = rle_row[2];
      for (col = cinfo->output_width; col > 0; col--) {
        *red++ = GETJSAMPLE(*output_row++);
        *green++ = GETJSAMPLE(*output_row++);
        *blue++ = GETJSAMPLE(*output_row++);
      }
      rle_putrow(rle_row, (int) cinfo->output_width, &header);
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

  
  rle_puteof(&header);
  fflush(dest->pub.output_file);
  if (ferror(dest->pub.output_file))
    ERREXIT(cinfo, JERR_FILE_WRITE);
}




GLOBAL(djpeg_dest_ptr)
jinit_write_rle (j_decompress_ptr cinfo)
{
  rle_dest_ptr dest;

  
  dest = (rle_dest_ptr)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
                                  sizeof(rle_dest_struct));
  dest->pub.start_output = start_output_rle;
  dest->pub.finish_output = finish_output_rle;

  
  jpeg_calc_output_dimensions(cinfo);

  
  dest->rle_row = (*cinfo->mem->alloc_sarray)
    ((j_common_ptr) cinfo, JPOOL_IMAGE,
     cinfo->output_width, (JDIMENSION) cinfo->output_components);

  
  dest->image = (*cinfo->mem->request_virt_sarray)
    ((j_common_ptr) cinfo, JPOOL_IMAGE, FALSE,
     (JDIMENSION) (cinfo->output_width * cinfo->output_components),
     cinfo->output_height, (JDIMENSION) 1);

  return (djpeg_dest_ptr) dest;
}

#endif 
