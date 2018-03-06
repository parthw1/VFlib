/*
 * drv_zeit.c - A font driver for ZEIT format fonts 
 *
 * by Hirotsugu Kakugawa
 *
 *  3 Dec 1996  First version.
 * 10 Dec 1996  Changed for VFlib version 3.1
 * 12 Dec 1996  Eliminated "do" capability.
 * 26 Feb 1997  Added 'query_font_type'.
 *  7 Aug 1997  VFlib 3.3  Changed API.
 * 28 Jan 1998  VFlib 3.4  Changed API.
 * 24 Nov 1998  Added get_fontbbx1() and get_fontbbx2().
 * 18 Oct 2001    Fixed memory leaks.
 *
 */
/*
 * Copyright (C) 1996-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */

/* debug flag in vflibcap (debug capability):
 *    f - print font file path
 *    i - print info on character index 
 *    o - print contents of outline data
 *    * - everything
 */

#include  "config.h"
#include  <stdio.h>
#include  <stdlib.h>
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif
#include  <ctype.h>
#include  <math.h>
#ifdef HAVE_SYS_PARAM_H
#include  <sys/param.h>
#endif
#include  "VFlib-3_7.h"
#include  "VFsys.h"
#include  "vflibcap.h"
#include  "bitmap.h"
#include  "cache.h"
#include  "fsearch.h"
#include  "str.h"
#include  "sexp.h"


#include  "zeit.h"

#define DIRECTION_HORIZONTAL  0
#define DIRECTION_VERTICAL    1

#define   POINTS_PER_INCH      72.27  /* 1 inch = 72.27 point */

#define   DEFAULT_DPI          72.27
#define   DEFAULT_POINT_SIZE   32.0
#define   DEFAULT_PIXEL_SIZE   32
#define   DEFAULT_DIRECTION    DIRECTION_HORIZONTAL  


Private SEXP_LIST    default_fontdirs;
Private SEXP_LIST    default_extensions;
Private SEXP_STRING  default_point_size;
Private double       v_default_point_size;
Private SEXP_STRING  default_pixel_size;
Private double       v_default_pixel_size;
Private SEXP_STRING  default_dpi, default_dpi_x, default_dpi_y;
Private double       v_default_dpi_x, v_default_dpi_y;
Private SEXP_STRING  default_aspect;
Private double       v_default_aspect;
Private SEXP_STRING  default_direction;
Private char         v_default_direction;
Private SEXP_LIST    default_vec_bbxul;
Private double       v_default_vec_bbxul_x, v_default_vec_bbxul_y;
Private SEXP_LIST    default_vec_next;
Private double       v_default_vec_next_x, v_default_vec_next_y;
Private SEXP_ALIST   default_properties;
Private SEXP_ALIST   default_variables;
Private SEXP_STRING  default_debug_mode;

struct s_font_zeit {
  int       zeit_id;
  char     *font_name;
  double    point_size;
  int       pixel_size;
  double    dpi_x, dpi_y;
  double    aspect;
  double    mag;
  double    slant;
  double    vec_bbxul_x, vec_bbxul_y;
  double    vec_next_x, vec_next_y;
  char      direction;
  SEXP      props;
};
typedef struct s_font_zeit  *FONT_ZEIT;

Private int        zeit_create(VF_FONT font, char *font_class,
			       char *font_name, int implicit, SEXP entry);
Private int        zeit_close(VF_FONT);
Private int        zeit_get_metric1(VF_FONT,long,VF_METRIC1,double,double);
Private int        zeit_get_metric2(VF_FONT,long,VF_METRIC2,double,double);
Private int        zeit_get_fontbbx1(VF_FONT,double,double,
				     double*,double*,double*,double*);
Private int        zeit_get_fontbbx2(VF_FONT,double,double, 
				     int*,int*,int*,int*);
Private VF_BITMAP  zeit_get_bitmap1(VF_FONT,long,double,double);
Private VF_BITMAP  zeit_get_bitmap2(VF_FONT,long,double,double);
Private VF_OUTLINE zeit_get_outline1(VF_FONT,long,double,double);
Private char*      zeit_get_font_prop(VF_FONT,char*);
Private VF_OUTLINE   get_outline2(VF_FONT,FONT_ZEIT,long,int,double,double);

Private ZEIT       ZEIT_GetZEIT(int zeit_id);
Private void       ZEIT_SetZEIT(int zeit_id, ZEIT zeit);
Private int        debug_on(char type);



