

#include "cdjpeg.h"             

#ifdef TARGA_SUPPORTED




#if BITS_IN_JSAMPLE != 8
  Sorry, this code only copes with 8-bit JSAMPLEs. 
#endif




typedef struct {
  struct djpeg_dest_struct pub; 

  char *iobuffer;               
  JDIMENSION buffer_width;      
} tga_dest_struct;

typedef tga_dest_struct *tga_dest_ptr;


LOCAL(void)
write_header (j_decompress_ptr cinfo, djpeg_dest_ptr dinfo, int num_colors)

{
  char targaheader[18];

  
  MEMZERO(targaheader, sizeof(targaheader));

  if (num_colors > 0) {
    targaheader[1] = 1;         
    targaheader[5] = (char) (num_colors & 0xFF);
    targaheader[6] = (char) (num_colors >> 8);
    targaheader[7] = 24;        
  }

  targaheader[12] = (char) (cinfo->output_width & 0xFF);
  targaheader[13] = (char) (cinfo->output_width >> 8);
  targaheader[14] = (char) (cinfo->output_height & 0xFF);
  targaheader[15] = (char) (cinfo->output_height >> 8);
  targaheader[17] = 0x20;       

  if (cinfo->out_color_space == JCS_GRAYSCALE) {
    targaheader[2] = 3;         
    targaheader[16] = 8;        
  } else {                      
    if (num_colors > 0) {
      targaheader[2] = 1;       
      targaheader[16] = 8;
    } else {
      targaheader[2] = 2;       
      targaheader[16] = 24;
    }
  }

  if (JFWRITE(dinfo->output_file, targaheader, 18) != (size_t) 18)
    ERREXIT(cinfo, JERR_FILE_WRITE);
}




METHODDEF(void)
put_pixel_rows (j_decompress_ptr cinfo, djpeg_dest_ptr dinfo,
                JDIMENSION rows_supplied)

{
  tga_dest_ptr dest = (tga_dest_ptr) dinfo;
  register JSAMPROW inptr;
  register char *outptr;
  register JDIMENSION col;

  inptr = dest->pub.buffer[0];
  outptr = dest->iobuffer;
  for (col = cinfo->output_width; col > 0; col--) {
    outptr[0] = (char) GETJSAMPLE(inptr[2]); 
    outptr[1] = (char) GETJSAMPLE(inptr[1]);
    outptr[2] = (char) GETJSAMPLE(inptr[0]);
    inptr += 3, outptr += 3;
  }
  (void) JFWRITE(dest->pub.output_file, dest->iobuffer, dest->buffer_width);
}

METHODDEF(void)
put_gray_rows (j_decompress_ptr cinfo, djpeg_dest_ptr dinfo,
               JDIMENSION rows_supplied)

{
  tga_dest_ptr dest = (tga_dest_ptr) dinfo;
  register JSAMPROW inptr;
  register char *outptr;
  register JDIMENSION col;

  inptr = dest->pub.buffer[0];
  outptr = dest->iobuffer;
  for (col = cinfo->output_width; col > 0; col--) {
    *outptr++ = (char) GETJSAMPLE(*inptr++);
  }
  (void) JFWRITE(dest->pub.output_file, dest->iobuffer, dest->buffer_width);
}




METHODDEF(void)
put_demapped_gray (j_decompress_ptr cinfo, djpeg_dest_ptr dinfo,
                   JDIMENSION rows_supplied)
{
  tga_dest_ptr dest = (tga_dest_ptr) dinfo;
  register JSAMPROW inptr;
  register char *outptr;
  register JSAMPROW color_map0 = cinfo->colormap[0];
  register JDIMENSION col;

  inptr = dest->pub.buffer[0];
  outptr = dest->iobuffer;
  for (col = cinfo->output_width; col > 0; col--) {
    *outptr++ = (char) GETJSAMPLE(color_map0[GETJSAMPLE(*inptr++)]);
  }
  (void) JFWRITE(dest->pub.output_file, dest->iobuffer, dest->buffer_width);
}




METHODDEF(void)
start_output_tga (j_decompress_ptr cinfo, djpeg_dest_ptr dinfo)
{
  tga_dest_ptr dest = (tga_dest_ptr) dinfo;
  int num_colors, i;
  FILE *outfile;

  if (cinfo->out_color_space == JCS_GRAYSCALE) {
    
    
    write_header(cinfo, dinfo, 0);
    if (cinfo->quantize_colors)
      dest->pub.put_pixel_rows = put_demapped_gray;
    else
      dest->pub.put_pixel_rows = put_gray_rows;
  } else if (cinfo->out_color_space == JCS_RGB) {
    if (cinfo->quantize_colors) {
      
      num_colors = cinfo->actual_number_of_colors;
      if (num_colors > 256)
        ERREXIT1(cinfo, JERR_TOO_MANY_COLORS, num_colors);
      write_header(cinfo, dinfo, num_colors);
      
      outfile = dest->pub.output_file;
      for (i = 0; i < num_colors; i++) {
        putc(GETJSAMPLE(cinfo->colormap[2][i]), outfile);
        putc(GETJSAMPLE(cinfo->colormap[1][i]), outfile);
        putc(GETJSAMPLE(cinfo->colormap[0][i]), outfile);
      }
      dest->pub.put_pixel_rows = put_gray_rows;
    } else {
      write_header(cinfo, dinfo, 0);
      dest->pub.put_pixel_rows = put_pixel_rows;
    }
  } else {
    ERREXIT(cinfo, JERR_TGA_COLORSPACE);
  }
}




METHODDEF(void)
finish_output_tga (j_decompress_ptr cinfo, djpeg_dest_ptr dinfo)
{
  
  fflush(dinfo->output_file);
  if (ferror(dinfo->output_file))
    ERREXIT(cinfo, JERR_FILE_WRITE);
}




GLOBAL(djpeg_dest_ptr)
jinit_write_targa (j_decompress_ptr cinfo)
{
  tga_dest_ptr dest;

  
  dest = (tga_dest_ptr)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
                                  sizeof(tga_dest_struct));
  dest->pub.start_output = start_output_tga;
  dest->pub.finish_output = finish_output_tga;

  
  jpeg_calc_output_dimensions(cinfo);

  
  dest->buffer_width = cinfo->output_width * cinfo->output_components;
  dest->iobuffer = (char *)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
                                (size_t) (dest->buffer_width * sizeof(char)));

  
  dest->pub.buffer = (*cinfo->mem->alloc_sarray)
    ((j_common_ptr) cinfo, JPOOL_IMAGE, dest->buffer_width, (JDIMENSION) 1);
  dest->pub.buffer_height = 1;

  return (djpeg_dest_ptr) dest;
}

#endif 
