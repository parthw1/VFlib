/* 
 * vflbanner.c - a banner by VFlib
 * by Hirotsugu Kakugawa
 *
 *
 */
/*
 * Copyright (C) 1996-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#if defined(HAVE_STRING_H) || defined(STDC_HEADERS)
#  include  <string.h>
#else
#  include  <strings.h>
#endif
#include "VFlib-3_7.h"

#define  DEFAULT_FONT  "timR18.pcf"


char    *vflibcap;
char    *fontname;
double   mag;

void  usage(void);
void  vflbanner(FILE *fp);


int
main(int argc, char **argv)
{
  vflibcap = NULL;
  fontname = DEFAULT_FONT;
  mag      = 1.0;

  --argc; argv++;
  while (argc > 0){
    if ((argc >= 1)
	&& ((strcmp(argv[0], "-h") == 0) || (strcmp(argv[0], "--help") == 0))){
      usage();
      exit(0);
    } else if ((argc >= 2) && (strcmp(argv[0], "-v") == 0)){
      --argc; argv++;
      vflibcap = argv[0];
      --argc; argv++;
    } else if ((argc >= 2) && (strcmp(argv[0], "-f") == 0)){
      --argc; argv++;
      fontname = argv[0];
      --argc; argv++;
    } else if ((argc >= 2) && (strcmp(argv[0], "-m") == 0)){
      --argc; argv++;
      mag = atof(argv[0]);
      --argc; argv++;
    } else {
      break;
    }
  }

  vflbanner(stdin);

  return 0;
}

void usage(void)
{
  printf("vflbanner - a banner program using VFlib\n");
  printf("Usage: vflbanner [-v vflibcap] [-m mag] [-f fontname]\n"); 
  printf("This program reads a text from standard input.  It supports\n");
  printf("1-byte encoded font only. Thus, `ctextpgm' is better than this.\n");
}


void
vflbanner(FILE  *fp)
{
  int  fid;
  int  ch; 
  int  pos_x, pos_y; 
  VF_BITMAP  bm, page_bm;
  struct vf_s_bitmaplist  PageBuff;

  if (VF_Init(vflibcap, NULL) < 0){
    printf("VFlib initialization error");
    switch (vf_error){
    case VF_ERR_INTERNAL:
      printf(" - Internal error.\n"); break;
    case VF_ERR_NO_MEMORY:
      printf(" - Server runs out of memory.\n"); break;
    case VF_ERR_NO_VFLIBCAP:
      printf(" -  No vflibcap.\n"); break;
    default: 
      printf(" -  Error code %d\n", vf_error); break;
    }
    fflush(stdout);
    exit(1);
  }

  if ((fid = VF_OpenFont1(fontname, -1, -1, -1, mag, mag)) < 0)
    return;
    
  VF_BitmapListInit(&PageBuff);

  pos_x = pos_y = 0; 
  while ((ch = getc(fp)) != EOF){
    if (iscntrl(ch))
      continue;
    if (!isprint(ch))
      ch = ' ';
    if ((bm = VF_GetBitmap1(fid, (long)ch, 1, 1)) == NULL)
      continue;
    VF_BitmapListPut(&PageBuff, bm, pos_x, pos_y);
    pos_x = pos_x + bm->mv_x;
  }

  page_bm = VF_BitmapListCompose(&PageBuff);
  VF_DumpBitmap(page_bm);
  VF_BitmapListFinish(&PageBuff);
  VF_FreeBitmap(page_bm);

  VF_CloseFont(fid);
}

/*EOF*/
