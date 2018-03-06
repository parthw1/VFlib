/*
 * drv_pk.c - A font driver for TeX PK format fonts.
 *
 * 28 Sep 1996  First version.
 * 14 Dec 1996  for VFlib 3.1
 * 26 Feb 1997  Added 'query_font_type'.
 *  4 Aug 1997  VFlib 3.3  Changed API.
 * 30 Jan 1998  VFlib 3.4  Changed API.
 * 21 Apr 1998  Debugged get_font_prop().
 * 24 Nov 1998  Added get_fontbbx1() and get_fontbbx2().
 *  9 Dec 1998  Fixed bugs in get_fontbbx1() and get_fontbbx2().  (^o^;)
 * 16 Sep 1999  Changed not to use TFM files.
 * 18 Oct 2001  Fixed memory leak.
 */
/*
 * Copyright (C) 1996-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */

/*

   (Suppose that default resolution is 300 dpi)

           ARGS FOR FONT OPEN                 BEHAVIOR of the DRIVER
 Case#  font_name device_dpi  mag       font_dpi dev_dpi bitmap_mag TFM_mag 
 ----------------------------------------------------------------------------
  1.    cmr10         -1      1            300    300        1        1 
  2.    cmr10         -1      1.2          360    300        1        1.2
  3.    cmr10        400      1            400    400        1        1
  4.    cmr10        400      1.2          480    400        1        1.2
  5.    cmr10.pk      -1      1            300    300        1        1
  6.    cmr10.pk      -1      1.2          360    300        1        1.2
  7.    cmr10.pk     400      1            400    400        1        1
  8.    cmr10.pk     400      1.2          480    400        1        1.2
  9.    cmr10.400pk   -1      1            400    400        1        1
 10.    cmr10.400pk   -1      1.2          480    400        1        1.2
 11.    cmr10.400pk  400      1            400    400        1        1
 12.    cmr10.400pk  400      1.2          400    400        1.2      1.2
 13.    cmr10.300pk  360      1            300    360        1.2      1
 14.    cmr10.300pk  360      1.2          300    360        1.44     1.2

 Font file is selected from the rules of the table shown above.
   a. A font file of 'font_dpi' dpi font and opened.
   b. Bitmaps of a font is scaled by 'bitmap_mag'.
   c. Font metrics are multiplied by 'TFM_mag'.

 Memo: 
  * Case 5:  We want a PK font file "cmr10" default device resolution.
        Thus, the bitmaps and metrics need not be scaled.

  * Case 6:  We want a PK font file "cmr10" default device resolution
        with maginication factor 1.2. Since default device resolution
	is 300 and magnification factor is 1.2, the font file we want
	is "cmr10.360pk". Since device resolution is 300, metrics
	must be scaled by 1.2.

  * Case 9:  This case requires a font "cmr10.400pk" and target device 
       is not considered. Thus bitmaps and metrics are not scaled.

  * Case 10: This case requires a font cmr10 for 400 dpi scaled by 1.2.
        Thus, "cmr10.480pk" is used for this request. 
        Since it is scaled by 1.2, font metrics are scaled by 1.2 but
        bitmaps in a font is not magnified.

  * Case 13: We want to use a PK font file "cmr10.300pk" for 360 dpi device. 
        Thus, the bitmaps of the font must be scaled by (360/300) = 1.2.
	Since the metrics in a TFM file are independent from device 
	resolution, metrics need not be scaled.

  * Case 14: We want to use a PK font file "cmr10.300pk" for 360 dpi device 
        and magnify it by 1.2. Thus, the bitmaps of the font must be scaled 
	by (1.2 x (360/300)) = 1.44.
	Since the metrics in a TFM file are independent from device 
	resolution, TFM metrics must be scaled by 1.2, which is a 
	magnification factor.
*/


#include  "config.h"
#include  "with.h"
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
#include  "str.h"
#include  "path.h"
#include  "fsearch.h"
#include  "sexp.h"
#include  "texfonts.h"
#include  "pk.h"


struct s_font_pk {
  int      pk_id;
  char     *font_name;
  char     *font_file;
  double   point_size;
  double   pixel_size;
  double   mag;
  double   dpi_x, dpi_y;
  SEXP     props;
  double   extra_mag;
};
typedef struct s_font_pk  *FONT_PK;


Private SEXP_LIST    default_fontdirs;
Private SEXP_LIST    default_extensions;
Private SEXP_STRING  default_point_size;
Private double       v_default_point_size;
Private SEXP_STRING  default_pixel_size;
Private double       v_default_pixel_size;
Private SEXP_STRING  default_dpi;
Private double       v_default_dpi_x, v_default_dpi_y;
Private SEXP_STRING  default_make_glyph;
Private int          v_default_make_glyph = 0;
Private SEXP_ALIST   default_properties;
Private SEXP_ALIST   default_variables;
Private SEXP_STRING  default_debug_mode;


