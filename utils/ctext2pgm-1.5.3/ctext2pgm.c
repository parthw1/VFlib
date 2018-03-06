/* ctext2pgm.c
 *  --- make a bitmap of a text encoded in compound text.
 *      Bitmap is printed in PBM or PGM format.
 *
 * by Hirotsugu Kakugawa
 * 
 * Copyright (C) 1998-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */
/*
 * The following character sets are supported:
 *    ASCII               (English), 
 *    ISO 8859-1,2,3,4,9  (English, German, Italy, French, Spanish, etc)
 *    ISO 8859-5          (Russian)
 *    ISO 8859-7          (Greek)
 *    ISO 8859-8          (Hebrew)
 *    JIS X 0201          (Japanese Roman and and Roman)
 *    JIS X 0208          (Japanese Kanji)
 *    GB 2312             (Chinese Hanzi)
 *    KSC 5601            (Hangle)
 *    Mule Arabic         (Arabic by the Mule editor)
 *    Mule Big 5          (Chinese BIG5 by the Mule editor)
 *    Mule Viscii 1.1     (Vietnamese by the Mule editor)
 *    Mule Ethiopic       (Ethiopic by the Mule editor)
 *
 * The following encodings are supported:
 *    Compound text      
 *    Japanese EUC 
 *    Korean EUC 
 *    Chinese EUC 
 *    Shift JIS 
 *
 *
 * Edition History
 *   5 Jun 1998  Support for character sets ISO8859-1, JIS X0208, KSC 5601, and
 *               GB 2312. Output in PBM format.  Font face selection feature. 
 *   6 Jun 1998  Added font family selection.
 *   7 Jun 1998  Support for output of PGM format and anti-aliasing.
 *   8 Jun 1998  Support for ISO8859-2,3,4,5,7,9 charsets.
 *   9 Jun 1998  Support for JIS X0201-Roman and JIS X0201-Kana charsets.
 *               Fixed bugs in the compound text parser.
 *  11 Jun 1998  Support for EUC-JP, EUC-KR, and EUC-GB, EUC-CNS for input 
 *               text encodings.
 *  12 Jun 1998  Support for right-to-left directionality (e.g., Hebrew)
 *               in left-to-right directionality text. 
 *  13 Jun 1998  Support for right-to-left directionality. ISO8859-8
 *               (Hebrew) charset is supported. Scripts of left-to-right 
 *               directionality in right-to-left script is also supported.
 *               Support for code sets 2 and 4 of EUC-JP encoding. 
 *  15 Jun 1998  Support for Shift-JIS encoding. Added a command line option 
 *               for page width and height specification. 
 *               Added reversed character command in input text.
 *               Support for arabic text file created by Mule.
 *  16 Jun 1998  Added page width/height and center/flush-left/flush-right
 *               features.
 *  18 Jun 1998  Enhanced searcing mechanism for opening fonts. 
 *               Added a feature to print font list.
 *  22 Jun 1998  Added ASCII art output format.
 *  23 Jun 1998  Added EPS and vertical ASCII art output formats.
 *  25 Nov 1998  Image file output code is adopted in VFlib; ctext2pgm
 *               is changed to use image output functions in VFlib.
 *  15 Dec 1998  Added -center-line, -h-center-line, -v-center-line,
 *               -left-line and -right-line options.
 *  22 Apr 1999  Added -bbx option for generating minimum image.
 *  11 Jan 1999  Added Mule-VISCII, Mule-Ethiopic Mule-Big5 charsets
 *  12 Jan 1999  Improved to handle jisx0201-kana designated in G0.
 *  13 Jan 1999  Added tab stop feature.
 */


#include "../../src/config.h"
#include <stdio.h>
#include <stdlib.h>
#if defined(HAVE_STRING_H) || defined(STDC_HEADERS)
#  include  <string.h>
#else
#  include  <strings.h>
#endif

#include <VFlib-3_7.h>

#include "ctext2pgm.h"
#include "fontdef.h"


void  parse_args(int *argcp, char ***argvp);
void  usage(int level);
void  make_text_bitmap(FILE *fp, char *title);
void  parser(FILE *fp, VF_BITMAPLIST page_buff);
void  parser_eol(VF_BITMAPLIST *line_buff_p, VF_BITMAPLIST page_buff, 
		 int vposx, int vposy);
void  parser_check_wdir(int i, VF_BITMAPLIST *line_buff_p);
void  parser_wdir_push(int wdir, VF_BITMAPLIST *line_buff_p);
void  parser_wdir_pop(VF_BITMAPLIST *line_buff_p);
void  parser_wdir_do_push(int wdir, VF_BITMAPLIST *line_buff_p);
void  parser_wdir_do_pop(VF_BITMAPLIST *line_buff_p);
int   charset_wdirection(int charset);
void  parser_cmd(long code_point, VF_BITMAPLIST page_buff, 
		 int *charset_saved_p, int g, int *posxp, int *posyp);
void  parser_init(void);
void  draw_char(long code_point, VF_BITMAPLIST bmlist, int g, 
		int *posxp, int *posyp);
void  put_bitmap(VF_BITMAPLIST buff, VF_BITMAP bm, 
		 int wdir, int *posxp, int *posyp);
long  cp_conv(long code_point, int i);
void  reverse_bitmap(VF_BITMAP bm);
void  swap_refpt_nextpt(VF_BITMAP bm);
void  change_fonts(int g);
int   try_font_open(int table_index);
char* charset_name(int charset, char *if_unknown);
void  show_font_list(void);
void  wprint(char *str, int w);


extern VF_BITMAP  vf_alloc_bitmap(int,int);


int     input_encoding;
int     output_format;

char    *vflibcap;
double  magx, magy;
int     xpixel;
int     pixel;
double  baselineskip;
int     pix_reverse;
int     output_format;
int     minimum_image;
int     shrink_factor;
double  tab_skip;
int     wdirection;
int     page_width, page_height;
int     margin_x, margin_y;
int     line_typeset;
int     image_position_h, image_position_v;
int     default_family;
int     default_face;
double  eps_ptsize;

/* For debugging */

int     debug_state;
int     debug_r2l;
int     debug_font;
int     debug_page_bitmap;
int     debug_line_bitmap;
int     debug_char_bitmap;
int     debug_vflib;


/* For encoding parser: parser states  */
int  current_family;           /* current font family */
int  current_face;             /* current font face */
int  current_reverse;          /* reverse black and white of char */
int  current_font_g[4];        /* current fonts for G0 and G1 */
int  font_exists_g[4];         /* font existence flag for G0 and G1 */
int  current_wdir_g[4];
int   type_g[4];                /* 94/96 of G0/G1 */
int   charset_g[4];             /* charset IDs of G0/G1 */
int   chlen_g[4];               /* bytes per char of G0/G1 */

struct s_dir_stack_elem dir_stack[MAX_DIR_STACK];  /* direction stack */
int  dir_sp;                                       /* stack pointer */
int  nchars_in_line;

/* Parser parameters */
int   use_esc;
int   use_csi;
int   use_si;
int   use_so;
int   use_ss2;
int   use_ss3;
int   use_sjis;
int   use_g1;
int   use_g2;
int   use_g3;



int
main(int argc, char **argv)
{
  FILE   *fp; 
  int     i;

  input_encoding = ENC_DEFAULT;
  output_format  = OFORM_DEFAULT;
  minimum_image  = 0;

  vflibcap         = DEFAULT_VFLIBCAP;
  magx             = DEFAULT_MAG;
  magy             = DEFAULT_MAG;
  pixel            = DEFAULT_PIXEL_SIZE;
  baselineskip     = DEFAULT_BASELINESKIP;
  shrink_factor    = 1;
  tab_skip         = DEFAULT_TAB_SKIP;
  pix_reverse      = DEFAULT_REVERSE;
  wdirection       = WDIR_DEFAULT;
  xpixel           = -1;
  eps_ptsize       = -1;
  margin_x         = DEFAULT_MARGIN;
  margin_y         = DEFAULT_MARGIN;
  page_width       = -1;
  page_height      = -1;
  line_typeset     = DEFAULT_LINE_POS;
  image_position_h = VF_IMAGEOUT_POSITION_NONE;
  image_position_v = VF_IMAGEOUT_POSITION_NONE;

  default_family  = FAM_DEFAULT;
  default_face    = FACE_DEFAULT;
  current_reverse = 0;

  debug_state       = 0;
  debug_r2l         = 0;
  debug_font        = 0;
  debug_page_bitmap = 0;
  debug_line_bitmap = 0;
  debug_char_bitmap = 0;
  debug_vflib       = 0;
 
  argc--; argv++;
  parse_args(&argc, &argv);

  current_family  = default_family;
  current_face    = default_face;

  if (debug_vflib == 1){
    if (vflibcap == NULL)
      printf("VF_Init(NULL, NULL)\n");
    else 
      printf("VF_Init(\"%s\", NULL)\n", vflibcap);
  }

  if (VF_Init(vflibcap, NULL) < 0){
    switch (vf_error){
    case VF_ERR_NO_VFLIBCAP:
      PR2("ctext2pgm: vflibcap is not found: \"%s\".\n",
	  (vflibcap==NULL)?DEFAULT_VFLIBCAP:vflibcap);
      break;
    default:
      PR1("ctext2pgm: failed to initialize.\n");
      break;
    }
    exit(0);
  }

  for (i = 0; font_info[i].font_id >= 0; i++){
    font_info[i].font_id = NOT_OPENED;
  }

  if (argc == 0){
    make_text_bitmap(stdin, "stdin");
  } else {
    if ((fp = fopen(argv[0], "r")) == NULL){
      perror(argv[0]);
      exit(1);
    }
    make_text_bitmap(fp, argv[0]);
    fclose(fp);
  }

  return 0;
}


