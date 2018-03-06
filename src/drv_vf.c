/*
 * drv_vf.c - A font driver for vf (virtual font) format fonts.
 * by Hirotsugu Kakugawa
 *
 * 30 Jan 1997  First implementation.
 *  7 Aug 1997  VFlib 3.3  Changed API.
 *  2 Feb 1998  VFlib 3.4
 * 21 Apr 1998  Debugged get_font_prop().
 * 24 Nov 1998  Added get_fontbbx1() and get_fontbbx2().
 * 16 Sep 1999  Bug fixed.
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
 *    s - print subfont information
 *    d - trace dvi instruction execution
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
#include  "VFlib-3_7.h"
#include  "VFsys.h"
#include  "vflibcap.h"
#include  "vflibcap.h"
#include  "bitmap.h"
#include  "metric.h"
#include  "bmlist.h"
#include  "cache.h"
#include  "str.h"
#include  "path.h"
#include  "fsearch.h"
#include  "sexp.h"
#include  "texfonts.h"
#include  "tfm.h"
#include  "vf.h"


struct s_font_vf {
  int      vf_id;
  char     *font_file;
  char     *font_name;
  double   point_size;
  int      pixel_size;
  int      open_style;
  int      glyph_style;
  double   mag;
  double   dpi_x, dpi_y;
  SEXP     props;
};
typedef struct s_font_vf  *FONT_VF;


Private SEXP_LIST    default_fontdirs       = NULL;
Private SEXP_LIST    default_extensions     = NULL;
Private SEXP_LIST    default_tfm_dirs       = NULL;
Private SEXP_LIST    default_tfm_extensions = NULL;
Private SEXP         default_font_mapping   = NULL;
Private SEXP_STRING  default_glyph_style    = NULL;
Private int          v_default_glyph_style  = TEX_GLYPH_STYLE_DEFAULT;
Private SEXP_STRING  default_open_style     = NULL;
Private int          v_default_open_style   = TEX_OPEN_STYLE_DEFAULT;
Private SEXP_STRING  default_point_size     = NULL;
Private double       v_default_point_size   = -1;
Private SEXP_STRING  default_pixel_size     = NULL;
Private double       v_default_pixel_size   = -1;
Private SEXP_STRING  default_dpi            = NULL;
Private double       v_default_dpi_x        = DEFAULT_DPI;
Private double       v_default_dpi_y        = DEFAULT_DPI;
Private SEXP_ALIST   default_properties     = NULL;
Private SEXP_ALIST   default_variables      = NULL;
Private SEXP_STRING  default_debug_mode     = NULL;


Private int         vf_create(VF_FONT,char*,char*,int,SEXP);
Private int         vf_close(VF_FONT);
Private int         vf_get_metric1(VF_FONT,long,VF_METRIC1,double,double);
Private int         vf_get_metric2(VF_FONT,long,VF_METRIC2,double,double);
Private int         vf_get_fontbbx1(VF_FONT,double,double,
				    double*,double*,double*,double*);
Private int         vf_get_fontbbx2(VF_FONT,double,double, 
				    int*,int*,int*,int*);
Private VF_BITMAP   vf_get_bitmap1(VF_FONT,long,double,double);
Private VF_BITMAP   vf_get_bitmap2(VF_FONT,long,double,double);
Private VF_OUTLINE  vf_get_outline(VF_FONT,long,double,double);
Private char*       vf_get_font_prop(VF_FONT,char*);
Private void        calc_mag_2(VF_FONT,FONT_VF, double mag_x, double mag_y, 
			       double *mx_p, double *my_p);
Private int         vf_debug(char type);

Private int         vf_vf_init(void);
Private int         vf_vf_open(VF_FONT font, FONT_VF font_vf, int implicit);
Private void        vf_vf_close(int);
Private VF_BITMAP   vf_vf_get_bitmap(int,int,long,double,double,int,int);
Private int         vf_vf_get_metric(int,long,VF_METRIC1,double*);
Private double      vf_vf_get_design_size(int);
Private VF          vf_vf_get_vf(int);


Glocal int
VF_Init_Driver_VF(void)
{
  struct s_capability_table  ct[20];
  int  z;

  z = 0;
  /* VF_CAPE_FONT_DIRECTORIES */
  ct[z].cap = VF_CAPE_FONT_DIRECTORIES;    ct[z].type = CAPABILITY_LIST;
  ct[z].ess = CAPABILITY_OPTIONAL;         ct[z++].val = &default_fontdirs;
  /* VF_CAPE_EXTENSIONS */
  ct[z].cap = VF_CAPE_EXTENSIONS;          ct[z].type = CAPABILITY_LIST;
  ct[z].ess = CAPABILITY_OPTIONAL;         ct[z++].val = &default_extensions;
  /* VF_CAPE_TEX_TFM_DIRECTORIES */
  ct[z].cap = VF_CAPE_TEX_TFM_DIRECTORIES; ct[z].type = CAPABILITY_LIST;
  ct[z].ess = CAPABILITY_OPTIONAL;         ct[z++].val = &default_tfm_dirs;
  /* VF_CAPE_TEX_TFM_EXTENSIONS */
  ct[z].cap = VF_CAPE_TEX_TFM_EXTENSIONS;  ct[z].type = CAPABILITY_LIST;
  ct[z].ess = CAPABILITY_OPTIONAL;       ct[z++].val = &default_tfm_extensions;
  /* VF_CAPE_TEX_FONT_MAPPING */
  ct[z].cap = VF_CAPE_TEX_FONT_MAPPING;    ct[z].type = CAPABILITY_LIST;
  ct[z].ess = CAPABILITY_OPTIONAL;         ct[z++].val = &default_font_mapping;
  /* VF_CAPE_TEX_GLYPH_STYLE */
  ct[z].cap = VF_CAPE_TEX_GLYPH_STYLE;     ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;         ct[z++].val = &default_glyph_style;
  /* VF_CAPE_TEX_OPEN_STYLE */
  ct[z].cap = VF_CAPE_TEX_OPEN_STYLE;      ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;         ct[z++].val = &default_open_style;
  /* VF_CAPE_POINT_SIZE */
  ct[z].cap = VF_CAPE_POINT_SIZE;          ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;         ct[z++].val = &default_point_size;
  /* VF_CAPE_PIXEL_SIZE */
  ct[z].cap = VF_CAPE_PIXEL_SIZE;          ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;         ct[z++].val = &default_pixel_size;
  /* VF_CAPE_DPI */
  ct[z].cap = VF_CAPE_DPI;                 ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;         ct[z++].val = &default_dpi;
  /* VF_CAPE_PROPERTIES */
  ct[z].cap = VF_CAPE_PROPERTIES;          ct[z].type = CAPABILITY_ALIST;
  ct[z].ess = CAPABILITY_OPTIONAL;         ct[z++].val = &default_properties;
  /* VF_CAPE_VARIABLE_VALUES */
  ct[z].cap = VF_CAPE_VARIABLE_VALUES;     ct[z].type = CAPABILITY_ALIST;
  ct[z].ess = CAPABILITY_OPTIONAL;         ct[z++].val = &default_variables;
  /* VF_CAPE_DEBUG */
  ct[z].cap = VF_CAPE_DEBUG;               ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;         ct[z++].val = &default_debug_mode;
  /* end */
  ct[z].cap = NULL; ct[z].type = 0; ct[z].ess = 0; ct[z++].val = NULL;


  if (vf_tex_init() < 0)
    return -1;
  if (vf_tfm_init() < 0)
    return -1;
  if (vf_vf_init() < 0)
    return -1;

  if (vf_cap_GetParsedClassDefault(FONTCLASS_NAME_VF, ct,
				   vf_tex_default_variables, NULL) 
      == VFLIBCAP_PARSED_ERROR)
    return -1;

  if (default_extensions == NULL)
    default_extensions = vf_sexp_cstring2list(DEFAULT_EXTENSIONS_VF);

  if (default_font_mapping != NULL){
    if (vf_tex_syntax_check_font_mapping(default_font_mapping) > 0){
      vf_sexp_free(&default_font_mapping);
      fprintf(stderr, 
	      "VFlib: capability %s is ignored because of syntax error.\n", 
	      VF_CAPE_TEX_FONT_MAPPING);
    }
  }

  v_default_glyph_style = TEX_GLYPH_STYLE_DEFAULT;
  if (default_glyph_style != NULL)
    v_default_glyph_style
      = vf_tex_parse_glyph_style(vf_sexp_get_cstring(default_glyph_style),
				 TEX_GLYPH_STYLE_DEFAULT);

  v_default_open_style = TEX_OPEN_STYLE_DEFAULT;
  if (default_open_style != NULL)
    v_default_open_style
      = vf_tex_parse_open_style(vf_sexp_get_cstring(default_open_style),
				TEX_OPEN_STYLE_DEFAULT);

  if (default_point_size != NULL)
    v_default_point_size = atof(vf_sexp_get_cstring(default_point_size));

  if (default_pixel_size != NULL)
    v_default_pixel_size = atof(vf_sexp_get_cstring(default_pixel_size));

  v_default_dpi_x  = DEFAULT_DPI;
  v_default_dpi_y  = DEFAULT_DPI;
  if (default_dpi != NULL)
    v_default_dpi_x = v_default_dpi_y = atof(vf_sexp_get_cstring(default_dpi));


  VF_InstallFontDriver(FONTCLASS_NAME_VF, (DRIVER_FUNC_TYPE)vf_create);
    
  return 0;
}


