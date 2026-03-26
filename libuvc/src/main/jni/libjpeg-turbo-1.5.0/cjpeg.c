

#include "cdjpeg.h"             
#include "jversion.h"           
#include "jconfigint.h"

#ifdef USE_CCOMMAND             
#ifdef __MWERKS__
#include <SIOUX.h>              
#include <console.h>            
#endif
#ifdef THINK_C
#include <console.h>            
#endif
#endif




#define JMESSAGE(code,string)   string ,

static const char * const cdjpeg_message_table[] = {
#include "cderror.h"
  NULL
};




static boolean is_targa;        


LOCAL(cjpeg_source_ptr)
select_file_type (j_compress_ptr cinfo, FILE *infile)
{
  int c;

  if (is_targa) {
#ifdef TARGA_SUPPORTED
    return jinit_read_targa(cinfo);
#else
    ERREXIT(cinfo, JERR_TGA_NOTCOMP);
#endif
  }

  if ((c = getc(infile)) == EOF)
    ERREXIT(cinfo, JERR_INPUT_EMPTY);
  if (ungetc(c, infile) == EOF)
    ERREXIT(cinfo, JERR_UNGETC_FAILED);

  switch (c) {
#ifdef BMP_SUPPORTED
  case 'B':
    return jinit_read_bmp(cinfo);
#endif
#ifdef GIF_SUPPORTED
  case 'G':
    return jinit_read_gif(cinfo);
#endif
#ifdef PPM_SUPPORTED
  case 'P':
    return jinit_read_ppm(cinfo);
#endif
#ifdef RLE_SUPPORTED
  case 'R':
    return jinit_read_rle(cinfo);
#endif
#ifdef TARGA_SUPPORTED
  case 0x00:
    return jinit_read_targa(cinfo);
#endif
  default:
    ERREXIT(cinfo, JERR_UNKNOWN_FORMAT);
    break;
  }

  return NULL;                  
}





static const char *progname;    
static char *outfilename;       
boolean memdst;                 


LOCAL(void)
usage (void)

{
  fprintf(stderr, "usage: %s [switches] ", progname);
#ifdef TWO_FILE_COMMANDLINE
  fprintf(stderr, "inputfile outputfile\n");
#else
  fprintf(stderr, "[inputfile]\n");
#endif

  fprintf(stderr, "Switches (names may be abbreviated):\n");
  fprintf(stderr, "  -quality N[,...]   Compression quality (0..100; 5-95 is most useful range,\n");
  fprintf(stderr, "                     default is 75)\n");
  fprintf(stderr, "  -grayscale     Create monochrome JPEG file\n");
  fprintf(stderr, "  -rgb           Create RGB JPEG file\n");
#ifdef ENTROPY_OPT_SUPPORTED
  fprintf(stderr, "  -optimize      Optimize Huffman table (smaller file, but slow compression)\n");
#endif
#ifdef C_PROGRESSIVE_SUPPORTED
  fprintf(stderr, "  -progressive   Create progressive JPEG file\n");
#endif
#ifdef TARGA_SUPPORTED
  fprintf(stderr, "  -targa         Input file is Targa format (usually not needed)\n");
#endif
  fprintf(stderr, "Switches for advanced users:\n");
#ifdef C_ARITH_CODING_SUPPORTED
  fprintf(stderr, "  -arithmetic    Use arithmetic coding\n");
#endif
#ifdef DCT_ISLOW_SUPPORTED
  fprintf(stderr, "  -dct int       Use integer DCT method%s\n",
          (JDCT_DEFAULT == JDCT_ISLOW ? " (default)" : ""));
#endif
#ifdef DCT_IFAST_SUPPORTED
  fprintf(stderr, "  -dct fast      Use fast integer DCT (less accurate)%s\n",
          (JDCT_DEFAULT == JDCT_IFAST ? " (default)" : ""));
#endif
#ifdef DCT_FLOAT_SUPPORTED
  fprintf(stderr, "  -dct float     Use floating-point DCT method%s\n",
          (JDCT_DEFAULT == JDCT_FLOAT ? " (default)" : ""));
#endif
  fprintf(stderr, "  -restart N     Set restart interval in rows, or in blocks with B\n");
#ifdef INPUT_SMOOTHING_SUPPORTED
  fprintf(stderr, "  -smooth N      Smooth dithered input (N=1..100 is strength)\n");
#endif
  fprintf(stderr, "  -maxmemory N   Maximum memory to use (in kbytes)\n");
  fprintf(stderr, "  -outfile name  Specify name for output file\n");
#if JPEG_LIB_VERSION >= 80 || defined(MEM_SRCDST_SUPPORTED)
  fprintf(stderr, "  -memdst        Compress to memory instead of file (useful for benchmarking)\n");
#endif
  fprintf(stderr, "  -verbose  or  -debug   Emit debug output\n");
  fprintf(stderr, "  -version       Print version information and exit\n");
  fprintf(stderr, "Switches for wizards:\n");
  fprintf(stderr, "  -baseline      Force baseline quantization tables\n");
  fprintf(stderr, "  -qtables file  Use quantization tables given in file\n");
  fprintf(stderr, "  -qslots N[,...]    Set component quantization tables\n");
  fprintf(stderr, "  -sample HxV[,...]  Set component sampling factors\n");
#ifdef C_MULTISCAN_FILES_SUPPORTED
  fprintf(stderr, "  -scans file    Create multi-scan JPEG per script file\n");
#endif
  exit(EXIT_FAILURE);
}


