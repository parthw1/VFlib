/* 
 * dbg_ml.c - 
 * by Hirotsugu Kakugawa
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

#ifdef __FreeBSD__
#include <sys/types.h> 
#include <sys/time.h> 
#include <sys/resource.h> 
#define RUSAGE_SELF 0
#endif


#include "VFlib-3_7.h"

#define  DEFAULT_FONT  "timR18.pcf"


char    *vflibcap;
char    *fontname;
double   mag;

void  usage(void);
void  test(int, int);


int
main(int argc, char **argv)
{
  int  code;
  int  w;

  code     = -1;
  vflibcap = NULL;
  fontname = DEFAULT_FONT;
  mag      = 1.0;
  w        = 1;

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
    } else if ((argc >= 2) && (strcmp(argv[0], "-w") == 0)){
      --argc; argv++;
      w = atof(argv[0]);
      --argc; argv++;
    } else if (argv[0][0] == '='){
      printf("Unknown option: %s\n", *argv);
      usage();
      exit(0);
    } else {
      sscanf(argv[0], "%i", &code);
      break;
    }
  }

  test(code, w);

  return 0;
}

void usage(void)
{
  printf("dbg_ml\n");
  printf("Usage: dbg_ml [-v vflibcap] [-m mag] [-f fontname] charcode\n"); 
}



void
test(int code, int w)
{
  int  fid, i;
  VF_BITMAP  bm;
#ifdef __FreeBSD__
  struct rusage  ru;
#endif


  if (VF_Init(vflibcap, NULL) < 0){
    printf("VFlib initialization error\n");
    exit(1);
  }

  printf("** font=%s, char=%d", fontname, code);

  i = 1;
  for (;;){
    if ((i % 50) == 1){
#ifdef __FreeBSD__
      printf("\n");
      if (getrusage(RUSAGE_SELF, &ru) >= 0){
	printf(" maxrss=%ldK", ru.ru_maxrss);
      }
#endif
      printf("\n");
      printf("% 6d ", i);
    }
    printf("*"); fflush(stdout);
    i++;
    if ((fid = VF_OpenFont1(fontname, -1, -1, -1, mag, mag)) < 0){
      printf("\nCan't open font\n");
      return;
    }
    if (code > 0){
      bm = VF_GetBitmap1(fid, code, 1, 1);
      if (bm == NULL){
	printf("\nCan't get bitmap\n");
	return;
      }
      VF_FreeBitmap(bm);
    }
    VF_CloseFont(fid);
    fid = -1;
    if (w > 0) 
      usleep(1000*w);
  }
}

/*EOF*/
