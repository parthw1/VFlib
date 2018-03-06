/*
 * drv_comic.c - A font driver for font composing like japanese comics.
 * by Hirotsugu Kakugawa
 *
 * 24 Feb 1997  First version.
 * 26 Feb 1997  Added 'query_font_type'.
 *  6 Aug 1997  VFlib 3.3  Changed API.
 *  3 Feb 1998  VFlib 3.4  Changed API.
 *  1 Sep 1998  Added capabilities symbol-font, alpha-numeric-font,
 *              hirakana-font, katakana-font, greek-font, cyrillic-font,
 *              and keisen-font.
 * 24 Nov 1998  Added get_fontbbx1() and get_fontbbx2().
 *  9 Dec 1999  Fixed a bug in debug_on().
 *
 */
/*
 * Copyright (C) 1996-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */

/* MEMO:
 *  "kanji-font" cabability:
 *       a font entry name for Kanji font.
 *  "kana-font"  cabability:
 *       a font entry name for Kana, Alphabets, Numerals font.
 *
 *  This driver assumes character set is JIS X-0208 1990, and 
 *  encoding is JIS style.  A glyph for a character whose code point
 *  is CODE_POINT is determinted by the code point value:
 *    (1) Case (CODE_POINT < 0x3021):
 *         --- A glyph for a character of CODE_POINT of a font 
 *             given by "kana-font" capability.
 *    (2) Otherwise:
 *         --- A glyph for a character of CODE_POINT of a font 
 *             given by "kanji-font" capability.
 *  Note that character set and encoding of font entries given 
 *  by "kanji-font" and "kana-font" capabilties must be JIS X 0208 
 *  and JIS style.
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
#include  "comic.h"

#define  I_KANJI      0
#define  I_KANA       1
#define  I_SYMBOL     2
#define  I_ALNUM      3
#define  I_HIRAKANA   4
#define  I_KATAKANA   5
#define  I_GREEK      6
#define  I_CYRILLIC   7
#define  I_KEISEN     8
#define  I_N          9


struct s_font_comic {
  char     *font_name;
  double   point_size;
  double   pixel_size;
  double   mag;
  double   dpi_x, dpi_y;
  int      sub_fid[I_N];
  SEXP     props;
};
typedef struct s_font_comic  *FONT_COMIC;


Private SEXP_STRING  default_point_size;
Private double       v_default_point_size;
Private SEXP_STRING  default_pixel_size;
Private double       v_default_pixel_size;
Private SEXP_STRING  default_dpi;
Private double       v_default_dpi_x, v_default_dpi_y;
Private SEXP_ALIST   default_properties;
Private SEXP_ALIST   default_variables;
Private SEXP_STRING  default_debug_mode;


Private int         comic_create(VF_FONT,char*,char*,int,SEXP);
Private int         comic_close(VF_FONT);
Private int         comic_get_metric1(VF_FONT,long,VF_METRIC1,
				      double,double);
Private int         comic_get_metric2(VF_FONT,long,VF_METRIC2,
				      double,double);
Private int         comic_get_fontbbx1(VF_FONT,double,double,
				       double*,double*,double*,double*);
Private int         comic_get_fontbbx2(VF_FONT,double,double, 
				       int*,int*,int*,int*);
Private VF_BITMAP   comic_get_bitmap1(VF_FONT,long,double,double);
Private VF_BITMAP   comic_get_bitmap2(VF_FONT,long,double,double);
Private VF_OUTLINE  comic_get_outline(VF_FONT,long,double,double);
Private char*       comic_get_font_prop(VF_FONT,char*);
Private int         comic_query_font_type(VF_FONT,long);
Private void        release_mem(FONT_COMIC);
Private int         font_mapping(FONT_COMIC,long);
Private int         debug_on(char type);


Glocal int
VF_Init_Driver_Comic(void)
{
  struct s_capability_table  ct[10];
  int  z;

  z = 0;
  /* VF_CAPE_POINT_SIZE */
  ct[z].cap = VF_CAPE_POINT_SIZE;      ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;     ct[z++].val = &default_point_size;
  /* VF_CAPE_PIXEL_SIZE */
  ct[z].cap = VF_CAPE_PIXEL_SIZE;      ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;     ct[z++].val = &default_pixel_size;
  /* VF_CAPE_DPI */
  ct[z].cap = VF_CAPE_DPI;             ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;     ct[z++].val = &default_dpi;
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


  if (vf_cap_GetParsedClassDefault(FONTCLASS_NAME_COMIC, ct, NULL, NULL) 
      == VFLIBCAP_PARSED_ERROR)
    return -1;

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

  VF_InstallFontDriver(FONTCLASS_NAME_COMIC, 
		       (DRIVER_FUNC_TYPE)comic_create);

  return 0;
}