Glocal int
VF_Init_Driver_ZEIT(void)
{
  char   *p;
  SEXP   s1, s2;
  struct s_capability_table  ct[20];
  int  z;

  z = 0;
  /* VF_CAPE_FONT_DIRECTORIES */
  ct[z].cap = VF_CAPE_FONT_DIRECTORIES;   ct[z].type = CAPABILITY_LIST;
  ct[z].ess = CAPABILITY_OPTIONAL;        ct[z++].val = &default_fontdirs;
  /* VF_CAPE_EXTENSIONS */
  ct[z].cap = VF_CAPE_EXTENSIONS;         ct[z].type = CAPABILITY_LIST;
  ct[z].ess = CAPABILITY_OPTIONAL;        ct[z++].val = &default_extensions;
  /* VF_CAPE_POINT_SIZE */
  ct[z].cap = VF_CAPE_POINT_SIZE;         ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;        ct[z++].val = &default_point_size;
  /* VF_CAPE_PIXEL_SIZE */
  ct[z].cap = VF_CAPE_PIXEL_SIZE;         ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;        ct[z++].val = &default_pixel_size;
  /* VF_CAPE_DPI */
  ct[z].cap = VF_CAPE_DPI;                ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;        ct[z++].val = &default_dpi;
  /* VF_CAPE_DPI_X */
  ct[z].cap = VF_CAPE_DPI_X;              ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;        ct[z++].val = &default_dpi_x;
  /* VF_CAPE_DPI_Y */
  ct[z].cap = VF_CAPE_DPI_Y;              ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;        ct[z++].val = &default_dpi_y;
  /* VF_CAPE_ASPECT_RATIO */
  ct[z].cap = VF_CAPE_ASPECT_RATIO;       ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;        ct[z++].val = &default_aspect;
  /* VF_CAPE_DIRECTION */
  ct[z].cap = VF_CAPE_DIRECTION;          ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;        ct[z++].val = &default_direction;
  /* VF_CAPE_VECTOR_TO_BBX_UL */
  ct[z].cap = VF_CAPE_VECTOR_TO_BBX_UL;   ct[z].type = CAPABILITY_VECTOR;
  ct[z].ess = CAPABILITY_OPTIONAL;        ct[z++].val = &default_vec_bbxul;
  /* VF_CAPE_VECTOR_TO_NEXT */
  ct[z].cap = VF_CAPE_VECTOR_TO_NEXT;     ct[z].type = CAPABILITY_VECTOR;
  ct[z].ess = CAPABILITY_OPTIONAL;        ct[z++].val = &default_vec_next;
  /* VF_CAPE_PROPERTIES */
  ct[z].cap = VF_CAPE_PROPERTIES;         ct[z].type = CAPABILITY_ALIST;
  ct[z].ess = CAPABILITY_OPTIONAL;        ct[z++].val = &default_properties;
  /* VF_CAPE_VARIABLE_VALUES */
  ct[z].cap = VF_CAPE_VARIABLE_VALUES;    ct[z].type = CAPABILITY_ALIST;
  ct[z].ess = CAPABILITY_OPTIONAL;        ct[z++].val = &default_variables;
  /* VF_CAPE_DEBUG */ 
  ct[z].cap = VF_CAPE_DEBUG;              ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;        ct[z++].val = &default_debug_mode;
  /* end */
  ct[z].cap = NULL; ct[z].type = 0; ct[z].ess = 0; ct[z++].val = NULL;


  if (vf_cap_GetParsedClassDefault(FONTCLASS_NAME, ct, NULL, NULL) 
      == VFLIBCAP_PARSED_ERROR)
    return -1;

  if (default_extensions == NULL)
    default_extensions = vf_sexp_cstring2list(DEFAULT_EXTENSIONS);

  v_default_point_size = DEFAULT_POINT_SIZE;
  if (default_point_size != NULL)
    v_default_point_size = atof(vf_sexp_get_cstring(default_point_size));
  if (v_default_point_size < 0)
    v_default_point_size = DEFAULT_POINT_SIZE;

  v_default_pixel_size = DEFAULT_PIXEL_SIZE;
  if (default_pixel_size != NULL)
    v_default_pixel_size = atof(vf_sexp_get_cstring(default_pixel_size));
  if (v_default_pixel_size < 0)
    v_default_pixel_size  = DEFAULT_PIXEL_SIZE;

  v_default_dpi_x  = DEFAULT_DPI;
  v_default_dpi_y  = DEFAULT_DPI;
  if (default_dpi != NULL)
    v_default_dpi_x = v_default_dpi_y = atof(vf_sexp_get_cstring(default_dpi));
  if (default_dpi_x != NULL)
    v_default_dpi_x = atof(vf_sexp_get_cstring(default_dpi_x));
  if (default_dpi_y != NULL)
    v_default_dpi_y = atof(vf_sexp_get_cstring(default_dpi_y));
  if (v_default_dpi_x < 0)
    v_default_dpi_x = DEFAULT_DPI;
  if (v_default_dpi_y < 0)
    v_default_dpi_y = DEFAULT_DPI;

  v_default_aspect = 1.0;
  if (default_aspect != NULL)
    v_default_aspect = atof(vf_sexp_get_cstring(default_aspect));
  if (v_default_aspect < 0)
    v_default_aspect = 1.0;

  v_default_direction = DEFAULT_DIRECTION;
  if (default_direction != NULL){
    p = vf_sexp_get_cstring(default_direction);
    switch (*p){
    case 'h': case 'H':
      v_default_direction = DIRECTION_HORIZONTAL;
      break;
    case 'v': case 'V':
      v_default_direction = DIRECTION_VERTICAL;
      break;
    default:
      fprintf(stderr, "VFlib warning: Unknown writing direction: %s\n", p);
      break;
    }
  }

  switch(v_default_direction){
  case DIRECTION_HORIZONTAL: default:
    v_default_vec_bbxul_x = 0;
    v_default_vec_bbxul_y = DEFAULT_TO_REF_PT_H;
    v_default_vec_next_x  = 1.0;
    v_default_vec_next_y  = 0.0;
    break;
  case DIRECTION_VERTICAL:
    v_default_vec_bbxul_x = DEFAULT_TO_REF_PT_V;
    v_default_vec_bbxul_y = 0;
    v_default_vec_next_x  = 0.0;
    v_default_vec_next_y  = -1.0;
    break;
  }

  if (default_vec_bbxul != NULL){
    s1 = vf_sexp_car(default_vec_bbxul);
    s2 = vf_sexp_cadr(default_vec_bbxul);
    v_default_vec_bbxul_x = atof(vf_sexp_get_cstring(s1));
    v_default_vec_bbxul_y = atof(vf_sexp_get_cstring(s2));
  }

  if (default_vec_next != NULL){
    s1 = vf_sexp_car(default_vec_next);
    s2 = vf_sexp_cadr(default_vec_next);
    v_default_vec_next_x = atof(vf_sexp_get_cstring(s1));
    v_default_vec_next_y = atof(vf_sexp_get_cstring(s2));
  }

  if (ZEIT_Init() < 0)
    return -1;

  VF_InstallFontDriver(FONTCLASS_NAME, (DRIVER_FUNC_TYPE)zeit_create);

  return 0;
}
  

