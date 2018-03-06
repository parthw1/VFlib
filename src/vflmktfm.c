/* 
 * vflmktfm.c 
 * by Hirotsugu Kakugawa
 * - a vflibcap entry generator for TeX TFM files
 * - This program prints vflibcap entries to standard output.
 * - Useful for generating vflibcap for TeX DVI drivers
 *
 *  10 May 2001
 */
/*
 * Copyright (C) 2001-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */

#include "config.h"
#include "with.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif
#include <ctype.h>
#ifdef HAVE_SYS_PARAM_H
#  include  <sys/param.h>
#endif
#ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
#endif

#include  "VFlib-3_7.h"
#include  "VFsys.h"
#include  "vflibcap.h"
#include  "texfonts.h"
#include  "tfm.h"
#include  "fsearch.h"
#include  "vflmklib.h"



void  gen_class_deafult(void);


char  *mode  = DEFAULT_KPS_MODE;
char  *dpi   = NULL;
int    dpi_i = DEFAULT_KPS_DPI;

#define NDIRS    64
int   n_tfmf; 
char  *tfm_fontdirs[NDIRS];

char *glyph_style = TEX_GLYPH_STYLE_FILL_STR;
char *cmdline = NULL; 



int 
main(int argc, char **argv)
{
  int     i;
  int    xargc;
  char **xargv;

  dpi = malloc(256);
  sprintf(dpi, "%d", dpi_i);

  cmdline = copy_cmdline(argc, argv);

  n_tfmf = 0;
  for (i = 0; i < NDIRS; i++){
    tfm_fontdirs[i] = NULL;
  }

  xargc = argc; 
  xargv = argv;

  for (xargc--,xargv++; xargc > 0; xargc--,xargv++){
    if ((strcmp(*xargv, "--help") == 0)
	|| (strcmp(*xargv, "-help") == 0)){
      printf("vflmktfm: generates vflibcap entries for TFM fonts\n");
      printf("Usage: vflmktfm [options]\n");
      printf("Options\n");
      printf("  -d DIR   : TFM font file directory\n");
      printf("  -r DPI   : Default device resolution\n");
      printf("  -n MODE  : Device mode name for kpathsea\n");
      printf("  -g STYLE : Glyph style, 'fill' (default) or 'empty'\n");
      printf("Example: vflmktfm -c -d TEXMF -d /usr/tex/fonts -g fill\n");
      exit(0);

    } else if (strcmp(*xargv, "-d") == 0){
      /* font dir */
      if (n_tfmf == NDIRS){
	fprintf(stderr, "Too many TFM font directories\n");
	exit(1);
      }
      xargv++; xargc--;
      tfm_fontdirs[n_tfmf++] = x_strdup(*xargv);

    } else if (strcmp(*xargv, "-r") == 0){
      xargv++; xargc--;
      check_argc(xargc);
      dpi = strdup(*xargv);

    } else if (strcmp(*xargv, "-n") == 0){
      /* mode */
      xargv++; xargc--;
      check_argc(xargc);
      mode = x_strdup(*xargv);

    } else if (strcmp(*xargv, "-g") == 0){
      xargv++; xargc--;
      check_argc(xargc);
      if ((strcmp(*xargv, TEX_GLYPH_STYLE_FILL_STR) != 0)
	  && (strcmp(*xargv, TEX_GLYPH_STYLE_EMPTY_STR) != 0)){
	fprintf(stderr, "Unknown glyph style name: %s\n", *xargv);
	fprintf(stderr, "(Must be '%s' or '%s'. Default is '%s'.\n", 
		TEX_GLYPH_STYLE_FILL_STR, TEX_GLYPH_STYLE_EMPTY_STR,
		glyph_style);
	exit(1);
      }
      glyph_style = strdup(*xargv);
    } else {
      if (*xargv[0] == '-'){
	fprintf(stderr, "vflmktfm: unknown option %s\n", *xargv);
	exit(1);
      }
      break;

    }
  }

  banner("TFM", "vflmktfm", cmdline);

  gen_class_deafult();    

  return 0;
}



void
gen_class_deafult(void)
{
  int   i;

  printf("(%s %s", 
	 VF_CAPE_VFLIBCAP_CLASS_DEFAULT_DEFINITION, FONTCLASS_NAME_TFM);
  printf("\n  (%s", VF_CAPE_FONT_DIRECTORIES);
  for (i = 0; i < n_tfmf; i++)
    printf("\n       \"%s\"", tfm_fontdirs[i]);
  printf(")");
  printf("\n  (%s %s)", VF_CAPE_DPI, dpi);
  printf("\n  (%s \".tfm\" \".ofm\")", VF_CAPE_EXTENSIONS);
  printf("\n  (%s \"%s\")", VF_CAPE_TEX_GLYPH_STYLE, glyph_style);
  printf(")\n");
  printf("\n");
}