Private int
comic_create(VF_FONT font, char *font_class,
	     char *font_name, int implicit, SEXP entry)
{
  FONT_COMIC  font_comic;
  SEXP        cap_ch[I_N];
  SEXP        cap_point, cap_pixel;
  SEXP        cap_mag, cap_dpi, cap_props;
  int         i;
  struct s_capability_table  ct[20];
  int  z;

  z = 0;
  /* VF_CAPE_FONT_CLASS */
  ct[z].cap = VF_CAPE_FONT_CLASS;          ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_ESSENTIAL;        ct[z++].val = NULL;
  /* VF_CAPE_COMIC_KANJI_FONT */
  ct[z].cap = VF_CAPE_COMIC_KANJI_FONT;    ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;         ct[z++].val = &cap_ch[I_KANJI];
  /* VF_CAPE_COMIC_KANA_FONT */
  ct[z].cap = VF_CAPE_COMIC_KANA_FONT;     ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;         ct[z++].val = &cap_ch[I_KANA];
  /* VF_CAPE_COMIC_SYMBOL_FONT */
  ct[z].cap = VF_CAPE_COMIC_SYMBOL_FONT;   ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;         ct[z++].val = &cap_ch[I_SYMBOL];
  /* VF_CAPE_COMIC_ALNUM_FONT */
  ct[z].cap = VF_CAPE_COMIC_ALNUM_FONT;    ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;         ct[z++].val = &cap_ch[I_ALNUM];
  /* VF_CAPE_COMIC_HIRAKANA_FONT */
  ct[z].cap = VF_CAPE_COMIC_HIRAKANA_FONT; ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;         ct[z++].val = &cap_ch[I_HIRAKANA];
  /* VF_CAPE_COMIC_KATAKANA_FONT */
  ct[z].cap = VF_CAPE_COMIC_KATAKANA_FONT; ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;         ct[z++].val = &cap_ch[I_KATAKANA];
  /* VF_CAPE_COMIC_GREEK_FONT */
  ct[z].cap = VF_CAPE_COMIC_GREEK_FONT;    ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;         ct[z++].val = &cap_ch[I_GREEK];
  /* VF_CAPE_COMIC_CYRILLIC_FONT */
  ct[z].cap = VF_CAPE_COMIC_CYRILLIC_FONT; ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;         ct[z++].val = &cap_ch[I_CYRILLIC];
  /* VF_CAPE_COMIC_KEISEN_FONT */
  ct[z].cap = VF_CAPE_COMIC_KEISEN_FONT;   ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;         ct[z++].val = &cap_ch[I_KEISEN];
  /* VF_CAPE_POINT_SIZE */
  ct[z].cap = VF_CAPE_POINT_SIZE;          ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;         ct[z++].val = &cap_point;
  /* VF_CAPE_PIXEL_SIZE */
  ct[z].cap = VF_CAPE_PIXEL_SIZE;          ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;         ct[z++].val = &cap_pixel;
  /* VF_CAPE_DPI */
  ct[z].cap = VF_CAPE_DPI;                 ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;         ct[z++].val = &cap_dpi;
  /* VF_CAPE_MAG */
  ct[z].cap = VF_CAPE_MAG;                 ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;         ct[z++].val = &cap_mag;
  /* VF_CAPE_PROPERTIES */
  ct[z].cap = VF_CAPE_PROPERTIES;          ct[z].type = CAPABILITY_ALIST;
  ct[z].ess = CAPABILITY_OPTIONAL;         ct[z++].val = &cap_props;
  /* end */
  ct[z].cap = NULL; ct[z].type = 0; ct[z].ess = 0; ct[z++].val = NULL;


  /* No support for implicit fonts */
  if (implicit == 1)  
    return -1;

  /* Only supports explicit fonts */
  if (vf_cap_GetParsedFontEntry(entry, font_name, ct, default_variables, NULL) 
      == VFLIBCAP_PARSED_ERROR)
    return -1;

  font->font_type       = -1;  /* Use comic_query_font_type() */
  font->get_metric1     = comic_get_metric1;
  font->get_metric2     = comic_get_metric2;
  font->get_fontbbx1    = comic_get_fontbbx1;
  font->get_fontbbx2    = comic_get_fontbbx2;
  font->get_bitmap1     = comic_get_bitmap1;
  font->get_bitmap2     = comic_get_bitmap2;
  font->get_outline     = comic_get_outline;
  font->get_font_prop   = comic_get_font_prop;
  font->query_font_type = comic_query_font_type;
  font->close           = comic_close;

  ALLOC_IF_ERR(font_comic, struct s_font_comic){
    vf_error = VF_ERR_NO_MEMORY;
    return -1;
  }
  if ((font_comic->font_name = vf_strdup(font_name)) == NULL){
    vf_error = VF_ERR_NO_MEMORY;
    vf_free(font_comic);
    return -1;
  }

  font_comic->point_size   = v_default_point_size;
  font_comic->pixel_size   = v_default_pixel_size;
  font_comic->dpi_x        = v_default_dpi_x;
  font_comic->dpi_y        = v_default_dpi_y;
  font_comic->mag          = 1.0;
  for (i = 0; i < I_N; i++)
    font_comic->sub_fid[i] = -1;
  font_comic->props        = NULL;

  if (implicit == 0){
    if (cap_point != NULL)
      font_comic->point_size = atof(vf_sexp_get_cstring(cap_point));
    if (cap_pixel != NULL)
      font_comic->pixel_size = atof(vf_sexp_get_cstring(cap_pixel));
    if (cap_dpi != NULL){
      font_comic->dpi_x = atof(vf_sexp_get_cstring(cap_dpi));
      font_comic->dpi_y = atof(vf_sexp_get_cstring(cap_dpi));
    }
    if (cap_props != NULL)
      font_comic->props = cap_props;
  }

  if (font->mode == 1){
    for (i = 0; i < I_N; i++){
      if ((cap_ch[i] != NULL)
	  && ((font_comic->sub_fid[i]
	       = VF_OpenFont1(vf_sexp_get_cstring(cap_ch[i]), 
			      font_comic->dpi_x, font_comic->dpi_y,
			      font_comic->point_size, 
			      font->mag_x * font_comic->mag, 
			      font->mag_y * font_comic->mag)) < 0)){
	goto CANT_OPEN;
      }
    }
  } else if (font->mode == 2){
    for (i = 0; i < I_N; i++){
      if ((cap_ch[i] != NULL)
	  && ((font_comic->sub_fid[i]
	       = VF_OpenFont2(vf_sexp_get_cstring(cap_ch[i]), 
			      font_comic->pixel_size, 
			      font->mag_x * font_comic->mag, 
			      font->mag_y * font_comic->mag)) < 0)){
	goto CANT_OPEN;
      }
    }
  } else {
    fprintf(stderr, "VFlib: internal error in comic_create()\n");
    abort();
  }

  font->private = font_comic;
  for (i = 0; i < I_N; i++){
    if (cap_ch[i] != NULL)
      vf_sexp_free(&cap_ch[i]);
  }
  vf_sexp_free4(&cap_point, &cap_pixel, &cap_mag, &cap_dpi);
  return 0;


CANT_OPEN:
  for (i = 0; i < I_N; i++){
    if (cap_ch[i] != NULL)
      vf_sexp_free(&cap_ch[i]);
  }
  vf_sexp_free4(&cap_point, &cap_pixel, &cap_mag, &cap_dpi);
  vf_error = VF_ERR_NO_FONT_ENTRY;
  release_mem(font_comic);
  return -1;
}


