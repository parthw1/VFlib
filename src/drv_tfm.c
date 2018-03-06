/*
 * drv_tfm.c - A font driver for TFM fonts.
 *
 * 30 Sep 1996  First version.
 * 17 Jan 1997  for VFlib 3.1
 * 24 Feb 1997  Changed metric computation.
 * 26 Feb 1997  Added 'query_font_type'.
 *  4 Aug 1997  VFlib 3.3  Changed API.
 *  1 Feb 1998  for VFlib 3.4
 * 21 Apr 1998  Debugged get_font_prop().
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
#include  "sexp.h"
#include  "cache.h"
#include  "bitmap.h"
#include  "str.h"
#include  "path.h"
#include  "fsearch.h"
#include  "texfonts.h"
#include  "tfm.h"



struct s_font_tfm {
  TFM      tfm;
  char     *font_name;
  char     *font_file;
  double   point_size;
  double   pixel_size;
  int      glyph_style;
  double   mag;
  double   aspect;
  double   dpi_x, dpi_y;
  SEXP     props;
  double   extra_mag;
};
typedef struct s_font_tfm  *FONT_TFM;


Private SEXP_LIST    default_fontdirs;
Private SEXP_LIST    default_extensions;
Private SEXP_STRING  default_glyph_style;
Private int          v_default_glyph_style;
Private SEXP_STRING  default_point_size;
Private double       v_default_point_size;
Private SEXP_STRING  default_pixel_size;
Private double       v_default_pixel_size;
Private SEXP_STRING  default_dpi;
Private double       v_default_dpi_x, v_default_dpi_y;
Private SEXP_STRING  default_aspect;
Private double       v_default_aspect;
Private SEXP_ALIST   default_properties;
Private SEXP_ALIST   default_variables;
Private SEXP_STRING  default_debug_mode;


Private int         tfm_create(VF_FONT,char*,char*,int,SEXP);
Private int         tfm_close(VF_FONT);
Private int         tfm_get_metric1(VF_FONT,long,VF_METRIC1,double,double);
Private int         tfm_get_metric2(VF_FONT,long,VF_METRIC2,double,double);
Private int         tfm_get_fontbbx1(VF_FONT,double,double,
				     double*,double*,double*,double*);
Private int         tfm_get_fontbbx2(VF_FONT,double,double, 
				     int*,int*,int*,int*);
Private VF_BITMAP   tfm_get_bitmap1(VF_FONT,long,double,double);
Private VF_BITMAP   tfm_get_bitmap2(VF_FONT,long,double,double);
Private VF_OUTLINE  tfm_get_outline(VF_FONT,long,double,double);
Private char*       tfm_get_font_prop(VF_FONT,char*);



Glocal int
VF_Init_Driver_TFM(void)
{
  struct s_capability_table  ct[20];
  int  z;

  z = 0;
  /* VF_CAPE_FONT_DIRECTORIES */
  ct[z].cap = VF_CAPE_FONT_DIRECTORIES;  ct[z].type = CAPABILITY_LIST;
  ct[z].ess = CAPABILITY_OPTIONAL;       ct[z++].val = &default_fontdirs;
  /* VF_CAPE_EXTENSIONS */
  ct[z].cap = VF_CAPE_EXTENSIONS;        ct[z].type = CAPABILITY_LIST;
  ct[z].ess = CAPABILITY_OPTIONAL;       ct[z++].val = &default_extensions;
  /* VF_CAPE_TEX_GLYPH_STYLE */
  ct[z].cap = VF_CAPE_TEX_GLYPH_STYLE;   ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;       ct[z++].val = &default_glyph_style;
  /* VF_CAPE_POINT_SIZE */
  ct[z].cap = VF_CAPE_POINT_SIZE;        ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;       ct[z++].val = &default_point_size;
  /* VF_CAPE_PIXEL_SIZE */
  ct[z].cap = VF_CAPE_PIXEL_SIZE;        ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;       ct[z++].val = &default_pixel_size;
  /* VF_CAPE_DPI */
  ct[z].cap = VF_CAPE_DPI;               ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;       ct[z++].val = &default_dpi;
  /* VF_CAPE_ASPECT_RATIO */
  ct[z].cap = VF_CAPE_ASPECT_RATIO;      ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;       ct[z++].val = &default_aspect;
  /* VF_CAPE_PROPERTIES */
  ct[z].cap = VF_CAPE_PROPERTIES;        ct[z].type = CAPABILITY_ALIST;
  ct[z].ess = CAPABILITY_OPTIONAL;       ct[z++].val = &default_properties;
  /* VF_CAPE_VARIABLE_VALUES */
  ct[z].cap = VF_CAPE_VARIABLE_VALUES;   ct[z].type = CAPABILITY_ALIST;
  ct[z].ess = CAPABILITY_OPTIONAL;       ct[z++].val = &default_variables;
  /* VF_CAPE_DEBUG */
  ct[z].cap = VF_CAPE_DEBUG;             ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;       ct[z++].val = &default_debug_mode;
  /* end */
  ct[z].cap = NULL; ct[z].type = 0; ct[z].ess = 0; ct[z++].val = NULL;


  if (vf_cap_GetParsedClassDefault(FONTCLASS_NAME_TFM, ct, 
				   vf_tex_default_variables, NULL) 
      == VFLIBCAP_PARSED_ERROR)
    return -1;

  if (default_extensions == NULL)
    default_extensions = vf_sexp_cstring2list(DEFAULT_EXTENSIONS_TFM);

  v_default_glyph_style = TEX_GLYPH_STYLE_EMPTY;
  if (default_glyph_style != NULL)
    v_default_glyph_style
      = vf_tex_parse_glyph_style(vf_sexp_get_cstring(default_glyph_style),
				 TEX_GLYPH_STYLE_EMPTY);

  v_default_point_size = -1;
  if (default_point_size != NULL)
    v_default_point_size = atof(vf_sexp_get_cstring(default_point_size));

  v_default_pixel_size = -1;
  if (default_pixel_size != NULL)
    v_default_pixel_size = atof(vf_sexp_get_cstring(default_pixel_size));

  v_default_dpi_x  = -1;
  v_default_dpi_y  = -1;
  if (default_dpi != NULL)
    v_default_dpi_x = v_default_dpi_y = atof(vf_sexp_get_cstring(default_dpi));

  v_default_aspect = 1.0;
  if (default_aspect != NULL)
    v_default_aspect = atof(vf_sexp_get_cstring(default_aspect));

  vf_tex_init();
  vf_tfm_init();

  VF_InstallFontDriver(FONTCLASS_NAME_TFM, (DRIVER_FUNC_TYPE)tfm_create);

  return 0;
}

