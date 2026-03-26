

#include "cdjpeg.h"             

#ifdef QUANT_2PASS_SUPPORTED    






LOCAL(void)
add_map_entry (j_decompress_ptr cinfo, int R, int G, int B)
{
  JSAMPROW colormap0 = cinfo->colormap[0];
  JSAMPROW colormap1 = cinfo->colormap[1];
  JSAMPROW colormap2 = cinfo->colormap[2];
  int ncolors = cinfo->actual_number_of_colors;
  int index;

  
  for (index = 0; index < ncolors; index++) {
    if (GETJSAMPLE(colormap0[index]) == R &&
        GETJSAMPLE(colormap1[index]) == G &&
        GETJSAMPLE(colormap2[index]) == B)
      return;                   
  }

  
  if (ncolors >= (MAXJSAMPLE+1))
    ERREXIT1(cinfo, JERR_QUANT_MANY_COLORS, (MAXJSAMPLE+1));

  
  colormap0[ncolors] = (JSAMPLE) R;
  colormap1[ncolors] = (JSAMPLE) G;
  colormap2[ncolors] = (JSAMPLE) B;
  cinfo->actual_number_of_colors++;
}




LOCAL(void)
read_gif_map (j_decompress_ptr cinfo, FILE *infile)
{
  int header[13];
  int i, colormaplen;
  int R, G, B;

  
  
  for (i = 1; i < 13; i++) {
    if ((header[i] = getc(infile)) == EOF)
      ERREXIT(cinfo, JERR_BAD_CMAP_FILE);
  }

  
  if (header[1] != 'I' || header[2] != 'F')
    ERREXIT(cinfo, JERR_BAD_CMAP_FILE);

  
  if ((header[10] & 0x80) == 0)
    ERREXIT(cinfo, JERR_BAD_CMAP_FILE);

  
  colormaplen = 2 << (header[10] & 0x07);

  for (i = 0; i < colormaplen; i++) {
    R = getc(infile);
    G = getc(infile);
    B = getc(infile);
    if (R == EOF || G == EOF || B == EOF)
      ERREXIT(cinfo, JERR_BAD_CMAP_FILE);
    add_map_entry(cinfo,
                  R << (BITS_IN_JSAMPLE-8),
                  G << (BITS_IN_JSAMPLE-8),
                  B << (BITS_IN_JSAMPLE-8));
  }
}





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
read_pbm_integer (j_decompress_ptr cinfo, FILE *infile)




{
  register int ch;
  register unsigned int val;

  
  do {
    ch = pbm_getc(infile);
    if (ch == EOF)
      ERREXIT(cinfo, JERR_BAD_CMAP_FILE);
  } while (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r');

  if (ch < '0' || ch > '9')
    ERREXIT(cinfo, JERR_BAD_CMAP_FILE);

  val = ch - '0';
  while ((ch = pbm_getc(infile)) >= '0' && ch <= '9') {
    val *= 10;
    val += ch - '0';
  }
  return val;
}




LOCAL(void)
read_ppm_map (j_decompress_ptr cinfo, FILE *infile)
{
  int c;
  unsigned int w, h, maxval, row, col;
  int R, G, B;

  
  c = getc(infile);             

  
  w = read_pbm_integer(cinfo, infile);
  h = read_pbm_integer(cinfo, infile);
  maxval = read_pbm_integer(cinfo, infile);

  if (w <= 0 || h <= 0 || maxval <= 0) 
    ERREXIT(cinfo, JERR_BAD_CMAP_FILE);

  
  if (maxval != (unsigned int) MAXJSAMPLE)
    ERREXIT(cinfo, JERR_BAD_CMAP_FILE);

  switch (c) {
  case '3':                     
    for (row = 0; row < h; row++) {
      for (col = 0; col < w; col++) {
        R = read_pbm_integer(cinfo, infile);
        G = read_pbm_integer(cinfo, infile);
        B = read_pbm_integer(cinfo, infile);
        add_map_entry(cinfo, R, G, B);
      }
    }
    break;

  case '6':                     
    for (row = 0; row < h; row++) {
      for (col = 0; col < w; col++) {
        R = getc(infile);
        G = getc(infile);
        B = getc(infile);
        if (R == EOF || G == EOF || B == EOF)
          ERREXIT(cinfo, JERR_BAD_CMAP_FILE);
        add_map_entry(cinfo, R, G, B);
      }
    }
    break;

  default:
    ERREXIT(cinfo, JERR_BAD_CMAP_FILE);
    break;
  }
}




GLOBAL(void)
read_color_map (j_decompress_ptr cinfo, FILE *infile)
{
  
  cinfo->colormap = (*cinfo->mem->alloc_sarray)
    ((j_common_ptr) cinfo, JPOOL_IMAGE,
     (JDIMENSION) (MAXJSAMPLE+1), (JDIMENSION) 3);
  cinfo->actual_number_of_colors = 0; 

  
  switch (getc(infile)) {
  case 'G':
    read_gif_map(cinfo, infile);
    break;
  case 'P':
    read_ppm_map(cinfo, infile);
    break;
  default:
    ERREXIT(cinfo, JERR_BAD_CMAP_FILE);
    break;
  }
}

#endif 