Private int
comic_close(VF_FONT font)
{
  release_mem((FONT_COMIC)(font->private));

  return 0; 
}


Private void
release_mem(FONT_COMIC font_comic)
{
  int  i;

  if (font_comic != NULL){
    vf_free(font_comic->font_name);
    vf_sexp_free1(&font_comic->props);
    for (i = 0; i < I_N; i++){
      if (font_comic->sub_fid[i] >= 0)
	VF_CloseFont(font_comic->sub_fid[i]);
    }
    vf_free(font_comic);
  }
}


Private int
comic_get_metric1(VF_FONT font, long code_point, VF_METRIC1 metric,
		  double mag_x, double mag_y)
{
  FONT_COMIC  font_comic;
  int            fid;
  
  if (metric == NULL){
    fprintf(stderr, "VFlib internal error: in comic_get_metric1()\n");
    abort();
  }

  if ((font_comic = (FONT_COMIC)font->private) == NULL){
    fprintf(stderr, "VFlib internal error in comic class.\n");
    abort();
  }
  if ((fid = font_mapping(font_comic, code_point)) < 0)
    return -1;

  VF_GetMetric1(fid, code_point, metric, mag_x, mag_y);

  return 0;
}


Private int
comic_get_fontbbx1(VF_FONT font, double mag_x, double mag_y,
		   double *w_p, double *h_p, double *xoff_p, double *yoff_p)
{
  FONT_COMIC  font_comic;
  int         i;
  double      w, h, xoff, yoff;
  
  if ((font_comic = (FONT_COMIC)font->private) == NULL){
    fprintf(stderr, "VFlib internal error in comic class.\n");
    abort();
  }

  *w_p = *h_p = *xoff_p = *yoff_p = 0;
  w = h = xoff = yoff = 0;
  for (i = 0; i < I_N; i++){
    if (font_comic->sub_fid[i] < 0)
      continue;
    if (VF_GetFontBoundingBox1(font_comic->sub_fid[i],
			       mag_x, mag_y, &w, &h, &xoff, &yoff) < 0)
      continue;
    if (w > *w_p)
      *w_p = w;
    if (h > *h_p)
      *h_p = h;
    if (xoff < *xoff_p)
      *xoff_p = xoff;
    if (yoff > *yoff_p)
      *yoff_p = yoff;
  }

  return 0;
}

