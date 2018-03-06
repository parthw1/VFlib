/*
 * bitmap.c - a module for bitmap related procedures
 * by Hirotsugu Kakugawa
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



/**
 ** VF_MakeScaledBitmap
 **/
Public VF_BITMAP
VF_MakeScaledBitmap(VF_BITMAP src_bm, double mag_x, double mag_y)
{ /* NOTE: CALLER MUST FREE THE BITMAP RETURNED BY THIS ROUTINE. */
  int            new_width, new_height;
  VF_BITMAP      new_bm;
  int            x0, y0, x1, y1, x2, xb, bw;
  double         o_mag_x, o_mag_y;
  unsigned char  *p0, *p1, *p1u, *p1l, d, d1, d2;
  static unsigned char  scale_bit_table[] = {
    0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01
  };

  vf_error = 0;
  if (src_bm == NULL)
    return NULL;

  o_mag_x = mag_x; 
  o_mag_y = mag_y; 

  if (mag_x < 0)
    mag_x = 0 - mag_x;
  if (mag_y < 0)
    mag_y = 0 - mag_y;

  new_width  = (int)ceil(mag_x * src_bm->bbx_width);
  new_height = (int)ceil(mag_y * src_bm->bbx_height);
  if (   (src_bm->bbx_width  == new_width)
      && (src_bm->bbx_height == new_height))
    return VF_CopyBitmap(src_bm);
  
  if ((new_bm = vf_alloc_bitmap(new_width, new_height)) == NULL){
    vf_error = VF_ERR_NO_MEMORY;
    return NULL;
  }

  new_bm->off_x = toint(mag_x * src_bm->off_x);
  new_bm->off_y = toint(mag_y * src_bm->off_y);
  new_bm->mv_x  = toint(mag_x * src_bm->mv_x);
  new_bm->mv_y  = toint(mag_y * src_bm->mv_y);

  if ((new_width < 2) || (new_height < 2))
    return new_bm;

  if ((mag_x <= 1.0) && (mag_y <= 1.0)){
    p0 = src_bm->bitmap;
    for (y0 = 0; y0 < src_bm->bbx_height; y0++){
      y1 = mag_y * y0;
      for (x0 = 0; x0 < src_bm->bbx_width; x0++){
	x1 = mag_x * x0;
	if ((p0[x0/8] & scale_bit_table[x0%8]) != 0){
	  new_bm->bitmap[y1 * new_bm->raster + x1/8]
	    |= scale_bit_table[x1%8];
	}
      }
      p0 = &p0[src_bm->raster];
    }

  } else if ((mag_x > 1.0) && (mag_y > 1.0)){
    p1 = new_bm->bitmap;
    for (y1 = 0; y1 < new_bm->bbx_height; y1++){
      y0 = y1 / mag_y;
      for (x1 = 0; x1 < new_bm->bbx_width; x1++){
	x0 = x1 / mag_x;
	if ((src_bm->bitmap[y0 * src_bm->raster + x0/8] 
	     & scale_bit_table[x0%8]) != 0){
	  p1[x1/8] |= scale_bit_table[x1%8];
	}
      }
      p1 = &p1[new_bm->raster];
    }

  } else if ((mag_x > 1.0) && (mag_y <= 1.0)){
    p0 = src_bm->bitmap;
    for (y0 = 0; y0 < src_bm->bbx_height; y0++){
      y1 = y0 * mag_y;
      for (x1 = 0; x1 < new_bm->bbx_width; x1++){
	x0 = x1 / mag_x;
	if ((p0[x0/8] & scale_bit_table[x0%8]) != 0){
	  new_bm->bitmap[y1 * new_bm->raster + (x1/8)] 
	    |= scale_bit_table[x1%8];
	}
      }
      p0 = &p0[src_bm->raster];
    }

  } else {/*((mag_x <= 1.0) && (mag_y > 1.0))*/
    p1 = new_bm->bitmap;
    for (y1 = 0; y1 < new_bm->bbx_height; y1++){
      y0 = y1 / mag_y;
      for (x0 = 0; x0 < src_bm->bbx_width; x0++){
	x1 = x0 * mag_x;
	if ((src_bm->bitmap[y0 * src_bm->raster + x0/8] 
	     & scale_bit_table[x0%8]) != 0){
	  p1[x1/8] |= scale_bit_table[x1%8];
	}
      }
      p1 = &p1[new_bm->raster];
    }
  }


  if (o_mag_y < 0){
    bw = (new_bm->bbx_width + 7) / 8;
    for (y1 = 0; y1 < new_bm->bbx_height/2; y1++){
      p1u = &new_bm->bitmap[new_bm->raster * y1];
      p1l = &new_bm->bitmap[new_bm->raster * (new_bm->bbx_height - y1 - 1)];
      for (xb = 0; xb < bw; xb++){
	d = *p1u;
	*p1u++ = *p1l;
	*p1l++ = d;
      }
    }
    new_bm->off_y = new_bm->bbx_height - new_bm->off_y;
    new_bm->mv_y  = 0 - new_bm->mv_y;
  }

  if (o_mag_x < 0){
    p1 = new_bm->bitmap;
    for (y1 = 0; y1 < new_bm->bbx_height; y1++){
      for (x1 = 0; x1 < new_bm->bbx_width/2; x1++){
	x2 = new_bm->bbx_width - x1 - 1;
	d1 = (p1[x1/8] & scale_bit_table[x1%8]);
	d2 = (p1[x2/8] & scale_bit_table[x2%8]);
	p1[x1/8] = p1[x1/8] & ~scale_bit_table[x1%8];
	p1[x2/8] = p1[x2/8] & ~scale_bit_table[x2%8];
	if (d1 != 0)
	  p1[x2/8] |= scale_bit_table[x2%8];
	if (d2 != 0)
	  p1[x1/8] |= scale_bit_table[x1%8];
      }
      p1 = &p1[new_bm->raster];
    }
    new_bm->off_x = new_bm->off_x - new_bm->bbx_width;
    new_bm->mv_x  = 0 - new_bm->mv_x;
  }

  return new_bm;
}