Private int
tfm_create(VF_FONT font, char *font_class, 
	   char *font_name, int implicit, SEXP entry)
{
  FONT_TFM  font_tfm;
  char      *font_file, *tfm_path, *p;
  int       dev_dpi;
  SEXP      cap_ffile, cap_glyph_style, cap_point, cap_pixel;
  SEXP      cap_dpi, cap_mag, cap_aspect, cap_props;
  struct s_capability_table  ct[20];
  int  z;

  z = 0;
  /* VF_CAPE_FONT_CLASS */
  ct[z].cap = VF_CAPE_FONT_CLASS;        ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_ESSENTIAL;      ct[z++].val = NULL;
  /* VF_CAPE_FONT_FILE */
  ct[z].cap = VF_CAPE_FONT_FILE;         ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;       ct[z++].val = &cap_ffile;
  /* VF_CAPE_TEX_GLYPH_STYLE */
  ct[z].cap = VF_CAPE_TEX_GLYPH_STYLE;   ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;       ct[z++].val = &cap_glyph_style;
  /* VF_CAPE_POINT_SIZE */
  ct[z].cap = VF_CAPE_POINT_SIZE;        ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;       ct[z++].val = &cap_point;
  /* VF_CAPE_PIXEL_SIZE */
  ct[z].cap = VF_CAPE_PIXEL_SIZE;        ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;       ct[z++].val = &cap_pixel;
  /* VF_CAPE_DPI */
  ct[z].cap = VF_CAPE_DPI;               ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;       ct[z++].val = &cap_dpi;
  /* VF_CAPE_MAG */
  ct[z].cap = VF_CAPE_MAG;               ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;       ct[z++].val = &cap_mag;
  /* VF_CAPE_ASPECT_RATIO */
  ct[z].cap = VF_CAPE_ASPECT_RATIO;      ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;       ct[z++].val = &cap_aspect;
  /* VF_CAPE_PROPERTIES */
  ct[z].cap = VF_CAPE_PROPERTIES;        ct[z].type = CAPABILITY_ALIST;
  ct[z].ess = CAPABILITY_OPTIONAL;       ct[z++].val = &cap_props;
  /* end */
  ct[z].cap = NULL; ct[z].type = 0; ct[z].ess = 0; ct[z++].val = NULL;


  if (implicit == 1){   /* implicit font */
    font_file = font_name;
  } else {              /* explicit font */
    if (vf_cap_GetParsedFontEntry(entry, font_name, ct,
				  default_variables, vf_tex_default_variables) 
	== VFLIBCAP_PARSED_ERROR)
      return -1;
    if (cap_ffile == NULL){
      /* Use font name as font file name if font file name is not given. */
      font_file = font_name;
    } else {
      font_file = vf_sexp_get_cstring(cap_ffile);
    }
  }

  font->font_type       = VF_FONT_TYPE_BITMAP;
  font->get_metric1     = tfm_get_metric1;
  font->get_metric2     = tfm_get_metric2;
  font->get_fontbbx1    = tfm_get_fontbbx1;
  font->get_fontbbx2    = tfm_get_fontbbx2;
  font->get_bitmap1     = tfm_get_bitmap1;
  font->get_bitmap2     = tfm_get_bitmap2;
  font->get_outline     = tfm_get_outline;
  font->get_font_prop   = tfm_get_font_prop;
  font->query_font_type = NULL;
  font->close           = tfm_close;

  tfm_path = NULL;
  font_tfm = NULL;
  ALLOC_IF_ERR(font_tfm, struct s_font_tfm)
    goto NoMemoryError;
  font->private = font_tfm;

  font_tfm->tfm        = NULL;
  font_tfm->font_name  = NULL; 
  font_tfm->font_file  = NULL; 
  font_tfm->point_size = v_default_point_size;
  font_tfm->pixel_size = v_default_pixel_size;
  font_tfm->glyph_style= v_default_glyph_style;
  font_tfm->dpi_x      = v_default_dpi_x;
  font_tfm->dpi_y      = v_default_dpi_y;
  font_tfm->mag        = 1.0;
  font_tfm->aspect     = v_default_aspect;
  font_tfm->props      = NULL;

  if (implicit == 0){
    if (cap_point != NULL)
      font_tfm->point_size = atof(vf_sexp_get_cstring(cap_point));
    if (cap_pixel != NULL)
      font_tfm->pixel_size = atof(vf_sexp_get_cstring(cap_pixel));
    if (cap_glyph_style != NULL){
      if (strcmp(vf_sexp_get_cstring(cap_glyph_style),
		 TEX_GLYPH_STYLE_EMPTY_STR) == 0)
	font_tfm->glyph_style = TEX_GLYPH_STYLE_EMPTY;
      else if (strcmp(vf_sexp_get_cstring(cap_glyph_style),
		      TEX_GLYPH_STYLE_FILL_STR) == 0)
	font_tfm->glyph_style = TEX_GLYPH_STYLE_FILL;
    }
    if (cap_dpi != NULL)
      font_tfm->dpi_x = font_tfm->dpi_y = atof(vf_sexp_get_cstring(cap_dpi));
    if (cap_mag != NULL)
      font_tfm->mag = atof(vf_sexp_get_cstring(cap_mag));
    if (cap_aspect != NULL)
      font_tfm->aspect = atof(vf_sexp_get_cstring(cap_aspect));
    if (cap_props != NULL)
      font_tfm->props = cap_props;
  }

  if ((font_tfm->font_file = vf_strdup(font_file)) == NULL)
    goto NoMemoryError;
  if ((font_tfm->font_name = vf_strdup(font_name)) == NULL)
    goto NoMemoryError;


  /* Parse font name.  Formats of file names that this routine supports:
   *    "cmr10.300XX" - A "cmr10" font for 300 dpi.
   *    "cmr10.XX"    - A "cmr10" font. Dpi value is default value.
   *    "cmr10"       -   ditto.
   * ("XX" can be any string such as "pk", "gf", and "tfm".)
   */
  p = vf_index(font_tfm->font_file, '.');
  if ((p != NULL) && (isdigit((int)*(p+1)))){   /* "cmr10.300tfm" */
    dev_dpi = atoi(p+1);
    if ((font_tfm->dpi_x < 0) || (font_tfm->dpi_y < 0)){
      font_tfm->dpi_x = (double)dev_dpi;
      font_tfm->dpi_y = (double)dev_dpi;
    }
  } else {                                 /* "cmr10" or "cmr10.tfm" */
    ;
  }
 
  tfm_path = vf_tex_search_file_tfm(font_tfm->font_file, 
				    default_fontdirs, default_extensions);
#if 0
  printf("* TFM: %s ==> %s\n", font_name, tfm_path);
#endif
  if (tfm_path == NULL)
    goto Error;

  font_tfm->tfm = vf_tfm_open(tfm_path);
  vf_free(tfm_path);
  if (font_tfm->tfm == NULL)
    goto Error;

  if (implicit == 0){
    vf_sexp_free2(&cap_ffile, &cap_glyph_style);
    vf_sexp_free2(&cap_point, &cap_pixel);
    vf_sexp_free3(&cap_dpi, &cap_mag, &cap_aspect);
  }

  return 0;


NoMemoryError:
  vf_error = VF_ERR_NO_MEMORY;
Error:
  if (implicit == 0){
    vf_sexp_free2(&cap_ffile, &cap_glyph_style);
    vf_sexp_free2(&cap_point, &cap_pixel);
    vf_sexp_free3(&cap_dpi, &cap_mag, &cap_aspect);
  }
  tfm_close(font);
  return -1;
}


