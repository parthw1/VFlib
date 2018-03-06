/* 
 * vflmktex.c 
 * by Hirotsugu Kakugawa
 * - a vflibcap entry generator for TeX font mapper
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


char  *dpi   = NULL;
int    dpi_i = DEFAULT_KPS_DPI;

#define NDIRS    64
int   n_tfmf; 
char  *tfm_fontdirs[NDIRS];

#define NMAPS    64
int n_map;
char  *map_class[NMAPS];
char  *map_map[NMAPS];
char  *map_opt[NMAPS];

char *cmdline = NULL; 

int res_col[5][32] = {
  { 240,   
    240,  263,  288,  312,  346,  415,  498,  597, -1 },
  { 300, 
    329,  360,  432,  518,  622,  746,  896, 1075, 1290, 240, 270, -1 },
  { 400,
    400,  438,  480,  576,  691,  829,  995, 1194, 1433, 1720, 320, 360, -1 },
  { 600,   
    600,  657,  720,  864, 1037, 1244, 1493, 1792, 2150, 2580, 480, 540, -1 },
};


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
      printf("vflmktex: generates vflibcap entries for TeX font mapper\n");
      printf("Usage: vflmktex [options]\n");
      printf("Options\n");
      printf("  -d DIR   : TFM file directory\n");
      printf("  -r DPI   : Default device resolution\n");
      printf("  -m CLASS FORMAT : Add font mapping rule\n");
      printf("Example: vflmkvf -d TEXMF -m type1 %%f.pfb -m gf %%f.gf\n");
      exit(0);

    } else if (strcmp(*xargv, "-d") == 0){
      /* font dir */
      if (n_tfmf == NDIRS){
	fprintf(stderr, "Too many Virtual Font directories\n");
	exit(1);
      }
      xargv++; xargc--;
      tfm_fontdirs[n_tfmf++] = x_strdup(*xargv);

    } else if (strcmp(*xargv, "-r") == 0){
      xargv++; xargc--;
      check_argc(xargc);
      dpi = strdup(*xargv);

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

    } else {
      if (*xargv[0] == '-'){
	fprintf(stderr, "vflmktex: unknown option %s\n", *xargv);
	exit(1);
      }
      break;

    }
  }

  banner("TeX Font Mapper", "vflmktex", cmdline);

  gen_class_deafult();    

  return 0;
}



void
gen_class_deafult(void)
{
  int   i, j;

  printf("(%s %s", 
	 VF_CAPE_VFLIBCAP_CLASS_DEFAULT_DEFINITION, FONTCLASS_NAME_TeX);
  printf("\n  (%s %s)", VF_CAPE_DPI, dpi);
  printf("\n  (%s", VF_CAPE_TEX_TFM_DIRECTORIES);
  for (i = 0; i < n_tfmf; i++)
    printf("\n       \"%s\"", tfm_fontdirs[i]);
  printf(")");
  printf("\n  (%s \".tfm\" \".ofm\")", VF_CAPE_TEX_TFM_EXTENSIONS);
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
  printf("\n  (%s 0.02)", VF_CAPE_RESOLUTION_ACCU);
  printf("\n  (%s", VF_CAPE_RESOLUTION_CORR);
  for (i = 0; res_col[i][0] > 0; i++){
    printf("\n    (%d  ; %d dpi devices", res_col[i][0], res_col[i][0]);
    printf("\n    ");
    for (j = 0; res_col[i][j] > 0; j++){
      printf(" %d", res_col[i][j]);
    }
    printf(")");
  }
  printf(")");

  printf(")");
  printf("\n");
  printf("\n");
}