void
parse_args(int *argcp, char ***argvp)
{
  int  argc; 
  char **argv;

  argc = *argcp; 
  argv = *argvp;

  while ((argc > 0) && (*argv[0] == '-')){
    if (strcmp(argv[0], "-v") == 0){
      vflibcap = argv[1];
      argc--; argv++;
    } else if (strcmp(argv[0], "-m") == 0){
      magx = magy = atof(argv[1]); 
      argc--; argv++;
    } else if (strcmp(argv[0], "-mx") == 0){
      magx = atof(argv[1]);
      argc--; argv++;
    } else if (strcmp(argv[0], "-my") == 0){
      magy = atof(argv[1]);
      argc--; argv++;
    } else if (strcmp(argv[0], "-b") == 0){
      baselineskip = atof(argv[1]);
      if (baselineskip <= 0)
	baselineskip = 1.2;
      argc--; argv++;
    } else if (strcmp(argv[0], "-bbx") == 0){
      minimum_image = 1;
    } else if (strcmp(argv[0], "-g") == 0){
      margin_x = margin_y = atoi(argv[1]);
      argc--; argv++;
    } else if (strcmp(argv[0], "-gx") == 0){
      margin_x = atoi(argv[1]);
      argc--; argv++;
    } else if (strcmp(argv[0], "-gy") == 0){
      margin_y = atoi(argv[1]);
      argc--; argv++;
    } else if (strcmp(argv[0], "-r") == 0){
      pix_reverse = 1;
    } else if (strcmp(argv[0], "-14") == 0){
      pixel = 14;
    } else if (strcmp(argv[0], "-16") == 0){
      pixel = 16;
    } else if (strcmp(argv[0], "-18") == 0){
      pixel = 18;
    } else if (strcmp(argv[0], "-24") == 0){
      pixel = 24;
    } else if (strcmp(argv[0], "-scale") == 0){
      pixel = 0;
      xpixel = atoi(argv[1]);
      argc--; argv++;
    } else if (strcmp(argv[0], "-fixed") == 0){
      default_family = FAM_FIXED;
    } else if (strcmp(argv[0], "-times") == 0){
      default_family = FAM_TIMES;
    } else if (strcmp(argv[0], "-helv") == 0){
      default_family = FAM_HELV;
    } else if (strcmp(argv[0], "-cour") == 0){
      default_family = FAM_COUR;
    } else if (strcmp(argv[0], "-normal") == 0){
      default_face = FACE_NORMAL;
    } else if (strcmp(argv[0], "-bold") == 0){
      default_face = FACE_BOLD;
    } else if (strcmp(argv[0], "-italic") == 0){
      default_face = FACE_ITALIC;
    } else if (strcmp(argv[0], "-ctext") == 0){
      input_encoding = ENC_CTEXT;
    } else if (strcmp(argv[0], "-iso-8859-1") == 0){
      input_encoding = ENC_ISO8859_1;
    } else if (strcmp(argv[0], "-latin-1") == 0){
      input_encoding = ENC_ISO8859_1;
    } else if (strcmp(argv[0], "-iso-8859-2") == 0){
      input_encoding = ENC_ISO8859_2;
    } else if (strcmp(argv[0], "-latin-2") == 0){
      input_encoding = ENC_ISO8859_2;
    } else if (strcmp(argv[0], "-iso-8859-3") == 0){
      input_encoding = ENC_ISO8859_3;
    } else if (strcmp(argv[0], "-latin-3") == 0){
      input_encoding = ENC_ISO8859_3;
    } else if (strcmp(argv[0], "-iso-8859-4") == 0){
      input_encoding = ENC_ISO8859_4;
    } else if (strcmp(argv[0], "-latin-4") == 0){
      input_encoding = ENC_ISO8859_4;
    } else if (strcmp(argv[0], "-iso-8859-5") == 0){
      input_encoding = ENC_ISO8859_5;
    } else if (strcmp(argv[0], "-cyrillic") == 0){
      input_encoding = ENC_ISO8859_5;
    } else if (strcmp(argv[0], "-russian") == 0){
      input_encoding = ENC_ISO8859_5;
    } else if (strcmp(argv[0], "-iso-8859-6") == 0){
      input_encoding = ENC_ISO8859_6;
    } else if (strcmp(argv[0], "-iso-8859-7") == 0){
      input_encoding = ENC_ISO8859_7;
    } else if (strcmp(argv[0], "-greek") == 0){
      input_encoding = ENC_ISO8859_7;
    } else if (strcmp(argv[0], "-iso-8859-8") == 0){
      input_encoding = ENC_ISO8859_8;
    } else if (strcmp(argv[0], "-hebrew") == 0){
      input_encoding = ENC_ISO8859_8;
    } else if (strcmp(argv[0], "-iso-8859-9") == 0){
      input_encoding = ENC_ISO8859_9;
    } else if (strcmp(argv[0], "-iso-2022-jp") == 0){
      input_encoding = ENC_ISO2022_JP;
    } else if (strcmp(argv[0], "-junet") == 0){
      input_encoding = ENC_ISO2022_JP;
#if 0
    } else if (strcmp(argv[0], "-iso-2022-kr") == 0){
      input_encoding = ENC_ISO2022_KR;
    } else if (strcmp(argv[0], "-iso-2022-cn") == 0){
      input_encoding = ENC_ISO2022_CN;
#endif
    } else if (strcmp(argv[0], "-euc-jp") == 0){
      input_encoding = ENC_EUC_JP1;
    } else if (strcmp(argv[0], "-euc-jp1") == 0){
      input_encoding = ENC_EUC_JP1;
    } else if (strcmp(argv[0], "-euc-jp2") == 0){
      input_encoding = ENC_EUC_JP2;
    } else if (strcmp(argv[0], "-euc-kr") == 0){
      input_encoding = ENC_EUC_KR;
    } else if (strcmp(argv[0], "-euc-ch") == 0){
      input_encoding = ENC_EUC_CH_GB;
    } else if (strcmp(argv[0], "-euc-gb") == 0){
      input_encoding = ENC_EUC_CH_GB;
    } else if (strcmp(argv[0], "-euc-cns") == 0){
      input_encoding = ENC_EUC_CH_CNS;
    } else if (strcmp(argv[0], "-sjis") == 0){
      input_encoding = ENC_SJIS;
    } else if (strcmp(argv[0], "-l2r") == 0){
      wdirection = WDIR_L2R;
    } else if (strcmp(argv[0], "-r2l") == 0){
      wdirection = WDIR_R2L;
    } else if (strcmp(argv[0], "-pbm") == 0){
      output_format = OFORM_PBM_ASCII;
    } else if (strcmp(argv[0], "-pgm") == 0){
      output_format = OFORM_PGM_RAW;
    } else if (strcmp(argv[0], "-pbm-ascii") == 0){
      output_format = OFORM_PBM_ASCII;
    } else if (strcmp(argv[0], "-pgm-ascii") == 0){
      output_format = OFORM_PGM_ASCII;
#if 0
    } else if (strcmp(argv[0], "-pbm-raw") == 0){
      output_format = OFORM_PBM_RAW;
#endif
    } else if (strcmp(argv[0], "-pgm-raw") == 0){
      output_format = OFORM_PGM_RAW;
    } else if (strcmp(argv[0], "-eps") == 0){
      output_format = OFORM_EPS;
    } else if (strcmp(argv[0], "-eps-ptsize") == 0){
      if ((eps_ptsize = atof(argv[1])) <= 0)
	eps_ptsize = DEFAULT_EPS_POINT_SIZE;
      argc--; argv++;
    } else if (strcmp(argv[0], "-ascii-art") == 0){
      output_format = OFORM_ASCII_ART;
    } else if (strcmp(argv[0], "-ascii-art-h") == 0){
      output_format = OFORM_ASCII_ART;
    } else if (strcmp(argv[0], "-ascii-art-v") == 0){
      output_format = OFORM_ASCII_ART_V;
    } else if (strcmp(argv[0], "-none") == 0){
      output_format = OFORM_NONE;
    } else if (strcmp(argv[0], "-s") == 0){
      shrink_factor = atoi(argv[1]);
      argc--; argv++;
      if (shrink_factor <= 0){
	PR1("Shrink factor is too small. ");
	shrink_factor = 1;
      }
      if (shrink_factor > 8){
	PR1("Shrink factor is too large. ");
	shrink_factor = 8;
      }
    } else if (strcmp(argv[0], "-tab") == 0){
      tab_skip = atof(argv[1]);
      argc--; argv++;
    } else if (strcmp(argv[0], "-pw") == 0){
      page_width = atoi(argv[1]);
      argc--; argv++;
    } else if (strcmp(argv[0], "-ph") == 0){
      page_height = atoi(argv[1]);
      argc--; argv++;
    } else if (strcmp(argv[0], "-flush-left") == 0){
      line_typeset = LINE_FLUSH_LEFT;
      image_position_h = VF_IMAGEOUT_POSITION_LEFT;
    } else if (strcmp(argv[0], "-flush-right") == 0){
      line_typeset = LINE_FLUSH_RIGHT;
      image_position_h = VF_IMAGEOUT_POSITION_RIGHT;
    } else if (strcmp(argv[0], "-center") == 0){
      line_typeset = LINE_CENTER;
      image_position_h = VF_IMAGEOUT_POSITION_CENTER;
      image_position_v = VF_IMAGEOUT_POSITION_CENTER;
    } else if (strcmp(argv[0], "-center-line") == 0){
      line_typeset = LINE_CENTER;
    } else if (strcmp(argv[0], "-left-line") == 0){
      line_typeset = LINE_FLUSH_LEFT;
    } else if (strcmp(argv[0], "-right-line") == 0){
      line_typeset = LINE_FLUSH_RIGHT;
    } else if (strcmp(argv[0], "-center-image") == 0){
      image_position_h = VF_IMAGEOUT_POSITION_CENTER;
      image_position_v = VF_IMAGEOUT_POSITION_CENTER;
    } else if (strcmp(argv[0], "-h-center-image") == 0){
      image_position_h = VF_IMAGEOUT_POSITION_CENTER;
    } else if (strcmp(argv[0], "-v-center-image") == 0){
      image_position_v = VF_IMAGEOUT_POSITION_CENTER;
    } else if (strcmp(argv[0], "-left-image") == 0){
      image_position_h = VF_IMAGEOUT_POSITION_LEFT;
    } else if (strcmp(argv[0], "-right-image") == 0){
      image_position_h = VF_IMAGEOUT_POSITION_RIGHT;
    } else if (strcmp(argv[0], "-top-image") == 0){
      image_position_v = VF_IMAGEOUT_POSITION_TOP;
    } else if (strcmp(argv[0], "-bottom-image") == 0){
      image_position_v = VF_IMAGEOUT_POSITION_BOTTOM;
    } else if (strcmp(argv[0], "-ds") == 0){
      debug_state = 1;
    } else if (strcmp(argv[0], "-dr2l") == 0){
      debug_r2l = 1;
    } else if (strcmp(argv[0], "-df") == 0){
      debug_font = 1;
    } else if (strcmp(argv[0], "-dbc") == 0){
      debug_char_bitmap = 1;
    } else if (strcmp(argv[0], "-dbl") == 0){
      debug_line_bitmap = 1;
    } else if (strcmp(argv[0], "-dbp") == 0){
      debug_page_bitmap = 1;
    } else if (strcmp(argv[0], "-dvflib") == 0){
      debug_vflib = 1;
    } else if (strcmp(argv[0], "-dall") == 0){
      debug_state       = 1;
      debug_r2l         = 1;
      debug_font        = 1;
      debug_char_bitmap = 1;
      debug_line_bitmap = 1;
      debug_page_bitmap = 1;
      debug_vflib       = 1;
    } else if (strcmp(argv[0], "-font-list") == 0){
      show_font_list();
      exit(0);
    } else if (strcmp(argv[0], "-h") == 0){
      usage(0);
    } else if (strcmp(argv[0], "-help") == 0){
      usage(0);
    } else if (strcmp(argv[0], "-more-help") == 0){
      usage(1);
    } else if (strcmp(argv[0], "-version") == 0){
      printf("%s %s\n", PROG_NAME, VERSION);
      exit(0);
    } else {
      printf("Unknown option: %s\n", *argv);
      usage(0);
    }
    argc--; argv++;
  }

  *argcp = argc; 
  *argvp = argv;
}