Private int
tfm_close(VF_FONT font)
{
  FONT_TFM  font_tfm;

  font_tfm = (FONT_TFM)font->private;
  if (font_tfm != NULL){
    vf_tfm_free(font_tfm->tfm);
    vf_free(font_tfm->font_name);
    vf_free(font_tfm->font_file);
    vf_sexp_free1(&font_tfm->props);
    vf_free(font_tfm);
  }

  return 0; 
}

Private int
tfm_get_metric1(VF_FONT font, long code_point, VF_METRIC1 metric,
		double mag_x, double mag_y)
{
  FONT_TFM  font_tfm;
  TFM       tfm;
  double    mx, my, ps, f;

  if (   (metric == NULL)
      || ((font_tfm = (FONT_TFM)font->private) == NULL)
      || ((tfm = font_tfm->tfm) == NULL) ){
    fprintf(stderr, "VFlib internal error: tfm_get_metric1()\n");
    abort();
  }

  if (vf_tfm_metric(tfm, code_point, metric) == NULL)
    return -1;

  if ((ps = font->point_size) < 0)
    if ((ps = font_tfm->point_size) < 0)
      ps = -1;

  if ((ps < 0) || (tfm->design_size < 1.0e-3)){
    mx = mag_x * font->mag_x * font_tfm->mag;
    my = mag_y * font->mag_y * font_tfm->mag;
  } else {
    f = ps / tfm->design_size;
    mx = mag_x * font->mag_x * font_tfm->mag * f;
    my = mag_y * font->mag_y * font_tfm->mag * f;
  }

  metric->bbx_width  *= mx;
  metric->bbx_height *= my;
  metric->off_x      *= mx;
  metric->off_y      *= my;
  metric->mv_x       *= mx;
  metric->mv_y       *= my;

  return 0;
}

