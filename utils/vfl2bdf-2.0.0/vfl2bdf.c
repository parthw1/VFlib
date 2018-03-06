/*
 * vf2bdf
 * --- Generate a BDF font file from VFlib fonts.
 *  by Hirotsugu KAKUGAWA
 *
 * Edition History
 *  18 Jan 1996  for VFlib 2
 *   6 May 1997  for VFlib 3.2
 *   6 Aug 1997  for VFlib 3.3
 *   1 Mar 1999  for VFlib 3.5, bug fix.
 *
 */
/*
 * Copyright (C) 1996-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <VFlib-3_7.h>

#define VFL2BDF_VERSION      "2.1"

#define GLYPH_TYPE_FONT_BBX  0
#define GLYPH_TYPE_MIN_BBX   1
#define ENCODING_TYPE_FLAT   0
#define ENCODING_TYPE_94x94  1


int   gtype        = GLYPH_TYPE_FONT_BBX;
int   etype        = ENCODING_TYPE_FLAT;
int   code_from    = -1;
int   code_to      = -1;
int   pixel        = -1;
int   xoff         = 0;
int   yoff         = 0;
int   fixed        = 0;
int   quiet        = 0;
char  font_xlfd[1024];

int   font_bbx_w, font_bbx_h; 
int   font_off_x, font_off_y; 
int   font_mv_x, font_mv_y;

char *font_creator      = "UNKNOWN";
char *font_family       = "UNKNOWN";
char *charset_registry  = "UNKNOWN";
char *charset_encoding  = "0";



void  usage(void);
void  DecideFontXLDF(int pixel_size, double point_size);
int   GenerateBDF(char *font_name, char *bdf_file_name);
int   BDF_PutChar(FILE *bdf_fp, VF_BITMAP bm, 
		  int code_point, int pixel_size);
int   BDF_Header(FILE *bdf_fp, int fid, int pixel_size, double point_size, 
		 int nchars, int default_char, 
		 int font_ascent, int font_descent, 
		 int font_bbx_w, int font_bbx_h, 
		 int font_off_x, int font_off_y);
int   BDF_Trailer(FILE *bdf_fp, int pixel_size);


int
main(argc, argv)
     int  argc;
     char **argv;
{
  int     iarg;
  char    *font_name, *bdf_file_name, *vflibcap;

  vflibcap      = NULL;
  font_name     = NULL;
  bdf_file_name = NULL;

  argv++;
  argc--;
  iarg = 0;  
  while (argc > 0){
    if (argv[0][0] != '-'){
      switch (iarg++){
      case 0:
	font_name = *argv;
	break;
      case 1:
	sscanf(*argv, "%i", &code_from);
	break;
      case 2:
	sscanf(*argv, "%i", &code_to);
	break;
      default:
	break;
      }
    } else {
      if (strcmp(argv[0], "-v") == 0){
	if (--argc == 0)
	  usage();
	vflibcap = *(++argv);
      } else if (strcmp(argv[0], "-f") == 0){
	if (--argc == 0)
	  usage();
	font_name = *(++argv);
      } else if (strcmp(argv[0], "-m") == 0){
	gtype = GLYPH_TYPE_MIN_BBX;
      } else if (strcmp(argv[0], "-p") == 0){
	if (--argc == 0)
	  usage();
	pixel = atoi(*(++argv));
	if (pixel <= 0){
	  fprintf(stderr, "Illegal pixel size=%d\n", pixel);
	  exit(1);
	} 
      } else if (strcmp(argv[0], "-xoff") == 0){
	if (--argc == 0)
	  usage();
	xoff = atoi(*(++argv));
      } else if (strcmp(argv[0], "-yoff") == 0){
	if (--argc == 0)
	  usage();
	yoff = atoi(*(++argv));
      } else if (strcmp(argv[0], "-o") == 0){
	if (--argc == 0)
	  usage();
	bdf_file_name = *(++argv);
      } else if (strcmp(argv[0], "-94x94") == 0){
	etype = ENCODING_TYPE_94x94;
      } else if (strcmp(argv[0], "-fixed") == 0){
	fixed = 1;
      } else if (strcmp(argv[0], "-q") == 0){
	quiet = 1;
      } else if (strcmp(argv[0], "-font-creator") == 0){
	if (--argc == 0)
	  usage();
	font_creator = *(++argv);
      } else if (strcmp(argv[0], "-font-family") == 0){
	if (--argc == 0)
	  usage();
	font_family = *(++argv);
      } else if (strcmp(argv[0], "-charset-registry") == 0){
	if (--argc == 0)
	  usage();
	charset_registry = *(++argv);
      } else if (strcmp(argv[0], "-charset-encoding") == 0){
	if (--argc == 0)
	  usage();
	charset_encoding = *(++argv);
      } else {
	printf("Unknown option: %s\n", *argv);
	usage();
      }
    }
    argc--;
    argv++;
  }

  if ((font_name == NULL) || (code_from < 0) || (code_to < 0))
    usage();

  if (VF_Init(vflibcap, NULL) < 0){
    fprintf(stderr, "VFlib initialization error.\n");
    exit(1);
  }

  GenerateBDF(font_name, bdf_file_name);

  return 0;
}

void usage(void)
{
  printf("Usage: vfl2bdf [Options] FONT START END\n");
  printf("   - Make a BDF font from VFlib font FONT.\n");
  printf("     Range of code points is from START to END.\n");
  printf("Options: \n");
  printf("  -v FILE  : vflibcap file\n");
  printf("  -p PIXEL : pixel size of BDF font\n");
  printf("  -o FILE  : output file name\n");
  printf("  -f FONT  : font name\n");
  printf("  -xoff N  : shift N pixels to right\n");
  printf("  -yoff N  : shift N pixels to up\n");
  printf("  -94x94   : assume the font is encoded in 94x94 style\n");
  printf("  -m       : make minimized bounding boxes\n");
  printf("  -q       : quiet mode\n");
  printf("  -h       : print how to use this program\n");
  printf("  -font-family NAME       : set font family name\n");
  printf("  -charset-registry NAME  : set charset registry name\n");
  printf("  -charset-encoding NAME  : set charset encoding name\n");
  exit(0);
}


int
GenerateBDF(char *font_name, char *bdf_file_name)
{
  FILE         *bdf_fp;
  int          fid1, fid2;
  int          code_point, nchars, default_char;
  int          font_ascent, font_descent;
  int          pixel_size;
  double       point_size;
  VF_BITMAP    bm, bm2;
  char         *p;

  font_bbx_w = font_bbx_h = 0;
  font_off_x = font_off_y = 0;
  font_mv_x  = font_mv_y  = 0;
  font_ascent = font_descent = 0;
  default_char = code_from;


  if (bdf_file_name != NULL){
    if ((bdf_fp = fopen(bdf_file_name, "w")) == NULL){
      fprintf(stderr, "Can't open output file: %s\n", bdf_file_name);
      exit(0);
    } 
    if (quiet == 0)
      fprintf(stderr, "Writing to %s\n", bdf_file_name);
  } else {
    bdf_fp = stdout;
    if (quiet == 0)
      fprintf(stderr, "Writing to %s\n", "standard output");
  }


  /* PASS 1 */

  if (quiet == 0)
    fprintf(stderr, "Pass 1\n");

  if ((fid1 = VF_OpenFont2(font_name, pixel, 1, 1)) < 0){
    fprintf(stderr, "Can't open font: %s.\n", font_name);
    exit(1);
  }

  nchars = 0;
  for (code_point = code_from; code_point <= code_to; code_point++){
    if ((etype == ENCODING_TYPE_94x94)
	&& (   ((code_point % 256) < 0x21) || (0x7e < (code_point % 256))
	    || ((code_point / 256) < 0x21) || (0x7e < (code_point / 256))))
      continue;
    if ((bm = VF_GetBitmap2(fid1, code_point, 1, 1)) != NULL){
      nchars++;
      if (font_bbx_w < bm->bbx_width)
	font_bbx_w = bm->bbx_width;
      if (font_bbx_h < bm->bbx_height)
	font_bbx_h = bm->bbx_height;
      if (font_mv_x < bm->mv_x)
	font_mv_x = bm->mv_x;
      if (font_mv_y < bm->mv_y)
	font_mv_y = bm->mv_y;
      if (font_off_x > bm->off_x)
	font_off_x = bm->off_x;
      if (font_off_y > (bm->bbx_height - bm->off_y))
	font_off_y = (bm->bbx_height - bm->off_y);
      VF_FreeBitmap(bm);
    }
  }

  if ((pixel_size = pixel) < 0){
    if ((p = VF_GetFontProp(fid1, "PIXEL_SIZE")) != NULL)
      pixel_size = atoi(p);
  }
  if ((p = VF_GetFontProp(fid1, "POINT_SIZE")) != NULL){
    point_size = (double)atof(p)/10.0;
  } else {
    point_size = pixel_size;
  }

  if (quiet == 0){
    fprintf(stderr, "  Pixel size = %d\n", pixel);
    fprintf(stderr, "  Point size = %.3f\n", point_size);
    fprintf(stderr, "  %d chracters\n", nchars);
  }

  DecideFontXLDF(pixel_size, point_size);

  /* PASS 2 */

  if (quiet == 0)
    fprintf(stderr, "Pass 2\n");
  if (quiet == 0)
    fprintf(stderr, "  XLFD = %s\n", font_xlfd);
  

  if ((fid2 = VF_OpenFont2(font_name, pixel, 1, 1)) < 0){
    fprintf(stderr, "Can't open font: %s.\n", font_name);
    exit(1);
  }

  BDF_Header(bdf_fp, fid2, pixel_size, point_size, 
	     nchars, default_char, font_ascent, font_descent, 
	     font_bbx_w, font_bbx_h, font_off_x, font_off_y);

  for (code_point = code_from; code_point <= code_to; code_point++){
    if ((etype == ENCODING_TYPE_94x94)
	&& (   ((code_point % 256) < 0x21) || (0x7e < (code_point % 256))
	    || ((code_point / 256) < 0x21) || (0x7e < (code_point / 256))))
      continue;
    if ((bm = VF_GetBitmap2(fid2, code_point, 1, 1)) != NULL){
      if (gtype == GLYPH_TYPE_FONT_BBX){
	BDF_PutChar(bdf_fp, bm, code_point, pixel_size);
      } else {
	if ((bm2 = VF_MinimizeBitmap(bm)) != NULL){
	  BDF_PutChar(bdf_fp, bm2, code_point, pixel_size);
	  VF_FreeBitmap(bm2);	  
	}
      } 
      VF_FreeBitmap(bm);
    }
  }

  BDF_Trailer(bdf_fp, pixel_size);

  fclose(bdf_fp);

  VF_CloseFont(fid1);
  VF_CloseFont(fid2);

  if (quiet == 0)
    fprintf(stderr, "done.\n");

  return nchars;
}