Private int
zeit_create(VF_FONT font, char *font_class,
	    char *font_name, int implicit, SEXP entry)
{
  FONT_ZEIT  font_zeit;
  ZEIT       zeit;
  char      *font_file, *p;
  int        zeit_id, val;
  SEXP       s1, s2;
  SEXP       cap_font, cap_point, cap_pixel;
  SEXP       cap_dpi, cap_dpi_x, cap_dpi_y, cap_mag, cap_aspect;
  SEXP       cap_direction, cap_vec_bbxul, cap_vec_next, cap_props;
  struct s_capability_table  ct[20];
  int  z;

  z = 0;
  /* VF_CAPE_FONT_CLASS */
  ct[z].cap = VF_CAPE_FONT_CLASS;       ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_ESSENTIAL;     ct[z++].val = NULL;
  /* VF_CAPE_FONT_FILE */
  ct[z].cap = VF_CAPE_FONT_FILE;        ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;      ct[z++].val = &cap_font;
  /* VF_CAPE_POINT_SIZE */
  ct[z].cap = VF_CAPE_POINT_SIZE;       ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;      ct[z++].val = &cap_point;
  /* VF_CAPE_PIXEL_SIZE */
  ct[z].cap = VF_CAPE_PIXEL_SIZE;       ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;      ct[z++].val = &cap_pixel;
  /* VF_CAPE_DPI */
  ct[z].cap = VF_CAPE_DPI;              ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;      ct[z++].val = &cap_dpi;
  /* VF_CAPE_DPI_X */
  ct[z].cap = VF_CAPE_DPI_X;            ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;      ct[z++].val = &cap_dpi_x;
  /* VF_CAPE_DPI_Y */
  ct[z].cap = VF_CAPE_DPI_Y;            ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;      ct[z++].val = &cap_dpi_y;
  /* VF_CAPE_MAG */
  ct[z].cap = VF_CAPE_MAG;              ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;      ct[z++].val = &cap_mag;
  /* VF_CAPE_ASPECT_RATIO */
  ct[z].cap = VF_CAPE_ASPECT_RATIO;     ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;      ct[z++].val = &cap_aspect;
  /* VF_CAPE_DIRECTION */
  ct[z].cap = VF_CAPE_DIRECTION;        ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;      ct[z++].val = &cap_direction;
  /* VF_CAPE_VECTOR_TO_BBX_UL */
  ct[z].cap = VF_CAPE_VECTOR_TO_BBX_UL; ct[z].type = CAPABILITY_VECTOR;
  ct[z].ess = CAPABILITY_OPTIONAL;      ct[z++].val = &cap_vec_bbxul;
  /* VF_CAPE_VECTOR_TO_NEXT */
  ct[z].cap = VF_CAPE_VECTOR_TO_NEXT;   ct[z].type = CAPABILITY_VECTOR;
  ct[z].ess = CAPABILITY_OPTIONAL;      ct[z++].val = &cap_vec_next;
  /* VF_CAPE_PROPERTIES */
  ct[z].cap = VF_CAPE_PROPERTIES;       ct[z].type = CAPABILITY_ALIST;
  ct[z].ess = CAPABILITY_OPTIONAL;      ct[z++].val = &cap_props;
  /* end */
  ct[z].cap = NULL; ct[z].type = 0; ct[z].ess = 0; ct[z++].val = NULL;


  if (implicit == 1){   /* implicit font */
    font_file = font_name;
  } else {              /* explicit font */
    if (vf_cap_GetParsedFontEntry(entry, font_name, ct,
				  default_variables, NULL) < 0)
      return -1;
    if (cap_font == NULL){
      /* Use font name as font file name if font file name is not given. */
      font_file = font_name;
    } else {
      font_file = vf_sexp_get_cstring(cap_font);
    }
  }

  val = -1;
  font_zeit = NULL;

  if ((zeit_id = ZEIT_Open(font_file)) < 0)
      goto End;

  font->font_type       = VF_FONT_TYPE_OUTLINE;
  font->get_metric1     = zeit_get_metric1;
  font->get_metric2     = zeit_get_metric2;
  font->get_fontbbx1    = zeit_get_fontbbx1;
  font->get_fontbbx2    = zeit_get_fontbbx2;
  font->get_bitmap1     = zeit_get_bitmap1;
  font->get_bitmap2     = zeit_get_bitmap2;
  font->get_outline     = zeit_get_outline1;
  font->get_font_prop   = zeit_get_font_prop;
  font->query_font_type = NULL;  /* Use font->font_type value. */
  font->close           = zeit_close;

  ALLOC_IF_ERR(font_zeit, struct s_font_zeit){
    vf_error = VF_ERR_NO_MEMORY;
    goto End;
  }

  font_zeit->zeit_id     = zeit_id;
  font_zeit->font_name   = NULL;
  font_zeit->point_size  = v_default_point_size;
  font_zeit->pixel_size  = v_default_pixel_size;
  font_zeit->mag         = 1.0;
  font_zeit->dpi_x       = v_default_dpi_x;
  font_zeit->dpi_y       = v_default_dpi_y;
  font_zeit->aspect      = v_default_aspect;
  font_zeit->direction   = v_default_direction;
  font_zeit->slant       = 0;
  font_zeit->vec_bbxul_x = v_default_vec_bbxul_x;
  font_zeit->vec_bbxul_y = v_default_vec_bbxul_y;
  font_zeit->vec_next_x  = v_default_vec_next_x;
  font_zeit->vec_next_y  = v_default_vec_next_y;

  if (implicit == 0){
    if (cap_point != NULL)
      font_zeit->point_size = atof(vf_sexp_get_cstring(cap_point));
    if (cap_pixel != NULL)
      font_zeit->pixel_size = atof(vf_sexp_get_cstring(cap_pixel));
    if (cap_dpi != NULL)
      font_zeit->dpi_x = font_zeit->dpi_y = atof(vf_sexp_get_cstring(cap_dpi));
    if (cap_dpi_x != NULL)
      font_zeit->dpi_x = atof(vf_sexp_get_cstring(cap_dpi_x));
    if (cap_dpi_y != NULL)
      font_zeit->dpi_y = atof(vf_sexp_get_cstring(cap_dpi_y));
    if (cap_mag != NULL)
      font_zeit->mag = atof(vf_sexp_get_cstring(cap_mag));
    if (cap_aspect != NULL)
      font_zeit->aspect = atof(vf_sexp_get_cstring(cap_aspect));
    if (cap_direction != NULL){
      p = vf_sexp_get_cstring(cap_direction);
      switch (*p){
      case 'h': case 'H':
	font_zeit->direction = DIRECTION_HORIZONTAL;  break;
      case 'v': case 'V':
	font_zeit->direction = DIRECTION_VERTICAL;    break;
      default:
	fprintf(stderr, "VFlib warning: Unknown writing direction: %s\n", p);
	break;
      }
    }
    switch(font_zeit->direction){
    case DIRECTION_HORIZONTAL: default:
      font_zeit->vec_bbxul_x = v_default_vec_bbxul_x;
      font_zeit->vec_bbxul_y = v_default_vec_bbxul_y;
      font_zeit->vec_next_x  = v_default_vec_next_x;
      font_zeit->vec_next_y  = v_default_vec_next_y;
      break;
    case DIRECTION_VERTICAL:
      font_zeit->vec_bbxul_x = v_default_vec_bbxul_x;
      font_zeit->vec_bbxul_y = v_default_vec_bbxul_y;
      font_zeit->vec_next_x  = v_default_vec_next_x;
      font_zeit->vec_next_y  = v_default_vec_next_y;
      break;
    }
    if (cap_vec_bbxul != NULL){
      s1 = vf_sexp_car(cap_vec_bbxul);
      s2 = vf_sexp_cadr(cap_vec_bbxul);
      font_zeit->vec_bbxul_x = atof(vf_sexp_get_cstring(s1));
      font_zeit->vec_bbxul_y = atof(vf_sexp_get_cstring(s2));
    }
    if (cap_vec_next != NULL){
      s1 = vf_sexp_car(cap_vec_next);
      s2 = vf_sexp_cadr(cap_vec_next);
      font_zeit->vec_next_x = atof(vf_sexp_get_cstring(s1));
      font_zeit->vec_next_y = atof(vf_sexp_get_cstring(s2));
    }
    if (cap_props != NULL)
      font_zeit->props = cap_props;
  }

  if ((font_zeit->font_name = vf_strdup(font_name)) == NULL){
    vf_error = VF_ERR_NO_MEMORY;
    goto End;
  }

  if ((zeit = ZEIT_GetZEIT(font_zeit->zeit_id)) == NULL){
    fprintf(stderr, "VFlib: internal error in zeit_create()\n");
    vf_error = VF_ERR_INTERNAL;
    goto End;
  }

  font->private = font_zeit;
  val = 0;

End:
  if (implicit == 0){
    vf_sexp_free3(&cap_font, &cap_point, &cap_pixel);
    vf_sexp_free3(&cap_dpi, &cap_dpi_x, &cap_dpi_y);
    vf_sexp_free3(&cap_direction, &cap_vec_bbxul, &cap_vec_next);
  }
  if (val < 0)
    zeit_close(font);

  return val;
}


