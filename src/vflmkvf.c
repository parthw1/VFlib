/* 
 * vflmkvf.c 
 * by Hirotsugu Kakugawa
 * - a vflibcap entry generator for TeX VF files
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
#include  "vf.h"
#include  "fsearch.h"
#include  "vflmklib.h"



void  gen_class_deafult(void);


char  *mode  = DEFAULT_KPS_MODE;
char  *dpi   = NULL;
int    dpi_i = DEFAULT_KPS_DPI;

#define NDIRS    64
int   n_vff; 
char  *vf_fontdirs[NDIRS];

#define NMAPS    64
int n_map;
char  *map_class[NMAPS];
char  *map_map[NMAPS];
char  *map_opt[NMAPS];

char *cmdline = NULL; 
char *glyph_style = TEX_GLYPH_STYLE_FILL_STR;
char *open_style  = TEX_OPEN_STYLE_TRY_STR;



int 
main(int argc, char **argv)
{
  int     i;
  int    xargc;
  char **xargv;

  dpi = malloc(256);
  sprintf(dpi, "%d", dpi_i);

  cmdline = copy_cmdline(argc, argv);

  n_vff = 0;
  for (i = 0; i < NDIRS; i++){
    vf_fontdirs[i] = NULL;
  }

  n_map = 0;
  for (i = 0; i < NMAPS; i++){
    map_class[i] = NULL;
    map_map[i] = NULL;
    map_opt[i] = NULL;
  }

  xargc = argc; 
  xargv = argv;

  for (xargc--,xargv++; xargc > 0; xargc--,xargv++){
    if ((strcmp(*xargv, "--help") == 0)
	|| (strcmp(*xargv, "-help") == 0)){
      printf("vflmkvf: generates vflibcap entries for TeX Virtual Fonts\n");
      printf("Usage: vflmkvf [options]\n");
      printf("Options\n");
      printf("  -d DIR   : Virtual Font file directory\n");
      printf("  -r DPI   : Default device resolution\n");
      printf("  -n MODE  : Device mode name for kpathsea\n");
      printf("  -g STYLE : Glyph style, 'fill' (default) or 'empty'\n");
      printf("  -o STYLE : Font open style, 'try', 'require', or 'non'\n");
      printf("  -m CLASS FORMAT : Add mapping rule\n");
      printf("Example: vflmkvf -d TEXMF -m type1 %%f.pfb -m gf .gf -g fill\n");
      exit(0);

    } else if (strcmp(*xargv, "-d") == 0){
      /* font dir */
      if (n_vff == NDIRS){
	fprintf(stderr, "Too many Virtual Font directories\n");
	exit(1);
      }
      xargv++; xargc--;
      check_argc(xargc);
      vf_fontdirs[n_vff++] = x_strdup(*xargv);

    } else if (strcmp(*xargv, "-r") == 0){
      xargv++; xargc--;
      check_argc(xargc);
      dpi = strdup(*xargv);

    } else if (strcmp(*xargv, "-n") == 0){
      /* mode */
      xargv++; xargc--;
      check_argc(xargc);
      mode = x_strdup(*xargv);

    } else if (strcmp(*xargv, "-m") == 0){
      if (n_map == NMAPS){
	fprintf(stderr, "Too many mapfont conversion rules\n");
	exit(1);
      }
      xargv++; xargc--;
      check_argc(xargc);
      if (strcmp(*xargv, "any") == 0)
	map_class[n_map] = strdup("*");
      else
	map_class[n_map] = strdup(*xargv);
      xargv++; xargc--;
      check_argc(xargc);
      map_map[n_map] = strdup(*xargv);
      n_map++;

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

    } else if (strcmp(*xargv, "-o") == 0){
      xargv++; xargc--;
      check_argc(xargc);
      if ((strcmp(*xargv, TEX_OPEN_STYLE_TRY_STR) != 0)
	  && (strcmp(*xargv, TEX_OPEN_STYLE_REQUIRE_STR) != 0)
	  && (strcmp(*xargv, TEX_OPEN_STYLE_NONE_STR) != 0)){
	fprintf(stderr, "Unknown open style name: %s\n", *xargv);
	fprintf(stderr, "(Must be '%s', '%s' or '%s'. Default is '%s'.\n", 
		TEX_OPEN_STYLE_TRY_STR,
		TEX_OPEN_STYLE_REQUIRE_STR,
		TEX_OPEN_STYLE_NONE_STR,
		open_style);
	exit(1);
      }
      open_style = strdup(*xargv);

    } else {
      if (*xargv[0] == '-'){
	fprintf(stderr, "vflmkvf: unknown option %s\n", *xargv);
	exit(1);
      }
      break;

    }
  }

  banner("Virtual Font", "vflmkvf", cmdline);

  gen_class_deafult();    

  return 0;
}



void
gen_class_deafult(void)
{
  int   i;

  printf("(%s %s", 
	 VF_CAPE_VFLIBCAP_CLASS_DEFAULT_DEFINITION, FONTCLASS_NAME_VF);
  printf("\n  (%s", VF_CAPE_FONT_DIRECTORIES);
  for (i = 0; i < n_vff; i++)
    printf("\n       \"%s\"", vf_fontdirs[i]);
  printf(")");
  printf("\n  (%s %s)", VF_CAPE_DPI, dpi);
  printf("\n  (%s \".vf\" \".ovf\")", VF_CAPE_EXTENSIONS);
  printf("\n  (%s \"%s\")", VF_CAPE_TEX_OPEN_STYLE, open_style);
  printf("\n  (%s \"%s\")", VF_CAPE_TEX_GLYPH_STYLE, glyph_style);
  printf("\n  (%s", VF_CAPE_TEX_FONT_MAPPING);
  for (i = 0; i < n_map; i++){
    if (map_need_tfm(map_class[i]) == 0){
      printf("\n    ((%s \"%s\") *)", 
	     map_class[i], map_map[i]);
    } else {
      printf("\n    ((%s \"%s\" %s) *)", 
	     map_class[i], map_map[i], TEX_FONT_MAPPING_PTSIZE);
    }
  }
  printf(")");
  printf(")\n");
  printf("\n");
}