Private int
vf_create(VF_FONT font, char *font_class, 
	  char *font_name, int implicit, SEXP entry)
{
  FONT_VF   font_vf;
  char      *font_file;
  int       val;
  SEXP      cap_ffile, cap_point, cap_pixel, cap_mag, cap_dpi, cap_props;
  struct s_capability_table  ct[10];
  int z;

  z = 0;
  /* VF_CAPE_FONT_CLASS */
  ct[z].cap = VF_CAPE_FONT_CLASS;   ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_ESSENTIAL; ct[z++].val = NULL;
  /* VF_CAPE_FONT_FILE */
  ct[z].cap = VF_CAPE_FONT_FILE;    ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;  ct[z++].val = &cap_ffile;
  /* VF_CAPE_POINT_SIZE */
  ct[z].cap = VF_CAPE_POINT_SIZE;   ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;  ct[z++].val = &cap_point;
  /* VF_CAPE_PIXEL_SIZE */
  ct[z].cap = VF_CAPE_PIXEL_SIZE;   ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;  ct[z++].val = &cap_pixel;
  /* VF_CAPE_DPI */
  ct[z].cap = VF_CAPE_DPI;          ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;  ct[z++].val = &cap_dpi;
  /* VF_CAPE_MAG */
  ct[z].cap = VF_CAPE_MAG;          ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;  ct[z++].val = &cap_mag;
  /* VF_CAPE_PROPERTIES */
  ct[z].cap = VF_CAPE_PROPERTIES;   ct[z].type = CAPABILITY_ALIST;
  ct[z].ess = CAPABILITY_OPTIONAL;  ct[z++].val = &cap_props;
  /* end */
  ct[z].cap = NULL; ct[z].type = 0; ct[z].ess = 0; ct[z++].val = NULL;


  val = -1;
  font_vf = NULL;

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
  font->get_metric1     = vf_get_metric1;
  font->get_metric2     = vf_get_metric2;
  font->get_fontbbx1    = vf_get_fontbbx1;
  font->get_fontbbx2    = vf_get_fontbbx2;
  font->get_bitmap1     = vf_get_bitmap1;
  font->get_bitmap2     = vf_get_bitmap2;
  font->get_outline     = vf_get_outline;
  font->get_font_prop   = vf_get_font_prop;
  font->query_font_type = NULL;
  font->close           = vf_close;


  ALLOC_IF_ERR(font_vf, struct s_font_vf)
    goto NoMemoryError;

  font_vf->vf_id       = -1;
  font_vf->font_name   = NULL; 
  font_vf->font_file   = NULL; 
  font_vf->open_style  = v_default_open_style;
  font_vf->glyph_style = v_default_glyph_style;
  font_vf->point_size  = v_default_point_size;
  font_vf->pixel_size  = v_default_pixel_size;
  font_vf->mag         = 1.0;
  font_vf->dpi_x       = v_default_dpi_x;
  font_vf->dpi_y       = v_default_dpi_y;
  font_vf->props       = NULL;

  if (implicit == 0){
    if (cap_point != NULL)
      font_vf->point_size = atof(vf_sexp_get_cstring(cap_point));
    if (cap_pixel != NULL)
      font_vf->pixel_size = atof(vf_sexp_get_cstring(cap_pixel));
    if (cap_dpi != NULL)
      font_vf->dpi_x = font_vf->dpi_y = atof(vf_sexp_get_cstring(cap_dpi));
    if (cap_mag != NULL)
      font_vf->mag = atof(vf_sexp_get_cstring(cap_mag));
    if (cap_props != NULL)
      font_vf->props = cap_props;
  }

  if ((font_vf->font_file = vf_strdup(font_file)) == NULL)
    goto NoMemoryError;
  if ((font_vf->font_name = vf_strdup(font_name)) == NULL)
    goto NoMemoryError;

  if ((font_vf->vf_id = vf_vf_open(font, font_vf, implicit)) < 0)
    goto Error;

  font->private = font_vf;

  val = 0;
  goto End;


NoMemoryError:
  vf_error = VF_ERR_NO_MEMORY;
Error:
  if (font_vf != NULL){
    vf_free(font_vf->font_name);
    vf_free(font_vf->font_file);
    vf_sexp_free1(&font_vf->props);
  }
  vf_free(font_vf); 
End:
  if (implicit == 0){
    vf_sexp_free3(&cap_ffile, &cap_point, &cap_pixel);
    vf_sexp_free2(&cap_mag, &cap_dpi);
  }

  return val;
}


