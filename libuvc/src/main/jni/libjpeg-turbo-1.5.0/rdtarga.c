

#include "cdjpeg.h"             

#ifdef TARGA_SUPPORTED




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




typedef struct _tga_source_struct *tga_source_ptr;

typedef struct _tga_source_struct {
  struct cjpeg_source_struct pub; 

  j_compress_ptr cinfo;         

  JSAMPARRAY colormap;          

  jvirt_sarray_ptr whole_image; 
  JDIMENSION current_row;       

  
  void (*read_pixel) (tga_source_ptr sinfo);

  
  U_CHAR tga_pixel[4];

  int pixel_size;               

  
  int block_count;              
  int dup_pixel_count;          

  
  JDIMENSION (*get_pixel_rows) (j_compress_ptr cinfo, cjpeg_source_ptr sinfo);
} tga_source_struct;




static const UINT8 c5to8bits[32] = {
    0,   8,  16,  25,  33,  41,  49,  58,
   66,  74,  82,  90,  99, 107, 115, 123,
  132, 140, 148, 156, 165, 173, 181, 189,
  197, 206, 214, 222, 230, 239, 247, 255
};



LOCAL(int)
read_byte (tga_source_ptr sinfo)

{
  register FILE *infile = sinfo->pub.input_file;
  register int c;

  if ((c = getc(infile)) == EOF)
    ERREXIT(sinfo->cinfo, JERR_INPUT_EOF);
  return c;
}


LOCAL(void)
read_colormap (tga_source_ptr sinfo, int cmaplen, int mapentrysize)

{
  int i;

  
  if (mapentrysize != 24)
    ERREXIT(sinfo->cinfo, JERR_TGA_BADCMAP);

  for (i = 0; i < cmaplen; i++) {
    sinfo->colormap[2][i] = (JSAMPLE) read_byte(sinfo);
    sinfo->colormap[1][i] = (JSAMPLE) read_byte(sinfo);
    sinfo->colormap[0][i] = (JSAMPLE) read_byte(sinfo);
  }
}




METHODDEF(void)
read_non_rle_pixel (tga_source_ptr sinfo)

{
  register FILE *infile = sinfo->pub.input_file;
  register int i;

  for (i = 0; i < sinfo->pixel_size; i++) {
    sinfo->tga_pixel[i] = (U_CHAR) getc(infile);
  }
}


METHODDEF(void)
read_rle_pixel (tga_source_ptr sinfo)

{
  register FILE *infile = sinfo->pub.input_file;
  register int i;

  
  if (sinfo->dup_pixel_count > 0) {
    sinfo->dup_pixel_count--;
    return;
  }

  
  if (--sinfo->block_count < 0) { 
    i = read_byte(sinfo);
    if (i & 0x80) {             
      sinfo->dup_pixel_count = i & 0x7F; 
      sinfo->block_count = 0;   
    } else {
      sinfo->block_count = i & 0x7F; 
    }
  }

  
  for (i = 0; i < sinfo->pixel_size; i++) {
    sinfo->tga_pixel[i] = (U_CHAR) getc(infile);
  }
}





METHODDEF(JDIMENSION)
get_8bit_gray_row (j_compress_ptr cinfo, cjpeg_source_ptr sinfo)

{
  tga_source_ptr source = (tga_source_ptr) sinfo;
  register JSAMPROW ptr;
  register JDIMENSION col;

  ptr = source->pub.buffer[0];
  for (col = cinfo->image_width; col > 0; col--) {
    (*source->read_pixel) (source); 
    *ptr++ = (JSAMPLE) UCH(source->tga_pixel[0]);
  }
  return 1;
}

METHODDEF(JDIMENSION)
get_8bit_row (j_compress_ptr cinfo, cjpeg_source_ptr sinfo)

{
  tga_source_ptr source = (tga_source_ptr) sinfo;
  register int t;
  register JSAMPROW ptr;
  register JDIMENSION col;
  register JSAMPARRAY colormap = source->colormap;

  ptr = source->pub.buffer[0];
  for (col = cinfo->image_width; col > 0; col--) {
    (*source->read_pixel) (source); 
    t = UCH(source->tga_pixel[0]);
    *ptr++ = colormap[0][t];
    *ptr++ = colormap[1][t];
    *ptr++ = colormap[2][t];
  }
  return 1;
}

