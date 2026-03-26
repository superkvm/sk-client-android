

#define JPEG_CJPEG_DJPEG        
#include "jinclude.h"           

#ifdef HAVE_LOCALE_H
#include <locale.h>             
#endif
#include <ctype.h>              
#ifdef USE_SETMODE
#include <fcntl.h>              

#include <io.h>                 
#endif

#ifdef USE_CCOMMAND             
#ifdef __MWERKS__
#include <SIOUX.h>              
#include <console.h>            
#endif
#ifdef THINK_C
#include <console.h>            
#endif
#endif

#ifdef DONT_USE_B_MODE          
#define READ_BINARY     "r"
#else
#define READ_BINARY     "rb"
#endif

#ifndef EXIT_FAILURE            
#define EXIT_FAILURE  1
#endif
#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS  0
#endif




static FILE *infile;            


#define NEXTBYTE()  getc(infile)



#define ERREXIT(msg)  (fprintf(stderr, "%s\n", msg), exit(EXIT_FAILURE))



static int
read_1_byte (void)
{
  int c;

  c = NEXTBYTE();
  if (c == EOF)
    ERREXIT("Premature EOF in JPEG file");
  return c;
}



static unsigned int
read_2_bytes (void)
{
  int c1, c2;

  c1 = NEXTBYTE();
  if (c1 == EOF)
    ERREXIT("Premature EOF in JPEG file");
  c2 = NEXTBYTE();
  if (c2 == EOF)
    ERREXIT("Premature EOF in JPEG file");
  return (((unsigned int) c1) << 8) + ((unsigned int) c2);
}




#define M_SOF0  0xC0            
#define M_SOF1  0xC1            
#define M_SOF2  0xC2            
#define M_SOF3  0xC3
#define M_SOF5  0xC5            
#define M_SOF6  0xC6
#define M_SOF7  0xC7
#define M_SOF9  0xC9
#define M_SOF10 0xCA
#define M_SOF11 0xCB
#define M_SOF13 0xCD
#define M_SOF14 0xCE
#define M_SOF15 0xCF
#define M_SOI   0xD8            
#define M_EOI   0xD9            
#define M_SOS   0xDA            
#define M_APP0  0xE0            
#define M_APP12 0xEC            
#define M_COM   0xFE            




static int
next_marker (void)
{
  int c;
  int discarded_bytes = 0;

  
  c = read_1_byte();
  while (c != 0xFF) {
    discarded_bytes++;
    c = read_1_byte();
  }
  
  do {
    c = read_1_byte();
  } while (c == 0xFF);

  if (discarded_bytes != 0) {
    fprintf(stderr, "Warning: garbage data found in JPEG file\n");
  }

  return c;
}




static int
first_marker (void)
{
  int c1, c2;

  c1 = NEXTBYTE();
  c2 = NEXTBYTE();
  if (c1 != 0xFF || c2 != M_SOI)
    ERREXIT("Not a JPEG file");
  return c2;
}




static void
skip_variable (void)

{
  unsigned int length;

  
  length = read_2_bytes();
  
  if (length < 2)
    ERREXIT("Erroneous JPEG marker length");
  length -= 2;
  
  while (length > 0) {
    (void) read_1_byte();
    length--;
  }
}




static void
process_COM (int raw)
{
  unsigned int length;
  int ch;
  int lastch = 0;

  
#ifdef HAVE_LOCALE_H
  setlocale(LC_CTYPE, "");
#endif

  
  length = read_2_bytes();
  
  if (length < 2)
    ERREXIT("Erroneous JPEG marker length");
  length -= 2;

  while (length > 0) {
    ch = read_1_byte();
    if (raw) {
      putc(ch, stdout);
    
    } else if (ch == '\r') {
      printf("\n");
    } else if (ch == '\n') {
      if (lastch != '\r')
        printf("\n");
    } else if (ch == '\\') {
      printf("\\\\");
    } else if (isprint(ch)) {
      putc(ch, stdout);
    } else {
      printf("\\%03o", ch);
    }
    lastch = ch;
    length--;
  }
  printf("\n");

  
#ifdef HAVE_LOCALE_H
  setlocale(LC_CTYPE, "C");
#endif
}