/**
 ** VF_RotatedBitmap
 **/
Public VF_BITMAP
VF_RotatedBitmap(VF_BITMAP bm_src, int angle)
{
  VF_BITMAP       bm_new;
  int             x, y, x2, y2;
  long            w, h;
  unsigned char  *p;
  static unsigned char  bits[] = 
    {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};

  if ((bm_src == NULL) || (bm_src->bitmap == NULL))
    return NULL;
  w = bm_src->bbx_width;
  h = bm_src->bbx_height;

  bm_new = NULL;
  switch (angle){
  default:
    fprintf(stderr, 
	    "VFlib: Unsupported rotation angle for VF_RotatedBitmap(): %d\n",
	    angle);
    break;

  case VF_BM_ROTATE_0:
    bm_new = VF_CopyBitmap(bm_src);
    break;

  case VF_BM_ROTATE_90:
    bm_new = vf_alloc_bitmap(bm_src->bbx_height, bm_src->bbx_width);
    if (bm_new == NULL)
      return NULL;
    bm_new->off_x = bm_src->off_y;
    bm_new->off_y = bm_src->off_x + w;
    bm_new->mv_x = -bm_src->mv_y;
    bm_new->mv_y = bm_src->mv_x;
    for (y = 0; y < bm_src->bbx_height; y++){
      p = &bm_src->bitmap[y * bm_src->raster];
      for (x = 0; x < bm_src->bbx_width; x++){
	if ((bits[x%8] & p[x/8]) != 0){
	  x2 = y;
	  y2 = (w-1) - x;
	  bm_new->bitmap[y2 * bm_new->raster + (x2/8)] |= bits[x2%8];
	}
      }
    }
    break;

  case VF_BM_ROTATE_180:
    bm_new = vf_alloc_bitmap(bm_src->bbx_width, bm_src->bbx_height);
    if (bm_new == NULL)
      return NULL;
    bm_new->off_x = -bm_src->off_x - w;
    bm_new->off_y = -bm_src->off_y + h;
    bm_new->mv_x = -bm_src->mv_x;
    bm_new->mv_y = -bm_src->mv_y;
    for (y = 0; y < bm_src->bbx_height; y++){
      p = &bm_src->bitmap[y * bm_src->raster];
      for (x = 0; x < bm_src->bbx_width; x++){
	if ((bits[x%8] & p[x/8]) != 0){
	  x2 = (w-1) - x;
	  y2 = (h-1) - y;
	  bm_new->bitmap[y2 * bm_new->raster + (x2/8)] |= bits[x2%8];
	}
      }
    }
    break;

  case VF_BM_ROTATE_270:
    bm_new = vf_alloc_bitmap(bm_src->bbx_height, bm_src->bbx_width);
    if (bm_new  == NULL)
      return NULL;
    bm_new->off_x = bm_src->off_y - h;
    bm_new->off_y = -bm_src->off_x;
    bm_new->mv_x = bm_src->mv_y;
    bm_new->mv_y = -bm_src->mv_x;
    for (y = 0; y < bm_src->bbx_height; y++){
      p = &bm_src->bitmap[y * bm_src->raster];
      for (x = 0; x < bm_src->bbx_width; x++){
	if ((bits[x%8] & p[x/8]) != 0){
	  x2 = (h-1) - y;
	  y2 = x;
	  bm_new->bitmap[y2 * bm_new->raster + (x2/8)] |= bits[x2%8];
	}
      }
    }
    break;
  }
  
  return bm_new;
}