void 
DecideFontXLDF(int pixel_size, double point_size)
{
  sprintf(font_xlfd, "-%s-%s-%s-%s-%s--%d-%d-75-75-C-140-%s-%s", 
	  font_creator, font_family, "medium", "r", "normal", 
	  pixel_size, (int)(point_size*10.0), 
	  charset_registry, charset_encoding);
}

int
BDF_Header(FILE *bdf_fp, int fid, int pixel_size, double point_size, 
	   int nchars, int default_char, 
	   int font_ascent, int font_descent, 
	   int font_bbx_w, int font_bbx_h, int font_off_x, int font_off_y)
{
  char  *p;

  fprintf(bdf_fp, "STARTFONT 2.1\n");
  fprintf(bdf_fp, "COMMENT BDF font by VFL2BDF %s\n", VFL2BDF_VERSION);
  fprintf(bdf_fp, "FONT %s\n", font_xlfd);
  fprintf(bdf_fp, "SIZE %d 75 75\n", (int)(point_size+0.5));
  fprintf(bdf_fp, "FONTBOUNDINGBOX %d %d %d %d\n", 
	  font_bbx_w, font_bbx_h, 
	  font_off_x + xoff, font_off_y + yoff);
  fprintf(bdf_fp, "STARTPROPERTIES 9\n");
  fprintf(bdf_fp, "PIXEL_SIZE %d\n", pixel_size);
  fprintf(bdf_fp, "POINT_SIZE %d\n", (int)(point_size*10.0));
  fprintf(bdf_fp, "RESOLUTION_X 75\n");
  fprintf(bdf_fp, "RESOLUTION_Y 75\n");
  if ((p = VF_GetFontProp(fid, "CHARSET_REGISTRY")) == NULL)
    p = "UNKNOWN";
  fprintf(bdf_fp, "CHARSET_REGISTRY \"%s\"\n", p);
  if ((p = VF_GetFontProp(fid, "CHARSET_ENCODING")) == NULL)
    p = "";
  fprintf(bdf_fp, "CHARSET_ENCODING \"%s\"\n", p);
  fprintf(bdf_fp, "DEFAULT_CHAR %d\n", default_char);
  fprintf(bdf_fp, "FONT_DESCENT %d\n", font_descent + yoff);
  fprintf(bdf_fp, "FONT_ASCENT %d\n", font_ascent  + yoff);
  fprintf(bdf_fp, "ENDPROPERTIES\n");
  fprintf(bdf_fp, "CHARS %d\n", nchars);
  return 0;
}

