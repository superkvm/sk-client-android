

#define JPEG_CJPEG_DJPEG        
#include "jinclude.h"           

#ifndef HAVE_STDLIB_H           
extern void *malloc ();
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
#define WRITE_BINARY    "w"
#else
#define READ_BINARY     "rb"
#define WRITE_BINARY    "wb"
#endif

#ifndef EXIT_FAILURE            
#define EXIT_FAILURE  1
#endif
#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS  0
#endif



#ifndef MAX_COM_LENGTH
#define MAX_COM_LENGTH 65000L   
#endif




static FILE *infile;            


#define NEXTBYTE()  getc(infile)

static FILE *outfile;           


#define PUTBYTE(x)  putc((x), outfile)



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




static void
write_1_byte (int c)
{
  PUTBYTE(c);
}

static void
write_2_bytes (unsigned int val)
{
  PUTBYTE((val >> 8) & 0xFF);
  PUTBYTE(val & 0xFF);
}

static void
write_marker (int marker)
{
  PUTBYTE(0xFF);
  PUTBYTE(marker);
}

static void
copy_rest_of_file (void)
{
  int c;

  while ((c = NEXTBYTE()) != EOF)
    PUTBYTE(c);
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
copy_variable (void)

{
  unsigned int length;

  
  length = read_2_bytes();
  write_2_bytes(length);
  
  if (length < 2)
    ERREXIT("Erroneous JPEG marker length");
  length -= 2;
  
  while (length > 0) {
    write_1_byte(read_1_byte());
    length--;
  }
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




static int
scan_JPEG_header (int keep_COM)
{
  int marker;

  
  if (first_marker() != M_SOI)
    ERREXIT("Expected SOI marker first");
  write_marker(M_SOI);

  
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
      return marker;

    case M_SOS:                 
      ERREXIT("SOS without prior SOFn");
      break;

    case M_EOI:                 
      return marker;

    case M_COM:                 
      if (keep_COM) {
        write_marker(marker);
        copy_variable();
      } else {
        skip_variable();
      }
      break;

    default:                    
      write_marker(marker);
      copy_variable();          
      break;
    }
  } 
}




static const char *progname;    


static void
usage (void)

