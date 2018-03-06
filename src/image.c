/* image.c
 *  --- Print bitmap in several graphics formats
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
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif
#include <math.h>
#include "config.h"
#include "VFlib-3_7.h"
#include "VFsys.h"
#include "bitmap.h"
#include "consts.h"


Private void culc_margins(int bm_w, int bm_h,
			  int image_width, int image_height, 
			  int position_x, int position_y, 
			  int *margin_l, int *margin_r, 
			  int *margin_t, int *margin_b);

Private unsigned char  bits[] = {
  0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};



/**
 ** VF_ImageOut_PBMAscii()
 ** --- PBM ASCII 
 **/

Public int
VF_ImageOut_PBMAscii(VF_BITMAP bm, FILE *fp, 
		     int image_width, int image_height,
		     int position_x, int position_y, 
		     int margin_l, int margin_r, int margin_t, int margin_b,
		     int reverse, int shrink,
		     char *prog, char *title)
{
  int            x, y, n;
  unsigned char  w, *p;
  char           pix0, pix1;
#define PBM_ASCII_BITS_PER_LINE  25

  culc_margins(bm->bbx_width, bm->bbx_height, 
	       image_width, image_height, position_x, position_y, 
	       &margin_l, &margin_r, &margin_t, &margin_b);

  if (shrink <= 0)
    shrink = 1;

  if (reverse == 0){
    pix0 = '0';
    pix1 = '1';
  } else {
    pix0 = '1';
    pix1 = '0';
  }

  fprintf(fp, "P1\n");
  fprintf(fp, "%d %d\n", 
	  margin_l + bm->bbx_width + margin_r, 
	  margin_t + bm->bbx_height + margin_b);
  if (prog == NULL)
    prog = "VFlib";
  fprintf(fp, "# Created by %s\n", prog);
  fprintf(fp, "# %s\n", title);

  n = 0;
  p = bm->bitmap;

  /* top margin */
  for (y = 0; y < margin_t; y++){
    for (x = 0; x < margin_l + bm->bbx_width + margin_r; x++){
      fprintf(fp, "%c ", pix0);
      if (++n >= PBM_ASCII_BITS_PER_LINE){
	n = 0; 
	fprintf(fp, "\n");
      }
    }
  }

  for (y = 0; y < bm->bbx_height; y++){
    /* left margin */
    for (x = 0; x < margin_l; x++)
      fprintf(fp, "%c ", pix0);
    /* bitmap */
    for (x = 0; x < bm->bbx_width; x++){
      w = p[x/8];
      fprintf(fp, "%c ", ((w&bits[x%8])==0)?pix0:pix1);
      if (++n >= PBM_ASCII_BITS_PER_LINE){
	n = 0;
	fprintf(fp, "\n");
      }
    }
    /* right margin */
    for (x = 0; x < margin_r; x++){
      fprintf(fp, "%c ", pix0);
      if (++n >= PBM_ASCII_BITS_PER_LINE){
	n = 0;
	fprintf(fp, "\n");
      }
    }

    p = p + bm->raster;
  }

  /* bottom margin */
  for (y = 0; y < margin_b; y++){
    for (x = 0; x < margin_l + bm->bbx_width + margin_r; x++){
      fprintf(fp, "%c ", pix0);
      if (++n >= PBM_ASCII_BITS_PER_LINE){
	n = 0;
	fprintf(fp, "\n");
      }
    }
  }
  fprintf(fp, "\n");

  return 0;
}


/**
 ** VF_ImageOut_PGMAscii()
 ** --- PGM ASCII
 **/