Private int         pk_create(VF_FONT,char*,char*,int,SEXP);
Private int         pk_close(VF_FONT);
Private int         pk_get_metric1(VF_FONT,long,VF_METRIC1,double,double);
Private int         pk_get_metric2(VF_FONT,long,VF_METRIC2,double,double);
Private int         pk_get_fontbbx1(VF_FONT,double,double,
				    double*,double*,double*,double*);
Private int         pk_get_fontbbx2(VF_FONT,double,double, 
				    int*,int*,int*,int*);
Private VF_BITMAP   pk_get_bitmap1(VF_FONT,long,double,double);
Private VF_BITMAP   pk_get_bitmap2(VF_FONT,long,double,double);
Private VF_OUTLINE  pk_get_outline(VF_FONT,long,double,double);
Private char*       pk_get_font_prop(VF_FONT,char*);


Private int         PK_Init(void);
Private int         PK_Open(FONT_PK font_pk, VF_FONT font, int implicit);
Private void        PK_Close(int pk_id);
Private VF_BITMAP   PK_GetBitmap(int pk_id, long code_point);
Private int         PK_GetMetric(int pk_id, long code_point, VF_METRIC1 me, 
				 double *ret_dpi_x, double *ret_dpi_y, 
				 double *ret_design_size);
Private void        PK_GetFontBBX(int pk_id, int *bbx_w_p, int *bbx_h_p,
				  int *bbx_xoff_p, int *bbx_yoff_p);
Private int         debug_on(char type);




Glocal int
VF_Init_Driver_PK(void)
{
  int  z;
  struct s_capability_table  ct[20];

  z = 0;
  /* VF_CAPE_FONT_DIRECTORIES */
  ct[z].cap = VF_CAPE_FONT_DIRECTORIES;     ct[z].type = CAPABILITY_LIST;
  ct[z].ess = CAPABILITY_OPTIONAL;          ct[z++].val = &default_fontdirs;
  /* VF_CAPE_EXTENSIONS */
  ct[z].cap = VF_CAPE_EXTENSIONS;           ct[z].type = CAPABILITY_LIST;
  ct[z].ess = CAPABILITY_OPTIONAL;          ct[z++].val = &default_extensions;
  /* VF_CAPE_POINT_SIZE */
  ct[z].cap = VF_CAPE_POINT_SIZE;           ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;          ct[z++].val = &default_point_size;
  /* VF_CAPE_PIXEL_SIZE */
  ct[z].cap = VF_CAPE_PIXEL_SIZE;           ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;          ct[z++].val = &default_pixel_size;
  /* VF_CAPE_DPI */
  ct[z].cap = VF_CAPE_DPI;                  ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;          ct[z++].val = &default_dpi;
  /* VF_CAPE_MAKE_MISSING_GLYPH */
  ct[z].cap = VF_CAPE_MAKE_MISSING_GLYPH;   ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;          ct[z++].val = &default_make_glyph;
  /* VF_CAPE_PROPERTIES */
  ct[z].cap = VF_CAPE_PROPERTIES;           ct[z].type = CAPABILITY_ALIST;
  ct[z].ess = CAPABILITY_OPTIONAL;          ct[z++].val = &default_properties;
  /* VF_CAPE_VARIABLE_VALUES */
  ct[z].cap = VF_CAPE_VARIABLE_VALUES;      ct[z].type = CAPABILITY_ALIST;
  ct[z].ess = CAPABILITY_OPTIONAL;          ct[z++].val = &default_variables;
  /* VF_CAPE_DEBUG */
  ct[z].cap = VF_CAPE_DEBUG;                ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;          ct[z++].val = &default_debug_mode;
  /* end */
  ct[z].cap = NULL; ct[z].type = 0; ct[z].ess = 0; ct[z++].val = NULL;

  if (vf_tex_init() < 0)
    return -1;

  if (vf_cap_GetParsedClassDefault(FONTCLASS_NAME_PK, ct, 
				   vf_tex_default_variables, NULL) 
      == VFLIBCAP_PARSED_ERROR)
    return -1;

  if (default_extensions == NULL)
    default_extensions = vf_sexp_cstring2list(DEFAULT_EXTENSIONS_PK);

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

  v_default_make_glyph = 0;
  if (default_make_glyph != NULL){
    v_default_make_glyph
      = vf_parse_bool(vf_sexp_get_cstring(default_make_glyph));
  }

  if (PK_Init() < 0)
    return -1;

  if ((v_default_dpi_x < 0) || (v_default_dpi_x < 0)){
    v_default_dpi_x = vf_tex_default_dpi();
    v_default_dpi_y = vf_tex_default_dpi();
  }
  
  VF_InstallFontDriver(FONTCLASS_NAME_PK, (DRIVER_FUNC_TYPE)pk_create);

  return 0;
}