Private int
vf_close(VF_FONT font)
{
  FONT_VF  font_vf;

  font_vf = (FONT_VF)font->private;
  if (font_vf != NULL){
    vf_vf_close(font_vf->vf_id);
    vf_free(font_vf->font_name);
    vf_free(font_vf->font_file);
    vf_sexp_free1(&font_vf->props);
    vf_free(font_vf);
  }

  return 0; 
}


Private int
vf_get_metric1(VF_FONT font, long code_point, VF_METRIC1 metric,
	       double mag_x, double mag_y)
{
  FONT_VF   font_vf;
  double    mx, my, ps, design_size;

  if (metric == NULL){
    fprintf(stderr, "VFlib internal error: in vf_get_metric1()\n");
    abort();
  }
  if ((font_vf = (FONT_VF)font->private) == NULL){
    fprintf(stderr, "VFlib internal error: in vf_get_metric1()\n");
    abort();
  }

  if (vf_vf_get_metric(font_vf->vf_id, code_point, metric, &design_size) < 0)
    return -1;

  if ((ps = font->point_size) < 0)
    if ((ps = font_vf->point_size) < 0)
      ps = -1;

  if ((ps < 0) || (design_size < 1.0e-3)){
    mx = mag_x * font->mag_x * font_vf->mag;
    my = mag_y * font->mag_y * font_vf->mag;
  } else {
    mx = mag_x * font->mag_x * font_vf->mag * (ps/design_size);
    my = mag_y * font->mag_y * font_vf->mag * (ps/design_size);
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
vf_get_fontbbx1(VF_FONT font, double mag_x, double mag_y,
		double *w_p, double *h_p, double *xoff_p, double *yoff_p)
{
  FONT_VF   font_vf;
  VF        vf; 
  double    mx, my, ps, design_size;

  if ((font_vf = (FONT_VF)font->private) == NULL){
    fprintf(stderr, "VFlib internal error: in vf_get_fontbbx1()\n");
    abort();
  }

  if (vf_vf_get_metric(font_vf->vf_id, -1, NULL, &design_size) < 0)
    return -1;

  if ((ps = font->point_size) < 0)
    if ((ps = font_vf->point_size) < 0)
      ps = -1;

  if ((ps < 0) || (design_size < 1.0e-3)){
    mx = mag_x * font->mag_x * font_vf->mag;
    my = mag_y * font->mag_y * font_vf->mag;
  } else {
    mx = mag_x * font->mag_x * font_vf->mag * (ps/design_size);
    my = mag_y * font->mag_y * font_vf->mag * (ps/design_size);
  }

  vf = vf_vf_get_vf(font_vf->vf_id);

  *w_p = vf->tfm->font_bbx_w * mx;
  *h_p = vf->tfm->font_bbx_h * my;
  *xoff_p = vf->tfm->font_bbx_xoff * mx;
  *yoff_p = vf->tfm->font_bbx_yoff * my;

  return 0;
}


Private VF_BITMAP
vf_get_bitmap1(VF_FONT font, long code_point,
	       double mag_x, double mag_y)
{
  FONT_VF    font_vf;
  double     ps, mx, my, design_size;

  if ((font_vf = (FONT_VF)font->private) == NULL){
    fprintf(stderr, "VFlib internal error in vf_get_bitmap1()\n");
    abort();
  }

  design_size = vf_vf_get_design_size(font_vf->vf_id);

  if ((ps = font->point_size) < 0)
    if ((ps = font_vf->point_size) < 0)
      ps = -1;

  if ((ps < 0) || (design_size < 1.0e-3)){
    /* Note: font->mag_x and font_vf->mag are already used to select 
       scaled glyph, e.g., cmr10.360gf for 300dpi with mag 1.2. */
    mx = mag_x;
    my = mag_y;
  } else {
    mx = mag_x * (ps/design_size);
    my = mag_y * (ps/design_size);
  }

  return vf_vf_get_bitmap(font_vf->vf_id, font->mode, code_point, mx, my, 
			  font_vf->open_style, font_vf->glyph_style);
}


Private int
vf_get_metric2(VF_FONT font, long code_point, VF_METRIC2 metric, 
	       double mag_x, double mag_y)
{
  VF_BITMAP  bm;

  if (metric == NULL){
    fprintf(stderr, "VFlib internal error: in vf_get_metric2()\n");
    abort();
  }

  bm = vf_get_bitmap2(font, code_point, mag_x, mag_y);
  if (bm == NULL)
    return -1;

  metric->bbx_width  = bm->bbx_width;
  metric->bbx_height = bm->bbx_height;
  metric->off_x      = bm->off_x;
  metric->off_y      = bm->off_y;
  metric->mv_x       = bm->mv_x;
  metric->mv_y       = bm->mv_y;

  VF_FreeBitmap(bm); 

  return 0;
}

Private int
vf_get_fontbbx2(VF_FONT font, double mag_x, double mag_y,
		int *w_p, int *h_p, int *xoff_p, int *yoff_p)
{
  FONT_VF   font_vf;
  VF        vf; 
  double    mx, my;

  if ((font_vf = (FONT_VF)font->private) == NULL){
    fprintf(stderr, "VFlib internal error: in vf_get_fontbbx2()\n");
    abort();
  }

  vf = vf_vf_get_vf(font_vf->vf_id);
  calc_mag_2(font, font_vf, mag_x, mag_y, &mx, &my);

  *w_p    = toint(vf->tfm->font_bbx_w * mx);
  *h_p    = toint(vf->tfm->font_bbx_h * my);
  *xoff_p = toint(vf->tfm->font_bbx_xoff * mx);
  *yoff_p = toint(vf->tfm->font_bbx_yoff * my);

  return 0;
}


Private VF_BITMAP
vf_get_bitmap2(VF_FONT font, long code_point,
	       double mag_x, double mag_y)
{
  FONT_VF  font_vf;
  double   mx, my;

  if ((font_vf = (FONT_VF)font->private) == NULL){
    fprintf(stderr, "VFlib internal error in vf_get_bitmap2()\n");
    abort();
  }

  calc_mag_2(font, font_vf, mag_x, mag_y, &mx, &my);

  return vf_vf_get_bitmap(font_vf->vf_id, font->mode, code_point, mx, my, 
			  font_vf->open_style, font_vf->glyph_style);
}


Private void
calc_mag_2(VF_FONT font, FONT_VF font_vf, double mag_x, double mag_y, 
	   double *mx_p, double *my_p)
{
  double  design_size, dpi_x, dpi_y, ps;

  design_size = vf_vf_get_design_size(font_vf->vf_id);

  if (((dpi_x = font->dpi_x) < 0) || ((dpi_y = font->dpi_y) < 0)){
    if (((dpi_x = font_vf->dpi_x) < 0) || ((dpi_y = font_vf->dpi_y) < 0)){
      dpi_x = vf_tex_default_dpi();
      dpi_y = vf_tex_default_dpi();
    }
  }

  if ((ps = font->point_size) < 0)
    if ((ps = font_vf->point_size) < 0)
      ps = -1;

  if (ps < 0){
    /* Note: font->mag_x and font_vf->mag are already used to select 
       scaled glyph, e.g., cmr10.360gf for 300dpi with mag 1.2. */
    *mx_p = mag_x;
    *my_p = mag_y;
  } else {
    *mx_p = mag_x * (ps*72.27)/(design_size*dpi_x);
    *my_p = mag_y * (ps*72.27)/(design_size*dpi_y);
  }
}


Private VF_OUTLINE
vf_get_outline(VF_FONT font, long code_point,
	       double mag_x, double mag_y)
{
  vf_error = VF_ERR_NOT_SUPPORTED_OP;

  return NULL;
}


Private char*
vf_get_font_prop(VF_FONT font, char *prop_name)
{
  FONT_VF  font_vf;
  double   dpi_x, dpi_y, ps, design_size;
  char     str[512];
  SEXP     v;

  if ((font_vf = (FONT_VF)font->private) == NULL){
    fprintf(stderr, "VFlib internal error: vf_get_font_prop()\n");
    abort();
  }

  if ((v = vf_sexp_assoc(prop_name, font_vf->props)) != NULL){
    return vf_strdup(vf_sexp_get_cstring(vf_sexp_cadr(v)));
  } else if ((v = vf_sexp_assoc(prop_name, default_properties)) != NULL){
    return vf_strdup(vf_sexp_get_cstring(vf_sexp_cadr(v)));
  } else {

    if (((dpi_x = font->dpi_x)<=0) || ((dpi_y = font->dpi_y)<=0)){
      if (((dpi_x = font_vf->dpi_x)<=0) || ((dpi_y = font_vf->dpi_y)<=0)){
	dpi_x = vf_tex_default_dpi();
	dpi_y = vf_tex_default_dpi();
      }
    }
    design_size = vf_vf_get_design_size(font_vf->vf_id);

    if (font->mode == 1){
      if ((ps = font->point_size) < 0)
	if ((ps = font_vf->point_size) < 0)
	  ps = design_size;
      ps = ps * font->mag_y * font_vf->mag;
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
	  if ((ps = font_vf->pixel_size) < 0){
	    sprintf(str, "%d", toint(design_size * 10.0 
				     * font->mag_y * font_vf->mag));
	    return vf_strdup(str);
	  }
	}
	ps = ps * font->mag_y * font_vf->mag;
	sprintf(str, "%d", toint(ps * 10.0 * 72.27 / dpi_y));
	return vf_strdup(str);
      } else if (strcmp(prop_name, "PIXEL_SIZE") == 0){
	if ((ps = font->pixel_size) < 0)
	  if ((ps = font_vf->pixel_size) < 0)
	    ps = design_size * dpi_y / 72.27;
	ps = ps * font->mag_y * font_vf->mag;
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



#include "vf.c"


Private int
vf_debug(char type)
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