Public int
VF_ImageOut_PGMAscii(VF_BITMAP bm, FILE *fp, 
		     int image_width, int image_height,
		     int position_x, int position_y, 
		     int margin_l, int margin_r, int margin_t, int margin_b,
		     int reverse, int shrink,
		     char *prog, char *title)
{
  int            x, y, s, n;
  unsigned char *p;
  int           *buff;
  int            w, h, y2, max_val;
#define PGM_ASCII_BITS_PER_LINE  15
#define PGM_BIT(x)    (reverse==1) ?        ((255*(x))/max_val) \
                                   : (255 - ((255*(x))/max_val))

  if (shrink <= 0)
    shrink = 1;
  max_val = shrink * shrink;
  w = (bm->bbx_width  + shrink - 1) / shrink;
  h = (bm->bbx_height + shrink - 1) / shrink;
  if ((buff = calloc(w, sizeof(int))) == NULL){
    vf_error = VF_ERR_NO_MEMORY;
    return -1;
  }

  culc_margins(w, h, image_width, image_height, position_x, position_y, 
	       &margin_l, &margin_r, &margin_t, &margin_b);

  fprintf(fp, "P2\n");
  if (prog == NULL)
    prog = "VFlib";
  fprintf(fp, "# Created by %s\n", prog);
  fprintf(fp, "# %s\n", title);
  fprintf(fp, "%d %d\n", margin_l + w + margin_l, margin_t + h + margin_b);
  fprintf(fp, "%d\n", 255);

  n = 0;

  /* top margin */
  for (y = 0; y < margin_t; y++){
    for (x = 0; x < margin_l + w + margin_r; x++){
      fprintf(fp, "%d ", PGM_BIT(0));
      if (++n >= PGM_ASCII_BITS_PER_LINE){
	n = 0; 
	fprintf(fp, "\n");
      }
    }
  }

  for (y = 0; y < h; y++){
    /* left margin */
    for (x = 0; x < margin_l; x++)
      fprintf(fp, "%d ", PGM_BIT(0));
    /* make graymap */
    for (x = 0; x < w; x++)
      buff[x] = 0;
    p = &bm->bitmap[y * shrink * bm->raster];
    s = ((bm->bbx_height/shrink) == y) ? (bm->bbx_height % shrink) : shrink;
    for (y2 = 0; y2 < s; y2++){
      for (x = 0; x < bm->bbx_width; x++){ 
	if ((p[x/8] & bits[x%8]) != 0)
	  buff[x/shrink] += 1;
      }
      p += bm->raster;
    }
    /* output graymap */
    for (x = 0; x < w; x++){
      fprintf(fp, "%d ", PGM_BIT(buff[x]));
      if (++n >= PGM_ASCII_BITS_PER_LINE){
	n = 0;
	fprintf(fp, "\n");
      }
    }
   /* right margin */
    for (x = 0; x < margin_r; x++){
      fprintf(fp, "%d ", PGM_BIT(0));
      if (++n >= PGM_ASCII_BITS_PER_LINE){
	n = 0;
	fprintf(fp, "\n");
      }
    }
  }

  /* bottom margin */
  for (y = 0; y < margin_b; y++){
    for (x = 0; x < margin_l + w + margin_r; x++){
      fprintf(fp, "%d ", PGM_BIT(0));
      if (++n >= PGM_ASCII_BITS_PER_LINE){
	n = 0;
	fprintf(fp, "\n");
      }
    }
  }
  fprintf(fp, "\n");

  vf_free(buff);
  return 0;
}


/**
 ** VF_ImageOut_PGMRaw()
 ** --- PGM RAW
 **/