Private int
zeit_close(VF_FONT font)
{
  FONT_ZEIT  font_zeit;

  font_zeit = (FONT_ZEIT)font->private;

  if (font_zeit != NULL){
    if (font_zeit->zeit_id >= 0)
      ZEIT_Close(font_zeit->zeit_id);
    vf_free(font_zeit->font_name); 
    vf_sexp_free1(&font_zeit->props);
    vf_free(font_zeit);
  }

  return 0; 
}



Private void
mag_mode_1(FONT_ZEIT font_zeit, VF_FONT font, 
	   double mag_x, double mag_y,
	   double *ret_point_size,
	   double *ret_bbx_w, double *ret_bbx_h, 
	   double *ret_mag_x, double *ret_mag_y, 
	   double *ret_dpix, double *ret_dpiy)
{
  double  mx, my, dpix, dpiy, ps, asp;

  if ((ps = font->point_size) < 0)
    if ((ps = font_zeit->point_size) < 0)
      ps = v_default_point_size;

  if (ret_point_size != NULL)
    *ret_point_size = ps;

  asp = (v_default_aspect * font_zeit->aspect);

  mx = mag_x * font_zeit->mag * font->mag_x * asp;
  my = mag_y * font_zeit->mag * font->mag_y;

  if (ret_mag_x != NULL)
    *ret_mag_x = mx;
  if (ret_mag_y != NULL)
    *ret_mag_y = my;

  if ((font->dpi_x > 0) && (font->dpi_y > 0)){
    dpix = font->dpi_x;
    dpiy = font->dpi_y;
  } else if ((font_zeit->dpi_x > 0) && (font_zeit->dpi_y > 0)){
    dpix = font_zeit->dpi_x;
    dpiy = font_zeit->dpi_y;
  } else {
    dpix = v_default_dpi_x;
    dpiy = v_default_dpi_y;
  }

  if (ret_dpix != NULL)
    *ret_dpix = dpix;
  if (ret_dpiy != NULL)
    *ret_dpiy = dpiy;

  if (ret_bbx_w != NULL)
    *ret_bbx_w = dpix * mx * (ps / POINTS_PER_INCH);
  if (ret_bbx_h != NULL)
    *ret_bbx_h = dpiy * my * (ps / POINTS_PER_INCH);

#if 0
  printf("*** %.3f %.3f %.3f\n", mag_x, font_zeit->mag, font->mag_x);
  printf("    %.3f %.3f %.3f\n", mag_y, font_zeit->mag, font->mag_y);
  printf("    dpix=%.3f  font_dpi_x=%.3f\n", dpix, font_dpi_x);
  printf("    dpiy=%.3f  font_dpi_y=%.3f\n", dpiy, font_dpi_y);
  printf("    asp=%.3f\n", asp);
  printf("    mx=%.3f, my=%.3f\n", mx, my);
  if (ret_bbx_w != NULL)
    printf("    W=%.3f  H=%.3f\n", *ret_bbx_w, *ret_bbx_h);
#endif

}


