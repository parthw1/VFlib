/* 
 * vflmkpk.c 
 * by Hirotsugu Kakugawa
 * - a vflibcap entry generator for TeX PK fonts
 * - This program prints vflibcap entries to standard output.
 * - Useful for generating vflibcap for TeX DVI drivers
 *
 *  10 May 2001
 */
/*
 * Copyright (C) 1996-2017 Hirotsugu Kakugawa. 
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
#include  "pk.h"
#include  "fsearch.h"
#include  "vflmklib.h"



void  gen_class_deafult(void);


char  *mode  = DEFAULT_KPS_MODE;
char  *dpi   = NULL;
int    dpi_i = DEFAULT_KPS_DPI;

#define NDIRS    64
int   n_pkf; 
char  *pk_fontdirs[NDIRS];

int   gen_missing_glyph = 0;
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

  n_pkf = 0;
  for (i = 0; i < NDIRS; i++){
    pk_fontdirs[i] = NULL;
  }

  xargc = argc; 
  xargv = argv;

  for (xargc--,xargv++; xargc > 0; xargc--,xargv++){
    if ((strcmp(*xargv, "--help") == 0)
	|| (strcmp(*xargv, "-help") == 0)){
      printf("vflmkpk: generates vflibcap entries for PK fonts\n");
      printf("Usage: vflmkpk [options]\n");
      printf("Options\n");
      printf("  -d DIR   : PK font file directory\n");
      printf("  -n MODE  : Device mode name for kpathsea\n");
      printf("  -r DPI   : Default device resolution\n");
      printf("  -g       : Emit code to generate PK file on-the-fly\n");

      printf("Example: vflmkpk -d TEXMF -d /usr/tex/fonts -g \n");
      exit(0);

    } else if (strcmp(*xargv, "-d") == 0){
      /* font dir */
      if (n_pkf == NDIRS){
	fprintf(stderr, "Too many PK font directories\n");
	exit(1);
      }
      xargv++; xargc--;
      check_argc(xargc);
      pk_fontdirs[n_pkf++] = x_strdup(*xargv);

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
      gen_missing_glyph = 1; 

    } else {
      if (*xargv[0] == '-'){
	fprintf(stderr, "vflmkpk: unknown option %s\n", *xargv);
	exit(1);
      }
      break;

    }
  }

  banner("PK", "vflmkpk", cmdline);

  gen_class_deafult();    

  return 0;
}



void
gen_class_deafult(void)
{
  int   i;

  printf("(%s %s", 
	 VF_CAPE_VFLIBCAP_CLASS_DEFAULT_DEFINITION, FONTCLASS_NAME_PK);
  printf("\n  (%s", VF_CAPE_FONT_DIRECTORIES);
  for (i = 0; i < n_pkf; i++)
    printf("\n       \"%s\"", pk_fontdirs[i]);
  printf(")");
  printf("\n  (%s %s)", VF_CAPE_DPI, dpi);
#if 0
  printf("\n  (%s \"%s\")", VF_CAPE_MAKE_MISSING_GLYPH, 
	 (gen_missing_glyph==1) ? "yes" : "no");
#endif
  printf(")\n");
  printf("\n");
}