LOCAL(int)
parse_switches (j_compress_ptr cinfo, int argc, char **argv,
                int last_file_arg_seen, boolean for_real)

{
  int argn;
  char *arg;
  boolean force_baseline;
  boolean simple_progressive;
  char *qualityarg = NULL;      
  char *qtablefile = NULL;      
  char *qslotsarg = NULL;       
  char *samplearg = NULL;       
  char *scansarg = NULL;        

  

  force_baseline = FALSE;       
  simple_progressive = FALSE;
  is_targa = FALSE;
  outfilename = NULL;
  memdst = FALSE;
  cinfo->err->trace_level = 0;

  

  for (argn = 1; argn < argc; argn++) {
    arg = argv[argn];
    if (*arg != '-') {
      
      if (argn <= last_file_arg_seen) {
        outfilename = NULL;     
        continue;               
      }
      break;                    
    }
    arg++;                      

    if (keymatch(arg, "arithmetic", 1)) {
      
#ifdef C_ARITH_CODING_SUPPORTED
      cinfo->arith_code = TRUE;
#else
      fprintf(stderr, "%s: sorry, arithmetic coding not supported\n",
              progname);
      exit(EXIT_FAILURE);
#endif

    } else if (keymatch(arg, "baseline", 1)) {
      
      force_baseline = TRUE;

    } else if (keymatch(arg, "dct", 2)) {
      
      if (++argn >= argc)       
        usage();
      if (keymatch(argv[argn], "int", 1)) {
        cinfo->dct_method = JDCT_ISLOW;
      } else if (keymatch(argv[argn], "fast", 2)) {
        cinfo->dct_method = JDCT_IFAST;
      } else if (keymatch(argv[argn], "float", 2)) {
        cinfo->dct_method = JDCT_FLOAT;
      } else
        usage();

    } else if (keymatch(arg, "debug", 1) || keymatch(arg, "verbose", 1)) {
      
      
      static boolean printed_version = FALSE;

      if (! printed_version) {
        fprintf(stderr, "%s version %s (build %s)\n",
                PACKAGE_NAME, VERSION, BUILD);
        fprintf(stderr, "%s\n\n", JCOPYRIGHT);
        fprintf(stderr, "Emulating The Independent JPEG Group's software, version %s\n\n",
                JVERSION);
        printed_version = TRUE;
      }
      cinfo->err->trace_level++;

    } else if (keymatch(arg, "version", 4)) {
      fprintf(stderr, "%s version %s (build %s)\n",
              PACKAGE_NAME, VERSION, BUILD);
      exit(EXIT_SUCCESS);

    } else if (keymatch(arg, "grayscale", 2) || keymatch(arg, "greyscale",2)) {
      
      jpeg_set_colorspace(cinfo, JCS_GRAYSCALE);

    } else if (keymatch(arg, "rgb", 3)) {
      
      jpeg_set_colorspace(cinfo, JCS_RGB);

    } else if (keymatch(arg, "maxmemory", 3)) {
      
      long lval;
      char ch = 'x';

      if (++argn >= argc)       
        usage();
      if (sscanf(argv[argn], "%ld%c", &lval, &ch) < 1)
        usage();
      if (ch == 'm' || ch == 'M')
        lval *= 1000L;
      cinfo->mem->max_memory_to_use = lval * 1000L;

    } else if (keymatch(arg, "optimize", 1) || keymatch(arg, "optimise", 1)) {
      
#ifdef ENTROPY_OPT_SUPPORTED
      cinfo->optimize_coding = TRUE;
#else
      fprintf(stderr, "%s: sorry, entropy optimization was not compiled in\n",
              progname);
      exit(EXIT_FAILURE);
#endif

    } else if (keymatch(arg, "outfile", 4)) {
      
      if (++argn >= argc)       
        usage();
      outfilename = argv[argn]; 

    } else if (keymatch(arg, "progressive", 1)) {
      
#ifdef C_PROGRESSIVE_SUPPORTED
      simple_progressive = TRUE;
      
#else
      fprintf(stderr, "%s: sorry, progressive output was not compiled in\n",
              progname);
      exit(EXIT_FAILURE);
#endif

    } else if (keymatch(arg, "memdst", 2)) {
      
#if JPEG_LIB_VERSION >= 80 || defined(MEM_SRCDST_SUPPORTED)
      memdst = TRUE;
#else
      fprintf(stderr, "%s: sorry, in-memory destination manager was not compiled in\n",
              progname);
      exit(EXIT_FAILURE);
#endif

    } else if (keymatch(arg, "quality", 1)) {
      
      if (++argn >= argc)       
        usage();
      qualityarg = argv[argn];

    } else if (keymatch(arg, "qslots", 2)) {
      
      if (++argn >= argc)       
        usage();
      qslotsarg = argv[argn];
      

    } else if (keymatch(arg, "qtables", 2)) {
      
      if (++argn >= argc)       
        usage();
      qtablefile = argv[argn];
      

    } else if (keymatch(arg, "restart", 1)) {
      
      long lval;
      char ch = 'x';

      if (++argn >= argc)       
        usage();
      if (sscanf(argv[argn], "%ld%c", &lval, &ch) < 1)
        usage();
      if (lval < 0 || lval > 65535L)
        usage();
      if (ch == 'b' || ch == 'B') {
        cinfo->restart_interval = (unsigned int) lval;
        cinfo->restart_in_rows = 0; 
      } else {
        cinfo->restart_in_rows = (int) lval;
        
      }

    } else if (keymatch(arg, "sample", 2)) {
      
      if (++argn >= argc)       
        usage();
      samplearg = argv[argn];
      

    } else if (keymatch(arg, "scans", 4)) {
      
#ifdef C_MULTISCAN_FILES_SUPPORTED
      if (++argn >= argc)       
        usage();
      scansarg = argv[argn];
      
#else
      fprintf(stderr, "%s: sorry, multi-scan output was not compiled in\n",
              progname);
      exit(EXIT_FAILURE);
#endif

    } else if (keymatch(arg, "smooth", 2)) {
      
      int val;

      if (++argn >= argc)       
        usage();
      if (sscanf(argv[argn], "%d", &val) != 1)
        usage();
      if (val < 0 || val > 100)
        usage();
      cinfo->smoothing_factor = val;

    } else if (keymatch(arg, "targa", 1)) {
      
      is_targa = TRUE;

    } else {
      usage();                  
    }
  }

  

  if (for_real) {

    
    
    if (qualityarg != NULL)     
      if (! set_quality_ratings(cinfo, qualityarg, force_baseline))
        usage();

    if (qtablefile != NULL)     
      if (! read_quant_tables(cinfo, qtablefile, force_baseline))
        usage();

    if (qslotsarg != NULL)      
      if (! set_quant_slots(cinfo, qslotsarg))
        usage();

    if (samplearg != NULL)      
      if (! set_sample_factors(cinfo, samplearg))
        usage();

#ifdef C_PROGRESSIVE_SUPPORTED
    if (simple_progressive)     
      jpeg_simple_progression(cinfo);
#endif

#ifdef C_MULTISCAN_FILES_SUPPORTED
    if (scansarg != NULL)       
      if (! read_scan_script(cinfo, scansarg))
        usage();
#endif
  }

  return argn;                  
}