/**
 ** VF_ReflectedBitmap
 **/
Public VF_BITMAP
VF_ReflectedBitmap(VF_BITMAP bm_src, int reflect_x, int reflect_y)
{
  VF_BITMAP       bm_new;
  int             x, x8, y, x2, y2;
  long            w, h;
  unsigned char  *p_src, *p_new;
  static unsigned char  bits[] = 
    {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};

  if ((bm_src == NULL) || (bm_src->bitmap == NULL))
    return NULL;

  if ((reflect_x == 0) && (reflect_y == 0))
    return VF_CopyBitmap(bm_src);

  w = bm_src->bbx_width;
  h = bm_src->bbx_height;
  if ((bm_new = vf_alloc_bitmap(w, h)) == NULL)
    return NULL;

  bm_new->off_x = bm_src->off_x;
  bm_new->off_y = bm_src->off_y;
  bm_new->mv_x  = bm_src->mv_x;
  bm_new->mv_y  = bm_src->mv_y;

  if ((reflect_x != 0) && (reflect_y == 0)){
    for (y = 0; y < h; y++){
      p_src = &bm_src->bitmap[bm_src->raster * y];
      p_new = &bm_new->bitmap[bm_new->raster * y];
      for (x = 0; x < w; x++){
	if ((bits[x % 8] & p_src[x / 8]) != 0){
	  x2 = w - x - 1;
	  p_new[x2 / 8] |= bits[x2 % 8];
	}
      }
    }

  } else if ((reflect_x == 0) && (reflect_y != 0)){ 
    for (y = 0; y < h; y++){
      p_src = &bm_src->bitmap[bm_src->raster * y];
      p_new = &bm_new->bitmap[bm_new->raster * (h - y - 1)];
      for (x8 = 0; x8 < (w+7)/8; x8++)
	*(p_new++) = *(p_src++);
    }

  } else /*if ((reflect_x != 0) && (reflect_x != 0))*/ { 
    for (y = 0; y < h; y++){
      y2 = h - y - 1;
      p_src = &bm_src->bitmap[bm_src->raster * y];
      p_new = &bm_new->bitmap[bm_new->raster * y2];
      for (x = 0; x < w; x++){
	if ((bits[x % 8] & p_src[x / 8]) != 0){
	  x2 = w - x - 1;
	  p_new[x2 / 8] |= bits[x2 % 8];
	}
      }
    }
    
  }

  return bm_new;
}



/**
 ** VF_CopyBitmap
 **/
Public VF_BITMAP
VF_CopyBitmap(VF_BITMAP bm_src)
{
  VF_BITMAP  bm_new;
  int        h;

  vf_error = 0;
  ALLOC_IF_ERR(bm_new, struct vf_s_bitmap){
    vf_error = VF_ERR_NO_MEMORY;
    return NULL;
  }
  bm_new->bbx_width  = bm_src->bbx_width;
  bm_new->bbx_height = bm_src->bbx_height;
  bm_new->off_x   = bm_src->off_x;
  bm_new->off_y   = bm_src->off_y;
  bm_new->mv_x    = bm_src->mv_x;
  bm_new->mv_y    = bm_src->mv_y;
  bm_new->raster  = bm_src->raster;
  bm_new->bitmap  = (unsigned char*)malloc(bm_src->raster*bm_src->bbx_height);

  if (bm_new->bitmap == NULL){
    vf_free(bm_new);
    vf_error = VF_ERR_NO_MEMORY;
    return NULL;
  }

  for (h = 0; h < bm_src->bbx_height; h++){
    memcpy(&bm_new->bitmap[h * bm_new->raster], 
	   &bm_src->bitmap[h * bm_src->raster], 
	   bm_src->raster);
  }
  return bm_new;
}