void
usage(int level)
{
  PR3("%s --- %s\n", PROG_NAME, 
      "Make a bitmap of multilingual text in compound text format");
  PR2("Usage:  %s [OPTIONS] [FILE]\n", PROG_NAME);
  PR1("Options:\n");
  PR2(" -v FILE     vflibcap file (default: %s)\n",   
      DEFAULT_VFLIBCAP);
  PR1(" -bbx        generate a minimun image file\n");
  PR1(" -ctext, -euc-jp, -euc-kr, -euc-ch\n");
  PR1("             select encoding of input text (default: -ctext)\n");
  if (level > 0){
    PR1(" -iso-8859-1, -iso-8859-2, ..., -iso-8859-9, \n");
    PR1(" -latin-1, -latin-2, ..., -latin-4, -greek, -hebrew, -cyrillic\n");
    PR1("             select encoding of input text (1-byte encoding)\n");
  }
  PR1(" -times, -helv, -cour, -fixed\n");
  PR1("             select times/helvetica/courie/fixed font family (default: times)\n");
  PR1(" -normal, -bold, -italic\n");
  PR1("             select normal/bold/italic font face\n");
  PR1(" -14, -16, -18, -24\n");
  PR2("             select 14-/16-/18-/24-dot font set (default: %d)\n", 
      (int)DEFAULT_PIXEL_SIZE);
  PR1(" -scale PIXEL\n");
  PR1("             select scalable font set and specify pixel size.\n"); 
  if (level > 0){
    PR1(" -center, -flush-left, -flush-right\n");
    PR1("             Each line is centered or flushed left/right.\n");
    PR1(" -l2r, -r2l\n");
    PR1("             Select writing directionality left-to-right/right-to-left.\n");
    PR2(" -b SKIP     baseline skip (default: %.2f)\n", 
	(double)DEFAULT_BASELINESKIP);
  }
  PR1(" -pbm-ascii, -pgm-ascii, -pgm-raw, -eps, -ascii-art, -none\n");
  PR1("             select output format (default: -pgm-ascii)\n");
  if (level > 0){
    PR1(" -eps-ptsize POINT\n");
    PR1("             select point size of characters (EPS mode only)\n");
  }
  PR2(" -s N        shrink factor for PGM output (default: %d)\n", 
      (int)DEFAULT_SHRINK);
  if (level > 0){
    PR1(" -g N        vertical and horizontal margins in pixels\n");
  }
  PR1(" -gx N, -gy N\n");
  PR1("             horizontal/vertical margin in pixels\n");
  PR1(" -r          reverse black and white\n");
  PR2(" -tab N      Tab skip (default: %d)\n", DEFAULT_TAB_SKIP);
  PR1(" -font-list  print list of defined fonts\n");
  PR1(" -more-help  print full descriptions of command line options\n");
  if (level > 0){
    PR1(" -ds         Print state transision of a compound text parser\n");    
    PR1(" -df         Print font open processes\n");    
    PR1(" -dr2l       Print state transision of bi-directionality system\n");        PR1(" -dbc        Print image of each chracter in ascii-art form\n");
    PR1(" -dbl        Print image of each line in ascii-art form\n");
    PR1(" -dbp        Print image of a page in ascii-art form\n");
  }
  exit(0);
}


void
make_text_bitmap(FILE *fp, char *title)
{
  struct vf_s_bitmaplist  the_page_buff;
  VF_BITMAP               bm;

  VF_BitmapListInit(&the_page_buff);

  parser(fp, &the_page_buff);

  { 
    VF_BITMAP  bm0;

    bm0 = VF_BitmapListCompose(&the_page_buff);
    VF_BitmapListFinish(&the_page_buff);

    if (minimum_image == 1){
      bm = VF_MinimizeBitmap(bm0);
      VF_FreeBitmap(bm0);
    } else {
      bm = bm0;
    }
  }

  bm->off_x = 0;
  bm->off_y = bm->bbx_height;
  bm->mv_x  = bm->bbx_width;
  bm->mv_y  = 0;

  if (debug_page_bitmap == 1)
    VF_DumpBitmap(bm);

  switch (output_format){
  default:
  case OFORM_PBM_ASCII:
    VF_ImageOut_PBMAscii(bm, stdout, page_width, page_height,
			 image_position_h, image_position_v, 
			 margin_x, margin_x, margin_y, margin_y,
			 pix_reverse, shrink_factor, "ctext2pgm", title);
    break;
  case OFORM_PGM_ASCII:
    VF_ImageOut_PGMAscii(bm, stdout, page_width, page_height,
			 image_position_h, image_position_v, 
			 margin_x, margin_x, margin_y, margin_y,
			 pix_reverse, shrink_factor, "ctext2pgm", title);
    break;
#if 0
  case OFORM_PBM_RAW:
    break;
#endif
  case OFORM_PGM_RAW:
    VF_ImageOut_PGMRaw(bm, stdout, page_width, page_height,
		       image_position_h, image_position_v, 
		       margin_x, margin_x, margin_y, margin_y,
		       pix_reverse, shrink_factor, "ctext2pgm", title);
    break;
  case OFORM_EPS:
    VF_ImageOut_EPS(bm, stdout, page_width, page_height,
		    image_position_h, image_position_v, 
		    margin_x, margin_x, margin_y, margin_y,
		    pix_reverse, shrink_factor, "ctext2pgm", title,
		    eps_ptsize, (pixel!=0) ? pixel : xpixel);
    break;
  case OFORM_ASCII_ART:
    VF_ImageOut_ASCIIArt(bm, stdout, page_width, page_height,
			 image_position_h, image_position_v, 
			 margin_x, margin_x, margin_y, margin_y,
			 pix_reverse, shrink_factor);
    break;
  case OFORM_ASCII_ART_V:
    VF_ImageOut_ASCIIArtV(bm, stdout, page_width, page_height,
			  image_position_h, image_position_v, 
			  margin_x, margin_x, margin_y, margin_y,
			  pix_reverse, shrink_factor);
    break;
  case OFORM_NONE:
    break;
  }
}


