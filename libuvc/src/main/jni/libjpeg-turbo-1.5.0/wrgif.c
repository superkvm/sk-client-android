



#include "cdjpeg.h"             

#ifdef GIF_SUPPORTED




typedef struct {
  struct djpeg_dest_struct pub; 

  j_decompress_ptr cinfo;       

  
  int n_bits;                   
  int maxcode;                  
  long cur_accum;               
  int cur_bits;                 

  
  int ClearCode;                
  int EOFCode;                  
  int code_counter;             

  
  int bytesinpkt;               
  char packetbuf[256];          

} gif_dest_struct;

typedef gif_dest_struct *gif_dest_ptr;


#define MAXCODE(n_bits) ((1 << (n_bits)) - 1)




LOCAL(void)
flush_packet (gif_dest_ptr dinfo)

{
  if (dinfo->bytesinpkt > 0) {  
    dinfo->packetbuf[0] = (char) dinfo->bytesinpkt++;
    if (JFWRITE(dinfo->pub.output_file, dinfo->packetbuf, dinfo->bytesinpkt)
        != (size_t) dinfo->bytesinpkt)
      ERREXIT(dinfo->cinfo, JERR_FILE_WRITE);
    dinfo->bytesinpkt = 0;
  }
}



#define CHAR_OUT(dinfo,c)  \
        { (dinfo)->packetbuf[++(dinfo)->bytesinpkt] = (char) (c);  \
            if ((dinfo)->bytesinpkt >= 255)  \
              flush_packet(dinfo);  \
        }




LOCAL(void)
output (gif_dest_ptr dinfo, int code)


{
  dinfo->cur_accum |= ((long) code) << dinfo->cur_bits;
  dinfo->cur_bits += dinfo->n_bits;

  while (dinfo->cur_bits >= 8) {
    CHAR_OUT(dinfo, dinfo->cur_accum & 0xFF);
    dinfo->cur_accum >>= 8;
    dinfo->cur_bits -= 8;
  }
}




LOCAL(void)
compress_init (gif_dest_ptr dinfo, int i_bits)

{
  
  dinfo->n_bits = i_bits;
  dinfo->maxcode = MAXCODE(dinfo->n_bits);
  dinfo->ClearCode = (1 << (i_bits - 1));
  dinfo->EOFCode = dinfo->ClearCode + 1;
  dinfo->code_counter = dinfo->ClearCode + 2;
  
  dinfo->bytesinpkt = 0;
  dinfo->cur_accum = 0;
  dinfo->cur_bits = 0;
  
  output(dinfo, dinfo->ClearCode);
}


LOCAL(void)
compress_pixel (gif_dest_ptr dinfo, int c)

{
  
  output(dinfo, c);
  
  if (dinfo->code_counter < dinfo->maxcode) {
    dinfo->code_counter++;
  } else {
    output(dinfo, dinfo->ClearCode);
    dinfo->code_counter = dinfo->ClearCode + 2; 
  }
}


LOCAL(void)
compress_term (gif_dest_ptr dinfo)

{
  
  output(dinfo, dinfo->EOFCode);
  
  if (dinfo->cur_bits > 0) {
    CHAR_OUT(dinfo, dinfo->cur_accum & 0xFF);
  }
  
  flush_packet(dinfo);
}





LOCAL(void)
put_word (gif_dest_ptr dinfo, unsigned int w)

{
  putc(w & 0xFF, dinfo->pub.output_file);
  putc((w >> 8) & 0xFF, dinfo->pub.output_file);
}


LOCAL(void)
put_3bytes (gif_dest_ptr dinfo, int val)

{
  putc(val, dinfo->pub.output_file);
  putc(val, dinfo->pub.output_file);
  putc(val, dinfo->pub.output_file);
}


LOCAL(void)
emit_header (gif_dest_ptr dinfo, int num_colors, JSAMPARRAY colormap)


