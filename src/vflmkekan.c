/* 
 * vflmkekan.c 
 * by Hirotsugu Kakugawa
 * - a vflibcap entry generator for eKanji bitmap fonts
 * - This program prints vflibcap entries to standard output.
 *
 *  23 May 2001
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
# include <unistd.h>
#endif
#ifdef HAVE_STRING_H
#  include <string.h>
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
#include  "ekan.h"
#include  "fsearch.h"
#include  "vflmklib.h"



void  gen_class_deafult(void);


char  *dpi   = NULL;
int    dpi_i = DEFAULT_KPS_DPI;

#define NDIRS    64
int   n_dirs; 
char  *fontdirs[NDIRS];

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

  n_dirs = 0;
  for (i = 0; i < NDIRS; i++){
    fontdirs[i] = NULL;
  }

  xargc = argc; 
  xargv = argv;

  for (xargc--,xargv++; xargc > 0; xargc--,xargv++){
    if ((strcmp(*xargv, "--help") == 0)
	|| (strcmp(*xargv, "-help") == 0)){
      printf("vflmkekan: generates vflibcap entries for eKanji fonts\n");
      printf("Usage: vflmkekan [options]\n");
      printf("Options\n");
      printf("  -d DIR   : eKanji font file directory\n");
      printf("  -r DPI   : Default device resolution\n");

      printf("Example: vflmkekan -d /usr/local/share/fonts/eKanji \n");
      exit(0);

    } else if (strcmp(*xargv, "-d") == 0){
      /* font dir */
      if (n_dirs == NDIRS){
	fprintf(stderr, "Too many eKanji font directories\n");
	exit(1);
      }
      xargv++; xargc--;
      check_argc(xargc);
      fontdirs[n_dirs++] = x_strdup(*xargv);

    } else if (strcmp(*xargv, "-r") == 0){
      xargv++; xargc--;
      check_argc(xargc);
      dpi = strdup(*xargv);

    } else {
      if (*xargv[0] == '-'){
	fprintf(stderr, "vflmkekan: unknown option %s\n", *xargv);
	exit(1);
      }
      break;

    }
  }

  banner("eKanji", "vflmkekan", cmdline);

  gen_class_deafult();    

  return 0;
}



void
gen_class_deafult(void)
{
  int   i;

  printf("(%s %s", 
	 VF_CAPE_VFLIBCAP_CLASS_DEFAULT_DEFINITION, FONTCLASS_NAME);
  printf("\n  (%s", VF_CAPE_FONT_DIRECTORIES);
  for (i = 0; i < n_dirs; i++)
    printf("\n       \"%s\"", fontdirs[i]);
  printf(")");
  printf("\n  (%s %d)", VF_CAPE_EK_FONT_DOT_SIZE, 24);
  printf(")\n");
  printf("\n");
}