METHODDEF(JDIMENSION)
get_16bit_row (j_compress_ptr cinfo, cjpeg_source_ptr sinfo)

{
  tga_source_ptr source = (tga_source_ptr) sinfo;
  register int t;
  register JSAMPROW ptr;
  register JDIMENSION col;

  ptr = source->pub.buffer[0];
  for (col = cinfo->image_width; col > 0; col--) {
    (*source->read_pixel) (source); 
    t = UCH(source->tga_pixel[0]);
    t += UCH(source->tga_pixel[1]) << 8;
    
    ptr[2] = (JSAMPLE) c5to8bits[t & 0x1F];
    t >>= 5;
    ptr[1] = (JSAMPLE) c5to8bits[t & 0x1F];
    t >>= 5;
    ptr[0] = (JSAMPLE) c5to8bits[t & 0x1F];
    ptr += 3;
  }
  return 1;
}

METHODDEF(JDIMENSION)
get_24bit_row (j_compress_ptr cinfo, cjpeg_source_ptr sinfo)

{
  tga_source_ptr source = (tga_source_ptr) sinfo;
  register JSAMPROW ptr;
  register JDIMENSION col;

  ptr = source->pub.buffer[0];
  for (col = cinfo->image_width; col > 0; col--) {
    (*source->read_pixel) (source); 
    *ptr++ = (JSAMPLE) UCH(source->tga_pixel[2]); 
    *ptr++ = (JSAMPLE) UCH(source->tga_pixel[1]);
    *ptr++ = (JSAMPLE) UCH(source->tga_pixel[0]);
  }
  return 1;
}



#define get_32bit_row  get_24bit_row




METHODDEF(JDIMENSION)
get_memory_row (j_compress_ptr cinfo, cjpeg_source_ptr sinfo)
{
  tga_source_ptr source = (tga_source_ptr) sinfo;
  JDIMENSION source_row;

  
  
  
  source_row = cinfo->image_height - source->current_row - 1;

  
  source->pub.buffer = (*cinfo->mem->access_virt_sarray)
    ((j_common_ptr) cinfo, source->whole_image,
     source_row, (JDIMENSION) 1, FALSE);

  source->current_row++;
  return 1;
}




METHODDEF(JDIMENSION)
preload_image (j_compress_ptr cinfo, cjpeg_source_ptr sinfo)
{
  tga_source_ptr source = (tga_source_ptr) sinfo;
  JDIMENSION row;
  cd_progress_ptr progress = (cd_progress_ptr) cinfo->progress;

  
  for (row = 0; row < cinfo->image_height; row++) {
    if (progress != NULL) {
      progress->pub.pass_counter = (long) row;
      progress->pub.pass_limit = (long) cinfo->image_height;
      (*progress->pub.progress_monitor) ((j_common_ptr) cinfo);
    }
    source->pub.buffer = (*cinfo->mem->access_virt_sarray)
      ((j_common_ptr) cinfo, source->whole_image, row, (JDIMENSION) 1, TRUE);
    (*source->get_pixel_rows) (cinfo, sinfo);
  }
  if (progress != NULL)
    progress->completed_extra_passes++;

  
  source->pub.get_pixel_rows = get_memory_row;
  source->current_row = 0;
  
  return get_memory_row(cinfo, sinfo);
}




