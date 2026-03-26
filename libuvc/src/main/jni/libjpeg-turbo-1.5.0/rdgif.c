

#include "cdjpeg.h"             

#ifdef GIF_SUPPORTED



GLOBAL(cjpeg_source_ptr)
jinit_read_gif (j_compress_ptr cinfo)
{
  fprintf(stderr, "GIF input is unsupported for legal reasons.  Sorry.\n");
  exit(EXIT_FAILURE);
  return NULL;                  
}

#endif 