/**
 ** VF_FillBitmap
 **/
Public void
VF_FillBitmap(VF_BITMAP bm)
{
  int            xd, xm, xw, x, y;
  unsigned char  *p, *q;
  static unsigned char pix_tbl[] =
    {0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe};

  if ((bm == NULL) || (bm->bitmap == NULL))
    return;

  xd = bm->bbx_width / 8;
  xm = bm->bbx_width % 8;
  p = bm->bitmap;
  for (x = 0; x < xd; x++)
    *(p++) = 0xff;
  if (xm != 0)
    *(p++) = pix_tbl[xm];
  xw = (bm->bbx_width + 7) / 8;
  for (y = bm->bbx_height-1; y > 0; --y){
    p = bm->bitmap;
    q = &bm->bitmap[bm->raster * y]; 
    for (x = xw; x > 0; --x)
      *(q++) = *(p++);
  }
}


/**
 ** VF_ClearBitmap
 **/
Public void
VF_ClearBitmap(VF_BITMAP bm)
{
  int            xw, x, y;
  unsigned char  *p, *q;

  if ((bm == NULL) || (bm->bitmap == NULL))
    return;

  xw = (bm->bbx_width + 7) / 8;
  for (y = bm->bbx_height-1; y > 0; --y){
    p = bm->bitmap;
    q = &bm->bitmap[bm->raster * y]; 
    for (x = xw; x > 0; --x)
      *(q++) = *(p++);
  }
}


/**
 ** VF_MinimizeBitmap
 **/
Public VF_BITMAP
VF_MinimizeBitmap(VF_BITMAP bm_src)
{
  VF_BITMAP      bm_new;
  int            yu, yl, xl, xr, y, x, xx, yy, w; 
  unsigned char  *p, *p0, *p1;
  Private unsigned char  bit_table[] = {
    0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01
  };

  vf_error = 0;
  if ((bm_src == NULL) || (bm_src->bitmap == NULL))
    return NULL;
  
  /* find upper */
  yu = 0;
  y = 0;
  p0 = bm_src->bitmap;
  while (y < bm_src->bbx_height){
    p = p0;
    w = (bm_src->bbx_width+7)/8;
    for (x = 0; x < w; x++, p++){
      if (*p != 0){
	yu = y;
	goto bbx_found_upper;
      }
    }
    y++;
    p0 += bm_src->raster; 
  }
  goto Empty;

bbx_found_upper:
  /* find lower */
  y = bm_src->bbx_height-1;
  p0 = &bm_src->bitmap[bm_src->raster * y];
  w = (bm_src->bbx_width+7)/8;
  while (y >= 0){
    p = p0;
    for (x = 0; x < w; x++, p++){
      if (*p != 0){
	yl = y;
	goto bbx_found_lower;
      }
    }
    --y;
    p0 -= bm_src->raster;
  }
  goto Empty;

bbx_found_lower:  
  /* find left */
  xl = bm_src->bbx_width-1;
  p0 = &bm_src->bitmap[bm_src->raster * yu];
  for (y = yu; y <= yl; y++){
    p = p0;
    for (x = 0; x < xl; x++){
      if ((p[x/8] & bit_table[x%8]) != 0){
	xl = x;
	break;
      }
    }
    p0 += bm_src->raster;
  }

  /* find right */
  xr = 0;
  p0 = &bm_src->bitmap[bm_src->raster * yu];
  for (y = yu; y <= yl; y++){
    p = p0;
    for (x = bm_src->bbx_width-1; x > xr; --x){
      if ((p[x/8] & bit_table[x%8]) != 0){
	xr = x;
	break;
      }
    }
    p0 += bm_src->raster;
  }

  /**printf("** yu=%d yl=%d xl=%d xr=%d\n", yu, yl, xl, xr);**/
  if ((bm_new = vf_alloc_bitmap(xr - xl + 1, yl - yu + 1)) == NULL)
    return NULL;
  bm_new->off_x = bm_src->off_x + xl;
  bm_new->off_y = bm_src->off_y - yu;
  bm_new->mv_x  = bm_src->mv_x;
  bm_new->mv_y  = bm_src->mv_y;
  p0 = &bm_src->bitmap[bm_src->raster*yu];
  p1 = bm_new->bitmap;
  for (yy = 0; yy < bm_new->bbx_height; yy++){   /* SLOW! */
    for (xx = 0; xx < bm_new->bbx_width; xx++){  
      if ((p0[(xx+xl)/8] & bit_table[(xx+xl)%8]) != 0){
	p1[xx/8] |= bit_table[xx%8];
      }
    }
    p0 += bm_src->raster;
    p1 += bm_new->raster;
  }
  return bm_new;

Empty:
  if ((bm_new = vf_alloc_bitmap(0, 0)) == NULL)
    return NULL;
  bm_new->off_x = 0;
  bm_new->off_y = 0;
  bm_new->mv_x  = bm_src->mv_x;
  bm_new->mv_y  = bm_src->mv_y;
  return bm_new;
}


