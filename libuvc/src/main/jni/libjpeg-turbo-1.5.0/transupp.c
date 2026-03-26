


#define JPEG_INTERNALS

#include "jinclude.h"
#include "jpeglib.h"
#include "transupp.h"           
#include "jpegcomp.h"
#include <ctype.h>              


#if JPEG_LIB_VERSION >= 70
#define dstinfo_min_DCT_h_scaled_size dstinfo->min_DCT_h_scaled_size
#define dstinfo_min_DCT_v_scaled_size dstinfo->min_DCT_v_scaled_size
#else
#define dstinfo_min_DCT_h_scaled_size DCTSIZE
#define dstinfo_min_DCT_v_scaled_size DCTSIZE
#endif


#if TRANSFORMS_SUPPORTED




LOCAL(void)
do_crop (j_decompress_ptr srcinfo, j_compress_ptr dstinfo,
         JDIMENSION x_crop_offset, JDIMENSION y_crop_offset,
         jvirt_barray_ptr *src_coef_arrays,
         jvirt_barray_ptr *dst_coef_arrays)

{
  JDIMENSION dst_blk_y, x_crop_blocks, y_crop_blocks;
  int ci, offset_y;
  JBLOCKARRAY src_buffer, dst_buffer;
  jpeg_component_info *compptr;

  
  for (ci = 0; ci < dstinfo->num_components; ci++) {
    compptr = dstinfo->comp_info + ci;
    x_crop_blocks = x_crop_offset * compptr->h_samp_factor;
    y_crop_blocks = y_crop_offset * compptr->v_samp_factor;
    for (dst_blk_y = 0; dst_blk_y < compptr->height_in_blocks;
         dst_blk_y += compptr->v_samp_factor) {
      dst_buffer = (*srcinfo->mem->access_virt_barray)
        ((j_common_ptr) srcinfo, dst_coef_arrays[ci], dst_blk_y,
         (JDIMENSION) compptr->v_samp_factor, TRUE);
      src_buffer = (*srcinfo->mem->access_virt_barray)
        ((j_common_ptr) srcinfo, src_coef_arrays[ci],
         dst_blk_y + y_crop_blocks,
         (JDIMENSION) compptr->v_samp_factor, FALSE);
      for (offset_y = 0; offset_y < compptr->v_samp_factor; offset_y++) {
        jcopy_block_row(src_buffer[offset_y] + x_crop_blocks,
                        dst_buffer[offset_y],
                        compptr->width_in_blocks);
      }
    }
  }
}


LOCAL(void)
do_flip_h_no_crop (j_decompress_ptr srcinfo, j_compress_ptr dstinfo,
                   JDIMENSION x_crop_offset,
                   jvirt_barray_ptr *src_coef_arrays)

{
  JDIMENSION MCU_cols, comp_width, blk_x, blk_y, x_crop_blocks;
  int ci, k, offset_y;
  JBLOCKARRAY buffer;
  JCOEFPTR ptr1, ptr2;
  JCOEF temp1, temp2;
  jpeg_component_info *compptr;

  
  MCU_cols = srcinfo->output_width /
    (dstinfo->max_h_samp_factor * dstinfo_min_DCT_h_scaled_size);

  for (ci = 0; ci < dstinfo->num_components; ci++) {
    compptr = dstinfo->comp_info + ci;
    comp_width = MCU_cols * compptr->h_samp_factor;
    x_crop_blocks = x_crop_offset * compptr->h_samp_factor;
    for (blk_y = 0; blk_y < compptr->height_in_blocks;
         blk_y += compptr->v_samp_factor) {
      buffer = (*srcinfo->mem->access_virt_barray)
        ((j_common_ptr) srcinfo, src_coef_arrays[ci], blk_y,
         (JDIMENSION) compptr->v_samp_factor, TRUE);
      for (offset_y = 0; offset_y < compptr->v_samp_factor; offset_y++) {
        
        for (blk_x = 0; blk_x * 2 < comp_width; blk_x++) {
          ptr1 = buffer[offset_y][blk_x];
          ptr2 = buffer[offset_y][comp_width - blk_x - 1];
          
          for (k = 0; k < DCTSIZE2; k += 2) {
            temp1 = *ptr1;      
            temp2 = *ptr2;
            *ptr1++ = temp2;
            *ptr2++ = temp1;
            temp1 = *ptr1;      
            temp2 = *ptr2;
            *ptr1++ = -temp2;
            *ptr2++ = -temp1;
          }
        }
        if (x_crop_blocks > 0) {
          
          for (blk_x = 0; blk_x < compptr->width_in_blocks; blk_x++) {
            jcopy_block_row(buffer[offset_y] + blk_x + x_crop_blocks,
                            buffer[offset_y] + blk_x,
                            (JDIMENSION) 1);
          }
        }
      }
    }
  }
}


LOCAL(void)
do_flip_h (j_decompress_ptr srcinfo, j_compress_ptr dstinfo,
           JDIMENSION x_crop_offset, JDIMENSION y_crop_offset,
           jvirt_barray_ptr *src_coef_arrays,
           jvirt_barray_ptr *dst_coef_arrays)

{
  JDIMENSION MCU_cols, comp_width, dst_blk_x, dst_blk_y;
  JDIMENSION x_crop_blocks, y_crop_blocks;
  int ci, k, offset_y;
  JBLOCKARRAY src_buffer, dst_buffer;
  JBLOCKROW src_row_ptr, dst_row_ptr;
  JCOEFPTR src_ptr, dst_ptr;
  jpeg_component_info *compptr;

  
  MCU_cols = srcinfo->output_width /
    (dstinfo->max_h_samp_factor * dstinfo_min_DCT_h_scaled_size);

  for (ci = 0; ci < dstinfo->num_components; ci++) {
    compptr = dstinfo->comp_info + ci;
    comp_width = MCU_cols * compptr->h_samp_factor;
    x_crop_blocks = x_crop_offset * compptr->h_samp_factor;
    y_crop_blocks = y_crop_offset * compptr->v_samp_factor;
    for (dst_blk_y = 0; dst_blk_y < compptr->height_in_blocks;
         dst_blk_y += compptr->v_samp_factor) {
      dst_buffer = (*srcinfo->mem->access_virt_barray)
        ((j_common_ptr) srcinfo, dst_coef_arrays[ci], dst_blk_y,
         (JDIMENSION) compptr->v_samp_factor, TRUE);
      src_buffer = (*srcinfo->mem->access_virt_barray)
        ((j_common_ptr) srcinfo, src_coef_arrays[ci],
         dst_blk_y + y_crop_blocks,
         (JDIMENSION) compptr->v_samp_factor, FALSE);
      for (offset_y = 0; offset_y < compptr->v_samp_factor; offset_y++) {
        dst_row_ptr = dst_buffer[offset_y];
        src_row_ptr = src_buffer[offset_y];
        for (dst_blk_x = 0; dst_blk_x < compptr->width_in_blocks; dst_blk_x++) {
          if (x_crop_blocks + dst_blk_x < comp_width) {
            
            dst_ptr = dst_row_ptr[dst_blk_x];
            src_ptr = src_row_ptr[comp_width - x_crop_blocks - dst_blk_x - 1];
            
            for (k = 0; k < DCTSIZE2; k += 2) {
              *dst_ptr++ = *src_ptr++;   
              *dst_ptr++ = - *src_ptr++; 
            }
          } else {
            
            jcopy_block_row(src_row_ptr + dst_blk_x + x_crop_blocks,
                            dst_row_ptr + dst_blk_x,
                            (JDIMENSION) 1);
          }
        }
      }
    }
  }
}


