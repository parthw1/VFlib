/*
 * drv_try.c - A font driver that try to open a font among listed fonts.
 *
 * by Hirotsugu Kakugawa
 *
 * 16 Jul 1998  First implementation
 * 24 Nov 1998  Added get_fontbbx1() and get_fontbbx2().
 *
 */
/*
 * Copyright (C) 1996-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */

#include  "config.h"
#include  <stdio.h>
#include  <stdlib.h>
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif
#include  <ctype.h>
#include  "VFlib-3_7.h"
#include  "VFsys.h"
#include  "vflibcap.h"
#include  "cache.h"
#include  "sexp.h"
#include  "str.h"
#include  "path.h"
#include  "try.h"

struct s_font_try {
  char     *font_name;
  double   point_size;
  double   pixel_size;
  double   mag;
  double   dpi_x, dpi_y;
  int      fid;
  SEXP     props;
};
typedef struct s_font_try  *FONT_TRY;


Private SEXP_ALIST   default_properties;
Private SEXP_ALIST   default_variables;
Private SEXP_STRING  default_debug_mode;


Private int         try_create(VF_FONT,char*,char*,int,SEXP);
Private int         try_close(VF_FONT);
Private int         try_get_metric1(VF_FONT,long,VF_METRIC1,double,double);
Private int         try_get_metric2(VF_FONT,long,VF_METRIC2,double,double);
Private int         try_get_fontbbx1(VF_FONT,double,double,
				     double*,double*,double*,double*);
Private int         try_get_fontbbx2(VF_FONT,double,double, 
				     int*,int*,int*,int*);
Private VF_BITMAP   try_get_bitmap1(VF_FONT,long,double,double);
Private VF_BITMAP   try_get_bitmap2(VF_FONT,long,double,double);
Private VF_OUTLINE  try_get_outline(VF_FONT,long,double,double);
Private char*       try_get_font_prop(VF_FONT,char*);
Private int         try_query_font_type(VF_FONT,long);
Private void        release_mem(FONT_TRY);
Private int         debug_on(char type);


Glocal int
VF_Init_Driver_Try(void)
{
  struct s_capability_table  ct[10];
  int  z;

  z = 0;
  /* VF_CAPE_PROPERTIES */
  ct[z].cap = VF_CAPE_PROPERTIES;      ct[z].type = CAPABILITY_ALIST;
  ct[z].ess = CAPABILITY_OPTIONAL;     ct[z++].val = &default_properties;
  /* VF_CAPE_VARIABLE_VALUES */
  ct[z].cap = VF_CAPE_VARIABLE_VALUES; ct[z].type = CAPABILITY_ALIST;
  ct[z].ess = CAPABILITY_OPTIONAL;     ct[z++].val = &default_variables;
  /* VF_CAPE_DEBUG */
  ct[z].cap = VF_CAPE_DEBUG;           ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;     ct[z++].val = &default_debug_mode;
  /* end */
  ct[z].cap = NULL; ct[z].type = 0; ct[z].ess = 0; ct[z++].val = NULL;


  if (vf_cap_GetParsedClassDefault(FONTCLASS_NAME_TRY, ct, NULL, NULL) 
      == VFLIBCAP_PARSED_ERROR)
    return -1;

  VF_InstallFontDriver(FONTCLASS_NAME_TRY, 
		       (DRIVER_FUNC_TYPE)try_create);

  return 0;
}