Private int
pk_create(VF_FONT font, char *font_class, 
	  char *font_name, int implicit, SEXP entry)
{
  FONT_PK   font_pk;
  SEXP      cap_ffile, cap_point, cap_pixel;
  SEXP      cap_dpi, cap_mag, cap_props;
  char      *font_file;
  int       val, pk_id;
  struct s_capability_table  ct[10];
  int z;

  z = 0;
  /* VF_CAPE_FONT_CLASS */
  ct[z].cap = VF_CAPE_FONT_CLASS;    ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_ESSENTIAL;  ct[z++].val = NULL;
  /* VF_CAPE_FONT_FILE */
  ct[z].cap = VF_CAPE_FONT_FILE;     ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;   ct[z++].val = &cap_ffile;
  /* VF_CAPE_POINT_SIZE */
  ct[z].cap = VF_CAPE_POINT_SIZE;    ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;   ct[z++].val = &cap_point;
  /* VF_CAPE_PIXEL_SIZE */
  ct[z].cap = VF_CAPE_PIXEL_SIZE;    ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;   ct[z++].val = &cap_pixel;
  /* VF_CAPE_DPI */
  ct[z].cap = VF_CAPE_DPI;           ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;   ct[z++].val = &cap_dpi;
  /* VF_CAPE_MAG */
  ct[z].cap = VF_CAPE_MAG;           ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;   ct[z++].val = &cap_mag;
  /* VF_CAPE_PROPERTIES */
  ct[z].cap = VF_CAPE_PROPERTIES;    ct[z].type = CAPABILITY_ALIST;
  ct[z].ess = CAPABILITY_OPTIONAL;   ct[z++].val = &cap_props;
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
  font->get_metric1     = pk_get_metric1;
  font->get_metric2     = pk_get_metric2;
  font->get_fontbbx1    = pk_get_fontbbx1;
  font->get_fontbbx2    = pk_get_fontbbx2;
  font->get_bitmap1     = pk_get_bitmap1;
  font->get_bitmap2     = pk_get_bitmap2;
  font->get_outline     = pk_get_outline;
  font->get_font_prop   = pk_get_font_prop;
  font->query_font_type = NULL;
  font->close           = pk_close;

  val = -1;
  font_pk = NULL;

  ALLOC_IF_ERR(font_pk, struct s_font_pk)
    goto NoMemoryError;

  font_pk->pk_id      = -1;
  font_pk->font_name  = NULL; 
  font_pk->font_file  = NULL; 
  font_pk->point_size = v_default_point_size;
  font_pk->pixel_size = v_default_pixel_size;
  font_pk->dpi_x      = v_default_dpi_x;
  font_pk->dpi_y      = v_default_dpi_y;
  font_pk->mag        = 1.0;
  font_pk->props      = NULL;
  font_pk->extra_mag  = 1.0;

  if (implicit == 0){
    if (cap_point != NULL)
      font_pk->point_size = atof(vf_sexp_get_cstring(cap_point));
    if (cap_pixel != NULL)
      font_pk->pixel_size = atof(vf_sexp_get_cstring(cap_pixel));
    if (cap_dpi != NULL)
      font_pk->dpi_x = font_pk->dpi_y = atof(vf_sexp_get_cstring(cap_dpi));
    if (cap_mag != NULL)
      font_pk->mag = atof(vf_sexp_get_cstring(cap_mag));
    if (cap_props != NULL)
      font_pk->props = cap_props;
  }

  if ((font_pk->font_file = vf_strdup(font_file)) == NULL)
    goto NoMemoryError;
  if ((font_pk->font_name = vf_strdup(font_name)) == NULL)
    goto NoMemoryError;

  if ((pk_id = PK_Open(font_pk, font, implicit)) < 0)
    goto Error;

  font_pk->pk_id = pk_id;
  font->private = font_pk;

  val = 0;
  goto End;


NoMemoryError:
  vf_error = VF_ERR_NO_MEMORY;
Error:
  if (font_pk != NULL){
    vf_free(font_pk->font_name);
    vf_free(font_pk->font_file);
  }
  if (implicit == 0)
    vf_sexp_free1(&cap_props);
  vf_free(font_pk); 

End:
  if (implicit == 0){
    vf_sexp_free3(&cap_ffile, &cap_point, &cap_pixel);
    vf_sexp_free2(&cap_dpi, &cap_mag);
  }

  return val;
}