LOCAL(void)
do_flip_v (j_decompress_ptr srcinfo, j_compress_ptr dstinfo,
           JDIMENSION x_crop_offset, JDIMENSION y_crop_offset,
           jvirt_barray_ptr *src_coef_arrays,
           jvirt_barray_ptr *dst_coef_arrays)

{
  JDIMENSION MCU_rows, comp_height, dst_blk_x, dst_blk_y;
  JDIMENSION x_crop_blocks, y_crop_blocks;
  int ci, i, j, offset_y;
  JBLOCKARRAY src_buffer, dst_buffer;
  JBLOCKROW src_row_ptr, dst_row_ptr;
  JCOEFPTR src_ptr, dst_ptr;
  jpeg_component_info *compptr;

  
  MCU_rows = srcinfo->output_height /
    (dstinfo->max_v_samp_factor * dstinfo_min_DCT_v_scaled_size);

  for (ci = 0; ci < dstinfo->num_components; ci++) {
    compptr = dstinfo->comp_info + ci;
    comp_height = MCU_rows * compptr->v_samp_factor;
    x_crop_blocks = x_crop_offset * compptr->h_samp_factor;
    y_crop_blocks = y_crop_offset * compptr->v_samp_factor;
    for (dst_blk_y = 0; dst_blk_y < compptr->height_in_blocks;
         dst_blk_y += compptr->v_samp_factor) {
      dst_buffer = (*srcinfo->mem->access_virt_barray)
        ((j_common_ptr) srcinfo, dst_coef_arrays[ci], dst_blk_y,
         (JDIMENSION) compptr->v_samp_factor, TRUE);
      if (y_crop_blocks + dst_blk_y < comp_height) {
        
        src_buffer = (*srcinfo->mem->access_virt_barray)
          ((j_common_ptr) srcinfo, src_coef_arrays[ci],
           comp_height - y_crop_blocks - dst_blk_y -
           (JDIMENSION) compptr->v_samp_factor,
           (JDIMENSION) compptr->v_samp_factor, FALSE);
      } else {
        
        src_buffer = (*srcinfo->mem->access_virt_barray)
          ((j_common_ptr) srcinfo, src_coef_arrays[ci],
           dst_blk_y + y_crop_blocks,
           (JDIMENSION) compptr->v_samp_factor, FALSE);
      }
      for (offset_y = 0; offset_y < compptr->v_samp_factor; offset_y++) {
        if (y_crop_blocks + dst_blk_y < comp_height) {
          
          dst_row_ptr = dst_buffer[offset_y];
          src_row_ptr = src_buffer[compptr->v_samp_factor - offset_y - 1];
          src_row_ptr += x_crop_blocks;
          for (dst_blk_x = 0; dst_blk_x < compptr->width_in_blocks;
               dst_blk_x++) {
            dst_ptr = dst_row_ptr[dst_blk_x];
            src_ptr = src_row_ptr[dst_blk_x];
            for (i = 0; i < DCTSIZE; i += 2) {
              
              for (j = 0; j < DCTSIZE; j++)
                *dst_ptr++ = *src_ptr++;
              
              for (j = 0; j < DCTSIZE; j++)
                *dst_ptr++ = - *src_ptr++;
            }
          }
        } else {
          
          jcopy_block_row(src_buffer[offset_y] + x_crop_blocks,
                          dst_buffer[offset_y],
                          compptr->width_in_blocks);
        }
      }
    }
  }
}


LOCAL(void)
do_transpose (j_decompress_ptr srcinfo, j_compress_ptr dstinfo,
              JDIMENSION x_crop_offset, JDIMENSION y_crop_offset,
              jvirt_barray_ptr *src_coef_arrays,
              jvirt_barray_ptr *dst_coef_arrays)

{
  JDIMENSION dst_blk_x, dst_blk_y, x_crop_blocks, y_crop_blocks;
  int ci, i, j, offset_x, offset_y;
  JBLOCKARRAY src_buffer, dst_buffer;
  JCOEFPTR src_ptr, dst_ptr;
  jpeg_component_info *compptr;

  
  for (ci = 0; ci < dstinfo->num_components; ci++) {
    compptr = dstinfo->comp_info + ci;
    x_crop_blocks = x_crop_offset * compptr->h_samp_factor;
    y_crop_blocks = y_crop_offset * compptr->v_samp_factor;
    for (dst_blk_y = 0; dst_blk_y < compptr->height_in_blocks;
         dst_blk_y += compptr->v_samp_factor) {
      dst_buffer = (*srcinfo->mem->access_virt_barray)
        ((j_common_ptr) srcinfo, dst_coef_arrays[ci], dst_blk_y,
         (JDIMENSION) compptr->v_samp_factor, TRUE);
      for (offset_y = 0; offset_y < compptr->v_samp_factor; offset_y++) {
        for (dst_blk_x = 0; dst_blk_x < compptr->width_in_blocks;
             dst_blk_x += compptr->h_samp_factor) {
          src_buffer = (*srcinfo->mem->access_virt_barray)
            ((j_common_ptr) srcinfo, src_coef_arrays[ci],
             dst_blk_x + x_crop_blocks,
             (JDIMENSION) compptr->h_samp_factor, FALSE);
          for (offset_x = 0; offset_x < compptr->h_samp_factor; offset_x++) {
            dst_ptr = dst_buffer[offset_y][dst_blk_x + offset_x];
            src_ptr = src_buffer[offset_x][dst_blk_y + offset_y + y_crop_blocks];
            for (i = 0; i < DCTSIZE; i++)
              for (j = 0; j < DCTSIZE; j++)
                dst_ptr[j*DCTSIZE+i] = src_ptr[i*DCTSIZE+j];
          }
        }
      }
    }
  }
}


LOCAL(void)
do_rot_90 (j_decompress_ptr srcinfo, j_compress_ptr dstinfo,
           JDIMENSION x_crop_offset, JDIMENSION y_crop_offset,
           jvirt_barray_ptr *src_coef_arrays,
           jvirt_barray_ptr *dst_coef_arrays)