{
  fprintf(stderr, "wrjpgcom inserts a textual comment in a JPEG file.\n");
  fprintf(stderr, "You can add to or replace any existing comment(s).\n");

  fprintf(stderr, "Usage: %s [switches] ", progname);
#ifdef TWO_FILE_COMMANDLINE
  fprintf(stderr, "inputfile outputfile\n");
#else
  fprintf(stderr, "[inputfile]\n");
#endif

  fprintf(stderr, "Switches (names may be abbreviated):\n");
  fprintf(stderr, "  -replace         Delete any existing comments\n");
  fprintf(stderr, "  -comment \"text\"  Insert comment with given text\n");
  fprintf(stderr, "  -cfile name      Read comment from named file\n");
  fprintf(stderr, "Notice that you must put quotes around the comment text\n");
  fprintf(stderr, "when you use -comment.\n");
  fprintf(stderr, "If you do not give either -comment or -cfile on the command line,\n");
  fprintf(stderr, "then the comment text is read from standard input.\n");
  fprintf(stderr, "It can be multiple lines, up to %u characters total.\n",
          (unsigned int) MAX_COM_LENGTH);
#ifndef TWO_FILE_COMMANDLINE
  fprintf(stderr, "You must specify an input JPEG file name when supplying\n");
  fprintf(stderr, "comment text from standard input.\n");
#endif

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
  int keep_COM = 1;
  char *comment_arg = NULL;
  FILE *comment_file = NULL;
  unsigned int comment_length = 0;
  int marker;

  
#ifdef USE_CCOMMAND
  argc = ccommand(&argv);
#endif

  progname = argv[0];
  if (progname == NULL || progname[0] == 0)
    progname = "wrjpgcom";      

  
  for (argn = 1; argn < argc; argn++) {
    arg = argv[argn];
    if (arg[0] != '-')
      break;                    
    arg++;                      
    if (keymatch(arg, "replace", 1)) {
      keep_COM = 0;
    } else if (keymatch(arg, "cfile", 2)) {
      if (++argn >= argc) usage();
      if ((comment_file = fopen(argv[argn], "r")) == NULL) {
        fprintf(stderr, "%s: can't open %s\n", progname, argv[argn]);
        exit(EXIT_FAILURE);
      }
    } else if (keymatch(arg, "comment", 1)) {
      if (++argn >= argc) usage();
      comment_arg = argv[argn];
      
      if (comment_arg[0] == '"') {
        comment_arg = (char *) malloc((size_t) MAX_COM_LENGTH);
        if (comment_arg == NULL)
          ERREXIT("Insufficient memory");
        if (strlen(argv[argn]) + 2 >= (size_t) MAX_COM_LENGTH) {
          fprintf(stderr, "Comment text may not exceed %u bytes\n",
                  (unsigned int) MAX_COM_LENGTH);
          exit(EXIT_FAILURE);
        }
        strcpy(comment_arg, argv[argn]+1);
        for (;;) {
          comment_length = (unsigned int) strlen(comment_arg);
          if (comment_length > 0 && comment_arg[comment_length-1] == '"') {
            comment_arg[comment_length-1] = '\0'; 
            break;
          }
          if (++argn >= argc)
            ERREXIT("Missing ending quote mark");
          if (strlen(comment_arg) + strlen(argv[argn]) + 2 >=
              (size_t) MAX_COM_LENGTH) {
            fprintf(stderr, "Comment text may not exceed %u bytes\n",
                    (unsigned int) MAX_COM_LENGTH);
            exit(EXIT_FAILURE);
          }
          strcat(comment_arg, " ");
          strcat(comment_arg, argv[argn]);
        }
      } else if (strlen(argv[argn]) >= (size_t) MAX_COM_LENGTH) {
        fprintf(stderr, "Comment text may not exceed %u bytes\n",
                (unsigned int) MAX_COM_LENGTH);
        exit(EXIT_FAILURE);
      }
      comment_length = (unsigned int) strlen(comment_arg);
    } else
      usage();
  }

  
  if (comment_arg != NULL && comment_file != NULL)
    usage();
  
  if (comment_arg == NULL && comment_file == NULL && argn >= argc)
    usage();

  
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

  
#ifdef TWO_FILE_COMMANDLINE
  
  if (argn != argc-2) {
    fprintf(stderr, "%s: must name one input and one output file\n",
            progname);
    usage();
  }
  if ((outfile = fopen(argv[argn+1], WRITE_BINARY)) == NULL) {
    fprintf(stderr, "%s: can't open %s\n", progname, argv[argn+1]);
    exit(EXIT_FAILURE);
  }
#else
  
  if (argn < argc-1) {
    fprintf(stderr, "%s: only one input file\n", progname);
    usage();
  }
  
#ifdef USE_SETMODE              
  setmode(fileno(stdout), O_BINARY);
#endif
#ifdef USE_FDOPEN               
  if ((outfile = fdopen(fileno(stdout), WRITE_BINARY)) == NULL) {
    fprintf(stderr, "%s: can't open stdout\n", progname);
    exit(EXIT_FAILURE);
  }
#else
  outfile = stdout;
#endif
#endif 

  
  if (comment_arg == NULL) {
    FILE *src_file;
    int c;

    comment_arg = (char *) malloc((size_t) MAX_COM_LENGTH);
    if (comment_arg == NULL)
      ERREXIT("Insufficient memory");
    comment_length = 0;
    src_file = (comment_file != NULL ? comment_file : stdin);
    while ((c = getc(src_file)) != EOF) {
      if (comment_length >= (unsigned int) MAX_COM_LENGTH) {
        fprintf(stderr, "Comment text may not exceed %u bytes\n",
                (unsigned int) MAX_COM_LENGTH);
        exit(EXIT_FAILURE);
      }
      comment_arg[comment_length++] = (char) c;
    }
    if (comment_file != NULL)
      fclose(comment_file);
  }

  
  marker = scan_JPEG_header(keep_COM);
  
  if (comment_length > 0) {
    write_marker(M_COM);
    write_2_bytes(comment_length + 2);
    while (comment_length > 0) {
      write_1_byte(*comment_arg++);
      comment_length--;
    }
  }
  
  write_marker(marker);
  copy_rest_of_file();

  
  exit(EXIT_SUCCESS);
  return 0;                     
}