Private int
pk_close(VF_FONT font)
{
  FONT_PK  font_pk;

  font_pk = (FONT_PK)font->private;
  if (font_pk != NULL){
    PK_Close(font_pk->pk_id);
    vf_sexp_free1(&font_pk->props);
    vf_free(font_pk->font_name);
    vf_free(font_pk->font_file);
  }
  vf_free(font_pk);

  return 0; 
}


Private int
pk_get_metric1(VF_FONT font, long code_point, VF_METRIC1 metric,
	       double mag_x, double mag_y)
{
  FONT_PK   font_pk;
  double    mx, my, ps, design_size;

  if (metric == NULL){
    fprintf(stderr, "VFlib internal error: in pk_get_metric1()\n");
    abort();
  }
  if ((font_pk = (FONT_PK)font->private) == NULL){
    fprintf(stderr, "VFlib internal error: in pk_get_metric1()\n");
    abort();
  }

  if (PK_GetMetric(font_pk->pk_id, code_point, metric, 
		   NULL, NULL, &design_size) < 0)
    return -1;

  if ((ps = font->point_size) < 0)
    if ((ps = font_pk->point_size) < 0)
      ps = -1;

  if ((ps < 0) || (design_size < 1.0e-3)){
    mx = mag_x * font->mag_x * font_pk->mag;
    my = mag_y * font->mag_y * font_pk->mag;
  } else {
    mx = mag_x * font->mag_x * font_pk->mag * (ps/design_size);
    my = mag_y * font->mag_y * font_pk->mag * (ps/design_size);
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
pk_get_fontbbx1(VF_FONT font, double mag_x, double mag_y,
		double *w_p, double *h_p, double *xoff_p, double *yoff_p)
{
  FONT_PK   font_pk;
  double    mx, my, ps, design_size, dpi_x, dpi_y;
  int       w, h, xoff, yoff;

  if ((font_pk = (FONT_PK)font->private) == NULL){
    fprintf(stderr, "VFlib internal error: in pk_get_fontbbx1()\n");
    abort();
  }

  if (PK_GetMetric(font_pk->pk_id, -1, NULL, 
		   &dpi_x, &dpi_y, &design_size) < 0)
    return -1;

  if ((ps = font->point_size) < 0)
    if ((ps = font_pk->point_size) < 0)
      ps = -1;

  if ((ps < 0) || (design_size < 1.0e-3)){
    mx = mag_x * font->mag_x * font_pk->mag;
    my = mag_y * font->mag_y * font_pk->mag;
  } else {
    mx = mag_x * font->mag_x * font_pk->mag * (ps/design_size);
    my = mag_y * font->mag_y * font_pk->mag * (ps/design_size);
  }

  PK_GetFontBBX(font_pk->pk_id, &w, &h, &xoff, &yoff);

#if 0
  printf("*** %d %d %d %d   %.3f %.3f   %.3f %.3f\n",
	 w, h, xoff, yoff, mx, my, dpi_x, dpi_y);
#endif

  *w_p = mx * w * 72.27 / dpi_x;
  *h_p = my * h * 72.27 / dpi_y;
  *xoff_p = mx * xoff * 72.27 / dpi_x;
  *yoff_p = my * yoff * 72.27 / dpi_y;

  return 0;
}

Private VF_BITMAP
pk_get_bitmap1(VF_FONT font, long code_point,
	       double mag_x, double mag_y)
{
  VF_BITMAP  bm;
  FONT_PK    font_pk;
  double     mx, my, ps, design_size;

  if ((font_pk = (FONT_PK)font->private) == NULL){
    fprintf(stderr, "VFlib internal error in pk_get_bitmap1()\n");
    abort();
  }

  if ((bm = PK_GetBitmap(font_pk->pk_id, code_point)) == NULL)
    return NULL;
  /* 'bm' SHOULD NOT BE RELEASED. */

  if (PK_GetMetric(font_pk->pk_id, code_point, NULL, 
		   NULL, NULL, &design_size) < 0)
    return NULL;

  if ((ps = font->point_size) < 0)
    if ((ps = font_pk->point_size) < 0)
      ps = -1;

  if ((ps < 0) || (design_size < 1.0e-3)){
    /* Note: font->mag_x and font_pk->mag are already used to select 
       scaled glyph, e.g., cmr10.360pk for 300dpi with mag 1.2. */
    mx = font_pk->extra_mag * mag_x;
    my = font_pk->extra_mag * mag_y; 
  } else {
#if 0
    m = font->mag_y * font_pk->mag;
    mx = font_pk->extra_mag * mag_x * m * (ps/design_size);
    my = font_pk->extra_mag * mag_y * m * (ps/design_size);
#endif
    mx = font_pk->extra_mag * mag_x * (ps/design_size);
    my = font_pk->extra_mag * mag_y * (ps/design_size);
  }

  if (debug_on('m'))
    printf("VFlib PK: get_bitmap1: bitmap mag: %.4f %.4f\n", mx, my);

  return VF_MakeScaledBitmap(bm, mx, my);
}

Private VF_OUTLINE
pk_get_outline(VF_FONT font, long code_point,
	       double mag_x, double mag_y)
{
  VF_BITMAP   bm;
  VF_OUTLINE  ol;
  FONT_PK     font_pk;
  int         bbx_w, bbx_h;
  double      dpi_x, dpi_y, design_size;

  if ((font_pk = (FONT_PK)font->private) == NULL){
    fprintf(stderr, "VFlib internal error in pk_get_outline()\n");
    abort();
  }

  if ((bm = pk_get_bitmap1(font, code_point, mag_x, mag_y)) == NULL)
    return NULL;
  
  if (PK_GetMetric(font_pk->pk_id, code_point, NULL, 
		   &dpi_x, &dpi_y, &design_size) < 0){
    VF_FreeBitmap(bm);
    return NULL;
  }

  PK_GetFontBBX(font_pk->pk_id, &bbx_w, &bbx_h, NULL, NULL);
  ol = vf_bitmap_to_outline(bm, bbx_w, bbx_h,
			    dpi_x, dpi_y, design_size, 1.0, 1.0);
  VF_FreeBitmap(bm);

  return ol;
}


Private int
pk_get_metric2(VF_FONT font, long code_point, VF_METRIC2 metric, 
	       double mag_x, double mag_y)
{
  VF_BITMAP  bm;
  FONT_PK    font_pk;
  double     mx, my, dpi_x, dpi_y, ps, design_size;

  if (   (metric == NULL)
      || ((font_pk = (FONT_PK)font->private) == NULL) ){
    fprintf(stderr, "VFlib internal error: in pk_get_metric2()\n");
    abort();
  }

  if ((bm = PK_GetBitmap(font_pk->pk_id, code_point)) == NULL)
    return -1;
  /* 'bm' SHOULD NOT BE RELEASED. */

  if (PK_GetMetric(font_pk->pk_id, code_point, NULL,
		   &dpi_x, &dpi_y, &design_size) < 0)
    return -1;

  if ((ps = font->pixel_size) < 0)
    if ((ps = font_pk->pixel_size) < 0)
      ps = -1;

  if ((ps < 0) || (design_size < 1.0e-3)){
    mx = font->mag_x * font_pk->mag * mag_x;
    my = font->mag_y * font_pk->mag * mag_y;
  } else {
    mx = font->mag_x * font_pk->mag * mag_x * (ps*72.27)/(design_size*dpi_x);
    my = font->mag_y * font_pk->mag * mag_y * (ps*72.27)/(design_size*dpi_y);
  }

  metric->bbx_width  = toint(mx * bm->bbx_width);
  metric->bbx_height = toint(my * bm->bbx_height);
  metric->off_x      = toint(mx * bm->off_x);
  metric->off_y      = toint(my * bm->off_y);
  metric->mv_x       = toint(mx * bm->mv_x);
  metric->mv_y       = toint(my * bm->mv_y);

  return 0;
}

Private int
pk_get_fontbbx2(VF_FONT font, double mag_x, double mag_y,
		int *w_p, int *h_p, int *xoff_p, int *yoff_p)
{
  FONT_PK   font_pk;
  double    mx, my, dpi_x, dpi_y, ps, design_size;

  if ((font_pk = (FONT_PK)font->private) == NULL){
    fprintf(stderr, "VFlib internal error: in pk_get_fontbbx2()\n");
    abort();
  }

  if (PK_GetMetric(font_pk->pk_id, -1, NULL,
		   &dpi_x, &dpi_y, &design_size) < 0)
    return -1;

  if ((ps = font->pixel_size) < 0)
    if ((ps = font_pk->pixel_size) < 0)
      ps = -1;

  if ((ps < 0) || (design_size < 1.0e-3)){
    mx = font->mag_x * font_pk->mag * mag_x;
    my = font->mag_y * font_pk->mag * mag_y;
  } else {
    mx = font->mag_x * font_pk->mag * mag_x * (ps*72.27)/(design_size*dpi_x);
    my = font->mag_y * font_pk->mag * mag_y * (ps*72.27)/(design_size*dpi_y);
  }

  PK_GetFontBBX(font_pk->pk_id, w_p, h_p, xoff_p, yoff_p);

  *w_p = mx * (*w_p);
  *h_p = mx * (*h_p);
  *xoff_p = mx * (*xoff_p);
  *yoff_p = mx * (*yoff_p);

  return 0;
}


Private VF_BITMAP
pk_get_bitmap2(VF_FONT font, long code_point,
	       double mag_x, double mag_y)
{
  VF_BITMAP  bm;
  FONT_PK    font_pk;
  double     mx, my, dpi_x, dpi_y, ps, design_size;

  if ((font_pk = (FONT_PK)font->private) == NULL){
    fprintf(stderr, "VFlib internal error in pk_get_bitmap2()\n");
    abort();
  }

  if ((bm = PK_GetBitmap(font_pk->pk_id, code_point)) == NULL)
    return NULL;
  /* 'bm' SHOULD NOT BE RELEASED. */

  if (PK_GetMetric(font_pk->pk_id, code_point, NULL, 
		   &dpi_x, &dpi_y, &design_size) < 0)
    return NULL;

  if ((ps = font->pixel_size) < 0)
    if ((ps = font_pk->pixel_size) < 0)
      ps = -1;

#if 0
  printf("** %.3f %.3f %.3f   %.3f   %.3f %.3f  %d %.3f\n", 
	 mag_x, mag_y, ps, design_size, dpi_x, dpi_y,
	 font->pixel_size, font_pk->pixel_size); 
#endif

  if ((ps < 0) || (design_size < 1.0e-3)){
    /* Note: font_pk->mag_x and font_pk->mag are already used to select 
       scaled glyph, e.g., cmr10.360pk for 300dpi with mag 1.2. */
    mx = font_pk->extra_mag * mag_x;
    my = font_pk->extra_mag * mag_y;
  } else {
#if 0
    m = font->mag_y * font_pk->mag;
    mx = font_pk->extra_mag * mag_x * m * (ps*72.27)/(design_size*dpi_x);
    my = font_pk->extra_mag * mag_y * m * (ps*72.27)/(design_size*dpi_y);
#endif
    mx = font_pk->extra_mag * mag_x 
         * (font->mag_x * font_pk->mag) * (ps*72.27)/(design_size*dpi_x);
    my = font_pk->extra_mag * mag_y 
         * (font->mag_y * font_pk->mag) * (ps*72.27)/(design_size*dpi_y);
  }

  return VF_MakeScaledBitmap(bm, mx, my);
}


Private char*
pk_get_font_prop(VF_FONT font, char *prop_name)
{
  FONT_PK  font_pk;
  double   dpi_x, dpi_y, design_size, ps, m;
  char     str[256];
  SEXP     v;

  if ((font_pk = (FONT_PK)font->private) == NULL){
    fprintf(stderr, "VFlib internal error in pk_get_font_prop()\n");
    abort();
  }

  if ((v = vf_sexp_assoc(prop_name, font_pk->props)) != NULL){
    return vf_strdup(vf_sexp_get_cstring(vf_sexp_cadr(v)));
  } else if ((v = vf_sexp_assoc(prop_name, default_properties)) != NULL){
    return vf_strdup(vf_sexp_get_cstring(vf_sexp_cadr(v)));
  } else {
    if (PK_GetMetric(font_pk->pk_id, -1, NULL, 
		     &dpi_x, &dpi_y, &design_size) < 0){
      return NULL;
    }
    m = font->mag_y * font_pk->mag;
#if 0
    printf("** %.3f %.3f %.3f   %.4f   %d %.3f   %.3f %.3f\n",
	   dpi_x, dpi_y, design_size, m,
	   font->pixel_size, font_pk->pixel_size,
	   font->point_size, font_pk->point_size);
#endif
    if (font->mode == 1){
      if ((ps = font->point_size) < 0)
	if ((ps = font_pk->point_size) < 0)
	  ps = design_size;
      if (strcmp(prop_name, "POINT_SIZE") == 0){
	sprintf(str, "%d", toint(ps * m * 10.0));
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
	  if ((ps = font_pk->pixel_size) < 0){
	    sprintf(str, "%d", toint(design_size * m));
	    return vf_strdup(str);
	  }
	}
	sprintf(str, "%d", toint(ps * 10.0 * (72.27 / dpi_y)));
	return vf_strdup(str);
      } else if (strcmp(prop_name, "PIXEL_SIZE") == 0){
	if ((ps = font->pixel_size) < 0)
	  if ((ps = font_pk->pixel_size) < 0){
	    ps = design_size * dpi_y / 72.27;
	    sprintf(str, "%d", toint(ps));
	    return vf_strdup(str);
	  }
	sprintf(str, "%d", toint(ps * m));
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



#ifndef CACHE_SIZE
#  define CACHE_SIZE  48
#endif
#ifndef HASH_SIZE
#  define HASH_SIZE   11
#endif


struct s_pk {
  int      type;
  char    *font_path;
};
typedef struct s_pk  *PK;

struct s_pk_glyph {
  int         code_min, code_max; 
  VF_BITMAP   bm_table;
  double      ds, hppp, vppp;
  int         font_bbx_w, font_bbx_h;
  int         font_bbx_xoff, font_bbx_yoff;
};
typedef struct s_pk_glyph  *PK_GLYPH;

Private VF_TABLE pk_table       = NULL;
Private VF_CACHE pk_glyph_cache = NULL;

Private void        PK_CacheDisposer(PK_GLYPH go);
Private PK_GLYPH    PK_CacheLoader(VF_CACHE c, char *path, int l);

Private PK          PK_GetPK(int pk_id);
Private void        PK_SetPKGlyph(char *path, PK_GLYPH go);
Private PK_GLYPH    PK_GetPKGlyph(char *path);


Private int
PK_Init(void)
{
  static int init_flag = 0;

  if (init_flag == 0){
    init_flag = 1;
    if ((pk_table = vf_table_create()) == NULL){
      vf_error = VF_ERR_NO_MEMORY;
      return -1;
    }
    pk_glyph_cache
      = vf_cache_create(CACHE_SIZE, HASH_SIZE,
			(void*(*)(VF_CACHE,void*,int))PK_CacheLoader, 
			(void(*)(void*))PK_CacheDisposer);
  }

  return 0;
}

Private int
PK_Open(FONT_PK font_pk, VF_FONT font, int implicit)
{
  PK      pk;
  char    *pk_path, *p;
  int     pk_id, dev_dpi;
  double  font_mag;

  pk = NULL;

  if (vf_dbg_drv_texfonts == 1)
    printf(">> PK Open: %s\n", font_pk->font_file);

  /* Parse font name.  Formats of file names that this routine supports:
   *    "cmr10.300XX" - A "cmr10" font for 300 dpi.
   *    "cmr10.XX"    - A "cmr10" font. Dpi value is default value.
   *    "cmr10"       -   ditto.
   * ("XX" can be any string such as "pk", "pk", and "tfm".)
   */
  p = vf_index(font_pk->font_file, '.');
  if ((p != NULL) && (isdigit((int)*(p+1)))){   /* "cmr10.300pk" */
    dev_dpi = atoi(p+1);
    if (font->dpi_y > 0)
      font_pk->extra_mag = (double)font->dpi_y / (double)dev_dpi;
    else
      font_pk->extra_mag = 1.0;
  } else {                                 /* "cmr10" or "cmr10.pk" */
    if ((dev_dpi = font->dpi_y) < 0)
      dev_dpi = font_pk->dpi_y;
    font_pk->extra_mag = 1.0;
  }

  font_mag = font->mag_y * font_pk->mag;
  pk_path = vf_tex_search_file_glyph(font_pk->font_file, implicit,
				     FSEARCH_FORMAT_TYPE_PK,
				     default_fontdirs, dev_dpi, font_mag,
				     default_extensions);
  if (pk_path == NULL){
    if (vf_dbg_drv_texfonts == 1)
      printf(">> PK Open: PK file not found\n");
    if (v_default_make_glyph == 0)
      return -1;
    if (vf_tex_make_glyph(FSEARCH_FORMAT_TYPE_PK, 
			  font_pk->font_file, dev_dpi, font_mag) < 0)
      return -1;
    pk_path = vf_tex_search_file_glyph(font_pk->font_file, implicit,
				       FSEARCH_FORMAT_TYPE_PK,
				       default_fontdirs, dev_dpi, font_mag,
				       default_extensions);
    if (pk_path == NULL){
      if (vf_dbg_drv_texfonts == 1)
	printf(">> PK Open: PK file not found\n");
      return -1;
    }
  }

  if (debug_on('f'))
    printf("VFlib PK: font:%s, dpi:%d, mag:%f, extra_mag:%f\n   ==> %s\n",
	   font_pk->font_file, dev_dpi, font_mag, font_pk->extra_mag, pk_path);

  ALLOC_IF_ERR(pk, struct s_pk){
    goto NoMemoryError;
  }
  pk->font_path = pk_path;
  if ((pk_id = (pk_table->put)(pk_table, pk, pk->font_path,
			       strlen(pk->font_path)+1)) < 0)
    goto NoMemoryError;

  return pk_id;


NoMemoryError:
  vf_error = VF_ERR_NO_MEMORY;
  if (pk != NULL)
    vf_free(pk->font_path);
  vf_free(pk);
  return -1;
}

Private void
PK_Close(int pk_id)
{
  PK   pk;

  pk = PK_GetPK(pk_id);

  if ((pk_table->unlink_by_id)(pk_table, pk_id) > 0)
    return;

  if (pk != NULL)
    vf_free(pk->font_path);
  vf_free(pk);
  PK_SetPKGlyph(NULL, NULL);
}


Private VF_BITMAP
PK_GetBitmap(int pk_id, long code_point)
     /* MEMO: CALLER MUST *NOT* 'FREE' THE BITMAP RETURNED BY THIS FUNC. */
{
  PK_GLYPH  go;
  PK        pk;

  pk = PK_GetPK(pk_id);
  if ((go = PK_GetPKGlyph(pk->font_path)) == NULL)
    return NULL;

  if ((go->code_min <= code_point) && (code_point <= go->code_max))
    return &go->bm_table[code_point - go->code_min];

  return NULL;
}

Private int
PK_GetMetric(int pk_id, long code_point, VF_METRIC1 me, 
	     double *ret_dpi_x, double *ret_dpi_y, double *ret_design_size)
{
  VF_BITMAP  bm;
  PK_GLYPH   go;
  PK         pk;

  pk = PK_GetPK(pk_id);
  if ((go = PK_GetPKGlyph(pk->font_path)) == NULL)
    return -1;

  if (code_point < 0)
    code_point = go->code_min;
  if ((code_point < go->code_min) || (go->code_max < code_point)){
    vf_error = VF_ERR_ILL_CODE_POINT;
    return -1;
  }

  if ((bm = &go->bm_table[code_point - go->code_min]) == NULL){
    vf_error = VF_ERR_ILL_CODE_POINT;
    return -1;
  }

  if (me != NULL){
    me->bbx_width  = bm->bbx_width  / go->hppp;
    me->bbx_height = bm->bbx_height / go->vppp;
    me->off_x      = bm->off_x / go->hppp;
    me->off_y      = bm->off_y / go->vppp;
    me->mv_x       = bm->mv_x  / go->hppp;
    me->mv_y       = bm->mv_y  / go->vppp;
  }

  if (ret_dpi_x != NULL)
    *ret_dpi_x = go->hppp * 72.27;
  if (ret_dpi_y != NULL)
    *ret_dpi_y = go->vppp * 72.27;
  if (ret_design_size != NULL)
    *ret_design_size = go->ds;

  return 0;
}

Private void
PK_GetFontBBX(int pk_id, int *bbx_w_p, int *bbx_h_p, 
	      int *bbx_xoff_p, int *bbx_yoff_p)
{
  PK_GLYPH  go;
  PK        pk;

  pk = PK_GetPK(pk_id);
  if ((go = PK_GetPKGlyph(pk->font_path)) == NULL)
    return;

  if (bbx_w_p != NULL)
    *bbx_w_p = go->font_bbx_w;
  if (bbx_h_p != NULL)
    *bbx_h_p = go->font_bbx_h;
  if (bbx_xoff_p != NULL)
    *bbx_xoff_p = go->font_bbx_xoff;
  if (bbx_yoff_p != NULL)
    *bbx_yoff_p = go->font_bbx_yoff;
}


/* 
 * PK file interface
 */

#include  "pk.c"


Private PK
PK_GetPK(int pk_id)
{
  if (pk_id < 0)
    abort();
  return (pk_table->get_obj_by_id)(pk_table, pk_id);
}


static char     *pk_last_go_path = NULL;
static PK_GLYPH  pk_last_go    = NULL;

Private void
PK_SetPKGlyph(char *path, PK_GLYPH go)
{
  pk_last_go_path = path;
  pk_last_go      = go;
}

Private PK_GLYPH
PK_GetPKGlyph(char *path)
{
  PK_GLYPH   go;

  if (path == NULL){
    pk_last_go_path = NULL;
    pk_last_go      = NULL;
    return NULL;
  }
  if ((pk_last_go_path != NULL)
      && (strcmp(pk_last_go_path, path) == 0)
      && (pk_last_go != NULL) ){
    return pk_last_go;
  }
  
  go = (pk_glyph_cache->get)(pk_glyph_cache, path, strlen(path)+1);
  pk_last_go_path = path;
  pk_last_go      = go;  

  return go;
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