{
  JDIMENSION MCU_cols, comp_width, dst_blk_x, dst_blk_y;
  JDIMENSION x_crop_blocks, y_crop_blocks;
  int ci, i, j, offset_x, offset_y;
  JBLOCKARRAY src_buffer, dst_buffer;
  JCOEFPTR src_ptr, dst_ptr;
  jpeg_component_info *compptr;

  
  MCU_cols = srcinfo->output_height /
    (dstinfo->max_h_samp_factor * dstinfo_min_DCT_h_scaled_size);

  for (ci = 0; ci < dstinfo->num_components; ci++) {
    compptr = dstinfo->comp_info + ci;
    comp_width = MCU_cols * compptr->h_samp_factor;
    x_crop_blocks = x_crop_offset * compptr->h_samp_factor;
    y_crop_blocks = y_crop_offset * compptr->v_samp_factor;
    for (dst_blk_y = 0; dst_blk_y < compptr->height_in_blocks;
         dst_blk_y += compptr->v_samp_factor) {
      dst_buffer = (*srcinfo->mem->access_virt_barray)
        ((j_common_ptr) srcinfo, dst_coef_arrays[ci], dst_blk_y,
         (JDIMENSION) compptr->v_samp_factor, TRUE);
      for (offset_y = 0; offset_y < compptr->v_samp_factor; offset_y++) {
        for (dst_blk_x = 0; dst_blk_x < compptr->width_in_blocks;
             dst_blk_x += compptr->h_samp_factor) {
          if (x_crop_blocks + dst_blk_x < comp_width) {
            
            src_buffer = (*srcinfo->mem->access_virt_barray)
              ((j_common_ptr) srcinfo, src_coef_arrays[ci],
               comp_width - x_crop_blocks - dst_blk_x -
               (JDIMENSION) compptr->h_samp_factor,
               (JDIMENSION) compptr->h_samp_factor, FALSE);
          } else {
            
            src_buffer = (*srcinfo->mem->access_virt_barray)
              ((j_common_ptr) srcinfo, src_coef_arrays[ci],
               dst_blk_x + x_crop_blocks,
               (JDIMENSION) compptr->h_samp_factor, FALSE);
          }
          for (offset_x = 0; offset_x < compptr->h_samp_factor; offset_x++) {
            dst_ptr = dst_buffer[offset_y][dst_blk_x + offset_x];
            if (x_crop_blocks + dst_blk_x < comp_width) {
              
              src_ptr = src_buffer[compptr->h_samp_factor - offset_x - 1]
                [dst_blk_y + offset_y + y_crop_blocks];
              for (i = 0; i < DCTSIZE; i++) {
                for (j = 0; j < DCTSIZE; j++)
                  dst_ptr[j*DCTSIZE+i] = src_ptr[i*DCTSIZE+j];
                i++;
                for (j = 0; j < DCTSIZE; j++)
                  dst_ptr[j*DCTSIZE+i] = -src_ptr[i*DCTSIZE+j];
              }
            } else {
              
              src_ptr = src_buffer[offset_x]
                [dst_blk_y + offset_y + y_crop_blocks];
              for (i = 0; i < DCTSIZE; i++)
                for (j = 0; j < DCTSIZE; j++)
                  dst_ptr[j*DCTSIZE+i] = src_ptr[i*DCTSIZE+j];
            }
          }
        }
      }
    }
  }
}


LOCAL(void)
do_rot_270 (j_decompress_ptr srcinfo, j_compress_ptr dstinfo,
            JDIMENSION x_crop_offset, JDIMENSION y_crop_offset,
            jvirt_barray_ptr *src_coef_arrays,
            jvirt_barray_ptr *dst_coef_arrays)

{
  JDIMENSION MCU_rows, comp_height, dst_blk_x, dst_blk_y;
  JDIMENSION x_crop_blocks, y_crop_blocks;
  int ci, i, j, offset_x, offset_y;
  JBLOCKARRAY src_buffer, dst_buffer;
  JCOEFPTR src_ptr, dst_ptr;
  jpeg_component_info *compptr;

  
  MCU_rows = srcinfo->output_width /
    (dstinfo->max_v_samp_factor * dstinfo_min_DCT_v_scaled_size);

  for (ci = 0; ci < dstinfo->num_components; ci++) {
    compptr = dstinfo->comp_info + ci;
    comp_height = MCU_rows * compptr->v_samp_factor;
    x_crop_blocks = x_crop_offset * compptr->h_samp_factor;
    y_crop_blocks = y_crop_offset * compptr->v_samp_factor;
    for (dst_blk_y = 0; dst_blk_y < compptr->height_in_blocks;
         dst_blk_y += compptr->v_samp_factor) {
      dst_buffer = (*srcinfo->mem->access_virt_barray)
        ((j_common_ptr) srcinfo, dst_coef_arrays[ci], dst_blk_y,
         (JDIMENSION) compptr->v_samp_factor, TRUE);
      for (offset_y = 0; offset_y < compptr->v_samp_factor; offset_y++) {
        for (dst_blk_x = 0; dst_blk_x < compptr->width_in_blocks;
             dst_blk_x += compptr->h_samp_factor) {
          src_buffer = (*srcinfo->mem->access_virt_barray)
            ((j_common_ptr) srcinfo, src_coef_arrays[ci],
             dst_blk_x + x_crop_blocks,
             (JDIMENSION) compptr->h_samp_factor, FALSE);
          for (offset_x = 0; offset_x < compptr->h_samp_factor; offset_x++) {
            dst_ptr = dst_buffer[offset_y][dst_blk_x + offset_x];
            if (y_crop_blocks + dst_blk_y < comp_height) {
              
              src_ptr = src_buffer[offset_x]
                [comp_height - y_crop_blocks - dst_blk_y - offset_y - 1];
              for (i = 0; i < DCTSIZE; i++) {
                for (j = 0; j < DCTSIZE; j++) {
                  dst_ptr[j*DCTSIZE+i] = src_ptr[i*DCTSIZE+j];
                  j++;
                  dst_ptr[j*DCTSIZE+i] = -src_ptr[i*DCTSIZE+j];
                }
              }
            } else {
              
              src_ptr = src_buffer[offset_x]
                [dst_blk_y + offset_y + y_crop_blocks];
              for (i = 0; i < DCTSIZE; i++)
                for (j = 0; j < DCTSIZE; j++)
                  dst_ptr[j*DCTSIZE+i] = src_ptr[i*DCTSIZE+j];
            }
          }
        }
      }
    }
  }
}


LOCAL(void)
do_rot_180 (j_decompress_ptr srcinfo, j_compress_ptr dstinfo,
            JDIMENSION x_crop_offset, JDIMENSION y_crop_offset,
            jvirt_barray_ptr *src_coef_arrays,
            jvirt_barray_ptr *dst_coef_arrays)