void
parser(FILE *fp, VF_BITMAPLIST page_buff)
{
  int   chlen, chmask, ch, ch1, ch2, ch3, ch4;
  long  code_point, last_code_point;
  int   charset_saved;
  int   v_pos_x, v_pos_y, lineskip;
  int   g, i;
  VF_BITMAPLIST   line_buff;

  parser_init();

  v_pos_x = 0;
  v_pos_y = 0;
  if (pixel > 0)
    lineskip = - pixel * baselineskip * magy;
  else
    lineskip = - xpixel * baselineskip * magy;
  last_code_point = 0;
  nchars_in_line = 0;

  dir_sp = 0;
  dir_stack[dir_sp].dir     = wdirection;
  dir_stack[dir_sp].h_pos_x = 0;
  dir_stack[dir_sp].h_pos_y = 0;
  VF_BitmapListInit(&dir_stack[dir_sp].the_line_buff);

  line_buff = &dir_stack[dir_sp].the_line_buff;


  while ((ch = getc(fp)) != EOF){

    /* HT (0x09) */
    if (ch == '\t'){
      int  p;
      if (debug_state == 1)
	printf("Code point: 0x%02x\n", ch);
      p = tab_skip * ((pixel != 0) ? pixel : xpixel);
      if ((dir_stack[dir_sp].h_pos_x >= 0)
	  && (dir_stack[dir_sp].dir == WDIR_L2R)){
	dir_stack[dir_sp].h_pos_x 
	  = ((dir_stack[dir_sp].h_pos_x / p) + 1) * p;
      } else {
	dir_stack[dir_sp].h_pos_x 
	  = ((dir_stack[dir_sp].h_pos_x / p) - 1) * p;
      }
      nchars_in_line++;

    /* CR (0x0d) */
    } else if (ch == 0x0d){
      ;

    /* LF (0x0a) */
    } else if (ch == 0x0a){
      nchars_in_line = 0;
      parser_eol(&line_buff, page_buff, v_pos_x, v_pos_y);
      v_pos_x = 0;
      v_pos_y += lineskip;
      dir_stack[dir_sp].h_pos_x = 0;
      dir_stack[dir_sp].h_pos_y = 0;

    /* ESC (0x1b) */
    } else if ((use_esc == 1) && (ch == 0x1b)){
      if ((ch1 = getc(fp)) == EOF)
	goto end_of_file;
      if ((ch2 = getc(fp)) == EOF)
	goto end_of_file;
      if (debug_state == 1)
	printf("Escape Sequence: %02x %02x %02x\n", 0x1b, ch1, ch2);
      switch (ch1){
      default:
	fprintf(stderr, "Parsing Error: %02x %02x %02x\n", 0x1b, ch1, ch2);
	break;
      case 0x28:
	type_g[0]         = TYPEID94;
	charset_g[0]      = CSID(TYPEID94,ch2);
	chlen_g[0]        = 1;
	current_wdir_g[0] = charset_wdirection(charset_g[0]);
	if (debug_state == 1)
	  printf("Designate a 94 charset \"%s\" to G0, G0 is invoked to GL\n",
		 charset_name(charset_g[0], "???"));
	change_fonts(0);
	g = 0;
	break;
      case 0x29:
	type_g[1]         = TYPEID94; 
	charset_g[1]      = CSID(TYPEID94,ch2);
	chlen_g[1]        = 1;
	current_wdir_g[1] = charset_wdirection(charset_g[1]);
	if (debug_state == 1)
	  printf("Designate a 94 charset \"%s\" to G1, G1 is invoked to GR\n",
		 charset_name(charset_g[1], "???"));
	change_fonts(1);
	g = 1;
	break;
      case 0x2d:
	type_g[1]         = TYPEID96;
	charset_g[1]      = CSID(TYPEID96,ch2);
	chlen_g[1]        = 1;
	current_wdir_g[1] = charset_wdirection(charset_g[1]);
	if (debug_state == 1)
	  printf("Designate a 96 charset \"%s\" to G1, G1 is invoked to GR\n",
		 charset_name(charset_g[1], "???"));
	change_fonts(1);
	g = 1;
	break;
      case 0x24: /* Designate a 94^n charset to G0 or G1 */
	switch (ch2){
	case 0x28:
	  /* XXX: support for 94^2 charsets only */
	  if ((ch3 = getc(fp)) == EOF)
	    goto end_of_file;
	  type_g[0]         = TYPEID94_2;
	  charset_g[0]      = CSID(TYPEID94_2,ch3);
	  chlen_g[0]        = 2;
	  current_wdir_g[0] = charset_wdirection(charset_g[0]);
	  if (debug_state == 1)
	    printf("Designate a 94^2 charset \"%s\" to G0, "
		   "G0 is invoked to GL\n",
		   charset_name(charset_g[0], "???"));
	  change_fonts(0);
	  g = 0;
	  break;
	case 0x29:
	  /* XXX: support for 94^2 charsets only */
	  if ((ch3 = getc(fp)) == EOF)
	    goto end_of_file;
	  type_g[1]         = TYPEID94_2;
	  charset_g[1]      = CSID(TYPEID94_2,ch3);
	  chlen_g[1]        = 2;
	  current_wdir_g[1] = charset_wdirection(charset_g[1]);
	  if (debug_state == 1)
	    printf("Designate a 94^2 charset \"%s\" to G1, "
		   "G1 is invoked to GR\n",
		   charset_name(charset_g[1], "???"));
	  change_fonts(1);
	  g = 1;
	  break;
	case 0x40:  /* JIS C6226-1978 */ 
	case 0x41:  /* GB 2312 */ 
	case 0x42:  /* JIS X 0208 */ 
	  /* XXX: support for 2-byte charsets only */
	  type_g[0]         = TYPEID94_2;
	  charset_g[0]      = CSID(TYPEID94_2,ch2);
	  chlen_g[0]        = 2;
	  current_wdir_g[0] = charset_wdirection(charset_g[0]);
	  if (debug_state == 1)
	    printf("Designate a 94^2 charset \"%s\" to G0, "
		   "G0 is invoked to GL\n",
		   charset_name(charset_g[0], "???"));
	  change_fonts(0);
	  g = 0;
	  break;
	default:
	  g = -1;
	  break;
	}
	if (debug_r2l == 1)
	  printf("Charset directionality: %s %s\n", 
		 charset_name(charset_g[g], "???"),
		 (charset_g[g]==WDIR_L2R)?"left-to-right":"right-to-left");
	parser_check_wdir(g, &line_buff);
	break;
      }

    /* SI (0x0f) */
    } else if ((use_si == 1) && (ch == 0x0f)){
      ;

    /* SO (0x0e) */
    } else if ((use_so == 1) && (ch == 0x0e)){
      ;

    /* CSI (0x9b) */
    } else if ((use_csi == 1) && (ch == 0x9b)){
      if ((ch1 = getc(fp)) == EOF)
	goto end_of_file;
      switch (ch1){
      case 0x31:  /* begin left-to-right text */
	if ((ch2 = getc(fp)) == EOF)
	  goto end_of_file;
	if (ch2 != 0x5d)
	  continue;
	if ((debug_r2l == 1) || (debug_state == 1))
	  printf("Begin left-to-right string\n");
	parser_wdir_push(WDIR_L2R, &line_buff);
	break;
      case 0x32:  /* begin right-to-left text */
	if ((ch2 = getc(fp)) == EOF)
	  goto end_of_file;
	if (ch2 != 0x5d)
	  continue;
	if ((debug_r2l == 1) || (debug_state == 1))
	  printf("Begin right-to-left string\n");
	parser_wdir_push(WDIR_R2L, &line_buff);
	break;
      case 0x5d:  /* end of string */
	if ((debug_r2l == 1) || (debug_state == 1))
	  printf("End of string\n");
	parser_wdir_pop(&line_buff);
	break;
      default:
	;
      }
      
    /* Code points */
    } else {
      if (use_sjis == 0){  /* non-sjis encodings */
	if ((use_ss2 == 1) && (ch == 0x8e)){ 
	  /* SS2: 0x8e */
	  code_point = 0;
	  chlen = chlen_g[2] + 1;
	  chmask = 0xff;
	  g = 2;
	} else if ((use_ss3 == 1) && (ch == 0x8f)){
	  /* SS3: 0x8f */
	  code_point = 0;
	  chlen = chlen_g[3] + 1;
	  chmask = 0x7f;
	  g = 3;
	}  else {
	  code_point = (long) ch;
	  chlen  = ((code_point & 0x80) == 0) ? chlen_g[0] : chlen_g[1];
	  chmask = (chlen >= 2) ? 0x7f : 0xff;
	  g      = ((code_point & 0x80) == 0) ? 0 : 1;
	}
	code_point = code_point & chmask;
	for (i = 1; i < chlen; i++){
	  if ((ch = getc(fp)) == EOF)
	    goto end_of_file;
	  code_point = code_point * 256 + (long)(ch & chmask);
	}
      } else {             /* sjis encoding */
	ch1 = ch;
	if (((ch1 >= 129) && (ch1 <= 159)) || ((ch1 >= 224) && (ch1 <=239))){
	  /* 1st byte of Kanji */
	  if ((ch2 = getc(fp)) == EOF)
	    goto end_of_file;
	  if ((ch2 >= 64) && (ch2 <= 252)){
	    /* 2nd byte of Kanji */
	    ch3 = (((ch1 - ((ch1<160)?112:176)) << 1) - ((ch2<159)?1:0));
	    ch4 = ch2 - ((ch2<159) ? (ch2>127?32:31) : 126);
	    code_point = ch3 * 256 + ch4;
	  } else
	    code_point = 0x2121;
	  g = 1;
	} else if ((ch >= 161) && (ch <= 223)){
	  /* kana, 1byte */
	  code_point = (long) ch;
	  g = 2;
	} else {
	  /* jisx0201 */
	  code_point = (long) ch;
	  g = 0;
	}
      }

      if (last_code_point != (long)'\\'){
	/* characters to be printed or backslash */
	if (code_point != (long)'\\'){
	  /* print char */
	  last_code_point = 0;
	  if (debug_state == 1){
	    if ((0x20 <= code_point) && (code_point < 0x7e)){
	      printf("Code point: 0x%lx '%c' (G%d)\n",
		     code_point, (int)code_point, g);
	    } else {
	      printf("Code point: 0x%lx (G%d)\n", code_point, g);
	    }
	  }
	  parser_check_wdir(g, &line_buff);
	  draw_char(code_point, line_buff, g, 
		    &dir_stack[dir_sp].h_pos_x, &dir_stack[dir_sp].h_pos_y);
	  nchars_in_line++;
	} else {
	  /* a command by backslash */
	  last_code_point = code_point;
	}

      } else {
	/* backslash command */
	parser_cmd(code_point, line_buff, &charset_saved, g, 
		   &dir_stack[dir_sp].h_pos_x, &dir_stack[dir_sp].h_pos_y);
	last_code_point = 0;
      }
    }
  }

end_of_file:
  if (nchars_in_line > 0)    /* the last line does not end by '\n' char */
    parser_eol(&line_buff, page_buff, v_pos_x, v_pos_y);
}