static void
process_SOFn (int marker)
{
  unsigned int length;
  unsigned int image_height, image_width;
  int data_precision, num_components;
  const char *process;
  int ci;

  length = read_2_bytes();      

  data_precision = read_1_byte();
  image_height = read_2_bytes();
  image_width = read_2_bytes();
  num_components = read_1_byte();

  switch (marker) {
  case M_SOF0:  process = "Baseline";  break;
  case M_SOF1:  process = "Extended sequential";  break;
  case M_SOF2:  process = "Progressive";  break;
  case M_SOF3:  process = "Lossless";  break;
  case M_SOF5:  process = "Differential sequential";  break;
  case M_SOF6:  process = "Differential progressive";  break;
  case M_SOF7:  process = "Differential lossless";  break;
  case M_SOF9:  process = "Extended sequential, arithmetic coding";  break;
  case M_SOF10: process = "Progressive, arithmetic coding";  break;
  case M_SOF11: process = "Lossless, arithmetic coding";  break;
  case M_SOF13: process = "Differential sequential, arithmetic coding";  break;
  case M_SOF14: process = "Differential progressive, arithmetic coding"; break;
  case M_SOF15: process = "Differential lossless, arithmetic coding";  break;
  default:      process = "Unknown";  break;
  }

  printf("JPEG image is %uw * %uh, %d color components, %d bits per sample\n",
         image_width, image_height, num_components, data_precision);
  printf("JPEG process: %s\n", process);

  if (length != (unsigned int) (8 + num_components * 3))
    ERREXIT("Bogus SOF marker length");

  for (ci = 0; ci < num_components; ci++) {
    (void) read_1_byte();       
    (void) read_1_byte();       
    (void) read_1_byte();       
  }
}




static int
scan_JPEG_header (int verbose, int raw)
{
  int marker;

  
  if (first_marker() != M_SOI)
    ERREXIT("Expected SOI marker first");

  
  for (;;) {
    marker = next_marker();
    switch (marker) {
      
    case M_SOF0:                
    case M_SOF1:                
    case M_SOF2:                
    case M_SOF3:                
    case M_SOF5:                
    case M_SOF6:                
    case M_SOF7:                
    case M_SOF9:                
    case M_SOF10:               
    case M_SOF11:               
    case M_SOF13:               
    case M_SOF14:               
    case M_SOF15:               
      if (verbose)
        process_SOFn(marker);
      else
        skip_variable();
      break;

    case M_SOS:                 
      return marker;

    case M_EOI:                 
      return marker;

    case M_COM:
      process_COM(raw);
      break;

    case M_APP12:
      
      if (verbose) {
        printf("APP12 contains:\n");
        process_COM(raw);
      } else
        skip_variable();
      break;

    default:                    
      skip_variable();          
      break;
    }
  } 
}




static const char *progname;    


static void
usage (void)

{
  fprintf(stderr, "rdjpgcom displays any textual comments in a JPEG file.\n");

  fprintf(stderr, "Usage: %s [switches] [inputfile]\n", progname);

  fprintf(stderr, "Switches (names may be abbreviated):\n");
  fprintf(stderr, "  -raw        Display non-printable characters in comments (unsafe)\n");
  fprintf(stderr, "  -verbose    Also display dimensions of JPEG image\n");

  exit(EXIT_FAILURE);
}


static int
keymatch (char *arg, const char *keyword, int minchars)



{
  register int ca, ck;
  register int nmatched = 0;

  while ((ca = *arg++) != '\0') {
    if ((ck = *keyword++) == '\0')
      return 0;                 
    if (isupper(ca))            
      ca = tolower(ca);
    if (ca != ck)
      return 0;                 
    nmatched++;                 
  }
  
  if (nmatched < minchars)
    return 0;
  return 1;                     
}




int
main (int argc, char **argv)
{
  int argn;
  char *arg;
  int verbose = 0, raw = 0;

  
#ifdef USE_CCOMMAND
  argc = ccommand(&argv);
#endif

  progname = argv[0];
  if (progname == NULL || progname[0] == 0)
    progname = "rdjpgcom";      

  
  for (argn = 1; argn < argc; argn++) {
    arg = argv[argn];
    if (arg[0] != '-')
      break;                    
    arg++;                      
    if (keymatch(arg, "verbose", 1)) {
      verbose++;
    } else if (keymatch(arg, "raw", 1)) {
      raw = 1;
    } else
      usage();
  }

  
  
  if (argn < argc-1) {
    fprintf(stderr, "%s: only one input file\n", progname);
    usage();
  }
  if (argn < argc) {
    if ((infile = fopen(argv[argn], READ_BINARY)) == NULL) {
      fprintf(stderr, "%s: can't open %s\n", progname, argv[argn]);
      exit(EXIT_FAILURE);
    }
  } else {
    
#ifdef USE_SETMODE              
    setmode(fileno(stdin), O_BINARY);
#endif
#ifdef USE_FDOPEN               
    if ((infile = fdopen(fileno(stdin), READ_BINARY)) == NULL) {
      fprintf(stderr, "%s: can't open stdin\n", progname);
      exit(EXIT_FAILURE);
    }
#else
    infile = stdin;
#endif
  }

  
  (void) scan_JPEG_header(verbose, raw);

  
  exit(EXIT_SUCCESS);
  return 0;                     
}