{
  JDIMENSION MCU_cols, MCU_rows, comp_width, comp_height, dst_blk_x, dst_blk_y;
  JDIMENSION x_crop_blocks, y_crop_blocks;
  int ci, i, j, offset_y;
  JBLOCKARRAY src_buffer, dst_buffer;
  JBLOCKROW src_row_ptr, dst_row_ptr;
  JCOEFPTR src_ptr, dst_ptr;
  jpeg_component_info *compptr;

  MCU_cols = srcinfo->output_width /
    (dstinfo->max_h_samp_factor * dstinfo_min_DCT_h_scaled_size);
  MCU_rows = srcinfo->output_height /
    (dstinfo->max_v_samp_factor * dstinfo_min_DCT_v_scaled_size);

  for (ci = 0; ci < dstinfo->num_components; ci++) {
    compptr = dstinfo->comp_info + ci;
    comp_width = MCU_cols * compptr->h_samp_factor;
    comp_height = MCU_rows * compptr->v_samp_factor;
    x_crop_blocks = x_crop_offset * compptr->h_samp_factor;
    y_crop_blocks = y_crop_offset * compptr->v_samp_factor;
    for (dst_blk_y = 0; dst_blk_y < compptr->height_in_blocks;
         dst_blk_y += compptr->v_samp_factor) {
      dst_buffer = (*srcinfo->mem->access_virt_barray)
        ((j_common_ptr) srcinfo, dst_coef_arrays[ci], dst_blk_y,
         (JDIMENSION) compptr->v_samp_factor, TRUE);
      if (y_crop_blocks + dst_blk_y < comp_height) {
        
        src_buffer = (*srcinfo->mem->access_virt_barray)
          ((j_common_ptr) srcinfo, src_coef_arrays[ci],
           comp_height - y_crop_blocks - dst_blk_y -
           (JDIMENSION) compptr->v_samp_factor,
           (JDIMENSION) compptr->v_samp_factor, FALSE);
      } else {
        
        src_buffer = (*srcinfo->mem->access_virt_barray)
          ((j_common_ptr) srcinfo, src_coef_arrays[ci],
           dst_blk_y + y_crop_blocks,
           (JDIMENSION) compptr->v_samp_factor, FALSE);
      }
      for (offset_y = 0; offset_y < compptr->v_samp_factor; offset_y++) {
        dst_row_ptr = dst_buffer[offset_y];
        if (y_crop_blocks + dst_blk_y < comp_height) {
          
          src_row_ptr = src_buffer[compptr->v_samp_factor - offset_y - 1];
          for (dst_blk_x = 0; dst_blk_x < compptr->width_in_blocks; dst_blk_x++) {
            dst_ptr = dst_row_ptr[dst_blk_x];
            if (x_crop_blocks + dst_blk_x < comp_width) {
              
              src_ptr = src_row_ptr[comp_width - x_crop_blocks - dst_blk_x - 1];
              for (i = 0; i < DCTSIZE; i += 2) {
                
                for (j = 0; j < DCTSIZE; j += 2) {
                  *dst_ptr++ = *src_ptr++;
                  *dst_ptr++ = - *src_ptr++;
                }
                
                for (j = 0; j < DCTSIZE; j += 2) {
                  *dst_ptr++ = - *src_ptr++;
                  *dst_ptr++ = *src_ptr++;
                }
              }
            } else {
              
              src_ptr = src_row_ptr[x_crop_blocks + dst_blk_x];
              for (i = 0; i < DCTSIZE; i += 2) {
                for (j = 0; j < DCTSIZE; j++)
                  *dst_ptr++ = *src_ptr++;
                for (j = 0; j < DCTSIZE; j++)
                  *dst_ptr++ = - *src_ptr++;
              }
            }
          }
        } else {
          
          src_row_ptr = src_buffer[offset_y];
          for (dst_blk_x = 0; dst_blk_x < compptr->width_in_blocks; dst_blk_x++) {
            if (x_crop_blocks + dst_blk_x < comp_width) {
              
              dst_ptr = dst_row_ptr[dst_blk_x];
              src_ptr = src_row_ptr[comp_width - x_crop_blocks - dst_blk_x - 1];
              for (i = 0; i < DCTSIZE2; i += 2) {
                *dst_ptr++ = *src_ptr++;
                *dst_ptr++ = - *src_ptr++;
              }
            } else {
              
              jcopy_block_row(src_row_ptr + dst_blk_x + x_crop_blocks,
                              dst_row_ptr + dst_blk_x,
                              (JDIMENSION) 1);
            }
          }
        }
      }
    }
  }
}


LOCAL(void)
do_transverse (j_decompress_ptr srcinfo, j_compress_ptr dstinfo,
               JDIMENSION x_crop_offset, JDIMENSION y_crop_offset,
               jvirt_barray_ptr *src_coef_arrays,
               jvirt_barray_ptr *dst_coef_arrays)

{
  JDIMENSION MCU_cols, MCU_rows, comp_width, comp_height, dst_blk_x, dst_blk_y;
  JDIMENSION x_crop_blocks, y_crop_blocks;
  int ci, i, j, offset_x, offset_y;
  JBLOCKARRAY src_buffer, dst_buffer;
  JCOEFPTR src_ptr, dst_ptr;
  jpeg_component_info *compptr;

  MCU_cols = srcinfo->output_height /
    (dstinfo->max_h_samp_factor * dstinfo_min_DCT_h_scaled_size);
  MCU_rows = srcinfo->output_width /
    (dstinfo->max_v_samp_factor * dstinfo_min_DCT_v_scaled_size);

  for (ci = 0; ci < dstinfo->num_components; ci++) {
    compptr = dstinfo->comp_info + ci;
    comp_width = MCU_cols * compptr->h_samp_factor;
    comp_height = MCU_rows * compptr->v_samp_factor;
    x_crop_blocks = x_crop_offset * compptr->h_samp_factor;
    y_crop_blocks = y_crop_offset * compptr->v_samp_factor;
    for (dst_blk_y = 0; dst_blk_y < compptr->height_in_blocks;
         dst_blk_y += compptr->v_samp_factor) {
      dst_buffer = (*srcinfo->mem->access_virt_barray)
        ((j_common_ptr) srcinfo, dst_coef_arrays[ci], dst_blk_y,
         (JDIMENSION) compptr->v_samp_factor, TRUE);
      for (offset_y = 0; offset_y < compptr->v_samp_factor; offset_y++) {
        for (dst_blk_x = 0; dst_blk_x < compptr->width_in_blocks;
             dst_blk_x += compptr->h_samp_factor) {
          if (x_crop_blocks + dst_blk_x < comp_width) {
            
            src_buffer = (*srcinfo->mem->access_virt_barray)
              ((j_common_ptr) srcinfo, src_coef_arrays[ci],
               comp_width - x_crop_blocks - dst_blk_x -
               (JDIMENSION) compptr->h_samp_factor,
               (JDIMENSION) compptr->h_samp_factor, FALSE);
          } else {
            src_buffer = (*srcinfo->mem->access_virt_barray)
              ((j_common_ptr) srcinfo, src_coef_arrays[ci],
               dst_blk_x + x_crop_blocks,
               (JDIMENSION) compptr->h_samp_factor, FALSE);
          }
          for (offset_x = 0; offset_x < compptr->h_samp_factor; offset_x++) {
            dst_ptr = dst_buffer[offset_y][dst_blk_x + offset_x];
            if (y_crop_blocks + dst_blk_y < comp_height) {
              if (x_crop_blocks + dst_blk_x < comp_width) {
                
                src_ptr = src_buffer[compptr->h_samp_factor - offset_x - 1]
                  [comp_height - y_crop_blocks - dst_blk_y - offset_y - 1];
                for (i = 0; i < DCTSIZE; i++) {
                  for (j = 0; j < DCTSIZE; j++) {
                    dst_ptr[j*DCTSIZE+i] = src_ptr[i*DCTSIZE+j];
                    j++;
                    dst_ptr[j*DCTSIZE+i] = -src_ptr[i*DCTSIZE+j];
                  }
                  i++;
                  for (j = 0; j < DCTSIZE; j++) {
                    dst_ptr[j*DCTSIZE+i] = -src_ptr[i*DCTSIZE+j];
                    j++;
                    dst_ptr[j*DCTSIZE+i] = src_ptr[i*DCTSIZE+j];
                  }
                }
              } else {
                
                src_ptr = src_buffer[offset_x]
                  [comp_height - y_crop_blocks - dst_blk_y - offset_y - 1];
                for (i = 0; i < DCTSIZE; i++) {
                  for (j = 0; j < DCTSIZE; j++) {
                    dst_ptr[j*DCTSIZE+i] = src_ptr[i*DCTSIZE+j];
                    j++;
                    dst_ptr[j*DCTSIZE+i] = -src_ptr[i*DCTSIZE+j];
                  }
                }
              }
            } else {
              if (x_crop_blocks + dst_blk_x < comp_width) {
                
                src_ptr = src_buffer[compptr->h_samp_factor - offset_x - 1]
                  [dst_blk_y + offset_y + y_crop_blocks];
                for (i = 0; i < DCTSIZE; i++) {
                  for (j = 0; j < DCTSIZE; j++)
                    dst_ptr[j*DCTSIZE+i] = src_ptr[i*DCTSIZE+j];
                  i++;
                  for (j = 0; j < DCTSIZE; j++)
                    dst_ptr[j*DCTSIZE+i] = -src_ptr[i*DCTSIZE+j];
                }
              } else {
                
                src_ptr = src_buffer[offset_x]
                  [dst_blk_y + offset_y + y_crop_blocks];
                for (i = 0; i < DCTSIZE; i++)
                  for (j = 0; j < DCTSIZE; j++)
                    dst_ptr[j*DCTSIZE+i] = src_ptr[i*DCTSIZE+j];
              }
            }
          }
        }
      }
    }
  }
}