void
parser_eol(VF_BITMAPLIST *line_buff_p, VF_BITMAPLIST page_buff, 
	   int vposx, int vposy)
{
  int        x;
  VF_BITMAP  line_bm;

  while (dir_sp > 0)
    parser_wdir_pop(line_buff_p);
  if ((line_bm = VF_BitmapListCompose(*line_buff_p)) == NULL){ 
    PR1("No memory\n");
    exit(1);
  }
  if (debug_line_bitmap == 1)
    VF_DumpBitmap(line_bm);
  VF_BitmapListFinish(*line_buff_p);

  switch (line_typeset){
  default:
  case LINE_DEFAULT:      
    x = vposx; 
    break;
  case LINE_FLUSH_LEFT:
    if (wdirection == WDIR_R2L)
      swap_refpt_nextpt(line_bm);
    x = vposx;
    break;
  case LINE_FLUSH_RIGHT:
    if (wdirection == WDIR_L2R){
      swap_refpt_nextpt(line_bm);
      x = vposx;
    } else {
      x = vposx - line_bm->bbx_width;
    }
    break;
  case LINE_CENTER:
    x = vposx - line_bm->bbx_width/2;
    break;
  }    
  VF_BitmapListPut(page_buff, line_bm, x, vposy);
}


void
parser_check_wdir(int i, VF_BITMAPLIST *line_buff_p)
{
  if ((i < 0) || (1 < i))
    return;
  
  if (debug_r2l == 1)
    printf("dir_stack[%d].dir=%s, current_wdir_g[%d]=%d\n", 
	   dir_sp, (dir_stack[dir_sp].dir==WDIR_L2R)?"WDIR_L2R":"WDIR_R2L",
	   i, current_wdir_g[i]);

  if (dir_stack[dir_sp].dir != current_wdir_g[i]){
    if  (current_wdir_g[i] != wdirection){
      if (current_wdir_g[i] == WDIR_L2R){
	if (debug_r2l == 1)
	  printf("Change directionality to left-to-right mode\n");
	parser_wdir_push(WDIR_L2R, line_buff_p);
      } else {
	if (debug_r2l == 1)
	  printf("Change directionality to right-to-left mode\n");
	parser_wdir_push(WDIR_R2L, line_buff_p);
      }
    } else {
      if (debug_r2l == 1)
	printf("End of directionality change\n");
      parser_wdir_pop(line_buff_p);
    }
  }
}

void
parser_wdir_push(int wdir, VF_BITMAPLIST *line_buff_p)
{
  if ((wdirection == WDIR_R2L)
      && (dir_sp == 1) && (wdir == WDIR_R2L)){
    /* darty trick for right-to-left text */
    parser_wdir_do_pop(line_buff_p);
  } else {
    parser_wdir_do_push(wdir, line_buff_p);
  }
}

void
parser_wdir_pop(VF_BITMAPLIST *line_buff_p)
{
  if ((wdirection == WDIR_R2L) 
      && (dir_sp == 0) && (dir_stack[dir_sp].dir == WDIR_R2L)){
    /* darty trick for right-to-left text */
    parser_wdir_do_push(WDIR_L2R, line_buff_p); 
  } else {
    parser_wdir_do_pop(line_buff_p);
  }
}

void
parser_wdir_do_push(int wdir, VF_BITMAPLIST *line_buff_p)
{
  int  s;

  dir_sp++;
  dir_stack[dir_sp].dir = wdir;
  dir_stack[dir_sp].h_pos_x = 0;
  VF_BitmapListInit(&dir_stack[dir_sp].the_line_buff);
  *line_buff_p = &dir_stack[dir_sp].the_line_buff;

  if (debug_r2l == 1){
    printf("Push to dir_stack\n");
    for (s = dir_sp; s >= 0; s--){
      printf("  dir_stack[%d].dir=%s\n", 
	     s, (dir_stack[s].dir==WDIR_L2R)?"WDIR_L2R":"WDIR_R2L");
    }
  }
}

void 
parser_wdir_do_pop(VF_BITMAPLIST *line_buff_p)
{
  int        s;
  VF_BITMAP  inline_bm;

  inline_bm = VF_BitmapListCompose(&dir_stack[dir_sp].the_line_buff);
  dir_sp--;
  *line_buff_p = &dir_stack[dir_sp].the_line_buff;

  if (debug_r2l == 1){
    printf("Pop dir_stack\n");
    for (s = dir_sp; s >= 0; s--){
      printf("  dir_stack[%d].dir=%s\n", 
	     s, (dir_stack[s].dir==WDIR_L2R)?"WDIR_L2R":"WDIR_R2L");
    }
    printf("current_wdir_g[0]=%s, current_wdir_g[1]=%s\n", 
	   (current_wdir_g[0]==WDIR_L2R)?"WDIR_L2R":"WDIR_R2L", 
	   (current_wdir_g[1]==WDIR_L2R)?"WDIR_L2R":"WDIR_R2L");
  }

  if (debug_r2l == 1)
    VF_DumpBitmap(inline_bm);
  if (dir_stack[dir_sp].dir != dir_stack[dir_sp+1].dir)
    swap_refpt_nextpt(inline_bm);
  if (debug_r2l == 1)
    VF_DumpBitmap(inline_bm);

  put_bitmap(*line_buff_p, inline_bm, dir_stack[dir_sp].dir, 
	     &dir_stack[dir_sp].h_pos_x, &dir_stack[dir_sp].h_pos_y);
}

int
charset_wdirection(int charset)
{
  int dir;

  switch (charset){
  case CS_ISO8859_6:  /* Latin/Arabic */
  case CS_ISO8859_8:  /* Latin/Hebrew */
  case CS_MULE_ARAB1: /* Mule Arabic 1 */
  case CS_MULE_ARAB2: /* Mule Arabic 2 */
    dir = WDIR_R2L;
    break;
  case CS_MULE_ARAB0: /* Mule Arabic 0 */
  default:
    dir = WDIR_L2R;
    break;
  }

  return dir;
}


void
draw_char(long code_point, VF_BITMAPLIST buff, int i, int *posxp, int *posyp)
{
  VF_BITMAP  bm;

  if ((font_exists_g[i] == 1) && (font_info[current_font_g[i]].font_id >= 0)){

    code_point = cp_conv(code_point, i);

    if (debug_vflib == 1){
      printf("VF_GetBitmap2(%d, 0x%lx, 1, 1);\n",
	     font_info[current_font_g[i]].font_id, code_point);
    }
    if ((bm = VF_GetBitmap2(font_info[current_font_g[i]].font_id,
			    code_point, 1, 1)) == NULL){
      PR3("Cannot get bitmap 0x%lx of font %s\n", 
	  code_point, font_info[current_font_g[i]].font_name);
      if (chlen_g[i] == 1){
	*posxp = *posxp + ((pixel != 0)?pixel:xpixel)/2;
      } else {
	*posxp = *posxp + ((pixel != 0)?pixel:xpixel);
      }
    } else {
      if (current_reverse == 1)
	reverse_bitmap(bm);
      if ((current_wdir_g[i] == WDIR_R2L) && (bm->mv_x > 0))
	swap_refpt_nextpt(bm);
      if (debug_char_bitmap == 1)
	VF_DumpBitmap(bm);
      put_bitmap(buff, bm, current_wdir_g[i], posxp, posyp);
    }
  }
}

