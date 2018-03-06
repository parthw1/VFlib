/* 
 * vflmkjpc.c 
 * by Hirotsugu Kakugawa
 * - a vflibcap entry generator for japanese-comic fonts
 * - This program prints vflibcap entries to standard output.
 *
 *  10 Dec 2001
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
#include  "fsearch.h"
#include  "vflmklib.h"
#include  "comic.h"


char  buff[BUFSIZ];
char  s1[BUFSIZ], s2[BUFSIZ], s3[BUFSIZ], s4[BUFSIZ], s5[BUFSIZ];

char *cmdline = NULL; 
char *fext = "";

void  gen_class_deafult(void);
void  gen_font_def(char *font, 
		   char *kanji, char *kana_h, char *kana_k, char *misc);
void   usage(void);



int 
main(int argc, char **argv)
{
  int    xargc;
  char **xargv;
  FILE  *fp;
  int   lno;

  cmdline = copy_cmdline(argc, argv);

  xargc = argc; 
  xargv = argv;

  for (xargc--,xargv++; xargc > 0; xargc--,xargv++){
    if ((strcmp(*xargv, "--help") == 0)
	|| (strcmp(*xargv, "-help") == 0)){
      usage();
    } if ((strcmp(*xargv, "-x") == 0) && (xargc > 1)){
      xargc--,xargv++;
      fext = *xargv;
    } else if (*xargv[0] != '-'){
      break;
    } else {
      usage();
    }
  }

  banner("japanese-comic", "vflmkjpc", cmdline);

  gen_class_deafult();    


  for ( ; xargc > 0; xargc--,xargv++){
    lno = 0;
    fp = fopen(*xargv, "r");
    if (fp == NULL){
      fprintf(stderr, "Cannot read %s\n", *xargv);
      exit(1);
    }
    while (fgets(buff, sizeof(buff), fp) != NULL){
      lno++;    
      if (sscanf(buff, "%s%s%s%s%s", s1, s2, s3, s4, s5) != 5){
	fprintf(stderr, "Format error (line %d) in %s\n", lno, *xargv);
	exit(1);
      }
#if 0
      printf("** %s: '%s' '%s' '%s' '%s'\n", s1, s2, s3, s4, s5);
#endif
      gen_font_def(s1, s2, s3, s4, s5);
    }
    fclose(fp);
  }

  return 0;
}


void 
usage(void)
{
  printf("vflmkjpc: generates vflibcap entries for japanese-comic fonts\n");
  printf("Usage: vflmkjpc DB-FILE...f\n");
  exit(0);
}



void
gen_class_deafult(void)
{
  printf("(%s %s", 
	 VF_CAPE_VFLIBCAP_CLASS_DEFAULT_DEFINITION, FONTCLASS_NAME_COMIC);
  printf(")\n");
  printf("\n");
}

void
gen_font_def(char *font,
	     char *kanji, char *kana_h, char *kana_k, char *misc)
{
  printf("(%s %s%s", 
	 VF_CAPE_VFLIBCAP_FONT_ENTRY_DEFINITION, font, fext);
  printf("\t  (%s \"%s\")", VF_CAPE_FONT_CLASS,  FONTCLASS_NAME_COMIC);
  printf("\n  (%s \"%s%s\")", VF_CAPE_COMIC_KANJI_FONT, kanji, fext);
  printf("\n  (%s \"%s%s\")", VF_CAPE_COMIC_HIRAKANA_FONT, kana_h, fext);
  printf(" (%s \"%s%s\")", VF_CAPE_COMIC_KATAKANA_FONT, kana_k, fext);
  printf("\n  (%s \"%s%s\")", VF_CAPE_COMIC_ALNUM_FONT, misc, fext);
  printf(" (%s \"%s%s\")", VF_CAPE_COMIC_SYMBOL_FONT,   misc, fext);
  printf(")\n");
}