LOCAL(boolean)
jt_read_integer (const char **strptr, JDIMENSION *result)
{
  const char *ptr = *strptr;
  JDIMENSION val = 0;

  for (; isdigit(*ptr); ptr++) {
    val = val * 10 + (JDIMENSION) (*ptr - '0');
  }
  *result = val;
  if (ptr == *strptr)
    return FALSE;               
  *strptr = ptr;
  return TRUE;
}




GLOBAL(boolean)
jtransform_parse_crop_spec (jpeg_transform_info *info, const char *spec)
{
  info->crop = FALSE;
  info->crop_width_set = JCROP_UNSET;
  info->crop_height_set = JCROP_UNSET;
  info->crop_xoffset_set = JCROP_UNSET;
  info->crop_yoffset_set = JCROP_UNSET;

  if (isdigit(*spec)) {
    
    if (! jt_read_integer(&spec, &info->crop_width))
      return FALSE;
    if (*spec == 'f' || *spec == 'F') {
      spec++;
      info->crop_width_set = JCROP_FORCE;
    } else
      info->crop_width_set = JCROP_POS;
  }
  if (*spec == 'x' || *spec == 'X') {
    
    spec++;
    if (! jt_read_integer(&spec, &info->crop_height))
      return FALSE;
    if (*spec == 'f' || *spec == 'F') {
      spec++;
      info->crop_height_set = JCROP_FORCE;
    } else
      info->crop_height_set = JCROP_POS;
  }
  if (*spec == '+' || *spec == '-') {
    
    info->crop_xoffset_set = (*spec == '-') ? JCROP_NEG : JCROP_POS;
    spec++;
    if (! jt_read_integer(&spec, &info->crop_xoffset))
      return FALSE;
  }
  if (*spec == '+' || *spec == '-') {
    
    info->crop_yoffset_set = (*spec == '-') ? JCROP_NEG : JCROP_POS;
    spec++;
    if (! jt_read_integer(&spec, &info->crop_yoffset))
      return FALSE;
  }
  
  if (*spec != '\0')
    return FALSE;
  info->crop = TRUE;
  return TRUE;
}




LOCAL(void)
trim_right_edge (jpeg_transform_info *info, JDIMENSION full_width)
{
  JDIMENSION MCU_cols;

  MCU_cols = info->output_width / info->iMCU_sample_width;
  if (MCU_cols > 0 && info->x_crop_offset + MCU_cols ==
      full_width / info->iMCU_sample_width)
    info->output_width = MCU_cols * info->iMCU_sample_width;
}

LOCAL(void)
trim_bottom_edge (jpeg_transform_info *info, JDIMENSION full_height)
{
  JDIMENSION MCU_rows;

  MCU_rows = info->output_height / info->iMCU_sample_height;
  if (MCU_rows > 0 && info->y_crop_offset + MCU_rows ==
      full_height / info->iMCU_sample_height)
    info->output_height = MCU_rows * info->iMCU_sample_height;
}