Private VF_BITMAP
comic_get_bitmap1(VF_FONT font, long code_point,
		  double mag_x, double mag_y)
{
  FONT_COMIC  font_comic;
  int            fid;

  if ((font_comic = (FONT_COMIC)font->private) == NULL){
    fprintf(stderr, "VFlib internal error in comic class.\n");
    abort();
  }
  if ((fid = font_mapping(font_comic, code_point)) < 0)
    return NULL;

  return VF_GetBitmap1(fid, code_point, mag_x, mag_y);
}


Private VF_OUTLINE
comic_get_outline(VF_FONT font, long code_point,
		  double mag_x, double mag_y)
{
  FONT_COMIC  font_comic;
  int            fid;

  if ((font_comic = (FONT_COMIC)font->private) == NULL){
    fprintf(stderr, "VFlib internal error in comic class.\n");
    abort();
  }
  if ((fid = font_mapping(font_comic, code_point)) < 0)
    return NULL;

  return VF_GetOutline(fid, code_point, mag_x, mag_y);
}


Private int
comic_get_metric2(VF_FONT font, long code_point, VF_METRIC2 metric, 
		  double mag_x, double mag_y)
{
  FONT_COMIC  font_comic;
  int            fid;

  if ((font_comic = (FONT_COMIC)font->private) == NULL){
    fprintf(stderr, "VFlib internal error in comic class.\n");
    abort();
  }
  if ((fid = font_mapping(font_comic, code_point)) < 0)
    return -1;

  VF_GetMetric2(fid, code_point, metric, mag_x, mag_y);

  return 0;
}

