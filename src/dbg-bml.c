/*
 * dbg-bml.c - test program for bitmaplist class.
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


#include <stdio.h>
#include <stdlib.h>
#include "VFlib-3_7.h"
#include "VFsys.h"


int display(int,int,double,double,int,double);

void  usage(void);

int
main(int argc, char **argv)
{
  char        *vflibcap;
  double      mag, std_mag;
  char        *font_name;
  int         font_id, code;
  int         rp_x, rp_y, mv_x, mv_y; 
  int         delta_x, delta_y, ign_mv;
  VF_BITMAP   bm, composed_bitmap;
  struct vf_s_bitmaplist   the_bitmaplist, *bitmaplist;

  vflibcap = NULL;
  std_mag  = 1.0;
  delta_x  = 0;
  delta_y  = 0;
  ign_mv   = 0;
  mv_x     = 0;
  mv_y     = 0;

  rp_x = 0;
  rp_y = 0;

  bitmaplist = &the_bitmaplist;
  VF_BitmapListInit(bitmaplist);

  argc--; argv++;
  while ((argc > 0) && (*argv[0] == '-')){
    if (strcmp(argv[0], "-v") == 0){
      vflibcap = argv[1];
      argc--; argv++;
    } else if (strcmp(argv[0], "-M") == 0){
      std_mag = atof(argv[1]); 
      argc--; argv++;
    } else if ((strcmp(argv[0], "-h") == 0)
	       || (strcmp(argv[0], "-help") == 0)){
      usage();
    } else {
      break;
    }
    argc--; argv++;
  }

  if (VF_Init(vflibcap, NULL) < 0){
    fprintf(stderr, "Error %d in VF_Init().\n", vf_error);
    exit(0);
  }
     
  mag = std_mag;
  while (argc > 0){
    if (strcmp(argv[0], "-m") == 0){
      mag = atof(argv[1]) * std_mag; 
      argc--; argv++;
    } else if (strcmp(argv[0], "-f") == 0){
      font_name = argv[1];
      argc--; argv++;
      if ((font_id = VF_OpenFont2(font_name, -1, 1, 1)) < 0){
	fprintf(stderr, "Error %d in VF_OpenFont2(%s)\n", 
		vf_error, font_name);
	exit(0);
      }
    } else if (strcmp(argv[0], "-x") == 0){
      mv_x += atoi(argv[1]);
      argc--; argv++;
    } else if (strcmp(argv[0], "-y") == 0){
      mv_y += atoi(argv[1]);
      argc--; argv++;
    } else if (strcmp(argv[0], "-dx") == 0){
      delta_x = atoi(argv[1]);
      argc--; argv++;
    } else if (strcmp(argv[0], "-dy") == 0){
      delta_y = atoi(argv[1]);
      argc--; argv++;
    } else if (strcmp(argv[0], "-n") == 0){
      ign_mv = 1;
    } else if (font_id < 0){
      fprintf(stderr, "Error: Font is not selected.\n");
      exit(0);
    } else if (font_id >= 0){
      sscanf(argv[0], "%i", &code);
      bm = VF_GetBitmap2(font_id, code, mag, mag);
      mag = std_mag;
      if (bm == NULL){
	fprintf(stderr, "Error: Fauiled to obtain a glyph.\n");
	exit(1);
      }
      if (ign_mv == 1){
	mv_x = 0;
	mv_y = 0;
      }
      VF_BitmapListPut(bitmaplist, bm, 
		       rp_x + mv_x + delta_x, rp_y + mv_y + delta_y);
      rp_x += mv_x;
      rp_y += mv_y;
      mv_x = bm->mv_x;
      mv_y = bm->mv_y;

      delta_x = 0;
      delta_y = 0;
      ign_mv  = 0;
    }
    argc--; argv++;
  }

  if ((composed_bitmap = VF_BitmapListCompose(bitmaplist)) != NULL){
    VF_DumpBitmap(composed_bitmap);
  }

  VF_BitmapListFinish(bitmaplist);

  return 0;
}


void
usage(void)
{
  printf("dbg-bml - Debug bitmaplist class\n");
  printf("Usage: bdg-bml [OPTIONS] [ARGS]\n");
  printf("OPTIONS: -v VFLIBCAP  Select vflibcap file\n");
  printf("         -M MAG       Change default magnification\n");
  printf("         -h           Print help\n");
  printf("ARGS:    -f FONT      Change fonts\n");
  printf("         -m MAG       Change magnification\n");
  printf("         -x NPIX      Move ref point NPIX pixels holizontally\n");
  printf("         -y NPIX      Move ref point NPIX pixels vertically\n");
  printf("         -dx NPIX     Shift next glyph NPIX pixels holizontally\n");
  printf("         -dx NPIX     Shift next glyph NPIX pixels vertically\n");
  printf("         -n           Do not move ref point\n");
  printf("         CODE_POINT   Add glyph for CODE_POINT\n");
  printf("  -m has effect on only current font.\n");
  printf("  -dx, -dy and -n have effects on only for next one glyph.\n");
  printf("Example 1:  dbg-bml -f timI24.pcf 0x69 0x66 0x66\n");
  printf("Example 2:  dbg-bml -f timR24.pcf -m 2 0x41\n");
  printf("Example 3:  dbg-bml -f timR24.pcf 0x41 -f timI24.pcf 0x42\n");
  exit(0);
}


/*EOF*/