GLOBAL(boolean)
jtransform_request_workspace (j_decompress_ptr srcinfo,
                              jpeg_transform_info *info)
{
  jvirt_barray_ptr *coef_arrays;
  boolean need_workspace, transpose_it;
  jpeg_component_info *compptr;
  JDIMENSION xoffset, yoffset;
  JDIMENSION width_in_iMCUs, height_in_iMCUs;
  JDIMENSION width_in_blocks, height_in_blocks;
  int ci, h_samp_factor, v_samp_factor;

  
  if (info->force_grayscale &&
      srcinfo->jpeg_color_space == JCS_YCbCr &&
      srcinfo->num_components == 3)
    
    info->num_components = 1;
  else
    
    info->num_components = srcinfo->num_components;

  
#if JPEG_LIB_VERSION >= 80
  jpeg_core_output_dimensions(srcinfo);
#else
  srcinfo->output_width = srcinfo->image_width;
  srcinfo->output_height = srcinfo->image_height;
#endif

  
  if (info->perfect) {
    if (info->num_components == 1) {
      if (!jtransform_perfect_transform(srcinfo->output_width,
          srcinfo->output_height,
          srcinfo->_min_DCT_h_scaled_size,
          srcinfo->_min_DCT_v_scaled_size,
          info->transform))
        return FALSE;
    } else {
      if (!jtransform_perfect_transform(srcinfo->output_width,
          srcinfo->output_height,
          srcinfo->max_h_samp_factor * srcinfo->_min_DCT_h_scaled_size,
          srcinfo->max_v_samp_factor * srcinfo->_min_DCT_v_scaled_size,
          info->transform))
        return FALSE;
    }
  }

  
  switch (info->transform) {
  case JXFORM_TRANSPOSE:
  case JXFORM_TRANSVERSE:
  case JXFORM_ROT_90:
  case JXFORM_ROT_270:
    info->output_width = srcinfo->output_height;
    info->output_height = srcinfo->output_width;
    if (info->num_components == 1) {
      info->iMCU_sample_width = srcinfo->_min_DCT_v_scaled_size;
      info->iMCU_sample_height = srcinfo->_min_DCT_h_scaled_size;
    } else {
      info->iMCU_sample_width =
        srcinfo->max_v_samp_factor * srcinfo->_min_DCT_v_scaled_size;
      info->iMCU_sample_height =
        srcinfo->max_h_samp_factor * srcinfo->_min_DCT_h_scaled_size;
    }
    break;
  default:
    info->output_width = srcinfo->output_width;
    info->output_height = srcinfo->output_height;
    if (info->num_components == 1) {
      info->iMCU_sample_width = srcinfo->_min_DCT_h_scaled_size;
      info->iMCU_sample_height = srcinfo->_min_DCT_v_scaled_size;
    } else {
      info->iMCU_sample_width =
        srcinfo->max_h_samp_factor * srcinfo->_min_DCT_h_scaled_size;
      info->iMCU_sample_height =
        srcinfo->max_v_samp_factor * srcinfo->_min_DCT_v_scaled_size;
    }
    break;
  }

  
  if (info->crop) {
    
    if (info->crop_xoffset_set == JCROP_UNSET)
      info->crop_xoffset = 0;   
    if (info->crop_yoffset_set == JCROP_UNSET)
      info->crop_yoffset = 0;   
    if (info->crop_xoffset >= info->output_width ||
        info->crop_yoffset >= info->output_height)
      ERREXIT(srcinfo, JERR_BAD_CROP_SPEC);
    if (info->crop_width_set == JCROP_UNSET)
      info->crop_width = info->output_width - info->crop_xoffset;
    if (info->crop_height_set == JCROP_UNSET)
      info->crop_height = info->output_height - info->crop_yoffset;
    
    if (info->crop_width <= 0 || info->crop_width > info->output_width ||
        info->crop_height <= 0 || info->crop_height > info->output_height ||
        info->crop_xoffset > info->output_width - info->crop_width ||
        info->crop_yoffset > info->output_height - info->crop_height)
      ERREXIT(srcinfo, JERR_BAD_CROP_SPEC);
    
    if (info->crop_xoffset_set == JCROP_NEG)
      xoffset = info->output_width - info->crop_width - info->crop_xoffset;
    else
      xoffset = info->crop_xoffset;
    if (info->crop_yoffset_set == JCROP_NEG)
      yoffset = info->output_height - info->crop_height - info->crop_yoffset;
    else
      yoffset = info->crop_yoffset;
    
    if (info->crop_width_set == JCROP_FORCE)
      info->output_width = info->crop_width;
    else
      info->output_width =
        info->crop_width + (xoffset % info->iMCU_sample_width);
    if (info->crop_height_set == JCROP_FORCE)
      info->output_height = info->crop_height;
    else
      info->output_height =
        info->crop_height + (yoffset % info->iMCU_sample_height);
    
    info->x_crop_offset = xoffset / info->iMCU_sample_width;
    info->y_crop_offset = yoffset / info->iMCU_sample_height;
  } else {
    info->x_crop_offset = 0;
    info->y_crop_offset = 0;
  }

  
  need_workspace = FALSE;
  transpose_it = FALSE;
  switch (info->transform) {
  case JXFORM_NONE:
    if (info->x_crop_offset != 0 || info->y_crop_offset != 0)
      need_workspace = TRUE;
    
    break;
  case JXFORM_FLIP_H:
    if (info->trim)
      trim_right_edge(info, srcinfo->output_width);
    if (info->y_crop_offset != 0 || info->slow_hflip)
      need_workspace = TRUE;
    
    break;
  case JXFORM_FLIP_V:
    if (info->trim)
      trim_bottom_edge(info, srcinfo->output_height);
    
    need_workspace = TRUE;
    break;
  case JXFORM_TRANSPOSE:
    
    
    need_workspace = TRUE;
    transpose_it = TRUE;
    break;
  case JXFORM_TRANSVERSE:
    if (info->trim) {
      trim_right_edge(info, srcinfo->output_height);
      trim_bottom_edge(info, srcinfo->output_width);
    }
    
    need_workspace = TRUE;
    transpose_it = TRUE;
    break;
  case JXFORM_ROT_90:
    if (info->trim)
      trim_right_edge(info, srcinfo->output_height);
    
    need_workspace = TRUE;
    transpose_it = TRUE;
    break;
  case JXFORM_ROT_180:
    if (info->trim) {
      trim_right_edge(info, srcinfo->output_width);
      trim_bottom_edge(info, srcinfo->output_height);
    }
    
    need_workspace = TRUE;
    break;
  case JXFORM_ROT_270:
    if (info->trim)
      trim_bottom_edge(info, srcinfo->output_width);
    
    need_workspace = TRUE;
    transpose_it = TRUE;
    break;
  }

  
  if (need_workspace) {
    coef_arrays = (jvirt_barray_ptr *)
      (*srcinfo->mem->alloc_small) ((j_common_ptr) srcinfo, JPOOL_IMAGE,
                sizeof(jvirt_barray_ptr) * info->num_components);
    width_in_iMCUs = (JDIMENSION)
      jdiv_round_up((long) info->output_width,
                    (long) info->iMCU_sample_width);
    height_in_iMCUs = (JDIMENSION)
      jdiv_round_up((long) info->output_height,
                    (long) info->iMCU_sample_height);
    for (ci = 0; ci < info->num_components; ci++) {
      compptr = srcinfo->comp_info + ci;
      if (info->num_components == 1) {
        
        h_samp_factor = v_samp_factor = 1;
      } else if (transpose_it) {
        h_samp_factor = compptr->v_samp_factor;
        v_samp_factor = compptr->h_samp_factor;
      } else {
        h_samp_factor = compptr->h_samp_factor;
        v_samp_factor = compptr->v_samp_factor;
      }
      width_in_blocks = width_in_iMCUs * h_samp_factor;
      height_in_blocks = height_in_iMCUs * v_samp_factor;
      coef_arrays[ci] = (*srcinfo->mem->request_virt_barray)
        ((j_common_ptr) srcinfo, JPOOL_IMAGE, FALSE,
         width_in_blocks, height_in_blocks, (JDIMENSION) v_samp_factor);
    }
    info->workspace_coef_arrays = coef_arrays;
  } else
    info->workspace_coef_arrays = NULL;

  return TRUE;
}




LOCAL(void)
transpose_critical_parameters (j_compress_ptr dstinfo)
{
  int tblno, i, j, ci, itemp;
  jpeg_component_info *compptr;
  JQUANT_TBL *qtblptr;
  JDIMENSION jtemp;
  UINT16 qtemp;

  
  jtemp = dstinfo->image_width;
  dstinfo->image_width = dstinfo->image_height;
  dstinfo->image_height = jtemp;
#if JPEG_LIB_VERSION >= 70
  itemp = dstinfo->min_DCT_h_scaled_size;
  dstinfo->min_DCT_h_scaled_size = dstinfo->min_DCT_v_scaled_size;
  dstinfo->min_DCT_v_scaled_size = itemp;
#endif

  
  for (ci = 0; ci < dstinfo->num_components; ci++) {
    compptr = dstinfo->comp_info + ci;
    itemp = compptr->h_samp_factor;
    compptr->h_samp_factor = compptr->v_samp_factor;
    compptr->v_samp_factor = itemp;
  }

  
  for (tblno = 0; tblno < NUM_QUANT_TBLS; tblno++) {
    qtblptr = dstinfo->quant_tbl_ptrs[tblno];
    if (qtblptr != NULL) {
      for (i = 0; i < DCTSIZE; i++) {
        for (j = 0; j < i; j++) {
          qtemp = qtblptr->quantval[i*DCTSIZE+j];
          qtblptr->quantval[i*DCTSIZE+j] = qtblptr->quantval[j*DCTSIZE+i];
          qtblptr->quantval[j*DCTSIZE+i] = qtemp;
        }
      }
    }
  }
}