long
cp_conv(long code_point, int i)
{
  int   r0, r1;
  static int  tbl_mule_visvii_l[] = {
      0,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,
    176,177,178,  0,  0,181,182,183,184,  0,  0,  0,  0,189,190,  0,
      0,  0,  0,  0,  0,  0,198,199,  0,  0,  0,  0,  0,  0,  0,207,
      0,209,  0,  0,  0,213,214,215,216,  0,  0,219,220,  0,222,223,
    224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
    240,241,242,243,244,245,246,247,248,249,250,251,252,253,254, 0
    };
  static int  tbl_mule_visvii_u[] = {
      0,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
    144,145,146,  0,  0,147,150,151,152,  0,  0,  0,  0,180,149,  0,
      0,  0,  0,  0,  0,  0,  2,  5,  0,  0,  0,  0,  0,  0,  0,159,
      0,186,  0,  0,  0,128, 20,187,188,  0,  0, 25, 30,  0,179,191,
    192,193,194,195,196,197,255,  6,200,201,202,203,204,205,206,155,
    208,185,210,211,212,160,153,154,158,217,218,157,156,221,148,  0
    };

  r0 = code_point % 0x100;
  r1 = code_point / 0x100;

  switch (font_info[current_font_g[i]].charset_id){
  default:
    break;
  case CS_JISX0201R:
    if (i == 1)
      r0 &= ~0x80;
    break;
  case CS_JISX0201K:
    if (i == 0)
      r0 |= 0x80;
    break;
  case CS_MULE_BIG5_L1:
  case CS_MULE_BIG5_L2:
    r0 = (r1 - 0x21) * 94 + (r0 - 0x21);
    r1 = (r0 / 157) + 0xa1;
    r0 = r0 % 157;
    if (r0 < 0x3f)
      r0 += 0x40;
    else
      r0 += 0x62;
    if (font_info[current_font_g[i]].charset_id == CS_MULE_BIG5_L2)
      r1 += 0x25;
    break;
  case CS_MULE_ETHIOPIC:
    r1 = (r1 - 33) * 94;
    r0 = (r0 - 33) + r1;
    if (r0 < 256){
      r1 = 0x12;
    } else if (r0 < 448){
      r1 = 0x13;
      r0 -= 256;
    } else {
      r1 = 0xfd;
      r0 -= 208;
    }
    break;
  case CS_MULE_VISCII_L:
    if (r0 < 128)
      break;
    r0 = tbl_mule_visvii_l[r0 - 160];
    break;
  case CS_MULE_VISCII_U:
    if (r0 < 128)
      break;
    r0 = tbl_mule_visvii_u[r0 - 160];
    break;
  }

  return  r1 * 0x100 + r0;
}

void
put_bitmap(VF_BITMAPLIST buff, VF_BITMAP bm, int wdir, int *posxp, int *posyp)
{
  VF_BitmapListPut(buff, bm, *posxp, *posyp);
  *posxp += bm->mv_x;
  *posyp += bm->mv_y;
}

void
reverse_bitmap(VF_BITMAP bm)
{
  int             x, y, b, m;
  unsigned char  *p, w;
  static unsigned char  bits[] = {
    0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff };


  if (bm == NULL)
    return;

  b = bm->bbx_width / 8;
  m = bm->bbx_width % 8;

  for (y = 0; y < bm->bbx_height; y++){
    p = &bm->bitmap[y * bm->raster];
    for (x = 0; x < b; x++){
      *p = *p ^ 0xff;
      p++;
    }
  }

  if (m != 0){
    w = bits[m];
    p = &bm->bitmap[b];
    for (y = 0; y < bm->bbx_height; y++){
      *p = *p ^ w;
      p = p + bm->raster;
    }
  }
}

void 
swap_refpt_nextpt(VF_BITMAP bm)
{
  int  offx, offy;
  int  movx, movy;

  offx = bm->off_x - bm->mv_x;
  offy = bm->off_y - bm->mv_y;
  movx = -bm->mv_x;
  movy = -bm->mv_y;

  bm->off_x = offx;
  bm->off_y = offy;
  bm->mv_x  = movx;
  bm->mv_y  = movy;
}

void parser_cmd(long code_point, VF_BITMAPLIST buff, int *charset_saved_p, 
		int g, int *posxp, int *posyp)
{
  switch ((char)code_point){
  default: 
    PR2("Unknown command: \\%c\n", (char)code_point);
    break;
  case '\\': 
    parser_check_wdir(g, &buff);
    draw_char((long)'\\', buff, 0, posxp, posyp);      
    nchars_in_line++;
    break;
  case '.': 
    current_family = default_family;
    current_face   = default_face; 
    break;
  case 'd': current_family = default_family; break;
  case 'f': current_family = FAM_FIXED;      break;
  case 't': current_family = FAM_TIMES;      break;
  case 'h': current_family = FAM_HELV;       break;
  case 'c': current_family = FAM_COUR;       break;
  case 'D': current_face = default_face;  break;
  case 'N': current_face = FACE_NORMAL;   break;
  case 'B': current_face = FACE_BOLD;     break;
  case 'I': current_face = FACE_ITALIC;   break;
  case '(': current_reverse = 1;  break;
  case ')': current_reverse = 0;  break;
    /* \< and \> commands swicthes fonts to iso8859-1 font */
    /* This is effective (but ad-hoc) if looks of fonts of GL and */
    /* GR are different. */
  case '<':			/* switch to a iso8859-1 font */
    *charset_saved_p = charset_g[0];
    charset_g[0] = CS_ISO8859_1;
    if (debug_state == 1)
      printf("Use iso8859-1 font temporarily\n");
    break;
  case '>':			/* switch back to the original */
    charset_g[0] = *charset_saved_p;
    if (debug_state == 1)
      printf("Go back to the original font\n");
    break;
  }

  change_fonts(0);
  if (use_g1 == 1)
    change_fonts(1);
  if (use_g2 == 2)
    change_fonts(2);
  if (use_g3 == 3)
    change_fonts(3);
}

void
change_fonts(int i)
{
  int   opened, j;

  opened = -1;

  /* search an exact font */
  for (j = 0; font_info[j].charset_id >= 0; j++){
    if (   (font_info[j].pixel_size == pixel)
	&& (font_info[j].charset_id == charset_g[i])
	&& (font_info[j].family_id  == current_family)
	&& (font_info[j].face_id    == current_face)){
      if (try_font_open(j) >= 0){
	opened = j;
	break;
      }
    }
  }
  /* search a font of the default face */
  if (   (opened < 0) 
      && (current_face != default_face)){
      for (j = 0; font_info[j].charset_id >= 0; j++){
      if (   (font_info[j].pixel_size == pixel)
	  && (font_info[j].charset_id == charset_g[i])
	  && (font_info[j].family_id  == current_family)
	  && (font_info[j].face_id    == default_face)){
	if (try_font_open(j) >= 0){
	  opened = j;
	  break;
	}
      }
    }
  }
  /* search a font of the default face */
  if (   (opened < 0) 
      && (current_face != FACE_DEFAULT)
      && (default_face != FACE_DEFAULT)){
      for (j = 0; font_info[j].charset_id >= 0; j++){
      if (   (font_info[j].pixel_size == pixel)
	  && (font_info[j].charset_id == charset_g[i])
	  && (font_info[j].family_id  == current_family)
	  && (font_info[j].face_id    == FACE_DEFAULT)){
	if (try_font_open(j) >= 0){
	  opened = j;
	  break;
	}
      }
    }
  }
  /* search a font of default face and default family */
  if (   (opened < 0)
      && (current_family != default_family)){
  for (j = 0; font_info[j].charset_id >= 0; j++){
      if (   (font_info[j].pixel_size == pixel)
	  && (font_info[j].charset_id == charset_g[i])
	  && (font_info[j].family_id  == default_family)
	  && (font_info[j].face_id    == FACE_DEFAULT)){
	if (try_font_open(j) >= 0){
	  opened = j;
	  break;
	}
      }
    }
  }
  /* search a font of default face of default family */
  if (   (opened < 0)
      && (current_family != FAM_DEFAULT)
      && (default_family != FAM_DEFAULT)){
    for (j = 0; font_info[j].charset_id >= 0; j++){
      if (   (font_info[j].pixel_size == pixel)
	  && (font_info[j].charset_id == charset_g[i])
	  && (font_info[j].family_id  == FAM_DEFAULT)
	  && (font_info[j].face_id    == FACE_DEFAULT)){
	if (try_font_open(j) >= 0){
	  opened = j;
	  break;
	}
      }
    }
  }
  /* search any font of the charset */
  if (opened < 0){
    for (j = 0; font_info[j].charset_id >= 0; j++){
      if (   (font_info[j].pixel_size == pixel)
	  && (font_info[j].charset_id == charset_g[i])){
	if (try_font_open(j) >= 0){
	  opened = j;
	  break;
	}
      }
    }
  }
  
  /* Not found */
  if (opened < 0){
    font_exists_g[i]  = 0;	/* NO FONT */
    current_font_g[i] = 0;
    PR2("No font for %s\n", charset_name(charset_g[i], "???"));
  } else {
    font_exists_g[i]  = 1;
    current_font_g[i] = opened;
    if (debug_state == 1)
      printf("** Charset: %s\n", charset_name(charset_g[i], "???"));
  }
}

