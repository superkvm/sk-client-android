


#ifndef TRANSFORMS_SUPPORTED
#define TRANSFORMS_SUPPORTED 1          
#endif






typedef enum {
  JXFORM_NONE,            
  JXFORM_FLIP_H,          
  JXFORM_FLIP_V,          
  JXFORM_TRANSPOSE,       
  JXFORM_TRANSVERSE,      
  JXFORM_ROT_90,          
  JXFORM_ROT_180,         
  JXFORM_ROT_270          
} JXFORM_CODE;



typedef enum {
  JCROP_UNSET,
  JCROP_POS,
  JCROP_NEG,
  JCROP_FORCE
} JCROP_CODE;



typedef struct {
  
  JXFORM_CODE transform;        
  boolean perfect;              
  boolean trim;                 
  boolean force_grayscale;      
  boolean crop;                 
  boolean slow_hflip;  

  
  JDIMENSION crop_width;        
  JCROP_CODE crop_width_set;    
  JDIMENSION crop_height;       
  JCROP_CODE crop_height_set;   
  JDIMENSION crop_xoffset;      
  JCROP_CODE crop_xoffset_set;  
  JDIMENSION crop_yoffset;      
  JCROP_CODE crop_yoffset_set;  

  
  int num_components;           
  jvirt_barray_ptr *workspace_coef_arrays; 
  JDIMENSION output_width;      
  JDIMENSION output_height;
  JDIMENSION x_crop_offset;     
  JDIMENSION y_crop_offset;
  int iMCU_sample_width;        
  int iMCU_sample_height;
} jpeg_transform_info;


#if TRANSFORMS_SUPPORTED


EXTERN(boolean) jtransform_parse_crop_spec
        (jpeg_transform_info *info, const char *spec);

EXTERN(boolean) jtransform_request_workspace
        (j_decompress_ptr srcinfo, jpeg_transform_info *info);

EXTERN(jvirt_barray_ptr *) jtransform_adjust_parameters
        (j_decompress_ptr srcinfo, j_compress_ptr dstinfo,
         jvirt_barray_ptr *src_coef_arrays, jpeg_transform_info *info);

EXTERN(void) jtransform_execute_transform
        (j_decompress_ptr srcinfo, j_compress_ptr dstinfo,
         jvirt_barray_ptr *src_coef_arrays, jpeg_transform_info *info);

EXTERN(boolean) jtransform_perfect_transform
        (JDIMENSION image_width, JDIMENSION image_height, int MCU_width,
         int MCU_height, JXFORM_CODE transform);


#define jtransform_execute_transformation       jtransform_execute_transform

#endif 




typedef enum {
  JCOPYOPT_NONE,          
  JCOPYOPT_COMMENTS,      
  JCOPYOPT_ALL            
} JCOPY_OPTION;

#define JCOPYOPT_DEFAULT  JCOPYOPT_COMMENTS     


EXTERN(void) jcopy_markers_setup
        (j_decompress_ptr srcinfo, JCOPY_OPTION option);

EXTERN(void) jcopy_markers_execute
        (j_decompress_ptr srcinfo, j_compress_ptr dstinfo,
         JCOPY_OPTION option);
