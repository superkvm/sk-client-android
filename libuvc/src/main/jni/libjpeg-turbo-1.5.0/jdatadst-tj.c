


#include "jinclude.h"
#include "jpeglib.h"
#include "jerror.h"

#ifndef HAVE_STDLIB_H           
extern void *malloc (size_t size);
extern void free (void *ptr);
#endif


#define OUTPUT_BUF_SIZE  4096   




typedef struct {
  struct jpeg_destination_mgr pub; 

  unsigned char **outbuffer;    
  unsigned long *outsize;
  unsigned char *newbuffer;     
  JOCTET *buffer;               
  size_t bufsize;
  boolean alloc;
} my_mem_destination_mgr;

typedef my_mem_destination_mgr *my_mem_dest_ptr;




METHODDEF(void)
init_mem_destination (j_compress_ptr cinfo)
{
  
}




METHODDEF(boolean)
empty_mem_output_buffer (j_compress_ptr cinfo)
{
  size_t nextsize;
  JOCTET *nextbuffer;
  my_mem_dest_ptr dest = (my_mem_dest_ptr) cinfo->dest;

  if (!dest->alloc) ERREXIT(cinfo, JERR_BUFFER_SIZE);

  
  nextsize = dest->bufsize * 2;
  nextbuffer = (JOCTET *) malloc(nextsize);

  if (nextbuffer == NULL)
    ERREXIT1(cinfo, JERR_OUT_OF_MEMORY, 10);

  MEMCOPY(nextbuffer, dest->buffer, dest->bufsize);

  if (dest->newbuffer != NULL)
    free(dest->newbuffer);

  dest->newbuffer = nextbuffer;

  dest->pub.next_output_byte = nextbuffer + dest->bufsize;
  dest->pub.free_in_buffer = dest->bufsize;

  dest->buffer = nextbuffer;
  dest->bufsize = nextsize;

  return TRUE;
}




METHODDEF(void)
term_mem_destination (j_compress_ptr cinfo)
{
  my_mem_dest_ptr dest = (my_mem_dest_ptr) cinfo->dest;

  if(dest->alloc) *dest->outbuffer = dest->buffer;
  *dest->outsize = (unsigned long)(dest->bufsize - dest->pub.free_in_buffer);
}




GLOBAL(void)
jpeg_mem_dest_tj (j_compress_ptr cinfo,
               unsigned char **outbuffer, unsigned long *outsize,
               boolean alloc)
{
  boolean reused = FALSE;
  my_mem_dest_ptr dest;

  if (outbuffer == NULL || outsize == NULL)     
    ERREXIT(cinfo, JERR_BUFFER_SIZE);

  
  if (cinfo->dest == NULL) {    
    cinfo->dest = (struct jpeg_destination_mgr *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
                                  sizeof(my_mem_destination_mgr));
    dest = (my_mem_dest_ptr) cinfo->dest;
    dest->newbuffer = NULL;
    dest->buffer = NULL;
  } else if (cinfo->dest->init_destination != init_mem_destination) {
    
    ERREXIT(cinfo, JERR_BUFFER_SIZE);
  }

  dest = (my_mem_dest_ptr) cinfo->dest;
  dest->pub.init_destination = init_mem_destination;
  dest->pub.empty_output_buffer = empty_mem_output_buffer;
  dest->pub.term_destination = term_mem_destination;
  if (dest->buffer == *outbuffer && *outbuffer != NULL && alloc)
    reused = TRUE;
  dest->outbuffer = outbuffer;
  dest->outsize = outsize;
  dest->alloc = alloc;

  if (*outbuffer == NULL || *outsize == 0) {
    if (alloc) {
      
      dest->newbuffer = *outbuffer = (unsigned char *) malloc(OUTPUT_BUF_SIZE);
      if (dest->newbuffer == NULL)
        ERREXIT1(cinfo, JERR_OUT_OF_MEMORY, 10);
      *outsize = OUTPUT_BUF_SIZE;
    }
    else ERREXIT(cinfo, JERR_BUFFER_SIZE);
  }

  dest->pub.next_output_byte = dest->buffer = *outbuffer;
  if (!reused)
    dest->bufsize = *outsize;
  dest->pub.free_in_buffer = dest->bufsize;
}