int
try_font_open(int table_index)
{
  if (debug_font == 1)
    printf("Try font open: %s", font_info[table_index].font_name);

  switch (font_info[table_index].font_id){
  case NOT_OPENED:
    if (debug_vflib == 1){
      printf("VF_OpenFont2(\"%s\", %d, %.3f, %.3f);\n",
	     font_info[table_index].font_name, xpixel, magx, magy);
    }
    font_info[table_index].font_id 
      = VF_OpenFont2(font_info[table_index].font_name, xpixel, magx, magy);
    if (debug_vflib == 1)
      printf("   VFlib font ID = %d\n", font_info[table_index].font_id );
    if (font_info[table_index].font_id < 0){
      font_info[table_index].font_id = NOT_FOUND;
      if (debug_font == 1)
	printf("... cannot open\n");
      return -1;
    }
    if (debug_font == 1)
      printf("... successfully opened!\n");
    break;
  case NOT_FOUND:
    if (debug_font == 1)
      printf("... could not opened before\n");
    return -1;
  default:
    if (debug_font == 1)
      printf("... opened before\n");
    break;
  }
  
  if (debug_font == 1)
    printf("Font switch: %s\n", font_info[table_index].font_name);
  return 1;
}

void
parser_init(void)
{
  int  i;

  for (i = 0; i < 4; i++){
    font_exists_g[i] = 0;  /* font does not exist */
    type_g[i]    = TYPEID94;
    charset_g[i] = CS_ISO8859_1;
    chlen_g[i]   = 1;
  }

  /* Default values */
  use_esc  = 0;
  use_csi  = 0;
  use_si   = 0;
  use_so   = 0;
  use_ss2  = 0;
  use_ss3  = 0;
  use_g1   = 0;
  use_g2   = 0;
  use_g3   = 0;
  use_sjis = 0;

  switch (input_encoding){
  case ENC_CTEXT:
    use_esc  = 1;
    use_csi  = 1;
    use_g1   = 1;
    /* Designate ISO8859-1 into G0 */
    type_g[0]    = TYPEID94;
    charset_g[0] = CS_ISO8859_1;
    chlen_g[0]   = 1;
    change_fonts(0);
    /* Designate right-hand of ISO Latin-1 into G1 */
    type_g[1]    = TYPEID96;
    charset_g[1] = CS_ISO8859_1;
    chlen_g[1]   = 1;
    change_fonts(1);
    break;
  case ENC_ISO8859_1:
    use_g1   = 1;
    /* Designate ISO8859-1 into G0 */
    type_g[0]    = TYPEID94;
    charset_g[0] = CS_ISO8859_1;
    chlen_g[0]   = 1;
    change_fonts(0);
    /* Designate right-hand of ISO Latin-1 into G1 */
    type_g[1]    = TYPEID96;
    charset_g[1] = CS_ISO8859_1;
    chlen_g[1]   = 1;
    change_fonts(1);
    break;
  case ENC_ISO8859_2:
    use_g1   = 1;
    /* Designate ISO8859-2 into G0 */
    type_g[0]    = TYPEID94;
    charset_g[0] = CS_ISO8859_2;
    chlen_g[0]   = 1;
    change_fonts(0);
    /* Designate right-hand of ISO8859-1 into G1 */
    type_g[1]    = TYPEID96;
    charset_g[1] = CS_ISO8859_2;
    chlen_g[1]   = 1;
    change_fonts(1);
    break;
  case ENC_ISO8859_3:
    use_g1   = 1;
    /* Designate ISO8859-3 into G0 */
    type_g[0]    = TYPEID94;
    charset_g[0] = CS_ISO8859_3;
    chlen_g[0]   = 1;
    change_fonts(0);
    /* Designate right-hand of ISO8859-2 into G1 */
    type_g[1]    = TYPEID96;
    charset_g[1] = CS_ISO8859_2;
    chlen_g[1]   = 1;
    change_fonts(1);
    break;
  case ENC_ISO8859_4:
    use_g1   = 1;
    /* Designate ISO8859-4 into G0 */
    type_g[0]    = TYPEID94;
    charset_g[0] = CS_ISO8859_4;
    chlen_g[0]   = 1;
    change_fonts(0);
    /* Designate right-hand of ISO8859-4 into G1 */
    type_g[1]    = TYPEID96;
    charset_g[1] = CS_ISO8859_4;
    chlen_g[1]   = 1;
    change_fonts(1);
    break;
  case ENC_ISO8859_5: /* Cryillic */
    use_g1   = 1;
    /* Designate ISO8859-1 into G0 */
    type_g[0]    = TYPEID94;
    charset_g[0] = CS_ISO8859_1;
    chlen_g[0]   = 1;
    change_fonts(0);
    /* Designate right-hand of ISO8859-5 into G1 */
    type_g[1]    = TYPEID96;
    charset_g[1] = CS_ISO8859_5;
    chlen_g[1]   = 1;
    change_fonts(1);
    break;
  case ENC_ISO8859_6: /* Arabic */
    use_g1   = 1;
    wdirection = WDIR_R2L;
    /* Designate ISO8859-1 into G0 */
    type_g[0]    = TYPEID94;
    charset_g[0] = CS_ISO8859_1;
    chlen_g[0]   = 1;
    change_fonts(0);
    /* Designate right-hand of ISO8859-6 into G1 */
    type_g[1]    = TYPEID96;
    charset_g[1] = CS_ISO8859_6;
    chlen_g[1]   = 1;
    change_fonts(1);
    break;
  case ENC_ISO8859_7: /* Greek */
    use_g1   = 1;
    /* Designate ISO8859-1 into G0 */
    type_g[0]    = TYPEID94;
    charset_g[0] = CS_ISO8859_1;
    chlen_g[0]   = 1;
    change_fonts(0);
    /* Designate right-hand of ISO8859-7 into G1 */
    type_g[1]    = TYPEID96;
    charset_g[1] = CS_ISO8859_7;
    chlen_g[1]   = 1;
    change_fonts(1);
    break;
  case ENC_ISO8859_8: /* Hebrew */
    use_g1   = 1;
    wdirection = WDIR_R2L;
    /* Designate ISO8859-1 into G0 */
    type_g[0]    = TYPEID94;
    charset_g[0] = CS_ISO8859_1;
    chlen_g[0]   = 1;
    change_fonts(0);
    /* Designate right-hand of ISO8859-8 into G1 */
    type_g[1]    = TYPEID96;
    charset_g[1] = CS_ISO8859_8;
    chlen_g[1]   = 1;
    change_fonts(1);
    break;
  case ENC_ISO8859_9:
    use_g1   = 1;
    /* Designate ISO8859-9 into G0 */
    type_g[0]    = TYPEID94;
    charset_g[0] = CS_ISO8859_9;
    chlen_g[0]   = 1;
    change_fonts(0);
    /* Designate right-hand of ISO8859-9 into G1 */
    type_g[1]    = TYPEID96;
    charset_g[1] = CS_ISO8859_9;
    chlen_g[1]   = 1;
    change_fonts(1);
    break;
  case ENC_ISO2022_JP:
    use_esc  = 1;
    /* Designate ASCII into G0 */
    type_g[0]    = TYPEID94;
    charset_g[0] = CS_ASCII;
    chlen_g[0]   = 1;
    change_fonts(0);
    break;
#if 0
  case ENC_ISO2022_KR:
    use_si   = 1;
    use_so   = 1;
    use_g1   = 1;
    /* Designate ASCII into G0 */
    type_g[0]    = TYPEID94;
    charset_g[0] = CS_ASCII;
    chlen_g[0]   = 1;
    change_fonts(0);
    /* Designate KSC5601 into G1 */
    type_g[1]    = TYPEID94_2;
    charset_g[1] = CS_KSC5601;
    chlen_g[1]   = 2;
    change_fonts(1);
    break;
  case ENC_ISO2022_CN:
    use_esc  = 1;
    use_si   = 1;
    use_so   = 1;
    use_ss2  = 1;
    use_ss3  = 1;
    use_g1   = 1;
    use_g2   = 1;
    /* Designate ASCII into G0 */
    type_g[0]    = TYPEID94;
    charset_g[0] = CS_ASCII;
    chlen_g[0]   = 1;
    change_fonts(0);
    break;
#endif
  case ENC_EUC_JP1:
  case ENC_EUC_JP2:  
    use_ss2  = 1;
    use_ss3  = 1;
    use_g1   = 1;
    use_g2   = 1;
    use_g3   = 1;
    if (input_encoding == ENC_EUC_JP1){
      /* Designate JIS X0201 into G0 (code set 0) */
      type_g[0]    = TYPEID94;
      charset_g[0] = CS_JISX0201R;
      chlen_g[0]   = 1;
      change_fonts(0);
    } else {
      /* Designate ISO8859-1 into G0 (code set 0) */
      type_g[0]    = TYPEID94;
      charset_g[0] = CS_ISO8859_1;
      chlen_g[0]   = 1;
      change_fonts(0);
    }
    /* Designate JIS X0208 into G1 (code set 1) */
    type_g[1]    = TYPEID94_2;
    charset_g[1] = CS_JISX0208;
    chlen_g[1]   = 2;
    change_fonts(1);
    /* Designate JIS X0208 into G2 (code set 2) */
    type_g[2]    = TYPEID94;
    charset_g[2] = CS_JISX0201K;
    chlen_g[2]   = 1;
    change_fonts(2);
    /* Designate JIS X0212 into G3 (code set 3) */
    type_g[3]    = TYPEID94_2;
    charset_g[3] = CS_JISX0212;
    chlen_g[3]   = 2;
    change_fonts(3);
    break;
  case ENC_EUC_KR:
    use_g1   = 1;
    /* Designate ISO8859-1 into G0 (code set 0) */
    type_g[0]    = TYPEID94;
    charset_g[0] = CS_ISO8859_1;
    chlen_g[0]   = 1;
    change_fonts(0);
    /* Designate KSC5601 into G1 (code set 1) */
    type_g[1]    = TYPEID94_2;
    charset_g[1] = CS_KSC5601;
    chlen_g[1]   = 2;
    change_fonts(1);
    break;
  case ENC_EUC_CH_GB:
    use_g1   = 1;
    /* Designate ISO8859-1 into G0 (code set 0) */
    type_g[0]    = TYPEID94;
    charset_g[0] = CS_ISO8859_1;
    chlen_g[0]   = 1;
    change_fonts(0);
    /* Designate GB2312 into G1 (code set 1) */
    type_g[1]    = TYPEID94_2;
    charset_g[1] = CS_GB2312;
    chlen_g[1]   = 2;
    change_fonts(1);
    break;
  case ENC_EUC_CH_CNS:
    use_ss2  = 1;
    use_ss3  = 1;
    use_g1   = 1;
    use_g3   = 1;
    /* Designate ISO8859-1 into G0 (code set 0) */
    type_g[0]    = TYPEID94;
    charset_g[0] = CS_ISO8859_1;
    chlen_g[0]   = 1;
    change_fonts(0);
    /* Designate CNS11643-1 into G1 (code set 1) */
    type_g[1]    = TYPEID94_2;
    charset_g[1] = CS_CNS11643_1;
    chlen_g[1]   = 2;
    change_fonts(1);
    /* Designate CNS11643-2 into G3 (code set 3) */
    type_g[3]    = TYPEID94_2;
    charset_g[3] = CS_CNS11643_2;
    chlen_g[3]   = 2;
    change_fonts(3);
    break;
  case ENC_SJIS:
    use_g1   = 1;
    use_g2   = 1;
    use_sjis = 1;
    /* JISX0201 Roman into G0 */
    type_g[0]    = TYPEID94;
    charset_g[0] = CS_JISX0201R;
    chlen_g[0]   = 1;
    change_fonts(0);
    /* JISX0208 into G1 */
    type_g[1]    = TYPEID94_2;
    charset_g[1] = CS_JISX0208;
    chlen_g[1]   = 2;
    change_fonts(1);
    /* JISX0201 Kana into G2 */
    type_g[2]    = TYPEID94;
    charset_g[2] = CS_JISX0201K;
    chlen_g[2]   = 1;
    change_fonts(2);
    break;
  default:
    fprintf(stderr, "Input encoding is unknown.\n");
    exit(1);
  }

  for (i = 0; i < 4; i++){
    current_wdir_g[i] = charset_wdirection(charset_g[i]);
  }
}

