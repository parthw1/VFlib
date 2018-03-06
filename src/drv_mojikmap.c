/*
 * drv_mojikmap.c - A font driver for mapping to mojikyo fonts.
 *   This font driver provides a single virtual mojikyo font which is
 *   internally mapped to real font files.
 *
 * by Hirotsugu Kakugawa
 *
 *  8 Dec 1999  First implementation.
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
#include  "ccv.h"
#include  "mojikmap.h"


#define  DELAYED_OPEN  

#define SUBFONT_NOT_OPENED_YET  -2
#define SUBFONT_NOT_EXIST       -1

#define CCV_NOT_TRIED  -2
#define CCV_NOT_EXIST  -1



struct s_font_mojikmap {
  char     *font_name;
  int       div_scheme;
  char     *subfont_name;
  int       nsubfonts;
  int      *subfont_fids;
  int       ttf_subfont_enc;     /* when div_scheme is TTF */
  int       ttf_subfont_ccv_id;  /* when div_scheme is TTF */
  SEXP      props;
};
typedef struct s_font_mojikmap  *FONT_MOJIKMAP;


Private SEXP_ALIST   default_properties = NULL;
Private SEXP_ALIST   default_variables  = NULL;
Private SEXP_STRING  default_debug_mode = NULL;


Private int         mojikmap_create(VF_FONT,char*,char*,int,SEXP);
Private int         mojikmap_close(VF_FONT);
Private int         mojikmap_get_metric1(VF_FONT,long,VF_METRIC1,
				      double,double);
Private int         mojikmap_get_metric2(VF_FONT,long,VF_METRIC2,
				      double,double);
Private int         mojikmap_get_fontbbx1(VF_FONT,double,double,
				       double*,double*,double*,double*);
Private int         mojikmap_get_fontbbx2(VF_FONT,double,double, 
				       int*,int*,int*,int*);
Private VF_BITMAP   mojikmap_get_bitmap1(VF_FONT,long,double,double);
Private VF_BITMAP   mojikmap_get_bitmap2(VF_FONT,long,double,double);
Private VF_OUTLINE  mojikmap_get_outline(VF_FONT,long,double,double);
Private char*       mojikmap_get_font_prop(VF_FONT,char*);
Private int         mojikmap_query_font_type(VF_FONT,long);
Private void        release_mem(FONT_MOJIKMAP);
Private int         font_mapping(VF_FONT,FONT_MOJIKMAP,long,long*);
Private int         mojik_delayed_open(VF_FONT,FONT_MOJIKMAP,int);
Private int         debug_on(char type);


Glocal int
VF_Init_Driver_Mojikmap(void)
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

  if (vf_cap_GetParsedClassDefault(FONTCLASS_NAME_MOJIKMAP, ct, NULL, NULL) 
      == VFLIBCAP_PARSED_ERROR)
    return -1;

  VF_InstallFontDriver(FONTCLASS_NAME_MOJIKMAP, 
		       (DRIVER_FUNC_TYPE)mojikmap_create);

  return 0;
}