{
  int BitsPerPixel, ColorMapSize, InitCodeSize, FlagByte;
  int cshift = dinfo->cinfo->data_precision - 8;
  int i;

  if (num_colors > 256)
    ERREXIT1(dinfo->cinfo, JERR_TOO_MANY_COLORS, num_colors);
  
  BitsPerPixel = 1;
  while (num_colors > (1 << BitsPerPixel))
    BitsPerPixel++;
  ColorMapSize = 1 << BitsPerPixel;
  if (BitsPerPixel <= 1)
    InitCodeSize = 2;
  else
    InitCodeSize = BitsPerPixel;
  
  putc('G', dinfo->pub.output_file);
  putc('I', dinfo->pub.output_file);
  putc('F', dinfo->pub.output_file);
  putc('8', dinfo->pub.output_file);
  putc('7', dinfo->pub.output_file);
  putc('a', dinfo->pub.output_file);
  
  put_word(dinfo, (unsigned int) dinfo->cinfo->output_width);
  put_word(dinfo, (unsigned int) dinfo->cinfo->output_height);
  FlagByte = 0x80;              
  FlagByte |= (BitsPerPixel-1) << 4; 
  FlagByte |= (BitsPerPixel-1); 
  putc(FlagByte, dinfo->pub.output_file);
  putc(0, dinfo->pub.output_file); 
  putc(0, dinfo->pub.output_file); 
  
  
  
  for (i=0; i < ColorMapSize; i++) {
    if (i < num_colors) {
      if (colormap != NULL) {
        if (dinfo->cinfo->out_color_space == JCS_RGB) {
          
          putc(GETJSAMPLE(colormap[0][i]) >> cshift, dinfo->pub.output_file);
          putc(GETJSAMPLE(colormap[1][i]) >> cshift, dinfo->pub.output_file);
          putc(GETJSAMPLE(colormap[2][i]) >> cshift, dinfo->pub.output_file);
        } else {
          
          put_3bytes(dinfo, GETJSAMPLE(colormap[0][i]) >> cshift);
        }
      } else {
        
        put_3bytes(dinfo, (i * 255 + (num_colors-1)/2) / (num_colors-1));
      }
    } else {
      
      put_3bytes(dinfo, 0);
    }
  }
  
  putc(',', dinfo->pub.output_file); 
  put_word(dinfo, 0);           
  put_word(dinfo, 0);
  put_word(dinfo, (unsigned int) dinfo->cinfo->output_width); 
  put_word(dinfo, (unsigned int) dinfo->cinfo->output_height);
  
  putc(0x00, dinfo->pub.output_file);
  
  putc(InitCodeSize, dinfo->pub.output_file);

  
  compress_init(dinfo, InitCodeSize+1);
}




METHODDEF(void)
start_output_gif (j_decompress_ptr cinfo, djpeg_dest_ptr dinfo)
{
  gif_dest_ptr dest = (gif_dest_ptr) dinfo;

  if (cinfo->quantize_colors)
    emit_header(dest, cinfo->actual_number_of_colors, cinfo->colormap);
  else
    emit_header(dest, 256, (JSAMPARRAY) NULL);
}




METHODDEF(void)
put_pixel_rows (j_decompress_ptr cinfo, djpeg_dest_ptr dinfo,
                JDIMENSION rows_supplied)
{
  gif_dest_ptr dest = (gif_dest_ptr) dinfo;
  register JSAMPROW ptr;
  register JDIMENSION col;

  ptr = dest->pub.buffer[0];
  for (col = cinfo->output_width; col > 0; col--) {
    compress_pixel(dest, GETJSAMPLE(*ptr++));
  }
}




METHODDEF(void)
finish_output_gif (j_decompress_ptr cinfo, djpeg_dest_ptr dinfo)
{
  gif_dest_ptr dest = (gif_dest_ptr) dinfo;

  
  compress_term(dest);
  
  putc(0, dest->pub.output_file);
  
  putc(';', dest->pub.output_file);
  
  fflush(dest->pub.output_file);
  if (ferror(dest->pub.output_file))
    ERREXIT(cinfo, JERR_FILE_WRITE);
}




GLOBAL(djpeg_dest_ptr)
jinit_write_gif (j_decompress_ptr cinfo)
{
  gif_dest_ptr dest;

  
  dest = (gif_dest_ptr)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
                                  sizeof(gif_dest_struct));
  dest->cinfo = cinfo;          
  dest->pub.start_output = start_output_gif;
  dest->pub.put_pixel_rows = put_pixel_rows;
  dest->pub.finish_output = finish_output_gif;

  if (cinfo->out_color_space != JCS_GRAYSCALE &&
      cinfo->out_color_space != JCS_RGB)
    ERREXIT(cinfo, JERR_GIF_COLORSPACE);

  
  if (cinfo->out_color_space != JCS_GRAYSCALE || cinfo->data_precision > 8) {
    
    cinfo->quantize_colors = TRUE;
    if (cinfo->desired_number_of_colors > 256)
      cinfo->desired_number_of_colors = 256;
  }

  
  jpeg_calc_output_dimensions(cinfo);

  if (cinfo->output_components != 1) 
    ERREXIT(cinfo, JERR_GIF_BUG);

  
  dest->pub.buffer = (*cinfo->mem->alloc_sarray)
    ((j_common_ptr) cinfo, JPOOL_IMAGE, cinfo->output_width, (JDIMENSION) 1);
  dest->pub.buffer_height = 1;

  return (djpeg_dest_ptr) dest;
}

#endif 