Private int
comic_get_fontbbx2(VF_FONT font, double mag_x, double mag_y,
		   int*w_p, int *h_p, int *xoff_p, int *yoff_p)
{
  FONT_COMIC  font_comic;
  int         w, h, xoff, yoff;
  int         i;
  
  if ((font_comic = (FONT_COMIC)font->private) == NULL){
    fprintf(stderr, "VFlib internal error in comic class.\n");
    abort();
  }

  *w_p = *h_p = *xoff_p = *yoff_p = 0;
  w = h = xoff = yoff = 0;
  for (i = 0; i < I_N; i++){
    if (font_comic->sub_fid[i] < 0)
      continue;
    if (VF_GetFontBoundingBox2(font_comic->sub_fid[i],
			       mag_x, mag_y, &w, &h, &xoff, &yoff) < 0)
      continue;
    if (w > *w_p)
      *w_p = w;
    if (h > *h_p)
      *h_p = h;
    if (xoff < *xoff_p)
      *xoff_p = xoff;
    if (yoff > *yoff_p)
      *yoff_p = yoff;
  }

  return 0;
}


Private VF_BITMAP
comic_get_bitmap2(VF_FONT font, long code_point, 
		  double mag_x, double mag_y)
{
  FONT_COMIC  font_comic;
  int            fid;

  if ((font_comic = (FONT_COMIC)font->private) == NULL){
    fprintf(stderr, "VFlib internal error in comic class.\n");
    abort();
  }
  if ((fid = font_mapping(font_comic, code_point)) < 0)
    return NULL;

  return VF_GetBitmap2(fid, code_point, mag_x, mag_y);
}


Private char*
comic_get_font_prop(VF_FONT font, char *prop_name)
{
  FONT_COMIC  font_comic;
  int            fid;

  if ((font_comic = (FONT_COMIC)font->private) == NULL){
    fprintf(stderr, "VFlib internal error in comic class.\n");
    abort();
  }
  /* Get prop for Kanji font, not Kana font */
  if ((fid = font_mapping(font_comic, (long)0x3021)) < 0)
    return NULL;

  return VF_GetFontProp(fid, prop_name);
}


Private int
comic_query_font_type(VF_FONT font, long code_point)
{
  FONT_COMIC  font_comic;
  int         fid;

  if ((font_comic = (FONT_COMIC)font->private) == NULL){
    fprintf(stderr, "VFlib internal error in comic class.\n");
    abort();
  }
  if ((fid = font_mapping(font_comic, code_point)) < 0)
    return -1;

  return VF_QueryFontType(fid, code_point);
}


Private int
font_mapping(FONT_COMIC font_comic, long code_point)
{
  int   code_hi;
  int   fid, alt;

  code_hi = code_point / 0x100;

  if ((code_hi < 0x21) || (0x78 < code_hi)){
    vf_error = VF_ERR_ILL_CODE_POINT;
    return -1;
  } else {
    switch (code_hi){
    default: 
      alt = -1;
      fid = font_comic->sub_fid[I_KANJI];    break;
    case 0x21:
    case 0x22:
      alt = font_comic->sub_fid[I_KANA];
      fid = font_comic->sub_fid[I_SYMBOL];   break;
    case 0x23:
      alt = font_comic->sub_fid[I_KANA];
      fid = font_comic->sub_fid[I_ALNUM];    break;
    case 0x24:
      alt = font_comic->sub_fid[I_KANA];
      fid = font_comic->sub_fid[I_HIRAKANA]; break;
    case 0x25:
      alt = font_comic->sub_fid[I_KANA];
      fid = font_comic->sub_fid[I_KATAKANA]; break;
    case 0x26:
      alt = font_comic->sub_fid[I_KANA];
      fid = font_comic->sub_fid[I_GREEK];    break;
    case 0x27:
      alt = font_comic->sub_fid[I_KANA];
      fid = font_comic->sub_fid[I_CYRILLIC]; break;
    case 0x28:
      alt = font_comic->sub_fid[I_KANA];
      fid = font_comic->sub_fid[I_KEISEN];   break;
    }
    if (fid < 0)
      fid = alt;
  }

  if (debug_on('m'))
    printf("VFlib Japanese Comic:  Code Point: 0x%lx, FID: %d\n",
	   code_point,  fid);

  return fid;
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

  return FALSE;
}


/*EOF*/