Private int
mojikmap_create(VF_FONT font, char *font_class,
	     char *font_name, int implicit, SEXP entry)
{
  FONT_MOJIKMAP  font_mojikmap;
  SEXP        cap_div_scheme, cap_subfont_name, cap_ttf_subfont_enc, cap_props;
  char       *s1;
  int         i;
  struct s_capability_table  ct[20];
  int  z;

  z = 0;
  /* VF_CAPE_FONT_CLASS */
  ct[z].cap = VF_CAPE_FONT_CLASS;          ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_ESSENTIAL;        ct[z++].val = NULL;
  /* VF_CAPE_DIV_SCHEME */
  ct[z].cap = VF_CAPE_DIV_SCHEME;          ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;         ct[z++].val = &cap_div_scheme;
  /* VF_CAPE_SUB_FONT_NAME */
  ct[z].cap = VF_CAPE_SUBFONT_NAME;        ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;         ct[z++].val = &cap_subfont_name;
  /* VF_CAPE_TTF_SUBFONT_ENC */
  ct[z].cap = VF_CAPE_TTF_SUBFONT_ENC;     ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;         ct[z++].val = &cap_ttf_subfont_enc;
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

  font->font_type       = -1;  /* Use mojikmap_query_font_type() */
  font->get_metric1     = mojikmap_get_metric1;
  font->get_metric2     = mojikmap_get_metric2;
  font->get_fontbbx1    = mojikmap_get_fontbbx1;
  font->get_fontbbx2    = mojikmap_get_fontbbx2;
  font->get_bitmap1     = mojikmap_get_bitmap1;
  font->get_bitmap2     = mojikmap_get_bitmap2;
  font->get_outline     = mojikmap_get_outline;
  font->get_font_prop   = mojikmap_get_font_prop;
  font->query_font_type = mojikmap_query_font_type;
  font->close           = mojikmap_close;

  ALLOC_IF_ERR(font_mojikmap, struct s_font_mojikmap){
    vf_error = VF_ERR_NO_MEMORY;
    return -1;
  }
  if ((font_mojikmap->font_name = vf_strdup(font_name)) == NULL){
    vf_error = VF_ERR_NO_MEMORY;
    vf_free(font_mojikmap);
    return -1;
  }

  font_mojikmap->font_name    = NULL;
  font_mojikmap->div_scheme   = DIVISION_SCHEME_TTF;
  font_mojikmap->subfont_name = NULL;
  font_mojikmap->subfont_fids = NULL;
  font_mojikmap->ttf_subfont_ccv_id = CCV_NOT_TRIED;
  font_mojikmap->ttf_subfont_enc    = -1;
  font_mojikmap->props        = default_properties;

  /* implicit == 0 */

  if (cap_props != NULL){
    font_mojikmap->props = cap_props;
  }

  if (cap_div_scheme != NULL){
    s1 = vf_sexp_get_cstring(cap_div_scheme);
    if ((vf_strcmp_ci(s1, "ttf") == 0)
	|| (vf_strcmp_ci(s1, "truetype") == 0)){
      font_mojikmap->div_scheme = DIVISION_SCHEME_TTF;
    } else if ((vf_strcmp_ci(s1, "type1") == 0) 
	       || (vf_strcmp_ci(s1, "pfb") == 0)){
      font_mojikmap->div_scheme = DIVISION_SCHEME_TYPE1;      
    } else {
      fprintf(stderr, 
	      "VFlib warning: unknown division scheme name for "
	      "capability '%s' in definition of font %s.\n",
	      VF_CAPE_DIV_SCHEME, font_name);
      font_mojikmap->div_scheme = DIVISION_SCHEME_TTF;
    }
  }

  if (cap_subfont_name != NULL){
    font_mojikmap->subfont_name
      = vf_strdup(vf_sexp_get_cstring(cap_subfont_name));
  } else {
    if (font_mojikmap->div_scheme == DIVISION_SCHEME_TTF){
      font_mojikmap->subfont_name = vf_strdup(DEFAULT_SUBFONT_NAME_TTF);
    } else {
      font_mojikmap->subfont_name = vf_strdup(DEFAULT_SUBFONT_NAME_TYPE1);
    }
  }

  switch (font_mojikmap->div_scheme){
  case DIVISION_SCHEME_TTF:
    font_mojikmap->nsubfonts = 21;
    font_mojikmap->ttf_subfont_enc = DEFAULT_TTF_SUBFONT_ENC;
    if (cap_ttf_subfont_enc != NULL){
      s1 = vf_sexp_get_cstring(cap_ttf_subfont_enc); 
      if (vf_strcmp_ci(s1, "unicode") == 0){
	font_mojikmap->ttf_subfont_enc = TTF_SUBFONT_ENC_UNICODE;
      } else if ((vf_strcmp_ci(s1, "ISO2022") == 0)
		 || (vf_strcmp_ci(s1, "ISO-2022") == 0)
		 || (vf_strcmp_ci(s1, "JIS") == 0)){
	font_mojikmap->ttf_subfont_enc = TTF_SUBFONT_ENC_JIS;
      }
    }
    break;
  case DIVISION_SCHEME_TYPE1:
    font_mojikmap->nsubfonts = 21*TYPE1_NSUBS;
    font_mojikmap->ttf_subfont_enc = -1;
    break;
  }

  ALLOCN_IF_ERR(font_mojikmap->subfont_fids, int, font_mojikmap->nsubfonts){
    vf_error = VF_ERR_NO_MEMORY;
    goto CANT_OPEN;
  }
  for (i = 0; i < font_mojikmap->nsubfonts; i++)
    font_mojikmap->subfont_fids[i] = SUBFONT_NOT_OPENED_YET;

#ifndef DELAYED_OPEN
  switch (font_mojikmap->div_scheme){
  case DIVISION_SCHEME_TTF:
    for (i = 0; i < font_mojikmap->nsubfonts; i++){
      sprintf(subfont_name, font_mojikmap->subfont_name, i+101);
      if (font->mode == 1){
	font_mojikmap->subfont_fids[i]
	  = VF_OpenFont1(subfont_name, font->dpi_x, font->dpi_y, 
			 font->point_size, font->mag_x, font->mag_y);
      } else if (font->mode == 2){
	font_mojikmap->subfont_fids[i]
	  = VF_OpenFont2(subfont_name, 
			 font->pixel_size, font->mag_x, font->mag_y);
      } else {
	fprintf(stderr, "VFlib: Internal error in mojikmap_create()\n");
	abort();
      }
      if (font_mojikmap->subfont_fids[i] < 0){
	font_mojikmap->subfont_fids[i] = SUBFONT_NOT_EXIST;
      }
      if (debug_on('f'))
	printf("VFlib mojikmap: subfont  name=%s fid=%d\n",
	       subfont_name, font_mojikmap->subfont_fids[i]);
    }
    break;
  case DIVISION_SCHEME_TYPE1:
    for (i = 0; i < font_mojikmap->nsubfonts; i++){
      for (j = 0; j < TYPE1_NSUBS; j++){
	sprintf(subfont_name, font_mojikmap->subfont_name, i+101, j+6);
	if (font->mode == 1){
	  font_mojikmap->subfont_fids[i*TYPE1_NSUBS + j]
	    = VF_OpenFont1(subfont_name, font->dpi_x, font->dpi_y, 
			   font->point_size, font->mag_x, font->mag_y);
	} else if (font->mode == 2){
	  font_mojikmap->subfont_fids[i*TYPE1_NSUBS + j]
	    = VF_OpenFont2(subfont_name, 
			   font->pixel_size, font->mag_x, font->mag_y);
	} else {
	  fprintf(stderr, "VFlib: Internal error in mojikmap_create()\n");
	  abort();
	}
	if (font_mojikmap->subfont_fids[i*TYPE1_NSUBS + j] < 0){
	  font_mojikmap->subfont_fids[i*TYPE1_NSUBS + j] = SUBFONT_NOT_EXIST;
	}
	if (debug_on('f'))
	  printf("VFlib mojikmap: subfont  name=%s fid=%d\n",
		 subfont_name, font_mojikmap->subfont_fids[i*TYPE1_NSUBS + j]);
      }
    }
    break;
  }
#endif /* DELAYED_OPEN */

  font->private = font_mojikmap;
  vf_sexp_free3(&cap_div_scheme, &cap_subfont_name, &cap_ttf_subfont_enc);
  return 0;


CANT_OPEN:
  vf_sexp_free3(&cap_div_scheme, &cap_subfont_name, &cap_ttf_subfont_enc);
  vf_error = VF_ERR_NO_FONT_ENTRY;
  release_mem(font_mojikmap);
  return -1;
}