int
main (int argc, char **argv)
{
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;
#ifdef PROGRESS_REPORT
  struct cdjpeg_progress_mgr progress;
#endif
  int file_index;
  cjpeg_source_ptr src_mgr;
  FILE *input_file;
  FILE *output_file = NULL;
  unsigned char *outbuffer = NULL;
  unsigned long outsize = 0;
  JDIMENSION num_scanlines;

  
#ifdef USE_CCOMMAND
  argc = ccommand(&argv);
#endif

  progname = argv[0];
  if (progname == NULL || progname[0] == 0)
    progname = "cjpeg";         

  
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_compress(&cinfo);
  
  jerr.addon_message_table = cdjpeg_message_table;
  jerr.first_addon_message = JMSG_FIRSTADDONCODE;
  jerr.last_addon_message = JMSG_LASTADDONCODE;

  

  cinfo.in_color_space = JCS_RGB; 
  jpeg_set_defaults(&cinfo);

  

  file_index = parse_switches(&cinfo, argc, argv, 0, FALSE);

#ifdef TWO_FILE_COMMANDLINE
  if (!memdst) {
    
    if (outfilename == NULL) {
      if (file_index != argc-2) {
        fprintf(stderr, "%s: must name one input and one output file\n",
                progname);
        usage();
      }
      outfilename = argv[file_index+1];
    } else {
      if (file_index != argc-1) {
        fprintf(stderr, "%s: must name one input and one output file\n",
                progname);
        usage();
      }
    }
  }
#else
  
  if (file_index < argc-1) {
    fprintf(stderr, "%s: only one input file\n", progname);
    usage();
  }
#endif 

  
  if (file_index < argc) {
    if ((input_file = fopen(argv[file_index], READ_BINARY)) == NULL) {
      fprintf(stderr, "%s: can't open %s\n", progname, argv[file_index]);
      exit(EXIT_FAILURE);
    }
  } else {
    
    input_file = read_stdin();
  }

  
  if (outfilename != NULL) {
    if ((output_file = fopen(outfilename, WRITE_BINARY)) == NULL) {
      fprintf(stderr, "%s: can't open %s\n", progname, outfilename);
      exit(EXIT_FAILURE);
    }
  } else if (!memdst) {
    
    output_file = write_stdout();
  }

#ifdef PROGRESS_REPORT
  start_progress_monitor((j_common_ptr) &cinfo, &progress);
#endif

  
  src_mgr = select_file_type(&cinfo, input_file);
  src_mgr->input_file = input_file;

  
  (*src_mgr->start_input) (&cinfo, src_mgr);

  
  jpeg_default_colorspace(&cinfo);

  
  file_index = parse_switches(&cinfo, argc, argv, 0, TRUE);

  
#if JPEG_LIB_VERSION >= 80 || defined(MEM_SRCDST_SUPPORTED)
  if (memdst)
    jpeg_mem_dest(&cinfo, &outbuffer, &outsize);
  else
#endif
    jpeg_stdio_dest(&cinfo, output_file);

  
  jpeg_start_compress(&cinfo, TRUE);

  
  while (cinfo.next_scanline < cinfo.image_height) {
    num_scanlines = (*src_mgr->get_pixel_rows) (&cinfo, src_mgr);
    (void) jpeg_write_scanlines(&cinfo, src_mgr->buffer, num_scanlines);
  }

  
  (*src_mgr->finish_input) (&cinfo, src_mgr);
  jpeg_finish_compress(&cinfo);
  jpeg_destroy_compress(&cinfo);

  
  if (input_file != stdin)
    fclose(input_file);
  if (output_file != stdout && output_file != NULL)
    fclose(output_file);

#ifdef PROGRESS_REPORT
  end_progress_monitor((j_common_ptr) &cinfo);
#endif

  if (memdst) {
    fprintf(stderr, "Compressed size:  %lu bytes\n", outsize);
    if (outbuffer != NULL)
      free(outbuffer);
  }

  
  exit(jerr.num_warnings ? EXIT_WARNING : EXIT_SUCCESS);
  return 0;                     
}