Public int
VF_ImageOut_PGMRaw(VF_BITMAP bm, FILE *fp, 
		   int image_width, int image_height,
		   int position_x, int position_y, 
		   int margin_l, int margin_r, int margin_t, int margin_b,
		   int reverse, int shrink,
		   char *prog, char *title)
{
  int            x, y;
  unsigned char *p;
  int           *buff;
  int            w, h, s, y2, max_val;
#define PGM_BIT(x)    (reverse==1) ?        ((255*(x))/max_val) \
                                   : (255 - ((255*(x))/max_val))

  if (shrink <= 0)
    shrink = 1;
  max_val = shrink * shrink;
  w = (bm->bbx_width  + shrink - 1) / shrink;
  h = (bm->bbx_height + shrink - 1) / shrink;
  if ((buff = calloc(w, sizeof(int))) == NULL){
    vf_error = VF_ERR_NO_MEMORY;
    return -1;
  }

  culc_margins(w, h, image_width, image_height, position_x, position_y, 
	       &margin_l, &margin_r, &margin_t, &margin_b);

  fprintf(fp, "P5\n");
  fprintf(fp, "# Created by %s\n", prog);
  fprintf(fp, "# %s\n", title);
  fprintf(fp, "%d %d\n", 
	  margin_l + w + margin_r, margin_t + h + margin_b);
  fprintf(fp, "%d\n", 255);

  /* top margin */
  for (y = 0; y < margin_t; y++){
    for (x = 0; x < margin_l + w + margin_r; x++){
      fprintf(fp, "%c", (unsigned char)PGM_BIT(0));
    }
  }

  for (y = 0; y < h; y++){
    /* left margin */
    for (x = 0; x < margin_l; x++)
      fprintf(fp, "%c", (unsigned char)PGM_BIT(0));
    /* make graymap */
    for (x = 0; x < w; x++)
      buff[x] = 0;
    p = &bm->bitmap[y * shrink * bm->raster];
    s = ((bm->bbx_height/shrink) == y) ? (bm->bbx_height % shrink) : shrink;
    for (y2 = 0; y2 < s; y2++){
      for (x = 0; x < bm->bbx_width; x++){ 
	if ((p[x/8] & bits[x%8]) != 0)
	  buff[x/shrink] += 1;
      }
      p += bm->raster;
    }
    /* output graymap */
    for (x = 0; x < w; x++)
      fprintf(fp, "%c", (unsigned char)PGM_BIT(buff[x]));
    /* right margin */
    for (x = 0; x < margin_r; x++)
      fprintf(fp, "%c", (unsigned char)PGM_BIT(0));
  }

  /* bottom margin */
  for (y = 0; y < margin_b; y++){
    for (x = 0; x < margin_l + w + margin_r; x++)
      fprintf(fp, "%c", (unsigned char)PGM_BIT(0));
  }

  vf_free(buff);
  return 0;
}


/**
 ** VF_ImageOut_EPS()
 ** --- EPS
 **/