Private int
zeit_get_metric1(VF_FONT font, long code, VF_METRIC1 metric, 
		 double mag_x, double mag_y)
{
  FONT_ZEIT  font_zeit;
  double     bbx_w, bbx_h;
  double     dpix, dpiy;

  if (   ((font_zeit = (FONT_ZEIT)font->private) == NULL)
      || (metric == NULL)){
    fprintf(stderr, "VFlib internal error: in zeit_get_metric1()\n");
    abort();
  }

  mag_mode_1(font_zeit, font, mag_x, mag_y, 
	     NULL, &bbx_w, &bbx_h, NULL, NULL, &dpix, &dpiy);

  metric->bbx_width  = bbx_w;
  metric->bbx_height = bbx_h;
  metric->off_x = bbx_w * font_zeit->vec_bbxul_x;
  metric->off_y = bbx_h * font_zeit->vec_bbxul_y;
  metric->mv_x  = bbx_w * font_zeit->vec_next_x;
  metric->mv_y  = bbx_h * font_zeit->vec_next_y;

  return 0;
}

Private int
zeit_get_fontbbx1(VF_FONT font, double mag_x, double mag_y,
		  double *w_p, double *h_p, double *xoff_p, double *yoff_p)
{
  FONT_ZEIT  font_zeit;
  double     bbx_w, bbx_h;
  double     dpix, dpiy;

  if ((font_zeit = (FONT_ZEIT)font->private) == NULL){
    fprintf(stderr, "VFlib internal error: in zeit_get_fontbbx1()\n");
    abort();
  }

  mag_mode_1(font_zeit, font, mag_x, mag_y, 
	     NULL, &bbx_w, &bbx_h, NULL, NULL, &dpix, &dpiy);

  *w_p = bbx_w;
  *h_p = bbx_h;
  *xoff_p = bbx_w * font_zeit->vec_bbxul_x;
  *yoff_p = bbx_h * (1.0 - font_zeit->vec_bbxul_y);

  return 0;
}