Private int
mojikmap_close(VF_FONT font)
{
  release_mem((FONT_MOJIKMAP)(font->private));

  return 0; 
}


Private void
release_mem(FONT_MOJIKMAP font_mojikmap)
{
  int  i;

  if (font_mojikmap != NULL){
    vf_free(font_mojikmap->font_name);
    vf_free(font_mojikmap->subfont_name);
    vf_sexp_free1(&font_mojikmap->props);
    for (i = 0; i < font_mojikmap->nsubfonts; i++){
      if (font_mojikmap->subfont_fids[i] >= 0)
	VF_CloseFont(font_mojikmap->subfont_fids[i]);
    }
    vf_free(font_mojikmap);
  }
}


Private int
mojikmap_get_metric1(VF_FONT font, long code_point, VF_METRIC1 metric,
		  double mag_x, double mag_y)
{
  FONT_MOJIKMAP  font_mojikmap;
  int            fid;
  long           cp;
  
  if (metric == NULL){
    fprintf(stderr, "VFlib internal error: in mojikmap_get_metric1()\n");
    abort();
  }

  if ((font_mojikmap = (FONT_MOJIKMAP)font->private) == NULL){
    fprintf(stderr, "VFlib internal error in mojikmap class.\n");
    abort();
  }

  if ((fid = font_mapping(font, font_mojikmap, code_point, &cp)) < 0)
    return -1;
  if (cp < 0)
    return -1;

  VF_GetMetric1(fid, cp, metric, mag_x, mag_y);

  return 0;
}