Public int
VF_ImageOut_EPS(VF_BITMAP bm, FILE *fp, 
		int image_width, int image_height,
		int position_x, int position_y, 
		int margin_l, int margin_r, int margin_t, int margin_b,
		int reverse, int shrink,
		char *prog, char *title, 
		double ptsize, int pixsize)
{
  int            x, y;
  unsigned char *p;
  int           *buff;
  int            w, h, y2, s, max_val;
  int            eps_w, eps_h, bbxx, bbxy, n;
#define EPS_SIZE(s) \
     ((ptsize <= 0) ? (12.0*(s)*shrink/16.0) \
                    : (ptsize*(s)*shrink/(double)pixsize))
#define EPS_PIX(x) \
     ((reverse==1) ? ((255*(x))/max_val) : (255 - ((255*(x))/max_val)))
#define EPS_PUT_PIX(b) \
     { fprintf(fp,"%02x",b); if (++n > 32){ fprintf(fp,"\n"); n=0;} }

  if (shrink < 0)
    shrink = 1;
  max_val = shrink * shrink;
  w = (bm->bbx_width  + shrink - 1) / shrink;
  h = (bm->bbx_height + shrink - 1) / shrink;
  if ((buff = calloc(w, sizeof(int))) == NULL){
    vf_error = VF_ERR_NO_MEMORY;
    return -1;
  }

  culc_margins(w, h, image_width, image_height, position_x, position_y, 
	       &margin_l, &margin_r, &margin_t, &margin_b);

  eps_w = margin_l + w + margin_r;
  eps_h = margin_t + h + margin_b;

  bbxx = 72;
  bbxy = 792 - EPS_SIZE(h);

  fprintf(fp, "%%!PS-Adobe-2.0 EPSF-2.0\n");
  fprintf(fp, "%%%%Creator: %s\n", prog);
  fprintf(fp, "%%%%Title: %s\n", title);
  fprintf(fp, "%%%%Pages: 1\n");
  fprintf(fp, "%%%%BoundingBox: %.3f %.3f %.3f %.3f\n", 
	  (double)bbxx, (double)bbxy,
	  bbxx+EPS_SIZE(eps_w), bbxy+EPS_SIZE(eps_h));
  fprintf(fp, "%%%%EndComments\n");
  fprintf(fp, "/readstr {\n");
  fprintf(fp, "  currentfile exch readhexstring pop\n");
  fprintf(fp, "} bind def\n");
  fprintf(fp, "/picstr %d string def\n", eps_w);
  fprintf(fp, "%%%%EndProlog\n");
  fprintf(fp, "%%%%Page: 1 1\n");
  fprintf(fp, "gsave\n");
  fprintf(fp, "%d %d translate\n", bbxx, bbxy);
  fprintf(fp, "%.3f %.3f scale\n", EPS_SIZE(eps_w), EPS_SIZE(eps_h));
  fprintf(fp, "%d %d 8\n", eps_w, eps_h);
  fprintf(fp, "[ %d 0 0 -%d 0 %d ]\n", eps_w, eps_h, eps_h);
  fprintf(fp, "{ picstr readstr }\n");
  fprintf(fp, "bind image\n");

  n = 0;
  /* top margin */
  for (y = 0; y < margin_t; y++){
    for (x = 0; x < margin_l + w + margin_r; x++)
      EPS_PUT_PIX(EPS_PIX(0));
  }
  p = bm->bitmap;
  for (y = 0; y < h; y++){
    /* left margin */
    for (x = 0; x < margin_l; x++)
      EPS_PUT_PIX(EPS_PIX(0));
    /* make graymap */
    for (x = 0; x < w; x++)
      buff[x] = 0;
    s = ((bm->bbx_height/shrink) == y) ? (bm->bbx_height % shrink) : shrink;
    for (y2 = 0; y2 < s; y2++){
      for (x = 0; x < bm->bbx_width; x++){ 
	if ((p[x/8] & bits[x%8]) != 0)
	  buff[x/shrink] += 1;
      }
      p += bm->raster;
    }
    /* output graymap */
    for (x = 0; x < w; x++)
      EPS_PUT_PIX(EPS_PIX(buff[x]));
    /* right margin */
    for (x = 0; x < margin_r; x++)
      EPS_PUT_PIX(EPS_PIX(0));
  }
  /* bottom margin */
  for (y = 0; y < margin_b; y++){
    for (x = 0; x < margin_l + w + margin_r; x++)
      EPS_PUT_PIX(EPS_PIX(0));
  }

  if (n != 0)
    fprintf(fp, "\n");
  fprintf(fp, "grestore\n");
  fprintf(fp, "showpage\n");
  fprintf(fp, "%%%%Trailer\n");

  vf_free(buff);
  return 0;
}

/**
 ** VF_ImageOut_ASCIIArt()
 ** --- ASCII Art (Horizontal)
 **/