Private VF_BITMAP
zeit_get_bitmap1(VF_FONT font, long code_point,
		 double mag_x, double mag_y)
{
  VF_OUTLINE   outline;
  VF_BITMAP    bm;

  if ((outline = zeit_get_outline1(font, code_point, mag_x, mag_y)) == NULL)
    return NULL;

  bm = vf_outline_to_bitmap(outline, -1, -1, -1, 1, 1);
  VF_FreeOutline(outline);

  return  bm;
}

Private VF_OUTLINE
zeit_get_outline1(VF_FONT font, long code_point,
		  double mag_x, double mag_y)
{
  FONT_ZEIT   font_zeit;
  VF_OUTLINE  outline;
  double      ps, mx, my, bbx_w, bbx_h, dpix, dpiy, em_mag;

  if ((font_zeit = (FONT_ZEIT)font->private) == NULL){
    fprintf(stderr, "VFlib: internal error in zeit_get_outline1()\n");
    abort();
  }

  mag_mode_1(font_zeit, font, mag_x, mag_y, 
	     &ps, &bbx_w, &bbx_h, &mx, &my, &dpix, &dpiy);

  em_mag = 1.0;
  if (mx > 1){
    em_mag /= mx;
    my = my/mx;
    mx = 1.0;
  }
  if (my > 1){
    em_mag /= my;
    mx = mx/my;
    my = 1.0;
  }

  outline = ZEIT_ReadOutline(font_zeit->zeit_id, (int)code_point, mx, my);
  if (outline == NULL)
    return NULL;

  outline[VF_OL_HEADER_INDEX_HEADER_TYPE] = VF_OL_OUTLINE_HEADER_TYPE0;
  outline[VF_OL_HEADER_INDEX_DPI_X]       = VF_OL_HEADER_ENCODE(dpix);
  outline[VF_OL_HEADER_INDEX_DPI_Y]       = VF_OL_HEADER_ENCODE(dpiy);
  outline[VF_OL_HEADER_INDEX_POINT_SIZE]  = VF_OL_HEADER_ENCODE(ps);
  outline[VF_OL_HEADER_INDEX_EM]          = VF_OL_COORD_RANGE * em_mag;
  outline[VF_OL_HEADER_INDEX_MAX_X]       = VF_OL_COORD_RANGE * mx;
  outline[VF_OL_HEADER_INDEX_MAX_Y]       = VF_OL_COORD_RANGE * my;
  outline[VF_OL_HEADER_INDEX_REF_X] 
    = VF_OL_COORD_RANGE * (0 -  font_zeit->vec_bbxul_x) * mx;
  outline[VF_OL_HEADER_INDEX_REF_Y] 
    = VF_OL_COORD_RANGE * font_zeit->vec_bbxul_y * my;
  outline[VF_OL_HEADER_INDEX_MV_X] 
    = VF_OL_COORD_RANGE * font_zeit->vec_next_x * mx;
  outline[VF_OL_HEADER_INDEX_MV_Y]
    = VF_OL_COORD_RANGE * font_zeit->vec_next_y * my;

  return outline;
}


Private void
mag_mode_2(FONT_ZEIT font_zeit, VF_FONT font, 
	   double mag_x, double mag_y,
	   double *ret_pixel_size,
	   double *ret_magx, double *ret_magy,
	   double *ret_bbx_w, double *ret_bbx_h)
{
  int     ps;
  double  mx, my, asp;

  if ((ps = font->pixel_size) < 0)
    if ((ps = font_zeit->pixel_size) < 0)
      ps = v_default_pixel_size;

  asp = (v_default_aspect * font_zeit->aspect);

  mx = mag_x * font_zeit->mag * font->mag_x * asp;
  my = mag_y * font_zeit->mag * font->mag_y;

  if (ret_pixel_size != NULL)
    *ret_pixel_size = ps;

  if (ret_magx != NULL)
    *ret_magx = mx;
  if (ret_magy != NULL)
    *ret_magy = my;

  if (ret_bbx_w != NULL)
    *ret_bbx_w = mx * ps;
  if (ret_bbx_h != NULL)
    *ret_bbx_h = my * ps;
}