Glocal VF_BITMAP
vf_alloc_bitmap(int width, int height)
{
  VF_BITMAP  bm_new;
  int        size, raster;

  ALLOC_IF_ERR(bm_new, struct vf_s_bitmap){
    vf_error = VF_ERR_NO_MEMORY;
    return NULL;
  }
  if (width == 0) 
    width = 1;
  if (height == 0)
    height = 1;
  raster = (width+7)/8;
  size = raster * height;

  bm_new->bbx_width  = width;
  bm_new->bbx_height = height;
  bm_new->raster     = raster;
  bm_new->off_x      = 0;
  bm_new->off_y      = height;
  bm_new->mv_x       = width;
  bm_new->mv_y       = 0;
  if ((bm_new->bitmap = (unsigned char*)malloc(size)) == NULL){
    vf_free(bm_new);
    vf_error = VF_ERR_NO_MEMORY;
    return NULL;
  }
  memclr(bm_new->bitmap, size);
  return bm_new;
}

Glocal VF_BITMAP
vf_alloc_bitmap_with_metric1(VF_METRIC1 me, double dpi_x, double dpi_y)
{
  int        size, raster, w, h;
  VF_BITMAP  bm_new;

  ALLOC_IF_ERR(bm_new, struct vf_s_bitmap){
    vf_error = VF_ERR_NO_MEMORY;
    return NULL;
  }
  
  if ((w = me->bbx_width  * (dpi_x / 72.27) + 0.5) == 0)
    w = 1;
  if ((h = me->bbx_height * (dpi_y / 72.27) + 0.5) == 0)
    h = 1;
  raster = (w+7)/8;
  size = raster * h;

  bm_new->bbx_width  = w;
  bm_new->bbx_height = h;
  bm_new->raster     = raster;
  bm_new->off_x      = me->off_x * (dpi_x / 72.27);
  bm_new->off_y      = me->off_y * (dpi_y / 72.27);
  bm_new->mv_x       = me->mv_x * (dpi_x / 72.27);
  bm_new->mv_y       = me->mv_y * (dpi_y / 72.27);
  if ((bm_new->bitmap = (unsigned char*)malloc(size)) == NULL){
    vf_free(bm_new);
    vf_error = VF_ERR_NO_MEMORY;
    return NULL;
  }
  memclr(bm_new->bitmap, size);
  return bm_new;
}

Glocal VF_BITMAP
vf_alloc_bitmap_with_metric2(VF_METRIC2 met)
{
  int        size, raster, w, h;
  VF_BITMAP  bm_new;

  ALLOC_IF_ERR(bm_new, struct vf_s_bitmap){
    vf_error = VF_ERR_NO_MEMORY;
    return NULL;
  }

  if ((w = met->bbx_width) == 0)
    w = 1;
  if ((h = met->bbx_height) == 0)
    h = 1;
  raster = (w + 7)/8;
  size = raster * h;

  bm_new->bbx_width  = w;
  bm_new->bbx_height = h;
  bm_new->raster     = raster;
  bm_new->off_x      = met->off_x;
  bm_new->off_y      = met->off_y;
  bm_new->mv_x       = met->mv_x;
  bm_new->mv_y       = met->mv_y;
  if ((bm_new->bitmap = (unsigned char*)malloc(size)) == NULL){
    vf_free(bm_new);
    vf_error = VF_ERR_NO_MEMORY;
    return NULL;
  }
  memclr(bm_new->bitmap, size);
  return bm_new;
}