METHODDEF(void)
start_input_tga (j_compress_ptr cinfo, cjpeg_source_ptr sinfo)
{
  tga_source_ptr source = (tga_source_ptr) sinfo;
  U_CHAR targaheader[18];
  int idlen, cmaptype, subtype, flags, interlace_type, components;
  unsigned int width, height, maplen;
  boolean is_bottom_up;

#define GET_2B(offset)  ((unsigned int) UCH(targaheader[offset]) + \
                         (((unsigned int) UCH(targaheader[offset+1])) << 8))

  if (! ReadOK(source->pub.input_file, targaheader, 18))
    ERREXIT(cinfo, JERR_INPUT_EOF);

  
  if (targaheader[16] == 15)
    targaheader[16] = 16;

  idlen = UCH(targaheader[0]);
  cmaptype = UCH(targaheader[1]);
  subtype = UCH(targaheader[2]);
  maplen = GET_2B(5);
  width = GET_2B(12);
  height = GET_2B(14);
  source->pixel_size = UCH(targaheader[16]) >> 3;
  flags = UCH(targaheader[17]); 

  is_bottom_up = ((flags & 0x20) == 0); 
  interlace_type = flags >> 6;  

  if (cmaptype > 1 ||           
      source->pixel_size < 1 || source->pixel_size > 4 ||
      (UCH(targaheader[16]) & 7) != 0 || 
      interlace_type != 0 ||      
      width == 0 || height == 0)  
    ERREXIT(cinfo, JERR_TGA_BADPARMS);

  if (subtype > 8) {
    
    source->read_pixel = read_rle_pixel;
    source->block_count = source->dup_pixel_count = 0;
    subtype -= 8;
  } else {
    
    source->read_pixel = read_non_rle_pixel;
  }

  
  components = 3;               
  cinfo->in_color_space = JCS_RGB;

  switch (subtype) {
  case 1:                       
    if (source->pixel_size == 1 && cmaptype == 1)
      source->get_pixel_rows = get_8bit_row;
    else
      ERREXIT(cinfo, JERR_TGA_BADPARMS);
    TRACEMS2(cinfo, 1, JTRC_TGA_MAPPED, width, height);
    break;
  case 2:                       
    switch (source->pixel_size) {
    case 2:
      source->get_pixel_rows = get_16bit_row;
      break;
    case 3:
      source->get_pixel_rows = get_24bit_row;
      break;
    case 4:
      source->get_pixel_rows = get_32bit_row;
      break;
    default:
      ERREXIT(cinfo, JERR_TGA_BADPARMS);
      break;
    }
    TRACEMS2(cinfo, 1, JTRC_TGA, width, height);
    break;
  case 3:                       
    components = 1;
    cinfo->in_color_space = JCS_GRAYSCALE;
    if (source->pixel_size == 1)
      source->get_pixel_rows = get_8bit_gray_row;
    else
      ERREXIT(cinfo, JERR_TGA_BADPARMS);
    TRACEMS2(cinfo, 1, JTRC_TGA_GRAY, width, height);
    break;
  default:
    ERREXIT(cinfo, JERR_TGA_BADPARMS);
    break;
  }

  if (is_bottom_up) {
    
    source->whole_image = (*cinfo->mem->request_virt_sarray)
      ((j_common_ptr) cinfo, JPOOL_IMAGE, FALSE,
       (JDIMENSION) width * components, (JDIMENSION) height, (JDIMENSION) 1);
    if (cinfo->progress != NULL) {
      cd_progress_ptr progress = (cd_progress_ptr) cinfo->progress;
      progress->total_extra_passes++; 
    }
    
    source->pub.buffer_height = 1; 
    source->pub.get_pixel_rows = preload_image;
  } else {
    
    source->whole_image = NULL;
    source->pub.buffer = (*cinfo->mem->alloc_sarray)
      ((j_common_ptr) cinfo, JPOOL_IMAGE,
       (JDIMENSION) width * components, (JDIMENSION) 1);
    source->pub.buffer_height = 1;
    source->pub.get_pixel_rows = source->get_pixel_rows;
  }

  while (idlen--)               
    (void) read_byte(source);

  if (maplen > 0) {
    if (maplen > 256 || GET_2B(3) != 0)
      ERREXIT(cinfo, JERR_TGA_BADCMAP);
    
    source->colormap = (*cinfo->mem->alloc_sarray)
      ((j_common_ptr) cinfo, JPOOL_IMAGE, (JDIMENSION) maplen, (JDIMENSION) 3);
    
    read_colormap(source, (int) maplen, UCH(targaheader[7]));
  } else {
    if (cmaptype)               
      ERREXIT(cinfo, JERR_TGA_BADPARMS);
    source->colormap = NULL;
  }

  cinfo->input_components = components;
  cinfo->data_precision = 8;
  cinfo->image_width = width;
  cinfo->image_height = height;
}




METHODDEF(void)
finish_input_tga (j_compress_ptr cinfo, cjpeg_source_ptr sinfo)
{
  
}




GLOBAL(cjpeg_source_ptr)
jinit_read_targa (j_compress_ptr cinfo)
{
  tga_source_ptr source;

  
  source = (tga_source_ptr)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
                                  sizeof(tga_source_struct));
  source->cinfo = cinfo;        
  
  source->pub.start_input = start_input_tga;
  source->pub.finish_input = finish_input_tga;

  return (cjpeg_source_ptr) source;
}

#endif 