Private int
zeit_get_metric2(VF_FONT font, long code_point, VF_METRIC2 metric, 
		 double mag_x, double mag_y)
{
  FONT_ZEIT  font_zeit;
  double     bbx_w, bbx_h;

  if (   ((font_zeit = (FONT_ZEIT)font->private) == NULL)
      || (metric == NULL)){
    fprintf(stderr, "VFlib internal error: in zeit_get_metric2()\n");
    abort();
  }

  mag_mode_2(font_zeit, font, mag_x, mag_y, NULL, NULL, NULL, &bbx_w, &bbx_h);

  metric->bbx_width  = (int)ceil(bbx_w);
  metric->bbx_height = (int)ceil(bbx_h);
  metric->off_x = toint(bbx_w * font_zeit->vec_bbxul_x);
  metric->off_y = toint(bbx_h * font_zeit->vec_bbxul_y);
  metric->mv_x  = toint(bbx_w * font_zeit->vec_next_x);
  metric->mv_y  = toint(bbx_h * font_zeit->vec_next_y);

  return 0;
}

Private int
zeit_get_fontbbx2(VF_FONT font, double mag_x, double mag_y,
		  int *w_p, int *h_p, int *xoff_p, int *yoff_p)
{
  FONT_ZEIT  font_zeit;
  double     bbx_w, bbx_h;

  if ((font_zeit = (FONT_ZEIT)font->private) == NULL){
    fprintf(stderr, "VFlib internal error: in zeit_get_fontbbx2()\n");
    abort();
  }

  mag_mode_2(font_zeit, font, mag_x, mag_y, NULL, NULL, NULL, &bbx_w, &bbx_h);

  *w_p  = toint(bbx_w);
  *h_p  = toint(bbx_h);
  *xoff_p = toint(bbx_w * font_zeit->vec_bbxul_x);
  *yoff_p = toint(bbx_w * (font_zeit->vec_bbxul_y - 1.0));

  return 0;
}

Private VF_BITMAP
zeit_get_bitmap2(VF_FONT font, long code_point, 
		 double mag_x, double mag_y)
{
  FONT_ZEIT   font_zeit;
  VF_OUTLINE  outline; 
  VF_BITMAP   bm;
  double      ps, mx, my, bbx_w, bbx_h;
  int         bbx_width, bbx_height;

  if ((font_zeit = (FONT_ZEIT)font->private) == NULL){
    fprintf(stderr, "VFlib internal error: in zeit_get_bitmap2()\n");
    abort();
  }

  mag_mode_2(font_zeit, font, mag_x, mag_y, &ps, &mx, &my, &bbx_w, &bbx_h);

  bbx_width  = (int)ceil(bbx_w);
  bbx_height = (int)ceil(bbx_h);

  if ((bm = vf_alloc_bitmap(bbx_width, bbx_height)) == NULL){
    vf_error = VF_ERR_NO_MEMORY;
    return NULL;
  }

  outline = get_outline2(font, font_zeit, code_point, ps, mx, my);
  if (outline == NULL)
    return NULL;

  if (vf_draw_outline(bm, outline) < 0){
    VF_FreeOutline(outline);
    VF_FreeBitmap(bm);
    return NULL;
  }
  VF_FreeOutline(outline);

  bm->off_x = toint(bbx_w * font_zeit->vec_bbxul_x);
  bm->off_y = toint(bbx_h * font_zeit->vec_bbxul_y);
  bm->mv_x  = toint(bbx_w * font_zeit->vec_next_x);
  bm->mv_y  = toint(bbx_h * font_zeit->vec_next_y);

  return bm;
}


Private VF_OUTLINE
get_outline2(VF_FONT font, FONT_ZEIT font_zeit, long code_point,
	     int pixel_size, double mx, double my)
{
  VF_OUTLINE   outline;
  double       em_mag;
  
  em_mag = 1.0; 
  if (mx > 1){
    em_mag /= mx; 
    my = my/mx;
    mx = 1.0;
  }
  if (my > 1){
    em_mag /= my; 
    mx = mx/my;
    my = 1.0;
  }
  
  outline = ZEIT_ReadOutline(font_zeit->zeit_id, (int)code_point, mx, my);
  if (outline == NULL)
    return NULL;

  outline[VF_OL_HEADER_INDEX_HEADER_TYPE] = VF_OL_OUTLINE_HEADER_TYPE0;
  outline[VF_OL_HEADER_INDEX_DPI_X] = VF_OL_HEADER_ENCODE(POINTS_PER_INCH);
  outline[VF_OL_HEADER_INDEX_DPI_Y] = VF_OL_HEADER_ENCODE(POINTS_PER_INCH);
  outline[VF_OL_HEADER_INDEX_POINT_SIZE] = VF_OL_HEADER_ENCODE(pixel_size);
  outline[VF_OL_HEADER_INDEX_EM]    = VF_OL_COORD_RANGE * my;
  outline[VF_OL_HEADER_INDEX_MAX_X] = VF_OL_COORD_RANGE * mx;
  outline[VF_OL_HEADER_INDEX_MAX_Y] = VF_OL_COORD_RANGE * my;
  outline[VF_OL_HEADER_INDEX_REF_X] 
    = VF_OL_COORD_RANGE * (0 -  font_zeit->vec_bbxul_x) * mx;
  outline[VF_OL_HEADER_INDEX_REF_Y]
    = VF_OL_COORD_RANGE * font_zeit->vec_bbxul_y * my;
  outline[VF_OL_HEADER_INDEX_MV_X] 
    = VF_OL_COORD_RANGE * font_zeit->vec_next_x * mx;
  outline[VF_OL_HEADER_INDEX_MV_Y]
    = VF_OL_COORD_RANGE * font_zeit->vec_next_y * my;

  return outline;
}