Public int
VF_ImageOut_ASCIIArt(VF_BITMAP bm, FILE *fp, 
		     int image_width, int image_height,
		     int position_x, int position_y, 
		     int margin_l, int margin_r, int margin_t, int margin_b,
		     int reverse, int shrink)
{
  int             x, y, w, h, y2, s, max_val, j;
  int            *buff;
  unsigned char  *p;
  char           *pixs;
  char           *chspec = "  ****";
  int             speclen;

  if (shrink < 0)
    shrink = 1;
  max_val = shrink * shrink;
  w = (bm->bbx_width  + shrink - 1) / shrink;
  h = (bm->bbx_height + shrink - 1) / shrink;
  if ((buff = (int*)calloc(w, sizeof(int))) == NULL){
    vf_error = VF_ERR_NO_MEMORY;
    return -1;
  }

  culc_margins(w, h, image_width, image_height, position_x, position_y, 
	       &margin_l, &margin_r, &margin_t, &margin_b);

  if ((pixs = (char*)malloc(max_val+1)) == NULL){
    vf_error = VF_ERR_NO_MEMORY;
    return -1;
  }

  speclen = strlen(chspec);
  if (reverse == 0){
    for (j = 0; j <= max_val; j++)
      pixs[j] = chspec[(speclen * j) / (max_val+1)];
    pixs[0] = chspec[0];
  } else {
    for (j = 0; j <= max_val; j++)
      pixs[max_val - j] = chspec[(speclen * j) / (max_val+1)];
    pixs[max_val] = chspec[0];
  }

  p = bm->bitmap;

  /* top margin */
  for (y = 0; y < margin_t; y++){
    for (x = 0; x < margin_l + w + margin_r; x++)
      fprintf(fp, "%c", pixs[0]);
    fprintf(fp, "\n");
  }

  for (y = 0; y < h; y++){
    /* left margin */
    for (x = 0; x < margin_l; x++)
      fprintf(fp, "%c", pixs[0]);
    /* make graymap */
    for (x = 0; x < w; x++)
      buff[x] = 0;
    p = &bm->bitmap[y * shrink * bm->raster];
    s = ((bm->bbx_height/shrink) == y) ? (bm->bbx_height % shrink) : shrink;
    for (y2 = 0; y2 < s; y2++){
      for (x = 0; x < bm->bbx_width; x++){ 
	if ((p[x/8] & bits[x%8]) != 0)
	  buff[x/shrink] += 1;
      }
      p += bm->raster;
    }
    /* output bitmap */
    for (x = 0; x < w; x++)
      fprintf(fp, "%c", pixs[buff[x]]);
    /* right margin */
    for (x = 0; x < margin_r; x++)
      fprintf(fp, "%c", pixs[0]);
    fprintf(fp, "\n");
  }

  /* bottom margin */
  for (y = 0; y < margin_b; y++){
    for (x = 0; x < margin_l + w + margin_r; x++)
      fprintf(fp, "%c", pixs[0]);
    fprintf(fp, "\n");
  }

  vf_free(buff);
  vf_free(pixs);
  return 0;
}


/**
 ** VF_ImageOut_ASCIIArtV()
 ** --- ASCII Art (Vertical)
 **/
Public int
VF_ImageOut_ASCIIArtV(VF_BITMAP bm, FILE *fp, 
		      int image_width, int image_height,
		      int position_x, int position_y, 
		      int margin_l, int margin_r, int margin_t, int margin_b,
		      int reverse, int shrink)
{
  int            x, y, w, h, x2, s, max_val, j;
  int            *buff;
  unsigned char  *p;
  char           *pixs;
  char           *chspec = "  ****";
  int             speclen;

  if (shrink < 0)
    shrink = 1;
  max_val = shrink * shrink;
  w = (bm->bbx_width  + shrink - 1) / shrink;
  h = (bm->bbx_height + shrink - 1) / shrink;
  if ((buff = (int*)calloc(h, sizeof(int))) == NULL){
    vf_error = VF_ERR_NO_MEMORY;
    return -1;
  }

  culc_margins(w, h, image_width, image_height, position_x, position_y, 
	       &margin_l, &margin_r, &margin_t, &margin_b);

  if ((pixs = (char*)malloc(max_val+1)) == NULL){
    vf_error = VF_ERR_NO_MEMORY;
    return -1;
  }
  speclen = strlen(chspec);
  if (reverse == 0){
    for (j = 0; j <= max_val; j++)
      pixs[j] = chspec[(speclen * j) / (max_val+1)];
    pixs[0] = chspec[0];
  } else {
    for (j = 0; j <= max_val; j++)
      pixs[max_val - j] = chspec[(speclen * j) / (max_val+1)];
    pixs[max_val] = chspec[0];
  }

  /* left margin */
  for (x = 0; x < margin_l; x++){
    for (y = margin_b-1; y >= 0; --y)
      fprintf(fp, "%c", pixs[0]);
    for (y = h-1; y >= 0; --y)
      fprintf(fp, "%c", pixs[0]);
    for (y = margin_t-1; y >= 0; --y)
      fprintf(fp, "%c", pixs[0]);
    fprintf(fp, "\n");
  }

  /* body */
  for (x = 0; x < w; x++){
    /* bottom margin */
    for (y = margin_b; y > 0; --y)
      fprintf(fp, "%c", pixs[0]);
    /* make graymap */
    for (y = 0; y < h; y++)
      buff[y] = 0;
    p = bm->bitmap;
    for (y = 0; y < bm->bbx_height; y++){
      s = ((bm->bbx_width/shrink) == x) ? (bm->bbx_width % shrink) : shrink;
      for (x2 = 0; x2 < s; x2++){ 
	if ((p[(x * shrink + x2)/8] & bits[(x * shrink + x2)%8]) != 0)
	  buff[y/shrink] += 1;
      }
      p += bm->raster;
    }
    /* output bitmap */
    for (y = h-1; y >= 0; --y)
      fprintf(fp, "%c", pixs[buff[y]]);
    /* top margin */
    for (y = margin_t; y > 0; --y)
      fprintf(fp, "%c", pixs[0]);
    fprintf(fp, "\n");
  }

  /* right margin */
  for (x = 0; x < margin_r; x++){
    for (y = margin_b-1; y >= 0; --y)
      fprintf(fp, "%c", pixs[0]);
    for (y = h-1; y >= 0; --y)
      fprintf(fp, "%c", pixs[0]);
    for (y = margin_t-1; y >= 0; --y)
      fprintf(fp, "%c", pixs[0]);
    fprintf(fp, "\n");
  }

  vf_free(buff);
  vf_free(pixs);
  return 0;
}