Private int
tfm_get_fontbbx1(VF_FONT font, double mag_x, double mag_y, 
		 double *w_p, double *h_p, double *xoff_p, double *yoff_p)
{
  FONT_TFM  font_tfm;
  TFM       tfm;
  double    mx, my, ps, f;

  if (((font_tfm = (FONT_TFM)font->private) == NULL)
      || ((tfm = font_tfm->tfm) == NULL) ){
    fprintf(stderr, "VFlib internal error: tfm_get_fontbbx1()\n");
    abort();
  }

  if ((ps = font->point_size) < 0)
    if ((ps = font_tfm->point_size) < 0)
      ps = -1;

  if ((ps < 0) || (tfm->design_size < 1.0e-3)){
    mx = mag_x * font->mag_x * font_tfm->mag;
    my = mag_y * font->mag_y * font_tfm->mag;
  } else {
    f = ps / tfm->design_size;
    mx = mag_x * font->mag_x * font_tfm->mag * f;
    my = mag_y * font->mag_y * font_tfm->mag * f;
  }

  *w_p    = tfm->font_bbx_w * mx;
  *h_p    = tfm->font_bbx_h * my;
  *xoff_p = tfm->font_bbx_xoff * mx;
  *yoff_p = tfm->font_bbx_yoff * my;

  return 0;
}

