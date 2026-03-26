

#ifdef PPM_SUPPORTED



typedef struct {
  struct djpeg_dest_struct pub; 

  
  char *iobuffer;               
  JSAMPROW pixrow;              
  size_t buffer_width;          
  JDIMENSION samples_per_row;   
} ppm_dest_struct;

typedef ppm_dest_struct *ppm_dest_ptr;

#endif