/**
 **   VF_FreeBitmap
 **/
Public void
VF_FreeBitmap(VF_BITMAP vf_bitmap)
{
  vf_error = 0;
  vf_free_bitmap(vf_bitmap);
}

Glocal void
vf_free_bitmap(VF_BITMAP vf_bitmap)
{
  if (vf_bitmap != NULL){
    vf_free(vf_bitmap->bitmap);
    vf_free(vf_bitmap);
  }
}


/**
 ** VF_DumpBitmap
 **/
Public void
VF_DumpBitmap(VF_BITMAP bm)
{
  vf_error = 0;
  vf_dump_bitmap(bm);
}

Glocal void
vf_dump_bitmap(VF_BITMAP bm)
{
  unsigned char *p, *p0;
  int           x, y, x0, y0, x1, y1;
  int           ex;
  char          d2c[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
  static unsigned char bit_table[] = {
    0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01
  };

  if ((bm == NULL) || ((p = bm->bitmap) == NULL))
    return;

  x0 = 0;
  if (bm->off_x > 0)
    x0 = -bm->off_x;
  y0 = 0;
  if (bm->off_y < 0)
    y0 = bm->off_y;
  x1 = bm->bbx_width;
  if (-bm->off_x > bm->bbx_width)
    x1 = -bm->off_x;
  y1 = bm->bbx_height;
  if (bm->off_y > bm->bbx_height)
    y1 = bm->off_y;
  if (-bm->off_x+bm->mv_x > x1)
    x1 = -bm->off_x+bm->mv_x;
  if (bm->off_y-bm->mv_y > y1)
    y1 = bm->off_y-bm->mv_y;
  ex = 1;

  putchar(' '); 
  for (x = x0-1-ex; x <= x1+1+ex; x++)
    printf("%c", d2c[((x % 10) + 10) % 10]);
  putchar('\n');
  putchar(' '); 
  putchar('+'); 
  for (x = x0-1-ex+1; x <= x1+1+ex-1; x++)
    putchar('-');
  putchar('+'); 
  putchar('\n');
  p0 = bm->bitmap;
  for (y = y0-ex; y < y1+ex; y++){
    if ((0 <= y) && (y <= bm->bbx_height))
      p = p0-1;
    printf("%c", d2c[((y % 10) + 10) % 10]);
    putchar('|');
    for (x = x0-ex; x <= x1+ex; x++){
      if ((0 <= x) && (x <= bm->bbx_width) && (x%8 == 0))
	p++;
      if ((x == -bm->off_x) && (y == bm->off_y)){
	if ((0 <= x) && (x < bm->bbx_width) 
	    && (0 <= y) && (y < bm->bbx_height)
	    && ((*p & bit_table[x%8]) != 0))
	  putchar('+');
	else
	  putchar('+');
      } else if ((x == (-bm->off_x + bm->mv_x))
		 && (y == (bm->off_y - bm->mv_y))){
	if ((*p & bit_table[x%8]) != 0)
	  putchar('o');
	else
	  putchar('o');
      } else if ((0 <= x) && (x < bm->bbx_width) 
		 && (0 <= y) && (y < bm->bbx_height)){
	if ((*p & bit_table[x%8]) != 0)
	  putchar('@');
	else
	  putchar('.');
      } else {
	putchar(' ');
      }
    }
    if ((0 <= y) && (y <= bm->bbx_height))
      p0 = p0 + bm->raster;
    putchar('|');
    printf("%c", d2c[((y % 10) + 10) % 10]);
    putchar('\n');
  }
  putchar(' '); 
  putchar('+'); 
  for (x = x0-1-ex+1; x <= x1+1+ex-1; x++)
    putchar('-');
  putchar('+'); 
  putchar('\n');
  putchar(' '); 
  for (x = x0-1-ex; x <= x1+1+ex; x++)
    printf("%c", d2c[((x % 10) + 10) % 10]);
  putchar('\n');
}

/*EOF*/