char*
charset_name(int charset, char *if_unknown)
{
  char *name;

  name = if_unknown;

  switch (charset){
  case CS_ASCII:       name = "ASCII";                       break;
  case CS_ISO8859_1:   name = "ISO 8859-1 (Latin-1)";        break;
  case CS_ISO8859_2:   name = "ISO 8859-2 (Latin-2)";        break;
  case CS_ISO8859_3:   name = "ISO 8859-3 (Latin-3)";        break;
  case CS_ISO8859_4:   name = "ISO 8859-4 (Latin-4)";        break;
  case CS_ISO8859_5:   name = "ISO 8859-5 (Cyrillic)";       break;
  case CS_ISO8859_6:   name = "ISO 8859-6 (Arabic)";         break;
  case CS_ISO8859_7:   name = "ISO 8859-7 (Greek)";          break;
  case CS_ISO8859_8:   name = "ISO 8859-8 (Hebrew)";         break;
  case CS_ISO8859_9:   name = "ISO 8859-9 (Latin-5)";        break;
  case CS_JISX0201R:   name = "JIS X 0201-Roman";            break;
  case CS_JISX0201K:   name = "JIS X 0201-Kana";             break;
  case CS_JISX0208:    name = "JIS X 0208 (Japanese)";       break;
  case CS_JISX0212:    name = "JIS X 0212 (Japanese)";       break;
  case CS_KSC5601:     name = "KSC 5601 (Hangle)";           break;
  case CS_GB2312:      name = "GB 2312 (Chinese)";           break;
  case CS_CNS11643_1:  name = "CNS 11643-1 (Chinese)";       break;
  case CS_CNS11643_2:  name = "CNS 11643-2 (Chinese)";       break;
  case CS_CNS11643_3:  name = "CNS 11643-3 (Chinese)";       break;
  case CS_CNS11643_4:  name = "CNS 11643-4 (Chinese)";       break;
  case CS_CNS11643_5:  name = "CNS 11643-5 (Chinese)";       break;
  case CS_CNS11643_6:  name = "CNS 11643-6 (Chinese)";       break;
  case CS_CNS11643_7:  name = "CNS 11643-7 (Chinese)";       break;
  case CS_MULE_BIG5_L1:   name = "BIG5 Level 1 (Chinese)";      break;
  case CS_MULE_BIG5_L2:   name = "BIG5 Level 2 (Chinese)";      break;
  case CS_MULE_ARAB0:     name = "Mule Arabic 0";            break;
  case CS_MULE_ARAB1:     name = "Mule Arabic 1";            break;
  case CS_MULE_ARAB2:     name = "Mule Arabic 2";            break;
  case CS_MULE_ETHIOPIC:  name = "Mule Ethiopic";            break;
  case CS_MULE_VISCII_L:  name = "Mule VISCII 1.1 Lower";    break;
  case CS_MULE_VISCII_U:  name = "Mule VISCII 1.1 Upper";    break;
  }
  return name;
}


#define FONTLIST_W_CHARSET   28
#define FONTLIST_W_FAM_FACE  17
#define FONTLIST_W_PIXEL     12
#define FONTLIST_W_FONT      -1


void  
show_font_list(void)
{
  char  buff[80], *s1, *s2;
  int   i;

  wprint("Character Set Name",   FONTLIST_W_CHARSET);
  wprint("Family&Face",          FONTLIST_W_FAM_FACE);
  wprint("Pixel Size",           FONTLIST_W_PIXEL);
  wprint("Font Name",            FONTLIST_W_FONT);
  wprint("\n\n", -1);
  
  for (i = 0; font_info[i].charset_id >= 0; i++){

    /* charset name */
    sprintf(buff, "%s", charset_name(font_info[i].charset_id, "???"));
    wprint(buff, FONTLIST_W_CHARSET);

    /* font family & font face */
    switch (font_info[i].family_id){
    default:
    case FAM_FIXED:   s1 = "Fixed";    break;
    case FAM_TIMES:   s1 = "Times";    break;
    case FAM_HELV:    s1 = "Helvetia"; break;
    case FAM_COUR:    s1 = "Courier";  break;
    }
    switch (font_info[i].face_id){
    default:
    case FACE_NORMAL:   s2 = "Normal";   break;
    case FACE_ITALIC:   s2 = "Italic";   break;
    case FACE_BOLD:     s2 = "Bold";     break;
    }
    sprintf(buff, "%s %s", s1, s2);
    wprint(buff, FONTLIST_W_FAM_FACE);

    /* pixel size */
    if (font_info[i].pixel_size == 0)
      sprintf(buff, "%s", "scalable");
    else
      sprintf(buff, "%d", font_info[i].pixel_size);
    wprint(buff, FONTLIST_W_PIXEL);

    /* font name */
    wprint(font_info[i].font_name, FONTLIST_W_FONT);

    wprint("\n", -1);
  }
}

void  
wprint(char *str, int w)
{
  int  i;
  char *p;

  if (w <= 0){
    printf("%s", str);
    return;
  }

  if (str == NULL){
    for (i = 0; i < w; i++){
      putchar(' ');
    }
    return;
  }

  i = w;
  p = str;
  while (i > 0){
    if (*p == '\0')
      break;
    putchar(*p);
    p++;
    --i;
  }
  while (i > 0){
    putchar(' ');
    --i;
  }
}


/*EOF*/