#if JPEG_LIB_VERSION >= 70
LOCAL(void)
adjust_exif_parameters (JOCTET *data, unsigned int length,
                        JDIMENSION new_width, JDIMENSION new_height)
{
  boolean is_motorola; 
  unsigned int number_of_tags, tagnum;
  unsigned int firstoffset, offset;
  JDIMENSION new_value;

  if (length < 12) return; 

  
  if (GETJOCTET(data[0]) == 0x49 && GETJOCTET(data[1]) == 0x49)
    is_motorola = FALSE;
  else if (GETJOCTET(data[0]) == 0x4D && GETJOCTET(data[1]) == 0x4D)
    is_motorola = TRUE;
  else
    return;

  
  if (is_motorola) {
    if (GETJOCTET(data[2]) != 0) return;
    if (GETJOCTET(data[3]) != 0x2A) return;
  } else {
    if (GETJOCTET(data[3]) != 0) return;
    if (GETJOCTET(data[2]) != 0x2A) return;
  }

  
  if (is_motorola) {
    if (GETJOCTET(data[4]) != 0) return;
    if (GETJOCTET(data[5]) != 0) return;
    firstoffset = GETJOCTET(data[6]);
    firstoffset <<= 8;
    firstoffset += GETJOCTET(data[7]);
  } else {
    if (GETJOCTET(data[7]) != 0) return;
    if (GETJOCTET(data[6]) != 0) return;
    firstoffset = GETJOCTET(data[5]);
    firstoffset <<= 8;
    firstoffset += GETJOCTET(data[4]);
  }
  if (firstoffset > length - 2) return; 

  
  if (is_motorola) {
    number_of_tags = GETJOCTET(data[firstoffset]);
    number_of_tags <<= 8;
    number_of_tags += GETJOCTET(data[firstoffset+1]);
  } else {
    number_of_tags = GETJOCTET(data[firstoffset+1]);
    number_of_tags <<= 8;
    number_of_tags += GETJOCTET(data[firstoffset]);
  }
  if (number_of_tags == 0) return;
  firstoffset += 2;

  
  for (;;) {
    if (firstoffset > length - 12) return; 
    
    if (is_motorola) {
      tagnum = GETJOCTET(data[firstoffset]);
      tagnum <<= 8;
      tagnum += GETJOCTET(data[firstoffset+1]);
    } else {
      tagnum = GETJOCTET(data[firstoffset+1]);
      tagnum <<= 8;
      tagnum += GETJOCTET(data[firstoffset]);
    }
    if (tagnum == 0x8769) break; 
    if (--number_of_tags == 0) return;
    firstoffset += 12;
  }

  
  if (is_motorola) {
    if (GETJOCTET(data[firstoffset+8]) != 0) return;
    if (GETJOCTET(data[firstoffset+9]) != 0) return;
    offset = GETJOCTET(data[firstoffset+10]);
    offset <<= 8;
    offset += GETJOCTET(data[firstoffset+11]);
  } else {
    if (GETJOCTET(data[firstoffset+11]) != 0) return;
    if (GETJOCTET(data[firstoffset+10]) != 0) return;
    offset = GETJOCTET(data[firstoffset+9]);
    offset <<= 8;
    offset += GETJOCTET(data[firstoffset+8]);
  }
  if (offset > length - 2) return; 

  
  if (is_motorola) {
    number_of_tags = GETJOCTET(data[offset]);
    number_of_tags <<= 8;
    number_of_tags += GETJOCTET(data[offset+1]);
  } else {
    number_of_tags = GETJOCTET(data[offset+1]);
    number_of_tags <<= 8;
    number_of_tags += GETJOCTET(data[offset]);
  }
  if (number_of_tags < 2) return;
  offset += 2;

  
  do {
    if (offset > length - 12) return; 
    
    if (is_motorola) {
      tagnum = GETJOCTET(data[offset]);
      tagnum <<= 8;
      tagnum += GETJOCTET(data[offset+1]);
    } else {
      tagnum = GETJOCTET(data[offset+1]);
      tagnum <<= 8;
      tagnum += GETJOCTET(data[offset]);
    }
    if (tagnum == 0xA002 || tagnum == 0xA003) {
      if (tagnum == 0xA002)
        new_value = new_width; 
      else
        new_value = new_height; 
      if (is_motorola) {
        data[offset+2] = 0; 
        data[offset+3] = 4;
        data[offset+4] = 0; 
        data[offset+5] = 0;
        data[offset+6] = 0;
        data[offset+7] = 1;
        data[offset+8] = 0;
        data[offset+9] = 0;
        data[offset+10] = (JOCTET)((new_value >> 8) & 0xFF);
        data[offset+11] = (JOCTET)(new_value & 0xFF);
      } else {
        data[offset+2] = 4; 
        data[offset+3] = 0;
        data[offset+4] = 1; 
        data[offset+5] = 0;
        data[offset+6] = 0;
        data[offset+7] = 0;
        data[offset+8] = (JOCTET)(new_value & 0xFF);
        data[offset+9] = (JOCTET)((new_value >> 8) & 0xFF);
        data[offset+10] = 0;
        data[offset+11] = 0;
      }
    }
    offset += 12;
  } while (--number_of_tags);
}
#endif