Private VF_BITMAP
tfm_get_bitmap1(VF_FONT font, long code_point,
		double mag_x, double mag_y)
{
  FONT_TFM   font_tfm;
  VF_BITMAP  bm;
  double     dpi_x, dpi_y;
  struct vf_s_metric1  met; 

  if ((font_tfm = (FONT_TFM)font->private) == NULL){
    fprintf(stderr, "VFlib internal error: tfm_get_bitmap1()\n");
    abort();
  }

  if (tfm_get_metric1(font, code_point, &met, mag_x, mag_y) < 0)
    return NULL;

  if (((dpi_x = font->dpi_x) < 0) || ((dpi_y = font->dpi_y) < 0)){
    if (((dpi_x = font_tfm->dpi_x) < 0) || ((dpi_y = font_tfm->dpi_y) < 0)){
      dpi_x = vf_tex_default_dpi();
      dpi_y = vf_tex_default_dpi();
    }
  }

  bm = vf_alloc_bitmap_with_metric1(&met, dpi_x, dpi_y);

  switch (font_tfm->glyph_style){
  default:
  case TEX_GLYPH_STYLE_EMPTY:
    break;
  case TEX_GLYPH_STYLE_FILL:
    VF_FillBitmap(bm);
    break;
  }

  return bm;
}