Private int
try_create(VF_FONT font, char *font_class,
	   char *font_name, int implicit, SEXP entry)
{
  FONT_TRY    font_try;
  SEXP        cap_fontlist, cap_point, cap_pixel;
  SEXP        cap_mag, cap_dpi, cap_props;
  SEXP        s;
  char        *fname = NULL;
  struct s_capability_table  ct[10];
  int  z;

  z = 0;
  /* VF_CAPE_FONT_CLASS */
  ct[z].cap = VF_CAPE_FONT_CLASS;      ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_ESSENTIAL;    ct[z++].val = NULL;
  /* VF_CAPE_TRY_FONT_LIST */
  ct[z].cap = VF_CAPE_TRY_FONT_LIST;   ct[z].type = CAPABILITY_STRING_LIST1;
  ct[z].ess = CAPABILITY_OPTIONAL;     ct[z++].val = &cap_fontlist;
  /* VF_CAPE_POINT_SIZE */
  ct[z].cap = VF_CAPE_POINT_SIZE;      ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;     ct[z++].val = &cap_point;
  /* VF_CAPE_PIXEL_SIZE */
  ct[z].cap = VF_CAPE_PIXEL_SIZE;      ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;     ct[z++].val = &cap_pixel;
  /* VF_CAPE_DPI */
  ct[z].cap = VF_CAPE_DPI;             ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;     ct[z++].val = &cap_dpi;
  /* VF_CAPE_MAG */
  ct[z].cap = VF_CAPE_MAG;             ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;     ct[z++].val = &cap_mag;
  /* VF_CAPE_PROPERTIES */
  ct[z].cap = VF_CAPE_PROPERTIES;      ct[z].type = CAPABILITY_ALIST;
  ct[z].ess = CAPABILITY_OPTIONAL;     ct[z++].val = &cap_props;
  /* end */
  ct[z].cap = NULL; ct[z].type = 0; ct[z].ess = 0; ct[z++].val = NULL;


  /* No support for implicit fonts */
  if (implicit == 1)  
    return -1;

  /* Only supports explicit fonts */
  if (vf_cap_GetParsedFontEntry(entry, font_name, ct, default_variables, NULL) 
      == VFLIBCAP_PARSED_ERROR)
    return -1;

  font->font_type       = -1;  /* Use try_query_font_type() */
  font->get_metric1     = try_get_metric1;
  font->get_metric2     = try_get_metric2;
  font->get_fontbbx1    = try_get_fontbbx1;
  font->get_fontbbx2    = try_get_fontbbx2;
  font->get_bitmap1     = try_get_bitmap1;
  font->get_bitmap2     = try_get_bitmap2;
  font->get_outline     = try_get_outline;
  font->get_font_prop   = try_get_font_prop;
  font->query_font_type = try_query_font_type;
  font->close           = try_close;

  ALLOC_IF_ERR(font_try, struct s_font_try){
    vf_error = VF_ERR_NO_MEMORY;
    return -1;
  }
  if ((font_try->font_name = vf_strdup(font_name)) == NULL){
    vf_error = VF_ERR_NO_MEMORY;
    vf_free(font_try);
    return -1;
  }

  font_try->point_size = -1;
  font_try->pixel_size = -1;
  font_try->dpi_x      = -1;
  font_try->dpi_y      = -1;
  font_try->mag        = 1.0;
  font_try->fid        = -1;
  font_try->props      = NULL;

  if (cap_point != NULL)
    font_try->point_size = atof(vf_sexp_get_cstring(cap_point));
  if (cap_pixel != NULL)
    font_try->pixel_size = atof(vf_sexp_get_cstring(cap_pixel));
  if (cap_dpi != NULL){
    font_try->dpi_x =atof(vf_sexp_get_cstring(cap_dpi));
    font_try->dpi_y = atof(vf_sexp_get_cstring(cap_dpi));
  }
  if (cap_props != NULL)
    font_try->props = cap_props;

  if (font->point_size >= 0)
    font_try->point_size = font->point_size;
  if ((font->dpi_x >= 0) && (font->dpi_y >= 0)){
    font_try->dpi_x = font->dpi_x;
    font_try->dpi_y = font->dpi_y;
  }
  if (font->pixel_size >= 0)
    font_try->pixel_size = font->pixel_size;

  if ((font->mode != 1) && (font->mode != 2)){
    fprintf(stderr, "VFlib: internal error in try_create()\n");
    abort();
  }

  for (s = cap_fontlist; vf_sexp_consp(s); s = vf_sexp_cdr(s)){
    fname = vf_sexp_get_cstring(vf_sexp_car(s));
    if (fname == NULL) 
      continue;
    if (debug_on('f'))
      printf("VFlib Try:  trying %s\n", fname);
    if (font->mode == 1){
      font_try->fid = VF_OpenFont1(fname, 
				   font_try->dpi_x, font_try->dpi_y,
				   font_try->point_size, 
				   font->mag_x * font_try->mag, 
				   font->mag_y * font_try->mag);
    } else {
      font_try->fid = VF_OpenFont2(fname, font_try->pixel_size, 
				   font->mag_x * font_try->mag, 
				   font->mag_y * font_try->mag);
    }
    if (font_try->fid >= 0)
      break;
  }

  if (debug_on('f')){
    if (font_try->fid >= 0)
      printf("VFlib Try:  opened: %s\n", fname);
    else
      printf("VFlib Try:  failed\n");
  }

  if (font_try->fid < 0)
    goto CANT_OPEN;

  font->private = font_try;
  vf_sexp_free1(&cap_fontlist);
  vf_sexp_free4(&cap_point, &cap_pixel, &cap_mag, &cap_dpi);
  return 0;

CANT_OPEN:
  vf_sexp_free1(&cap_fontlist);
  vf_sexp_free4(&cap_point, &cap_pixel, &cap_mag, &cap_dpi);
  vf_error = VF_ERR_CANT_OPEN;
  release_mem(font_try);
  return -1;
}


Private int
try_close(VF_FONT font)
{
  release_mem((FONT_TRY)(font->private));

  return 0; 
}


Private void
release_mem(FONT_TRY font_try)
{
  if (font_try != NULL){
    vf_free(font_try->font_name);
    if (font_try->fid >= 0)
      VF_CloseFont(font_try->fid);
    vf_sexp_free1(&font_try->props);
    vf_free(font_try);
  }
}


Private int
try_get_metric1(VF_FONT font, long code_point, VF_METRIC1 metric,
		double mag_x, double mag_y)
{
  FONT_TRY  font_try;
  
  if (metric == NULL){
    fprintf(stderr, "VFlib internal error: in try_get_metric1()\n");
    abort();
  }

  if ((font_try = (FONT_TRY)font->private) == NULL){
    fprintf(stderr, "VFlib internal error in try class.\n");
    abort();
  }

  if (font_try->fid < 0){
    vf_error = VF_ERR_NO_GLYPH;
    return -1;
  }

  VF_GetMetric1(font_try->fid, code_point, metric, mag_x, mag_y);

  return 0;
}