Private int
mojikmap_get_fontbbx1(VF_FONT font, double mag_x, double mag_y,
		   double *w_p, double *h_p, double *xoff_p, double *yoff_p)
{
  FONT_MOJIKMAP  font_mojikmap;
  int         fid, i;
  double      w, h, xoff, yoff;
  
  if ((font_mojikmap = (FONT_MOJIKMAP)font->private) == NULL){
    fprintf(stderr, "VFlib internal error in mojikmap class.\n");
    abort();
  }

  *w_p = *h_p = *xoff_p = *yoff_p = 0;
  w = h = xoff = yoff = 0;
  for (i = 0; i < font_mojikmap->nsubfonts; i++){
    if ((fid = font_mojikmap->subfont_fids[i]) < 0)
      continue;
    if (VF_GetFontBoundingBox1(fid, mag_x, mag_y, &w, &h, &xoff, &yoff) < 0)
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
mojikmap_get_bitmap1(VF_FONT font, long code_point,
		  double mag_x, double mag_y)
{
  FONT_MOJIKMAP  font_mojikmap;
  int            fid;
  long           cp;

  if ((font_mojikmap = (FONT_MOJIKMAP)font->private) == NULL){
    fprintf(stderr, "VFlib internal error in mojikmap class.\n");
    abort();
  }
  if ((fid = font_mapping(font, font_mojikmap, code_point, &cp)) < 0)
    return NULL;
  if (cp < 0)
    return NULL;

  return VF_GetBitmap1(fid, cp, mag_x, mag_y);
}


Private VF_OUTLINE
mojikmap_get_outline(VF_FONT font, long code_point,
		  double mag_x, double mag_y)
{
  FONT_MOJIKMAP  font_mojikmap;
  int            fid;
  long           cp;

  if ((font_mojikmap = (FONT_MOJIKMAP)font->private) == NULL){
    fprintf(stderr, "VFlib internal error in mojikmap class.\n");
    abort();
  }
  if ((fid = font_mapping(font, font_mojikmap, code_point, &cp)) < 0)
    return NULL;
  if (cp < 0)
    return NULL;

  return VF_GetOutline(fid, cp, mag_x, mag_y);
}


Private int
mojikmap_get_metric2(VF_FONT font, long code_point, VF_METRIC2 metric, 
		  double mag_x, double mag_y)
{
  FONT_MOJIKMAP  font_mojikmap;
  int            fid;
  long           cp;

  if ((font_mojikmap = (FONT_MOJIKMAP)font->private) == NULL){
    fprintf(stderr, "VFlib internal error in mojikmap class.\n");
    abort();
  }
  if ((fid = font_mapping(font, font_mojikmap, code_point, &cp)) < 0)
    return -1;
  if (cp < 0)
    return -1;

  VF_GetMetric2(fid, cp, metric, mag_x, mag_y);

  return 0;
}

Private int
mojikmap_get_fontbbx2(VF_FONT font, double mag_x, double mag_y,
		   int*w_p, int *h_p, int *xoff_p, int *yoff_p)
{
  FONT_MOJIKMAP  font_mojikmap;
  int         w, h, xoff, yoff;
  int         fid, i;
  
  if ((font_mojikmap = (FONT_MOJIKMAP)font->private) == NULL){
    fprintf(stderr, "VFlib internal error in mojikmap class.\n");
    abort();
  }

  *w_p = *h_p = *xoff_p = *yoff_p = 0;
  w = h = xoff = yoff = 0;
  for (i = 0; i < font_mojikmap->nsubfonts; i++){
    if ((fid = font_mojikmap->subfont_fids[i]) < 0)
      continue;
    if (VF_GetFontBoundingBox2(fid, mag_x, mag_y, &w, &h, &xoff, &yoff) < 0)
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
mojikmap_get_bitmap2(VF_FONT font, long code_point, 
		  double mag_x, double mag_y)
{
  FONT_MOJIKMAP  font_mojikmap;
  int            fid;
  long           cp;

  if ((font_mojikmap = (FONT_MOJIKMAP)font->private) == NULL){
    fprintf(stderr, "VFlib internal error in mojikmap class.\n");
    abort();
  }
  if ((fid = font_mapping(font, font_mojikmap, code_point, &cp)) < 0)
    return NULL;
  if (cp < 0)
    return NULL;

  return VF_GetBitmap2(fid, cp, mag_x, mag_y);
}


Private char*
mojikmap_get_font_prop(VF_FONT font, char *prop_name)
{
  FONT_MOJIKMAP  font_mojikmap;
  SEXP           v;
  int            fid;
  long           cp;

  if ((font_mojikmap = (FONT_MOJIKMAP)font->private) == NULL){
    fprintf(stderr, "VFlib internal error in mojikmap class.\n");
    abort();
  }

  if ((v = vf_sexp_assoc(prop_name, font_mojikmap->props)) != NULL){
    return vf_strdup(vf_sexp_get_cstring(vf_sexp_cadr(v)));
  } else if ((v = vf_sexp_assoc(prop_name, default_properties)) != NULL){
    return vf_strdup(vf_sexp_get_cstring(vf_sexp_cadr(v)));
  }

  if ((fid = font_mapping(font, font_mojikmap, (long)1, &cp)) < 0)
    return NULL;
  if (cp < 0)
    return NULL;

  return VF_GetFontProp(fid, prop_name);
}


Private int
mojikmap_query_font_type(VF_FONT font, long code_point)
{
  FONT_MOJIKMAP  font_mojikmap;
  int            fid;
  long           cp;

  if ((font_mojikmap = (FONT_MOJIKMAP)font->private) == NULL){
    fprintf(stderr, "VFlib internal error in mojikmap class.\n");
    abort();
  }
  if ((fid = font_mapping(font, font_mojikmap, code_point, &cp)) < 0)
    return -1;
  if (cp < 0)
    return -1;

  return VF_QueryFontType(fid, cp);
}


Private int
font_mapping(VF_FONT font, FONT_MOJIKMAP font_mojikmap, 
	     long code_point, long *cp)
{
  int   g, fid, c1, c2;
  long  c;

  if (code_point == 0){
    if (cp != NULL)
      *cp = -1;
    return -1;
  }

  g = -1;
  c = 0;

  switch (font_mojikmap->div_scheme){
  case DIVISION_SCHEME_TTF:
    g = code_point / (94*60);
    c = code_point % (94*60);
    if (c == 0){
      g += 100;
      c = (94*60);
    } else {
      g += 101;
    }
    c1 = ((c-1) / 94);
    c2 = ((c-1) % 94);
    if (c1 < 30){
      c1 += 16;
    } else {
      c1 += 18;
    }
    c = (c1+0x20) * 0x100 + (c2+0x21);
    g -= 101;
    if (font_mojikmap->ttf_subfont_enc == TTF_SUBFONT_ENC_JIS){
      ;
    } else if (font_mojikmap->ttf_subfont_enc == TTF_SUBFONT_ENC_UNICODE){
      if (font_mojikmap->ttf_subfont_ccv_id == CCV_NOT_TRIED){
	font_mojikmap->ttf_subfont_ccv_id 
	  = vf_ccv_require("JISX0208", "ISO2022", "UNICODE", "UNICODE");
	if (debug_on('f')){
	  printf("VFlib mojikmap:  CCV: id=%d\n",
		 font_mojikmap->ttf_subfont_ccv_id);
	}
	if (font_mojikmap->ttf_subfont_ccv_id < 0){
	  font_mojikmap->ttf_subfont_ccv_id = CCV_NOT_EXIST;
	  return -1; /* failed to invoke ccv */
	}
      }
      if (font_mojikmap->ttf_subfont_ccv_id >= 0){
	c = vf_ccv_conv(font_mojikmap->ttf_subfont_ccv_id, c);
      }
    } else { /* ??? */
      ;
    }
    break;
  case DIVISION_SCHEME_TYPE1:
    g = (code_point) / (94*60);
    c = (code_point) % (94*60);
    if (c == 0){
      g += 100;
      c = 5640;
    } else {
      g += 101;
    }
    if (c > 2820){
      c += 380;
    } else {
      c += 192;
    }
    g -= 101;
    g = g * TYPE1_NSUBS + (c / 256);
    c = (c % 256);
    break;
  default:
    break;
  }

  if ((g < 0) || (g >= font_mojikmap->nsubfonts))
    return -1;

  if (cp != NULL)
    *cp = c;
  fid = mojik_delayed_open(font, font_mojikmap, g);

  if (debug_on('m')){
    printf("VFlib mojikmap:  Code Point: %ld => FID: %d, Code: 0x%lx\n",
	   code_point, fid, c);
  }

  return  fid;
}



Private int
mojik_delayed_open(VF_FONT font, FONT_MOJIKMAP font_mojikmap, int g)
{
  char  subfont[1024];

  if ((g < 0) || (g >= font_mojikmap->nsubfonts))
    return -1;
  if (font_mojikmap->subfont_fids[g] >= 0)
    return font_mojikmap->subfont_fids[g];
  if (font_mojikmap->subfont_fids[g] == SUBFONT_NOT_EXIST)
    return -1;

  switch (font_mojikmap->div_scheme){

  case DIVISION_SCHEME_TTF:
    sprintf(subfont, font_mojikmap->subfont_name, g+101);
    if (font->mode == 1){
      font_mojikmap->subfont_fids[g]
	= VF_OpenFont1(subfont, font->dpi_x, font->dpi_y, 
		       font->point_size, font->mag_x, font->mag_y);
    } else if (font->mode == 2){
      font_mojikmap->subfont_fids[g]
	= VF_OpenFont2(subfont, 
		       font->pixel_size, font->mag_x, font->mag_y);
    } else {
      fprintf(stderr, "VFlib: Internal error in mojikmap_delayed_open()\n");
      abort();
    }
    break;

  case DIVISION_SCHEME_TYPE1:
    sprintf(subfont, font_mojikmap->subfont_name, 
	    (g / TYPE1_NSUBS) + 101, (g % TYPE1_NSUBS) + 6);
    if (font->mode == 1){
      font_mojikmap->subfont_fids[g]
	= VF_OpenFont1(subfont, font->dpi_x, font->dpi_y, 
		       font->point_size, font->mag_x, font->mag_y);
    } else if (font->mode == 2){
      font_mojikmap->subfont_fids[g]
	= VF_OpenFont2(subfont, 
		       font->pixel_size, font->mag_x, font->mag_y);
    } else {
      fprintf(stderr, "VFlib: Internal error in mojikmap_delayed_open()\n");
      abort();
    }
    break;
  }

  if (font_mojikmap->subfont_fids[g] < 0){
    font_mojikmap->subfont_fids[g] = SUBFONT_NOT_EXIST;
  }
  if (debug_on('f'))
    printf("VFlib mojikmap: subfont  name=%s fid=%d\n",
	   subfont, font_mojikmap->subfont_fids[g]);

  return font_mojikmap->subfont_fids[g];
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