Private VF_OUTLINE
tfm_get_outline(VF_FONT font, long code_point,
		double mag_x, double mag_y)
{
  FONT_TFM            font_tfm;
  TFM                 tfm;
  VF_OUTLINE          ol;
  double              dpi_x, dpi_y, bbx, ps;
  int                 size, x1, y1, x2, y2, index;
  struct vf_s_metric1 met;

  if ( ((font_tfm = (FONT_TFM)font->private) == NULL)
      || ((tfm = font_tfm->tfm) == NULL) ){
    fprintf(stderr, "VFlib internal error: tfm_get_outline()\n");
    abort();
  }

  if (tfm_get_metric1(font, code_point, &met, mag_x, mag_y) < 0)
    return NULL;

  if ((ps = font->point_size) < 0)
    if ((ps = font_tfm->point_size) < 0)
      ps = tfm->design_size;

  if (((dpi_x = font->dpi_x) < 0) || ((dpi_y = font->dpi_y) < 0)){
    if (((dpi_x = font_tfm->dpi_x) < 0) || ((dpi_y = font_tfm->dpi_y) < 0)){
      dpi_x = vf_tex_default_dpi();
      dpi_y = vf_tex_default_dpi();
    }
  }

  if ((bbx = tfm->font_bbx_w) < tfm->font_bbx_h)
    bbx = tfm->font_bbx_h;

  size = VF_OL_OUTLINE_HEADER_SIZE_TYPE0 + 6;
  if ((ol = (VF_OUTLINE)calloc(size, sizeof(VF_OUTLINE_ELEM))) == NULL){
    vf_error = VF_ERR_NO_MEMORY;
    return NULL;
  }

  ol[VF_OL_HEADER_INDEX_HEADER_TYPE] = VF_OL_OUTLINE_HEADER_TYPE0;
  ol[VF_OL_HEADER_INDEX_DATA_SIZE]   = size;
  ol[VF_OL_HEADER_INDEX_DPI_X]       = VF_OL_HEADER_ENCODE(dpi_x);
  ol[VF_OL_HEADER_INDEX_DPI_Y]       = VF_OL_HEADER_ENCODE(dpi_y);
  ol[VF_OL_HEADER_INDEX_POINT_SIZE]  = VF_OL_HEADER_ENCODE(ps);
  ol[VF_OL_HEADER_INDEX_EM]    = VF_OL_COORD_RANGE;
  ol[VF_OL_HEADER_INDEX_MAX_X] = VF_OL_COORD_RANGE * met.bbx_width/bbx;
  ol[VF_OL_HEADER_INDEX_MAX_Y] = VF_OL_COORD_RANGE * met.bbx_height/bbx;
  ol[VF_OL_HEADER_INDEX_REF_X] = VF_OL_COORD_RANGE * (0.0 - met.off_x/bbx);
  ol[VF_OL_HEADER_INDEX_REF_Y] = VF_OL_COORD_RANGE * met.off_y/bbx;
  ol[VF_OL_HEADER_INDEX_MV_X]  = VF_OL_COORD_RANGE * met.mv_x/bbx;
  ol[VF_OL_HEADER_INDEX_MV_Y]  = VF_OL_COORD_RANGE * met.mv_y/bbx;

  x1 = VF_OL_COORD_OFFSET + 0;
  x2 = VF_OL_COORD_OFFSET + VF_OL_COORD_RANGE * met.bbx_width / bbx;
  y1 = VF_OL_COORD_OFFSET + 0;
  y2 = VF_OL_COORD_OFFSET + VF_OL_COORD_RANGE * met.bbx_height / bbx;

  index = VF_OL_OUTLINE_HEADER_SIZE_TYPE0;
  ol[index++] =   VF_OL_INSTR_TOKEN | VF_OL_INSTR_CCWCURV
                | VF_OL_INSTR_LINE  | VF_OL_INSTR_CHAR;
  ol[index++] = VF_OL_MAKE_XY(x1, y1);
  ol[index++] = VF_OL_MAKE_XY(x1, y2);
  ol[index++] = VF_OL_MAKE_XY(x2, y2);
  ol[index++] = VF_OL_MAKE_XY(x2, y1);
  ol[index++] = 0L;

  return ol;
}

