

#include "cdjpeg.h"             

#ifdef PPM_SUPPORTED







#ifdef HAVE_UNSIGNED_CHAR
typedef unsigned char U_CHAR;
#define UCH(x)  ((int) (x))
#else 
#ifdef __CHAR_UNSIGNED__
typedef char U_CHAR;
#define UCH(x)  ((int) (x))
#else
typedef char U_CHAR;
#define UCH(x)  ((int) (x) & 0xFF)
#endif
#endif 


#define ReadOK(file,buffer,len) (JFREAD(file,buffer,len) == ((size_t) (len)))




typedef struct {
  struct cjpeg_source_struct pub; 

  
  U_CHAR *iobuffer;             
  JSAMPROW pixrow;              
  size_t buffer_width;          
  JSAMPLE *rescale;             
  int maxval;
} ppm_source_struct;

typedef ppm_source_struct *ppm_source_ptr;


LOCAL(int)
pbm_getc (FILE *infile)


{
  register int ch;

  ch = getc(infile);
  if (ch == '#') {
    do {
      ch = getc(infile);
    } while (ch != '\n' && ch != EOF);
  }
  return ch;
}


LOCAL(unsigned int)
read_pbm_integer (j_compress_ptr cinfo, FILE *infile, unsigned int maxval)




{
  register int ch;
  register unsigned int val;

  
  do {
    ch = pbm_getc(infile);
    if (ch == EOF)
      ERREXIT(cinfo, JERR_INPUT_EOF);
  } while (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r');

  if (ch < '0' || ch > '9')
    ERREXIT(cinfo, JERR_PPM_NONNUMERIC);

  val = ch - '0';
  while ((ch = pbm_getc(infile)) >= '0' && ch <= '9') {
    val *= 10;
    val += ch - '0';
  }

  if (val > maxval)
    ERREXIT(cinfo, JERR_PPM_TOOLARGE);

  return val;
}





METHODDEF(JDIMENSION)
get_text_gray_row (j_compress_ptr cinfo, cjpeg_source_ptr sinfo)

{
  ppm_source_ptr source = (ppm_source_ptr) sinfo;
  FILE *infile = source->pub.input_file;
  register JSAMPROW ptr;
  register JSAMPLE *rescale = source->rescale;
  JDIMENSION col;
  unsigned int maxval = source->maxval;

  ptr = source->pub.buffer[0];
  for (col = cinfo->image_width; col > 0; col--) {
    *ptr++ = rescale[read_pbm_integer(cinfo, infile, maxval)];
  }
  return 1;
}


METHODDEF(JDIMENSION)
get_text_rgb_row (j_compress_ptr cinfo, cjpeg_source_ptr sinfo)

{
  ppm_source_ptr source = (ppm_source_ptr) sinfo;
  FILE *infile = source->pub.input_file;
  register JSAMPROW ptr;
  register JSAMPLE *rescale = source->rescale;
  JDIMENSION col;
  unsigned int maxval = source->maxval;

  ptr = source->pub.buffer[0];
  for (col = cinfo->image_width; col > 0; col--) {
    *ptr++ = rescale[read_pbm_integer(cinfo, infile, maxval)];
    *ptr++ = rescale[read_pbm_integer(cinfo, infile, maxval)];
    *ptr++ = rescale[read_pbm_integer(cinfo, infile, maxval)];
  }
  return 1;
}


METHODDEF(JDIMENSION)
get_scaled_gray_row (j_compress_ptr cinfo, cjpeg_source_ptr sinfo)

{
  ppm_source_ptr source = (ppm_source_ptr) sinfo;
  register JSAMPROW ptr;
  register U_CHAR *bufferptr;
  register JSAMPLE *rescale = source->rescale;
  JDIMENSION col;

  if (! ReadOK(source->pub.input_file, source->iobuffer, source->buffer_width))
    ERREXIT(cinfo, JERR_INPUT_EOF);
  ptr = source->pub.buffer[0];
  bufferptr = source->iobuffer;
  for (col = cinfo->image_width; col > 0; col--) {
    *ptr++ = rescale[UCH(*bufferptr++)];
  }
  return 1;
}


METHODDEF(JDIMENSION)
get_scaled_rgb_row (j_compress_ptr cinfo, cjpeg_source_ptr sinfo)

{
  ppm_source_ptr source = (ppm_source_ptr) sinfo;
  register JSAMPROW ptr;
  register U_CHAR *bufferptr;
  register JSAMPLE *rescale = source->rescale;
  JDIMENSION col;

  if (! ReadOK(source->pub.input_file, source->iobuffer, source->buffer_width))
    ERREXIT(cinfo, JERR_INPUT_EOF);
  ptr = source->pub.buffer[0];
  bufferptr = source->iobuffer;
  for (col = cinfo->image_width; col > 0; col--) {
    *ptr++ = rescale[UCH(*bufferptr++)];
    *ptr++ = rescale[UCH(*bufferptr++)];
    *ptr++ = rescale[UCH(*bufferptr++)];
  }
  return 1;
}


METHODDEF(JDIMENSION)
get_raw_row (j_compress_ptr cinfo, cjpeg_source_ptr sinfo)

{
  ppm_source_ptr source = (ppm_source_ptr) sinfo;

  if (! ReadOK(source->pub.input_file, source->iobuffer, source->buffer_width))
    ERREXIT(cinfo, JERR_INPUT_EOF);
  return 1;
}


METHODDEF(JDIMENSION)
get_word_gray_row (j_compress_ptr cinfo, cjpeg_source_ptr sinfo)

{
  ppm_source_ptr source = (ppm_source_ptr) sinfo;
  register JSAMPROW ptr;
  register U_CHAR *bufferptr;
  register JSAMPLE *rescale = source->rescale;
  JDIMENSION col;
  unsigned int maxval = source->maxval;

  if (! ReadOK(source->pub.input_file, source->iobuffer, source->buffer_width))
    ERREXIT(cinfo, JERR_INPUT_EOF);
  ptr = source->pub.buffer[0];
  bufferptr = source->iobuffer;
  for (col = cinfo->image_width; col > 0; col--) {
    register int temp;
    temp  = UCH(*bufferptr++) << 8;
    temp |= UCH(*bufferptr++);
    if (temp > maxval)
      ERREXIT(cinfo, JERR_PPM_TOOLARGE);
    *ptr++ = rescale[temp];
  }
  return 1;
}


METHODDEF(JDIMENSION)
get_word_rgb_row (j_compress_ptr cinfo, cjpeg_source_ptr sinfo)

{
  ppm_source_ptr source = (ppm_source_ptr) sinfo;
  register JSAMPROW ptr;
  register U_CHAR *bufferptr;
  register JSAMPLE *rescale = source->rescale;
  JDIMENSION col;
  unsigned int maxval = source->maxval;

  if (! ReadOK(source->pub.input_file, source->iobuffer, source->buffer_width))
    ERREXIT(cinfo, JERR_INPUT_EOF);
  ptr = source->pub.buffer[0];
  bufferptr = source->iobuffer;
  for (col = cinfo->image_width; col > 0; col--) {
    register int temp;
    temp  = UCH(*bufferptr++) << 8;
    temp |= UCH(*bufferptr++);
    if (temp > maxval)
      ERREXIT(cinfo, JERR_PPM_TOOLARGE);
    *ptr++ = rescale[temp];
    temp  = UCH(*bufferptr++) << 8;
    temp |= UCH(*bufferptr++);
    if (temp > maxval)
      ERREXIT(cinfo, JERR_PPM_TOOLARGE);
    *ptr++ = rescale[temp];
    temp  = UCH(*bufferptr++) << 8;
    temp |= UCH(*bufferptr++);
    if (temp > maxval)
      ERREXIT(cinfo, JERR_PPM_TOOLARGE);
    *ptr++ = rescale[temp];
  }
  return 1;
}




METHODDEF(void)
start_input_ppm (j_compress_ptr cinfo, cjpeg_source_ptr sinfo)
{
  ppm_source_ptr source = (ppm_source_ptr) sinfo;
  int c;
  unsigned int w, h, maxval;
  boolean need_iobuffer, use_raw_buffer, need_rescale;

  if (getc(source->pub.input_file) != 'P')
    ERREXIT(cinfo, JERR_PPM_NOT);

  c = getc(source->pub.input_file); 

  
  switch (c) {
  case '2':                     
  case '3':                     
  case '5':                     
  case '6':                     
    break;
  default:
    ERREXIT(cinfo, JERR_PPM_NOT);
    break;
  }

  
  w = read_pbm_integer(cinfo, source->pub.input_file, 65535);
  h = read_pbm_integer(cinfo, source->pub.input_file, 65535);
  maxval = read_pbm_integer(cinfo, source->pub.input_file, 65535);

  if (w <= 0 || h <= 0 || maxval <= 0) 
    ERREXIT(cinfo, JERR_PPM_NOT);

  cinfo->data_precision = BITS_IN_JSAMPLE; 
  cinfo->image_width = (JDIMENSION) w;
  cinfo->image_height = (JDIMENSION) h;
  source->maxval = maxval;

  
  need_iobuffer = TRUE;         
  use_raw_buffer = FALSE;       
  need_rescale = TRUE;          

  switch (c) {
  case '2':                     
    cinfo->input_components = 1;
    cinfo->in_color_space = JCS_GRAYSCALE;
    TRACEMS2(cinfo, 1, JTRC_PGM_TEXT, w, h);
    source->pub.get_pixel_rows = get_text_gray_row;
    need_iobuffer = FALSE;
    break;

  case '3':                     
    cinfo->input_components = 3;
    cinfo->in_color_space = JCS_RGB;
    TRACEMS2(cinfo, 1, JTRC_PPM_TEXT, w, h);
    source->pub.get_pixel_rows = get_text_rgb_row;
    need_iobuffer = FALSE;
    break;

  case '5':                     
    cinfo->input_components = 1;
    cinfo->in_color_space = JCS_GRAYSCALE;
    TRACEMS2(cinfo, 1, JTRC_PGM, w, h);
    if (maxval > 255) {
      source->pub.get_pixel_rows = get_word_gray_row;
    } else if (maxval == MAXJSAMPLE && sizeof(JSAMPLE) == sizeof(U_CHAR)) {
      source->pub.get_pixel_rows = get_raw_row;
      use_raw_buffer = TRUE;
      need_rescale = FALSE;
    } else {
      source->pub.get_pixel_rows = get_scaled_gray_row;
    }
    break;

  case '6':                     
    cinfo->input_components = 3;
    cinfo->in_color_space = JCS_RGB;
    TRACEMS2(cinfo, 1, JTRC_PPM, w, h);
    if (maxval > 255) {
      source->pub.get_pixel_rows = get_word_rgb_row;
    } else if (maxval == MAXJSAMPLE && sizeof(JSAMPLE) == sizeof(U_CHAR)) {
      source->pub.get_pixel_rows = get_raw_row;
      use_raw_buffer = TRUE;
      need_rescale = FALSE;
    } else {
      source->pub.get_pixel_rows = get_scaled_rgb_row;
    }
    break;
  }

  
  if (need_iobuffer) {
    source->buffer_width = (size_t) w * cinfo->input_components *
      ((maxval <= 255) ? sizeof(U_CHAR) : (2 * sizeof(U_CHAR)));
    source->iobuffer = (U_CHAR *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
                                  source->buffer_width);
  }

  
  if (use_raw_buffer) {
    
    
    source->pixrow = (JSAMPROW) source->iobuffer;
    source->pub.buffer = & source->pixrow;
    source->pub.buffer_height = 1;
  } else {
    
    source->pub.buffer = (*cinfo->mem->alloc_sarray)
      ((j_common_ptr) cinfo, JPOOL_IMAGE,
       (JDIMENSION) w * cinfo->input_components, (JDIMENSION) 1);
    source->pub.buffer_height = 1;
  }

  
  if (need_rescale) {
    long val, half_maxval;

    
    source->rescale = (JSAMPLE *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
                                  (size_t) (((long) maxval + 1L) *
                                            sizeof(JSAMPLE)));
    half_maxval = maxval / 2;
    for (val = 0; val <= (long) maxval; val++) {
      
      source->rescale[val] = (JSAMPLE) ((val * MAXJSAMPLE + half_maxval) /
                                        maxval);
    }
  }
}




METHODDEF(void)
finish_input_ppm (j_compress_ptr cinfo, cjpeg_source_ptr sinfo)
{
  
}




GLOBAL(cjpeg_source_ptr)
jinit_read_ppm (j_compress_ptr cinfo)
{
  ppm_source_ptr source;

  
  source = (ppm_source_ptr)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
                                  sizeof(ppm_source_struct));
  
  source->pub.start_input = start_input_ppm;
  source->pub.finish_input = finish_input_ppm;

  return (cjpeg_source_ptr) source;
}

#endif 