Private int
try_get_fontbbx1(VF_FONT font, double mag_x, double mag_y,
		 double *w_p, double *h_p, double *xoff_p, double *yoff_p)
{
  FONT_TRY  font_try;
  
  if ((font_try = (FONT_TRY)font->private) == NULL){
    fprintf(stderr, "VFlib internal error in try class.\n");
    abort();
  }

  if (font_try->fid < 0){
    vf_error = VF_ERR_NO_GLYPH;
    return -1;
  }

  return VF_GetFontBoundingBox1(font_try->fid,
				mag_x, mag_y, w_p, h_p, xoff_p, yoff_p);
}

Private VF_BITMAP
try_get_bitmap1(VF_FONT font, long code_point,
		double mag_x, double mag_y)
{
  FONT_TRY  font_try;

  if ((font_try = (FONT_TRY)font->private) == NULL){
    fprintf(stderr, "VFlib internal error in try class.\n");
    abort();
  }

  if (font_try->fid < 0){
    vf_error = VF_ERR_NO_GLYPH;
    return NULL;
  }

  return VF_GetBitmap1(font_try->fid, code_point, mag_x, mag_y);
}


Private VF_OUTLINE
try_get_outline(VF_FONT font, long code_point,
		double mag_x, double mag_y)
{
  FONT_TRY  font_try;

  if ((font_try = (FONT_TRY)font->private) == NULL){
    fprintf(stderr, "VFlib internal error in try class.\n");
    abort();
  }

  if (font_try->fid < 0){
    vf_error = VF_ERR_NO_GLYPH;
    return NULL;
  }

  return VF_GetOutline(font_try->fid, code_point, mag_x, mag_y);
}


Private int
try_get_metric2(VF_FONT font, long code_point, VF_METRIC2 metric, 
		  double mag_x, double mag_y)
{
  FONT_TRY  font_try;

  if ((font_try = (FONT_TRY)font->private) == NULL){
    fprintf(stderr, "VFlib internal error in try class.\n");
    abort();
  }

  if (font_try->fid < 0){
    vf_error = VF_ERR_NO_GLYPH;
    return -1;
  }

  VF_GetMetric2(font_try->fid, code_point, metric, mag_x, mag_y);

  return 0;
}

Private int
try_get_fontbbx2(VF_FONT font, double mag_x, double mag_y,
		 int *w_p, int *h_p, int *xoff_p, int *yoff_p)
{
  FONT_TRY  font_try;
  
  if ((font_try = (FONT_TRY)font->private) == NULL){
    fprintf(stderr, "VFlib internal error in try class.\n");
    abort();
  }

  if (font_try->fid < 0){
    vf_error = VF_ERR_NO_GLYPH;
    return -1;
  }

  return VF_GetFontBoundingBox2(font_try->fid,
				mag_x, mag_y, w_p, h_p, xoff_p, yoff_p);
}


Private VF_BITMAP
try_get_bitmap2(VF_FONT font, long code_point, 
		double mag_x, double mag_y)
{
  FONT_TRY  font_try;

  if ((font_try = (FONT_TRY)font->private) == NULL){
    fprintf(stderr, "VFlib internal error in try class.\n");
    abort();
  }

  if (font_try->fid < 0){
    vf_error = VF_ERR_NO_GLYPH;
    return NULL;
  }

  return VF_GetBitmap2(font_try->fid, code_point, mag_x, mag_y);
}


Private char*
try_get_font_prop(VF_FONT font, char *prop_name)
{
  FONT_TRY  font_try;

  if ((font_try = (FONT_TRY)font->private) == NULL){
    fprintf(stderr, "VFlib internal error in try class.\n");
    abort();
  }

  if (font_try->fid < 0){
    vf_error = VF_ERR_NO_GLYPH;
    return NULL;
  }

  return VF_GetFontProp(font_try->fid, prop_name);
}


Private int
try_query_font_type(VF_FONT font, long code_point)
{
  FONT_TRY  font_try;

  if ((font_try = (FONT_TRY)font->private) == NULL){
    fprintf(stderr, "VFlib internal error in try class.\n");
    abort();
  }

  if (font_try->fid < 0){
    vf_error = VF_ERR_NO_GLYPH;
    return -1;
  }

  return VF_QueryFontType(font_try->fid, code_point);
}



Private int
debug_on(char type)
{
  char  *p;

  if (default_debug_mode == NULL)
    return FALSE;
  if ((p = vf_sexp_get_cstring(default_debug_mode)) == NULL)
    return FALSE;
  
  while (*p != '\0'){
    if (*p == type)
      return TRUE;
    p++;
  }

  while (*p != '\0'){
    if (*p == '*')
      return TRUE;
    p++;
  }

  return TRUE;
}


/*EOF*/