Private int
tfm_get_metric2(VF_FONT font, long code_point, VF_METRIC2 metric, 
		double mag_x, double mag_y)
{
  FONT_TFM             font_tfm;
  TFM                  tfm;
  struct vf_s_metric1  met1;
  int                  ps;
  double               dpi_x, dpi_y, mx, my;

  if (   (metric == NULL)
      || ((font_tfm = (FONT_TFM)font->private) == NULL)
      || ((tfm = font_tfm->tfm) == NULL) ){
    fprintf(stderr, "VFlib internal error in tfm_get_metric2()\n");
    abort();
  }

  if (vf_tfm_metric(tfm, code_point, &met1) == NULL)
    return -1;

  if (((dpi_x = font_tfm->dpi_x) <= 0) || ((dpi_y = font_tfm->dpi_y) <= 0)){
    dpi_x = vf_tex_default_dpi();
    dpi_y = vf_tex_default_dpi();
  }

  if ((ps = font->pixel_size) < 0)
    if ((ps = font_tfm->pixel_size) < 0)
      ps = -1;

  if ((ps < 0) || (tfm->design_size < 1.0e-3)){
    mx = mag_x * font->mag_x * font_tfm->mag * dpi_x / 72.27;
    my = mag_y * font->mag_y * font_tfm->mag * dpi_y / 72.27;
  } else {
    mx = mag_x * font->mag_x * font_tfm->mag * (ps/tfm->design_size);
    my = mag_y * font->mag_y * font_tfm->mag * (ps/tfm->design_size);
  }

  metric->bbx_width  = toint(met1.bbx_width  * mx);
  metric->bbx_height = toint(met1.bbx_height * my);
  metric->off_x      = toint(met1.off_x * mx);
  metric->off_y      = toint(met1.off_y * my);
  metric->mv_x       = toint(met1.mv_x  * mx);
  metric->mv_y       = toint(met1.mv_y  * my);

  return 0;
}

Private int
tfm_get_fontbbx2(VF_FONT font, double mag_x, double mag_y,
		 int *w_p, int *h_p, int *xoff_p, int *yoff_p)
{
  FONT_TFM             font_tfm;
  TFM                  tfm;
  int                  ps;
  double               dpi_x, dpi_y, mx, my;

  if (((font_tfm = (FONT_TFM)font->private) == NULL)
      || ((tfm = font_tfm->tfm) == NULL) ){
    fprintf(stderr, "VFlib internal error in tfm_get_fontbbx2()\n");
    abort();
  }

  if (((dpi_x = font_tfm->dpi_x) <= 0) || ((dpi_y = font_tfm->dpi_y) <= 0)){
    dpi_x = vf_tex_default_dpi();
    dpi_y = vf_tex_default_dpi();
  }

  if ((ps = font->pixel_size) < 0)
    if ((ps = font_tfm->pixel_size) < 0)
      ps = -1;

  if ((ps < 0) || (tfm->design_size < 1.0e-3)){
    mx = mag_x * font->mag_x * font_tfm->mag * dpi_x / 72.27;
    my = mag_y * font->mag_y * font_tfm->mag * dpi_y / 72.27;
  } else {
    mx = mag_x * font->mag_x * font_tfm->mag * (ps/tfm->design_size);
    my = mag_y * font->mag_y * font_tfm->mag * (ps/tfm->design_size);
  }

  *w_p    = toint(tfm->font_bbx_w  * mx);
  *h_p    = toint(tfm->font_bbx_h  * my);
  *xoff_p = toint(tfm->font_bbx_xoff  * mx);
  *yoff_p = toint(tfm->font_bbx_yoff  * my);

  return 0;
}

Private VF_BITMAP
tfm_get_bitmap2(VF_FONT font, long code_point, 
		double mag_x, double mag_y)
{
  FONT_TFM   font_tfm;
  VF_BITMAP  bm;
  struct vf_s_metric2  met;

  if ((font_tfm = (FONT_TFM)font->private) == NULL){
    fprintf(stderr, "VFlib internal error in tfm_get_bitmap2()\n");
    abort();
  }

  if (tfm_get_metric2(font, code_point, &met, mag_x, mag_y) < 0)
    return NULL;
  
  bm = vf_alloc_bitmap_with_metric2(&met);

  switch (font_tfm->glyph_style){
  default:
  case TEX_GLYPH_STYLE_EMPTY:
    break;
  case TEX_GLYPH_STYLE_FILL:
    VF_FillBitmap(bm);
    break;
  }

  return bm;
}