int
BDF_Trailer(FILE *bdf_fp, int pixel_size)
{
  fprintf(bdf_fp, "ENDFONT\n");
  return 0;
}

int
BDF_PutChar(FILE *bdf_fp, VF_BITMAP bm, 
	    int code_point, int pixel_size)
{
  int            x, y;
  unsigned char  *p;

  fprintf(bdf_fp, "STARTCHAR 0x%X\n", code_point);
  fprintf(bdf_fp, "ENCODING %d\n", code_point);
  fprintf(bdf_fp, "SWIDTH %d %d\n", 
	  (int)(1000.0*(double)bm->mv_x/(double)pixel_size),
	  (int)(1000.0*(double)bm->mv_y/(double)pixel_size));
  if (fixed == 0){     /* propotional font */
    fprintf(bdf_fp, "DWIDTH %d %d\n", bm->mv_x, bm->mv_y);
    fprintf(bdf_fp, "BBX %d %d %d %d\n", 
	    bm->bbx_width, bm->bbx_height, 
	    bm->off_x + xoff, bm->off_y - bm->bbx_height + yoff);
    fprintf(bdf_fp, "BITMAP\n");
    for (y = 0; y < bm->bbx_height; y++){
      p = &bm->bitmap[y*bm->raster];
      for (x = 0; x < (bm->bbx_width+7)/8; x++)
	fprintf(bdf_fp, "%02X", p[x]);
      fprintf(bdf_fp, "\n");
    }
  } else {             /* monospace font */
    fprintf(bdf_fp, "DWIDTH %d %d\n", font_mv_x, font_mv_y);
    fprintf(bdf_fp, "BBX %d %d %d %d\n", 
	    font_bbx_w, font_bbx_h, 
	    bm->off_x + xoff, bm->off_y - bm->bbx_height + yoff);
    fprintf(bdf_fp, "BITMAP\n");
    for (y = 0; y < font_bbx_h - bm->bbx_height; y++){
      for (x = 0; x < (bm->bbx_width+7)/8; x++)
	fprintf(bdf_fp, "00");
      fprintf(bdf_fp, "\n");
    }
    for (y = 0; y < bm->bbx_height; y++){
      p = &bm->bitmap[y*bm->raster];
      for (x = 0; x < (bm->bbx_width+7)/8; x++)
	fprintf(bdf_fp, "%02X", p[x]);
      for (x = (bm->bbx_width+7)/8; x < (font_bbx_w+7)/8; x++)
	fprintf(bdf_fp, "00");
      fprintf(bdf_fp, "\n");
    }
  }
  fprintf(bdf_fp, "ENDCHAR\n");
  return 0;
}


/*EOF*/