Private char*
zeit_get_font_prop(VF_FONT font, char *prop_name)
     /* CALLER MUST RELEASE RETURNED STRING */
{
  FONT_ZEIT   font_zeit;
  char        str[512];
  double      ps, dpix, dpiy;
  SEXP        v;

  if ((font_zeit = (FONT_ZEIT)font->private) == NULL){
    fprintf(stderr, "VFlib: internal error in zeit_get_font_prop()\n");
    abort();
  }
  
  if ((v = vf_sexp_assoc(prop_name, font_zeit->props)) != NULL){
    return vf_strdup(vf_sexp_get_cstring(vf_sexp_cadr(v)));
  } else if ((v = vf_sexp_assoc(prop_name, default_properties)) != NULL){
    return vf_strdup(vf_sexp_get_cstring(vf_sexp_cadr(v)));
  }

  if (font->mode == 1){
    mag_mode_1(font_zeit, font, 1.0, 1.0,
	       &ps, NULL, NULL, NULL, NULL, &dpix, &dpiy);
    /**printf("** Mode1 %.3f %.3f %.3f\n", ps, dpix, dpiy);**/
    if (strcmp(prop_name, "POINT_SIZE") == 0){
      sprintf(str, "%d", toint(10.0 * ps)); 
      return vf_strdup(str);
    } else if (strcmp(prop_name, "PIXEL_SIZE") == 0){
      sprintf(str, "%d", toint(ps * dpiy / POINTS_PER_INCH));
      return vf_strdup(str);
    } else if (strcmp(prop_name, "RESOLUTION_X") == 0){
      sprintf(str, "%d", toint(dpix)); 
      return vf_strdup(str);
    } else if (strcmp(prop_name, "RESOLUTION_Y") == 0){
      sprintf(str, "%d", toint(dpiy)); 
      return vf_strdup(str);
    } 
  } else if (font->mode == 2){
    mag_mode_2(font_zeit, font, 1.0, 1.0, &ps, NULL, NULL, NULL, NULL);
    /**printf("** Mode2 %.3f\n", ps);**/
    if (strcmp(prop_name, "POINT_SIZE") == 0){
      sprintf(str, "%d", toint(10.0 * ps * POINTS_PER_INCH / DEFAULT_DPI)); 
      return vf_strdup(str);
    } else if (strcmp(prop_name, "PIXEL_SIZE") == 0){
      sprintf(str, "%d", toint(ps)); 
      return vf_strdup(str);
    } else if (strcmp(prop_name, "RESOLUTION_X") == 0){
      sprintf(str, "%d", toint(DEFAULT_DPI)); 
      return vf_strdup(str);
    } else if (strcmp(prop_name, "RESOLUTION_Y") == 0){
      sprintf(str, "%d", toint(DEFAULT_DPI)); 
      return vf_strdup(str);
    } 
  }

  return NULL; 
}


/* 
 * Include Low-Level Font Interface Routine. 
 */ 

#include "zeit.c"


static int    zeit_last_zeit_id = -1;
static ZEIT   zeit_last_zeit   = NULL;

Private void
ZEIT_SetZEIT(int zeit_id, ZEIT zeit)
{
  zeit_last_zeit_id  = zeit_id;
  zeit_last_zeit     = zeit;
}

Private ZEIT
ZEIT_GetZEIT(int zeit_id)
{
  ZEIT   zeit;

  if (zeit_id == -1){
    zeit_last_zeit_id  = -1;
    zeit_last_zeit     = NULL;
    return NULL;
  }

  if (   (zeit_last_zeit_id == zeit_id) 
      && (zeit_last_zeit != NULL) 
      && (zeit_last_zeit_id != -1))
    return zeit_last_zeit;
  
  zeit = (zeit_table->get_obj_by_id)(zeit_table, zeit_id);
  zeit_last_zeit_id = zeit_id;
  zeit_last_zeit    = zeit;  

  return zeit;
}


Private int
debug_on(char type)
{
  char  *p, *p0;

  if (default_debug_mode == NULL)
    return FALSE;
  if ((p0 = vf_sexp_get_cstring(default_debug_mode)) == NULL)
    return FALSE;
  
  for (p = p0; *p != '\0'; p++){
    if (*p == type)
      return TRUE;
  }
  for (p = p0; *p != '\0'; p++){
    if (*p == '*')
      return TRUE;
  }
  return FALSE;
}

/*EOF*/
