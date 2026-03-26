


#include "jinclude.h"
#include "jpeglib.h"
#include "jerror.h"




METHODDEF(void)
init_mem_source (j_decompress_ptr cinfo)
{
  
}




METHODDEF(boolean)
fill_mem_input_buffer (j_decompress_ptr cinfo)
{
  static const JOCTET mybuffer[4] = {
    (JOCTET) 0xFF, (JOCTET) JPEG_EOI, 0, 0
  };

  
  WARNMS(cinfo, JWRN_JPEG_EOF);

  

  cinfo->src->next_input_byte = mybuffer;
  cinfo->src->bytes_in_buffer = 2;

  return TRUE;
}




METHODDEF(void)
skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
  struct jpeg_source_mgr *src = cinfo->src;

  
  if (num_bytes > 0) {
    while (num_bytes > (long) src->bytes_in_buffer) {
      num_bytes -= (long) src->bytes_in_buffer;
      (void) (*src->fill_input_buffer) (cinfo);
      
    }
    src->next_input_byte += (size_t) num_bytes;
    src->bytes_in_buffer -= (size_t) num_bytes;
  }
}







METHODDEF(void)
term_source (j_decompress_ptr cinfo)
{
  
}




GLOBAL(void)
jpeg_mem_src_tj (j_decompress_ptr cinfo,
                 const unsigned char *inbuffer, unsigned long insize)
{
  struct jpeg_source_mgr *src;

  if (inbuffer == NULL || insize == 0)  
    ERREXIT(cinfo, JERR_INPUT_EMPTY);

  
  if (cinfo->src == NULL) {     
    cinfo->src = (struct jpeg_source_mgr *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
                                  sizeof(struct jpeg_source_mgr));
  } else if (cinfo->src->init_source != init_mem_source) {
    
    ERREXIT(cinfo, JERR_BUFFER_SIZE);
  }

  src = cinfo->src;
  src->init_source = init_mem_source;
  src->fill_input_buffer = fill_mem_input_buffer;
  src->skip_input_data = skip_input_data;
  src->resync_to_restart = jpeg_resync_to_restart; 
  src->term_source = term_source;
  src->bytes_in_buffer = (size_t) insize;
  src->next_input_byte = (const JOCTET *) inbuffer;
}