GLOBAL(jvirt_barray_ptr *)
jtransform_adjust_parameters (j_decompress_ptr srcinfo,
                              j_compress_ptr dstinfo,
                              jvirt_barray_ptr *src_coef_arrays,
                              jpeg_transform_info *info)
{
  
  if (info->force_grayscale) {
    
    if (((dstinfo->jpeg_color_space == JCS_YCbCr &&
          dstinfo->num_components == 3) ||
         (dstinfo->jpeg_color_space == JCS_GRAYSCALE &&
          dstinfo->num_components == 1)) &&
        srcinfo->comp_info[0].h_samp_factor == srcinfo->max_h_samp_factor &&
        srcinfo->comp_info[0].v_samp_factor == srcinfo->max_v_samp_factor) {
      
      int sv_quant_tbl_no = dstinfo->comp_info[0].quant_tbl_no;
      jpeg_set_colorspace(dstinfo, JCS_GRAYSCALE);
      dstinfo->comp_info[0].quant_tbl_no = sv_quant_tbl_no;
    } else {
      
      ERREXIT(dstinfo, JERR_CONVERSION_NOTIMPL);
    }
  } else if (info->num_components == 1) {
    
    dstinfo->comp_info[0].h_samp_factor = 1;
    dstinfo->comp_info[0].v_samp_factor = 1;
  }

  
#if JPEG_LIB_VERSION >= 70
  dstinfo->jpeg_width = info->output_width;
  dstinfo->jpeg_height = info->output_height;
#endif

  
  switch (info->transform) {
  case JXFORM_TRANSPOSE:
  case JXFORM_TRANSVERSE:
  case JXFORM_ROT_90:
  case JXFORM_ROT_270:
#if JPEG_LIB_VERSION < 70
    dstinfo->image_width = info->output_height;
    dstinfo->image_height = info->output_width;
#endif
    transpose_critical_parameters(dstinfo);
    break;
  default:
#if JPEG_LIB_VERSION < 70
    dstinfo->image_width = info->output_width;
    dstinfo->image_height = info->output_height;
#endif
    break;
  }

  
  if (srcinfo->marker_list != NULL &&
      srcinfo->marker_list->marker == JPEG_APP0+1 &&
      srcinfo->marker_list->data_length >= 6 &&
      GETJOCTET(srcinfo->marker_list->data[0]) == 0x45 &&
      GETJOCTET(srcinfo->marker_list->data[1]) == 0x78 &&
      GETJOCTET(srcinfo->marker_list->data[2]) == 0x69 &&
      GETJOCTET(srcinfo->marker_list->data[3]) == 0x66 &&
      GETJOCTET(srcinfo->marker_list->data[4]) == 0 &&
      GETJOCTET(srcinfo->marker_list->data[5]) == 0) {
    
    dstinfo->write_JFIF_header = FALSE;
#if JPEG_LIB_VERSION >= 70
    
    if (dstinfo->jpeg_width != srcinfo->image_width ||
        dstinfo->jpeg_height != srcinfo->image_height)
      
      adjust_exif_parameters(srcinfo->marker_list->data + 6,
        srcinfo->marker_list->data_length - 6,
        dstinfo->jpeg_width, dstinfo->jpeg_height);
#endif
  }

  
  if (info->workspace_coef_arrays != NULL)
    return info->workspace_coef_arrays;
  return src_coef_arrays;
}




GLOBAL(void)
jtransform_execute_transform (j_decompress_ptr srcinfo,
                              j_compress_ptr dstinfo,
                              jvirt_barray_ptr *src_coef_arrays,
                              jpeg_transform_info *info)
{
  jvirt_barray_ptr *dst_coef_arrays = info->workspace_coef_arrays;

  
  switch (info->transform) {
  case JXFORM_NONE:
    if (info->x_crop_offset != 0 || info->y_crop_offset != 0)
      do_crop(srcinfo, dstinfo, info->x_crop_offset, info->y_crop_offset,
              src_coef_arrays, dst_coef_arrays);
    break;
  case JXFORM_FLIP_H:
    if (info->y_crop_offset != 0 || info->slow_hflip)
      do_flip_h(srcinfo, dstinfo, info->x_crop_offset, info->y_crop_offset,
                src_coef_arrays, dst_coef_arrays);
    else
      do_flip_h_no_crop(srcinfo, dstinfo, info->x_crop_offset,
                        src_coef_arrays);
    break;
  case JXFORM_FLIP_V:
    do_flip_v(srcinfo, dstinfo, info->x_crop_offset, info->y_crop_offset,
              src_coef_arrays, dst_coef_arrays);
    break;
  case JXFORM_TRANSPOSE:
    do_transpose(srcinfo, dstinfo, info->x_crop_offset, info->y_crop_offset,
                 src_coef_arrays, dst_coef_arrays);
    break;
  case JXFORM_TRANSVERSE:
    do_transverse(srcinfo, dstinfo, info->x_crop_offset, info->y_crop_offset,
                  src_coef_arrays, dst_coef_arrays);
    break;
  case JXFORM_ROT_90:
    do_rot_90(srcinfo, dstinfo, info->x_crop_offset, info->y_crop_offset,
              src_coef_arrays, dst_coef_arrays);
    break;
  case JXFORM_ROT_180:
    do_rot_180(srcinfo, dstinfo, info->x_crop_offset, info->y_crop_offset,
               src_coef_arrays, dst_coef_arrays);
    break;
  case JXFORM_ROT_270:
    do_rot_270(srcinfo, dstinfo, info->x_crop_offset, info->y_crop_offset,
               src_coef_arrays, dst_coef_arrays);
    break;
  }
}



GLOBAL(boolean)
jtransform_perfect_transform(JDIMENSION image_width, JDIMENSION image_height,
                             int MCU_width, int MCU_height,
                             JXFORM_CODE transform)
{
  boolean result = TRUE; 

  switch (transform) {
  case JXFORM_FLIP_H:
  case JXFORM_ROT_270:
    if (image_width % (JDIMENSION) MCU_width)
      result = FALSE;
    break;
  case JXFORM_FLIP_V:
  case JXFORM_ROT_90:
    if (image_height % (JDIMENSION) MCU_height)
      result = FALSE;
    break;
  case JXFORM_TRANSVERSE:
  case JXFORM_ROT_180:
    if (image_width % (JDIMENSION) MCU_width)
      result = FALSE;
    if (image_height % (JDIMENSION) MCU_height)
      result = FALSE;
    break;
  default:
    break;
  }

  return result;
}

#endif 




GLOBAL(void)
jcopy_markers_setup (j_decompress_ptr srcinfo, JCOPY_OPTION option)
{
#ifdef SAVE_MARKERS_SUPPORTED
  int m;

  
  if (option != JCOPYOPT_NONE) {
    jpeg_save_markers(srcinfo, JPEG_COM, 0xFFFF);
  }
  
  if (option == JCOPYOPT_ALL) {
    for (m = 0; m < 16; m++)
      jpeg_save_markers(srcinfo, JPEG_APP0 + m, 0xFFFF);
  }
#endif 
}



GLOBAL(void)
jcopy_markers_execute (j_decompress_ptr srcinfo, j_compress_ptr dstinfo,
                       JCOPY_OPTION option)
{
  jpeg_saved_marker_ptr marker;

  
  for (marker = srcinfo->marker_list; marker != NULL; marker = marker->next) {
    if (dstinfo->write_JFIF_header &&
        marker->marker == JPEG_APP0 &&
        marker->data_length >= 5 &&
        GETJOCTET(marker->data[0]) == 0x4A &&
        GETJOCTET(marker->data[1]) == 0x46 &&
        GETJOCTET(marker->data[2]) == 0x49 &&
        GETJOCTET(marker->data[3]) == 0x46 &&
        GETJOCTET(marker->data[4]) == 0)
      continue;                 
    if (dstinfo->write_Adobe_marker &&
        marker->marker == JPEG_APP0+14 &&
        marker->data_length >= 5 &&
        GETJOCTET(marker->data[0]) == 0x41 &&
        GETJOCTET(marker->data[1]) == 0x64 &&
        GETJOCTET(marker->data[2]) == 0x6F &&
        GETJOCTET(marker->data[3]) == 0x62 &&
        GETJOCTET(marker->data[4]) == 0x65)
      continue;                 
    jpeg_write_marker(dstinfo, marker->marker,
                      marker->data, marker->data_length);
  }
}