Private void  
culc_margins(int  bm_w, int bm_h,
	     int image_width, int image_height, 
	     int position_x, int position_y, 
	     int *margin_l, int *margin_r, int *margin_t, int *margin_b)
{
  /* top and bottom margins */
  if (image_height < 0){
    image_height = bm_h;
    if (*margin_t >= 0)
      image_height += *margin_t;
    else 
      *margin_t = 0; 
    if (*margin_b >= 0)
      image_height += *margin_b;
    else 
      *margin_b = 0; 
  } else /* image_height >= 0 */ { 
    switch (position_y){
    default:
    case VF_IMAGEOUT_POSITION_NONE:
    case VF_IMAGEOUT_POSITION_TOP:
      if (*margin_t < 0)
	*margin_t = 0;
      *margin_b = image_height - *margin_t - bm_h; 
      break;
    case VF_IMAGEOUT_POSITION_BOTTOM:
      if (*margin_b < 0)
	*margin_b = image_height - bm_h;
      *margin_t = image_height - bm_h - *margin_b; 
      break;
    case VF_IMAGEOUT_POSITION_CENTER:
      *margin_t = (image_height - bm_h) / 2;
      *margin_b = image_height - *margin_t - bm_h;
      break;
    }
  }
  if (*margin_t < 0){
    fprintf(stderr, "VFlib Warning: page height is small. (>=%d)\n", 
	    *margin_b + bm_h);
    *margin_t = 0;
  }
  if (*margin_b < 0){
    fprintf(stderr, "VFlib Warning: page height is small. (>=%d)\n", 
	    *margin_t + bm_h);
    *margin_b = 0;
  }

  /* left and right margins */
  if (image_width < 0){
    image_width = bm_w;
    if (*margin_l >= 0)
      image_width += *margin_l;
    else 
      *margin_l = 0; 
    if (*margin_r >= 0)
      image_width += *margin_r;
    else 
      *margin_r = 0; 
  } else /* image_width >= 0 */ { 
    switch (position_x){
    default:
    case VF_IMAGEOUT_POSITION_NONE:
    case VF_IMAGEOUT_POSITION_LEFT:
      if (*margin_l < 0)
	*margin_l = 0;
      *margin_r = image_width - *margin_l - bm_w; 
      break;
    case VF_IMAGEOUT_POSITION_RIGHT:
      if (*margin_r < 0)
	*margin_r = image_width - bm_w;
      *margin_l = image_width - bm_w - *margin_l; 
      break;
    case VF_IMAGEOUT_POSITION_CENTER:
      *margin_l = (image_width - bm_w) / 2;
      *margin_r = image_width - *margin_l - bm_w;
      break;
    }
  }
  if (*margin_l < 0){
    fprintf(stderr, "VFlib Warning: page width is small. (>=%d)\n", 
	    *margin_r + bm_w);
    *margin_l = 0;
  }
  if (*margin_r < 0){
    fprintf(stderr, "VFlib Warning: page width is small. (>=%d)\n", 
	    *margin_l + bm_w);
    *margin_r = 0;
  }

}


/*EOF*/
