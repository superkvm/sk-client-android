

#include "cdjpeg.h"             
#include "wrppm.h"

#ifdef PPM_SUPPORTED




#if BITS_IN_JSAMPLE == 8
#define PUTPPMSAMPLE(ptr,v)  *ptr++ = (char) (v)
#define BYTESPERSAMPLE 1
#define PPM_MAXVAL 255
#else
#ifdef PPM_NORAWWORD
#define PUTPPMSAMPLE(ptr,v)  *ptr++ = (char) ((v) >> (BITS_IN_JSAMPLE-8))
#define BYTESPERSAMPLE 1
#define PPM_MAXVAL 255
#else

#define PUTPPMSAMPLE(ptr,v)                     \
        { register int val_ = v;                \
          *ptr++ = (char) ((val_ >> 8) & 0xFF); \
          *ptr++ = (char) (val_ & 0xFF);        \
        }
#define BYTESPERSAMPLE 2
#define PPM_MAXVAL ((1<<BITS_IN_JSAMPLE)-1)
#endif
#endif







METHODDEF(void)
put_pixel_rows (j_decompress_ptr cinfo, djpeg_dest_ptr dinfo,
                JDIMENSION rows_supplied)
{
  ppm_dest_ptr dest = (ppm_dest_ptr) dinfo;

  (void) JFWRITE(dest->pub.output_file, dest->iobuffer, dest->buffer_width);
}




METHODDEF(void)
copy_pixel_rows (j_decompress_ptr cinfo, djpeg_dest_ptr dinfo,
                 JDIMENSION rows_supplied)
{
  ppm_dest_ptr dest = (ppm_dest_ptr) dinfo;
  register char *bufferptr;
  register JSAMPROW ptr;
  register JDIMENSION col;

  ptr = dest->pub.buffer[0];
  bufferptr = dest->iobuffer;
  for (col = dest->samples_per_row; col > 0; col--) {
    PUTPPMSAMPLE(bufferptr, GETJSAMPLE(*ptr++));
  }
  (void) JFWRITE(dest->pub.output_file, dest->iobuffer, dest->buffer_width);
}




METHODDEF(void)
put_demapped_rgb (j_decompress_ptr cinfo, djpeg_dest_ptr dinfo,
                  JDIMENSION rows_supplied)
{
  ppm_dest_ptr dest = (ppm_dest_ptr) dinfo;
  register char *bufferptr;
  register int pixval;
  register JSAMPROW ptr;
  register JSAMPROW color_map0 = cinfo->colormap[0];
  register JSAMPROW color_map1 = cinfo->colormap[1];
  register JSAMPROW color_map2 = cinfo->colormap[2];
  register JDIMENSION col;

  ptr = dest->pub.buffer[0];
  bufferptr = dest->iobuffer;
  for (col = cinfo->output_width; col > 0; col--) {
    pixval = GETJSAMPLE(*ptr++);
    PUTPPMSAMPLE(bufferptr, GETJSAMPLE(color_map0[pixval]));
    PUTPPMSAMPLE(bufferptr, GETJSAMPLE(color_map1[pixval]));
    PUTPPMSAMPLE(bufferptr, GETJSAMPLE(color_map2[pixval]));
  }
  (void) JFWRITE(dest->pub.output_file, dest->iobuffer, dest->buffer_width);
}


METHODDEF(void)
put_demapped_gray (j_decompress_ptr cinfo, djpeg_dest_ptr dinfo,
                   JDIMENSION rows_supplied)
{
  ppm_dest_ptr dest = (ppm_dest_ptr) dinfo;
  register char *bufferptr;
  register JSAMPROW ptr;
  register JSAMPROW color_map = cinfo->colormap[0];
  register JDIMENSION col;

  ptr = dest->pub.buffer[0];
  bufferptr = dest->iobuffer;
  for (col = cinfo->output_width; col > 0; col--) {
    PUTPPMSAMPLE(bufferptr, GETJSAMPLE(color_map[GETJSAMPLE(*ptr++)]));
  }
  (void) JFWRITE(dest->pub.output_file, dest->iobuffer, dest->buffer_width);
}




METHODDEF(void)
start_output_ppm (j_decompress_ptr cinfo, djpeg_dest_ptr dinfo)
{
  ppm_dest_ptr dest = (ppm_dest_ptr) dinfo;

  
  switch (cinfo->out_color_space) {
  case JCS_GRAYSCALE:
    
    fprintf(dest->pub.output_file, "P5\n%ld %ld\n%d\n",
            (long) cinfo->output_width, (long) cinfo->output_height,
            PPM_MAXVAL);
    break;
  case JCS_RGB:
    
    fprintf(dest->pub.output_file, "P6\n%ld %ld\n%d\n",
            (long) cinfo->output_width, (long) cinfo->output_height,
            PPM_MAXVAL);
    break;
  default:
    ERREXIT(cinfo, JERR_PPM_COLORSPACE);
  }
}




METHODDEF(void)
finish_output_ppm (j_decompress_ptr cinfo, djpeg_dest_ptr dinfo)
{
  
  fflush(dinfo->output_file);
  if (ferror(dinfo->output_file))
    ERREXIT(cinfo, JERR_FILE_WRITE);
}




GLOBAL(djpeg_dest_ptr)
jinit_write_ppm (j_decompress_ptr cinfo)
{
  ppm_dest_ptr dest;

  
  dest = (ppm_dest_ptr)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
                                  sizeof(ppm_dest_struct));
  dest->pub.start_output = start_output_ppm;
  dest->pub.finish_output = finish_output_ppm;

  
  jpeg_calc_output_dimensions(cinfo);

  
  dest->samples_per_row = cinfo->output_width * cinfo->out_color_components;
  dest->buffer_width = dest->samples_per_row * (BYTESPERSAMPLE * sizeof(char));
  dest->iobuffer = (char *) (*cinfo->mem->alloc_small)
    ((j_common_ptr) cinfo, JPOOL_IMAGE, dest->buffer_width);

  if (cinfo->quantize_colors || BITS_IN_JSAMPLE != 8 ||
      sizeof(JSAMPLE) != sizeof(char)) {
    
    dest->pub.buffer = (*cinfo->mem->alloc_sarray)
      ((j_common_ptr) cinfo, JPOOL_IMAGE,
       cinfo->output_width * cinfo->output_components, (JDIMENSION) 1);
    dest->pub.buffer_height = 1;
    if (! cinfo->quantize_colors)
      dest->pub.put_pixel_rows = copy_pixel_rows;
    else if (cinfo->out_color_space == JCS_GRAYSCALE)
      dest->pub.put_pixel_rows = put_demapped_gray;
    else
      dest->pub.put_pixel_rows = put_demapped_rgb;
  } else {
    
    
    dest->pixrow = (JSAMPROW) dest->iobuffer;
    dest->pub.buffer = & dest->pixrow;
    dest->pub.buffer_height = 1;
    dest->pub.put_pixel_rows = put_pixel_rows;
  }

  return (djpeg_dest_ptr) dest;
}

#endif 