Private char*
tfm_get_font_prop(VF_FONT font, char *prop_name)
{
  FONT_TFM  font_tfm;
  TFM       tfm;
  SEXP      v;
  double    ps, dpi_x, dpi_y;
  char      str[256];

  if (   ((font_tfm = (FONT_TFM)font->private) == NULL)
      || ((tfm = font_tfm->tfm) == NULL) ){
    fprintf(stderr, "VFlib internal error in tfm_get_font_prop()\n");
    abort();
  }

  if ((v = vf_sexp_assoc(prop_name, font_tfm->props)) != NULL){
    return vf_strdup(vf_sexp_get_cstring(vf_sexp_cadr(v)));
  } else if ((v = vf_sexp_assoc(prop_name, default_properties)) != NULL){
    return vf_strdup(vf_sexp_get_cstring(vf_sexp_cadr(v)));
  } else {

    if (((dpi_x = font->dpi_x)<=0) || ((dpi_y = font->dpi_y)<=0)){
      if (((dpi_x = font_tfm->dpi_x)<=0) || ((dpi_y = font_tfm->dpi_y)<=0)){
	dpi_x = vf_tex_default_dpi();
	dpi_y = vf_tex_default_dpi();
      }
    }
#if 0
    printf("** %.3f %.3f %.3f  %d %.3f %.3f %.3f\n",
	   dpi_x, dpi_y, tfm->design_size, 
	   font->pixel_size, font_tfm->pixel_size,
	   font->point_size, font_tfm->point_size);
#endif

    if (font->mode == 1){
      if ((ps = font->point_size) < 0)
	if ((ps = font_tfm->point_size) < 0)
	  ps = tfm->design_size;
      ps = ps * font->mag_y * font_tfm->mag;
      if (strcmp(prop_name, "POINT_SIZE") == 0){
	sprintf(str, "%d", toint(ps * 10.0));
	return vf_strdup(str);
      } else if (strcmp(prop_name, "PIXEL_SIZE") == 0){
	sprintf(str, "%d", toint(ps * dpi_y / 72.27));
	return vf_strdup(str);
      } else if (strcmp(prop_name, "RESOLUTION_X") == 0){
	sprintf(str, "%d", toint(dpi_x));
	return vf_strdup(str);
      } else if (strcmp(prop_name, "RESOLUTION_Y") == 0){
	sprintf(str, "%d", toint(dpi_y));
	return vf_strdup(str);
      }

    } else if (font->mode == 2){
      if (strcmp(prop_name, "POINT_SIZE") == 0){
	if ((ps = font->pixel_size) < 0){
	  if ((ps = font_tfm->pixel_size) < 0){
	    sprintf(str, "%d", toint(tfm->design_size * 10.0 
				     * font->mag_y * font_tfm->mag));
	    return vf_strdup(str);
	  }
	}
	ps = ps * font->mag_y * font_tfm->mag;
	sprintf(str, "%d", toint(ps * 10.0 * 72.27 / dpi_y));
	return vf_strdup(str);
      } else if (strcmp(prop_name, "PIXEL_SIZE") == 0){
	if ((ps = font->pixel_size) < 0)
	  if ((ps = font_tfm->pixel_size) < 0)
	    ps = tfm->design_size * dpi_y / 72.27;
	ps = ps * font->mag_y * font_tfm->mag;
	sprintf(str, "%d", toint(ps));
	return vf_strdup(str);
      } else if (strcmp(prop_name, "RESOLUTION_X") == 0){
	sprintf(str, "%d", toint(dpi_x));
	return vf_strdup(str);
      } else if (strcmp(prop_name, "RESOLUTION_Y") == 0){
	sprintf(str, "%d", toint(dpi_y));
	return vf_strdup(str);
      }
    }
  }

  return NULL;
}


/*EOF*/
