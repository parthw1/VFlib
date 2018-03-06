/*
 * texfonts.c - Pseudo font class for TeX-related files  (GF, PK, VF, TFM)
 *
 * 28 Sep 1996
 * 25 Mar 1997  Added setting a program name for kpathsea by variable.
 * 02 Apr 1997  Added support for .ofm files (Omega metrics file) (WL)
 *  3 Jul 1997  Added Virtual Font support.
 *  8 Aug 1997  for VFlib 3.3  
 *  1 Feb 1998  for VFlib 3.4
 * 22 Jul 1998  Added TeX font mapping feafure.
 * 24 Nov 1998  Added get_fontbbx1() and get_fontbbx2().
 * 16 Sep 1999  Modified TeX-font-mapper not to open TFM files as possible.
 */
/*
 * Copyright (C) 1996-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */

#include  "config.h"
#include  "with.h"
#include  <stdio.h>
#include  <stdlib.h>
#include  <ctype.h>
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif
#include  "VFlib-3_7.h"
#include  "VFsys.h"
#include  "vflibcap.h"
#include  "consts.h"
#include  "cache.h"
#include  "bitmap.h"
#include  "sexp.h"
#include  "str.h"
#include  "path.h"
#include  "fsearch.h"
#include  "texfonts.h"
#include  "tfm.h"

Private SEXP_STRING  default_tex_dpi            = NULL;
Private int          v_default_tex_dpi          = DEFAULT_DPI;
Private SEXP_LIST    default_fontdirs           = NULL;
Private SEXP_LIST    default_tfm_dirs           = NULL;
Private SEXP_LIST    default_tfm_extensions     = NULL;
Private SEXP         default_font_mapping       = NULL;
Private SEXP         default_resolution_corr    = NULL;
Private SEXP         default_resolution_accu    = NULL;
Private double       v_default_resolution_accu  = DEFAULT_RESOLUTION_ACCU;
Private SEXP_STRING  default_debug_mode         = NULL;

Glocal SEXP_ALIST    vf_tex_default_properties  = NULL;
Glocal SEXP_ALIST    vf_tex_default_variables   = NULL;
Glocal int           vf_dbg_drv_texfonts        = 0;


Private int   tex_create(VF_FONT,char*,char*,int,SEXP);
Private int   syntax_check_resolution_corr(void);

Private int          tex_create(VF_FONT font, char *font_class, 
				char *font_name, int implicit, SEXP entry);
Private int          tex_close(VF_FONT font);
Private int          tex_get_metric1(VF_FONT font, long code_point,
				     VF_METRIC1 metric1, double,double);
Private int          tex_get_metric2(VF_FONT font, long code_point,
				     VF_METRIC2 metric2, double,double);
Private int          tex_get_fontbbx1(VF_FONT,double,double,
				      double*,double*,double*,double*);
Private int          tex_get_fontbbx2(VF_FONT,double,double, 
				      int*,int*,int*,int*);
Private VF_BITMAP    tex_get_bitmap1(VF_FONT,long,double,double);
Private VF_BITMAP    tex_get_bitmap2(VF_FONT,long,double,double);
Private VF_OUTLINE   tex_get_outline(VF_FONT,long,double,double);
Private char*        tex_get_font_prop(VF_FONT,char*);
Private int          tex_query_font_type(VF_FONT font, long code_point);


Private void  tex_map_name(char *fontname, int n, char *mapping, 
			   char *name_str, char *dpi_str, char *ext_str);
Private int   try_open_mapped_font(SEXP s,
				   char *name_str, char *dpi_str,
				   char *ext_str, VF_FONT font, 
				   char *name, double design_size,
				   SEXP tfm_dirs, SEXP tfm_extensions,
				   double opt_mag);
Private double get_design_size_from_tfm(char *font_name, 
					SEXP tfm_dirs, SEXP tfm_extensions);
Private int    match_font_name(char *name, char *pat);
Private int    decompose_filename(char*,char**,char**,char**,char**);




Glocal int
VF_Init_Driver_TeX(void)
{
  return  vf_tex_init();  
}


Glocal int
vf_tex_init(void)
{
  static int  inited = 0;
  struct s_capability_table  ct[20];
  int  z;
  
  z = 0;
  /* VF_CAPE_FONT_DIRECTORIES */
  ct[z].cap = VF_CAPE_FONT_DIRECTORIES;
  ct[z].type = CAPABILITY_STRING_LIST0;
  ct[z].ess = CAPABILITY_OPTIONAL;
  ct[z++].val = &default_fontdirs;
  /* VF_CAPE_DPI */
  ct[z].cap = VF_CAPE_DPI;
  ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;
  ct[z++].val = &default_tex_dpi;
  /* VF_CAPE_TEX_TFM_DIRECTORIES */
  ct[z].cap = VF_CAPE_TEX_TFM_DIRECTORIES; 
  ct[z].type = CAPABILITY_STRING_LIST0;
  ct[z].ess = CAPABILITY_OPTIONAL;
  ct[z++].val = &default_tfm_dirs;
  /* VF_CAPE_TEX_TFM_EXTENSIONS */
  ct[z].cap = VF_CAPE_TEX_TFM_EXTENSIONS; 
  ct[z].type = CAPABILITY_STRING_LIST0;
  ct[z].ess = CAPABILITY_OPTIONAL;
  ct[z++].val = &default_tfm_extensions;
  /* VF_CAPE_TEX_FONT_MAPPING */
  ct[z].cap = VF_CAPE_TEX_FONT_MAPPING;
  ct[z].type = CAPABILITY_LIST;
  ct[z].ess = CAPABILITY_OPTIONAL;
  ct[z++].val = &default_font_mapping;
  /* VF_CAPE_PROPERTIES */
  ct[z].cap = VF_CAPE_PROPERTIES;
  ct[z].type = CAPABILITY_ALIST;
  ct[z].ess = CAPABILITY_OPTIONAL;
  ct[z++].val = &vf_tex_default_properties;
  /* VF_CAPE_VARIABLE_VALUES */
  ct[z].cap = VF_CAPE_VARIABLE_VALUES;
  ct[z].type = CAPABILITY_ALIST;
  ct[z].ess = CAPABILITY_OPTIONAL;
  ct[z++].val = &vf_tex_default_variables;
  /* VF_CAPE_RESOLUTION_CORR */
  ct[z].cap = VF_CAPE_RESOLUTION_CORR;
  ct[z].type = CAPABILITY_LIST;
  ct[z].ess = CAPABILITY_OPTIONAL;
  ct[z++].val = &default_resolution_corr;
  /* VF_CAPE_RESOLUTION_ACCU */
  ct[z].cap = VF_CAPE_RESOLUTION_ACCU;
  ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;
  ct[z++].val = &default_resolution_accu;
  /* VF_CAPE_DEBUG */
  ct[z].cap = VF_CAPE_DEBUG;
  ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;
  ct[z++].val = &default_debug_mode;
  /* end */
  ct[z].cap = NULL; ct[z].type = 0; ct[z].ess = 0; ct[z++].val = NULL;


  if (inited == 1)
    return 0;
  inited = 1;

  if (getenv("VFLIB_DEBUG_TEXFONTS") != NULL)
    vf_dbg_drv_texfonts = 1;

  if (vf_cap_GetParsedClassDefault(FONTCLASS_NAME_TeX, ct, NULL, NULL)
      == VFLIBCAP_PARSED_ERROR)
    return -1;

  v_default_tex_dpi  = DEFAULT_DPI;
  if (default_tex_dpi != NULL)
    v_default_tex_dpi = atoi(vf_sexp_get_cstring(default_tex_dpi));

  if (default_tfm_extensions == NULL)
    default_tfm_extensions = vf_sexp_cstring2list(DEFAULT_EXTENSIONS_TFM);
  vf_tfm_init();

  if (default_resolution_corr != NULL){
    if (syntax_check_resolution_corr() > 0){
      vf_sexp_free(&default_resolution_corr);
      fprintf(stderr, 
	      "VFlib: value for capability %s is ignored.\n", 
	      VF_CAPE_RESOLUTION_CORR);
    }
  }

  v_default_resolution_accu = DEFAULT_RESOLUTION_ACCU; 
  if (default_resolution_accu != NULL){
    v_default_resolution_accu 
      = atof(vf_sexp_get_cstring(default_resolution_accu));
    if (v_default_resolution_accu <= 0)
      v_default_resolution_accu = DEFAULT_RESOLUTION_ACCU; 
  }

  if (default_font_mapping != NULL){
    if (vf_tex_syntax_check_font_mapping(default_font_mapping) > 0){
      vf_sexp_free(&default_font_mapping);
      fprintf(stderr, 
	      "VFlib: capability %s is ignored because of syntax error.\n", 
	      VF_CAPE_TEX_FONT_MAPPING);
    }
  }

  VF_InstallFontDriver(FONTCLASS_NAME_TeX, (DRIVER_FUNC_TYPE)tex_create);

  return 0;
}

Private int 
syntax_check_resolution_corr(void)
{
  int   syntax_err, syntax_nerrs;
  SEXP  s, t, u;
  
  syntax_nerrs = 0;

  for (s = default_resolution_corr; vf_sexp_consp(s); s = vf_sexp_cdr(s)){
    syntax_err = 0;
    u = t = vf_sexp_car(s); 
    /* e.g., u = (300  300 329 360 394 ...) */
    if (!vf_sexp_listp(u)){
      syntax_err = 1;
      syntax_nerrs++;
      goto rc_try_next;
    }
    for ( ; vf_sexp_consp(u); u = vf_sexp_cdr(u)){
      if (!vf_sexp_stringp(vf_sexp_car(u))
	  || (atoi(vf_sexp_get_cstring(vf_sexp_car(u))) <= 0)){
	syntax_err = 1;
	syntax_nerrs++;
	goto rc_try_next;
      }
    }
  rc_try_next:
    if (syntax_err != 0){
      fprintf(stderr, 
	      "VFlib: %s in capability %s of %s font class default: \n", 
	      "Syntax error", VF_CAPE_RESOLUTION_CORR, FONTCLASS_NAME_TeX);
      vf_sexp_pp_fp(t, stderr);
    }
  }

  return syntax_nerrs;
}



Private int
tex_create(VF_FONT font, char *font_class, char *font_name, 
	   int implicit, SEXP entry)
{
  int    fid;

  if (implicit == 0){
    vf_error = VF_ERR_NO_FONT_ENTRY;
    return -1;  /* this driver does not support explicit fonts. */
  }

  if (default_font_mapping == NULL){
    vf_error = VF_ERR_NO_FONT_ENTRY;
    return -1;
  }

#if 1
  fid = vf_tex_try_map_and_open_font(font, font_name, default_font_mapping,
				     -1, default_tfm_dirs, 
				     default_tfm_extensions, 1.0);
#else
  fid = vf_tex_try_map_and_open_font(font, font_name, default_font_mapping,
				     default_tfm_dirs, 
				     NULL, default_tfm_extensions,
				     1.0);
#endif
  if (fid < 0)
    return -1;
  
  font->font_type       = VF_FONT_TYPE_UNDEF;
  font->get_metric1     = tex_get_metric1;
  font->get_metric2     = tex_get_metric2;
  font->get_fontbbx1    = tex_get_fontbbx1;
  font->get_fontbbx2    = tex_get_fontbbx2;
  font->get_bitmap1     = tex_get_bitmap1;
  font->get_bitmap2     = tex_get_bitmap2;
  font->get_outline     = tex_get_outline;
  font->get_font_prop   = tex_get_font_prop;
  font->query_font_type = tex_query_font_type;
  font->close           = tex_close;

  font->private         = (void*)fid;

  return 0;
}



Glocal int
vf_tex_try_map_and_open_font(VF_FONT font, char *font_name, SEXP font_mapping,
			     double tfm_design_size, 
			     SEXP tfm_dirs, SEXP tfm_extensions,
			     double opt_mag)
{
  char   *name_str, *dpi_str, *ext_str, *ns, *pat;
  int    fid;
  SEXP   s, t;

  if (font_mapping == NULL){
    vf_error = VF_ERR_NO_FONT_ENTRY;
    return -1;
  }

  fid = -1;
  name_str = dpi_str = ext_str = ns = NULL;

  if (decompose_filename(font_name, &name_str, &dpi_str, &ext_str, &ns) < 0)
    goto search_end;

  for (s = font_mapping; vf_sexp_consp(s); s = vf_sexp_cdr(s)){
    for (t = vf_sexp_car(s); 
	 vf_sexp_consp(t) && !vf_sexp_stringp(vf_sexp_car(t));
	 t = vf_sexp_cdr(t))
      ; /* empty */ 
    if (!vf_sexp_consp(t)) 
      goto search_end;
    while (vf_sexp_consp(t) && vf_sexp_stringp(vf_sexp_car(t))){
      pat = vf_sexp_get_cstring(vf_sexp_car(t));
      if ((strcmp(ns, pat) == 0) || (strcmp(pat, "*") == 0) 
	  || (match_font_name(ns, pat) == 1)){
	if ((fid = try_open_mapped_font(vf_sexp_car(s), 
					name_str, dpi_str, ext_str, 
					font, font_name, 
					tfm_design_size, 
					tfm_dirs, tfm_extensions,
					opt_mag)) >= 0)
	  goto search_end;
      }
      t = vf_sexp_cdr(t);
    }
  }
  if (fid < 0)
    vf_error = VF_ERR_NO_FONT_ENTRY;

search_end:
  vf_free(name_str);
  vf_free(dpi_str);
  vf_free(ext_str);

  return fid;
}

Private int
match_font_name(char *name, char *patt)
{
  while (*name != '\0'){
    if (*patt == '*'){
      return 1;
    } else {
      if (*patt != *name)
	return -1;
    }
    name++;
    patt++;
  }

  if (*patt != '\0')
    return -1;
  return 0;
}

Private int
decompose_filename(char *font_name, 
		   char **name_str_p, char **dpi_str_p, 
		   char **ext_str_p, char **ns_p)
{
  int     len, i, i0, i1, dellen;
  char   *name_str, *dpi_str, *ext_str, *ns;

  name_str = dpi_str = ext_str = ns = NULL;

  len = strlen(font_name);
  for (i0 = len-1; i0 >= 0; i0--){
    if (font_name[i0] == '.'){ 
      /* "cmr10.360pk" ==>           len=11
       *     name_str = "cmr10"      i0=5
       *     dpi_str  = "360"        i1=9
       *     ext_str  = "pk" 
       */
      if ((name_str = (char*)malloc(i0+1)) == NULL){
	vf_error = VF_ERR_NO_MEMORY;
	goto end;
      }
      strncpy(name_str, font_name, i0);
      name_str[i0] = '\0';
      for (i1 = i0+1; isdigit((int)font_name[i1]); i1++)
	   ;
      if ((dpi_str = (char*)malloc(i1-i0+1)) == NULL){
	vf_error = VF_ERR_NO_MEMORY;
	goto end;
      }
      strncpy(dpi_str, &font_name[i0+1], i1-i0-1);
      dpi_str[i1-i0-1] = '\0';
      if ((ext_str = (char*)malloc(strlen(&font_name[i1])+1)) == NULL){
	vf_error = VF_ERR_NO_MEMORY;
	goto end;
      }
      strcpy(ext_str, &font_name[i1]);
      break;
    }
  }

  if ((len > 0)
      && (name_str == NULL) && (dpi_str == NULL) && (ext_str == NULL)){
    if ((name_str = (char*)malloc(len+1)) == NULL){
      vf_error = VF_ERR_NO_MEMORY;
      goto end;
    }
    strcpy(name_str, font_name);
  }


  if (name_str == NULL){
    if ((name_str = (char*)malloc(1)) == NULL){
      vf_error = VF_ERR_NO_MEMORY;
      goto end;
    }
    name_str[0] = '\0';
  } 
  if (dpi_str == NULL){
    if ((dpi_str = (char*)malloc(1)) == NULL){
      vf_error = VF_ERR_NO_MEMORY;
      goto end;
    }
    dpi_str[0] = '\0';
  } 
  if (ext_str == NULL){
    if ((ext_str = (char*)malloc(1)) == NULL){
      vf_error = VF_ERR_NO_MEMORY;
      goto end;
    }
    ext_str[0] = '\0';
  }
  
  dellen = strlen(vf_directory_delimiter);
  if ((ns = name_str) != NULL){
    for (i = strlen(name_str)-1; i >= 0; i--){
      if (strncmp(&name_str[i], vf_directory_delimiter, dellen) == 0){
	ns = &name_str[i+dellen];
	break;
      }
    }
  }
#if 0
  printf("*** \"%s\"  => \"%s\", \"%s\", \"%s\", \"%s\"\n",
	 font_name, name_str, ns, dpi_str, ext_str);
#endif

end:
  if ((name_str == NULL) || (dpi_str == NULL) || (ext_str == NULL)){
    vf_free(name_str);
    vf_free(dpi_str);
    vf_free(ext_str);
    return -1;
  }

  *name_str_p = name_str;
  *dpi_str_p  = dpi_str;
  *ext_str_p  = ext_str;
  *ns_p       = ns;

  return 0;
}

Private int
try_open_mapped_font(SEXP s, char *name_str, char *dpi_str, char *ext_str, 
		     VF_FONT font, char *font_name, 
		     double tfm_design_size, 
		     SEXP tfm_dirs, SEXP tfm_extensions,		     
		     double opt_mag)
{
  SEXP    t, u;
  char   *drvname, *mapping;
  char    mapped_name[1024];
  int     fid, dpix, dpiy;
  double  ptsize, mag_adj, mag_x, mag_y;

  fid = -1;
  for (fid = -1; 
       vf_sexp_consp(s) && vf_sexp_consp(vf_sexp_car(s)); 
       s = vf_sexp_cdr(s)){

    t = vf_sexp_car(s);
    if (   !vf_sexp_listp(t) 
	|| (vf_sexp_length(t) < 2)
	|| !vf_sexp_stringp(vf_sexp_car(t)) 
	|| !vf_sexp_stringp(vf_sexp_cadr(t)))
      continue;

    drvname = vf_sexp_get_cstring(vf_sexp_car(t));
    mapping = vf_sexp_get_cstring(vf_sexp_cadr(t));

    ptsize  = font->point_size;
    mag_adj = 1.0;
    for (t = vf_sexp_cddr(t); vf_sexp_consp(t); t = vf_sexp_cdr(t)){
      u = vf_sexp_car(t);
      if (vf_sexp_stringp(u)){
	if (strcmp(TEX_FONT_MAPPING_PTSIZE, vf_sexp_get_cstring(u)) == 0){
	  if (tfm_design_size > 0){
	    ptsize = tfm_design_size;
	  } else {
	    ptsize = get_design_size_from_tfm(font_name, 
					      tfm_dirs, tfm_extensions);
	  }
	  if (ptsize < 0)
	    continue;
	} else {
	  fprintf(stderr, 
		  "VFlib: %s %s in capability %s for %s class default.\n", 
		  "Unknown optional keyword", vf_sexp_get_cstring(u), 
		  VF_CAPE_TEX_FONT_MAPPING, FONTCLASS_NAME_TeX);
	}
      } else if (vf_sexp_listp(u) && (vf_sexp_length(u) >= 2)){
	if (vf_sexp_stringp(vf_sexp_car(u))
	    && vf_sexp_stringp(vf_sexp_cadr(u))
	    && (strcmp(TEX_FONT_MAPPING_MAG_ADJ, 
		       vf_sexp_get_cstring(vf_sexp_car(u))) == 0)){
	  mag_adj = atof(vf_sexp_get_cstring(vf_sexp_cadr(u)));
	  if (mag_adj <= 0)
	    mag_adj = 1.0;
	} else {
	  fprintf(stderr, 
		  "VFlib: %s %s for %s class default: ", 
		  "Unknown option in capability", 
		  VF_CAPE_TEX_FONT_MAPPING, FONTCLASS_NAME_TeX);
	  vf_sexp_pp_fp(u, stderr);
	}
      }
    }

    mag_x = font->mag_x * opt_mag;
    mag_y = font->mag_y * opt_mag;

    tex_map_name(mapped_name, sizeof(mapped_name), mapping, 
		 name_str, dpi_str, ext_str);
    if (((dpix = font->dpi_x) <= 0) || ((dpiy = font->dpi_y) <= 0)){
      dpix = v_default_tex_dpi;
      dpiy = v_default_tex_dpi;
    }
    if (vf_dbg_drv_texfonts == 1){
      printf(">> TeX font mapping: %s=>%s (driver:%s)\n", 
	     font_name, mapped_name, drvname);
      printf(">>     pt:%.3f, dpi:%d,%d, mag:%.3f,%.3f, mag_adj=%.3f\n", 
	     ptsize, dpix, dpiy, mag_x, mag_y, mag_adj); 
    }
    if (strcmp(drvname, "*") != 0){ 
      if (font->mode == 1){
	fid = vf_openfont1(mapped_name, drvname, dpix, dpiy, ptsize,
			   mag_x * mag_adj, mag_y * mag_adj);
      } else {
	fid = vf_openfont2(mapped_name, drvname, font->pixel_size, 
			   mag_x * mag_adj, mag_y * mag_adj);
      } 
    } else {
      if (font->mode == 1){
	fid = VF_OpenFont1(mapped_name, dpix, dpiy, ptsize,
			   mag_x * mag_adj, mag_y * mag_adj);
      } else {
	fid = VF_OpenFont2(mapped_name, font->pixel_size, 
			   mag_x * mag_adj, mag_y * mag_adj);
      }
    }

    if (fid >= 0)
      break;
  }  

  return fid;
}

Private double 
get_design_size_from_tfm(char *font_name, SEXP tfm_dirs, SEXP tfm_extensions)
{
  TFM     tfm;
  char   *tfm_path;
  double  ds;

  if (vf_dbg_drv_texfonts == 1)
    printf(">> TeX font mapper: search TFM path for %s\n", font_name);
  tfm_path = vf_tex_search_file_tfm(font_name, tfm_dirs, tfm_extensions);
  if (vf_dbg_drv_texfonts == 1){
    if (tfm_path != NULL){
      printf(">> TeX font mapper: TFM path: %s\n", tfm_path);
    } else {
      printf(">> TeX font mapper: TFM not found: %s\n", font_name);
      return -1;
    }
  }
  if (tfm_path == NULL)
    return -1;

  if ((tfm = vf_tfm_open(tfm_path)) == NULL){
    if (vf_dbg_drv_texfonts == 1)
      printf(">> TeX font mapper: failed to open TFM: %s\n", tfm_path);
    return -1;
  }

  ds = tfm->design_size;

  vf_tfm_free(tfm);
  vf_free(tfm_path);

  return ds;
}

Private void
tex_map_name(char *mapped_name, int n, char *mapping, 
	     char *name_str, char *dpi_str, char *ext_str)
{
  char  *p, *s, *t;
  int  r;

  p = mapping;
  s = mapped_name;
  r = n;
  while (*p != '\0'){
    if (r <= 0)
      break;
    if (*p != '%'){
      *(s++) = *(p++);
      r--;
    } else {
      p++;
      switch (*p){
      default:
	*(s++) = *p;
	r--;
	break;
      case '\0':
	*(s++) = '\0';
	r--;
	break;
      case 'f':
	/* %f ... font name of requested font name (e.g., "cmr10") */
	if (name_str == NULL)
	  break;
	for (t = name_str; (*t != '\0') && (r > 0); r--)
	  *(s++) = *(t++);
	break;
      case 'd':
	/* %d ... device resolution of requested font name (e.g., "300") */
	if (dpi_str == NULL)
	  break;
	for (t = dpi_str; (*t != '\0') && (r > 0); r--)
	  *(s++) = *(t++);
	break;
      case 'e':
	/* %d ... font name extension of requested font name (e.g., "pk") */
	if (ext_str == NULL)
	  break;
	for (t = ext_str; (*t != '\0') && (r > 0); r--)
	  *(s++) = *(t++);
	break;
      }
      p++;
    }
  }
  *s = '\0';
}


Glocal int
vf_tex_syntax_check_font_mapping(SEXP font_mapping)
{ 
  int   syntax_err, syntax_nerrs;
  SEXP  s, t, u, v;

  syntax_nerrs = 0;
  for (s = font_mapping; vf_sexp_consp(s); s = vf_sexp_cdr(s)){
    syntax_err = 0;
    u = t = vf_sexp_car(s); 
    /* e.g., u = ((pk "%f.pk") (tfm "%f.tfm") "cmr10" "cmti10") */
    for ( ; vf_sexp_consp(u); u = vf_sexp_cdr(u)){
      v = vf_sexp_car(u);
      if (vf_sexp_stringp(v))
	break;
      if (   !vf_sexp_listp(v) || (vf_sexp_length(v) < 2)
	  || !vf_sexp_stringp(vf_sexp_car(v)) 
	  || !vf_sexp_stringp(vf_sexp_cadr(v)) ){
	syntax_err = 1;
	syntax_nerrs++;
	goto fm_try_next;
      }
    }
    /* e.g., u = ("cmr10" "cmti10") */
    for ( ; vf_sexp_consp(u); u = vf_sexp_cdr(u)){
      v = vf_sexp_car(u);
      if (!vf_sexp_stringp(v)){
	syntax_err = 1;
	syntax_nerrs++;
	goto fm_try_next;
      }
    }
  fm_try_next:
    if (syntax_err != 0){
      fprintf(stderr, 
	      "VFlib: %s in capability %s of %s font class default: \n", 
	      "Syntax error", VF_CAPE_TEX_FONT_MAPPING, FONTCLASS_NAME_TeX);
      vf_sexp_pp_fp(t, stderr);
    }
  }
  
  return syntax_nerrs;
}



Private int
tex_close(VF_FONT font)
{
  return VF_CloseFont((int)font->private);
}

Private int
tex_get_metric1(VF_FONT font, long code_point, VF_METRIC1 metric, 
		double mag_x, double mag_y)
{
  VF_METRIC1  met;

  met = VF_GetMetric1((int)font->private, code_point, metric, mag_x, mag_y);
  if (met == NULL)
    return -1;
  return 0;
}

Private int
tex_get_fontbbx1(VF_FONT font, double mag_x, double mag_y,
		 double *w_p, double *h_p, double *xoff_p, double *yoff_p)
{
  return VF_GetFontBoundingBox1((int)font->private, 
			       mag_x, mag_y, w_p, h_p, xoff_p, yoff_p);
}

Private VF_BITMAP
tex_get_bitmap1(VF_FONT font, long code_point, 
		double mag_x, double mag_y)
{
  return  VF_GetBitmap1((int)font->private, code_point, mag_x, mag_y);
}

Private VF_OUTLINE
tex_get_outline(VF_FONT font, long code_point,
		double mag_x, double mag_y)
{
  return  VF_GetOutline((int)font->private, code_point, mag_x, mag_y);
}

Private int
tex_get_metric2(VF_FONT font, long code_point, VF_METRIC2 metric,
		double mag_x, double mag_y)
{
  VF_METRIC2 met;

  met = VF_GetMetric2((int)font->private, code_point, metric, mag_x, mag_y);
  if (met == NULL)
    return -1;
  return 0;
}

Private int
tex_get_fontbbx2(VF_FONT font, double mag_x, double mag_y,
		 int *w_p, int *h_p, int *xoff_p, int *yoff_p)
{
  return VF_GetFontBoundingBox2((int)font->private, 
				mag_x, mag_y, w_p, h_p, xoff_p, yoff_p);
}

Private VF_BITMAP
tex_get_bitmap2(VF_FONT font, long code_point, 
		double mag_x, double mag_y)
{
  return  VF_GetBitmap2((int)font->private, code_point, mag_x, mag_y);
}

Private char*
tex_get_font_prop(VF_FONT font, char *prop_name)
{
  return  VF_GetFontProp((int)font->private, prop_name);
}

Private int
tex_query_font_type(VF_FONT font, long code_point)
{
  return  VF_QueryFontType((int)font->private, code_point);
}





Glocal int
vf_tex_default_dpi(void)
{
  return v_default_tex_dpi;
}


Glocal int
vf_tex_fix_resolution(int dev_dpi, double mag)
{
  int   mdpi, fixed_mdpi, d, dpi_lo, dpi_hi;
  SEXP  s, t;

  if (dev_dpi <= 0)
    dev_dpi = v_default_tex_dpi;

  fixed_mdpi = mdpi = toint(dev_dpi * mag);

  if (default_resolution_corr == NULL)
    goto found;
  for (s = default_resolution_corr; vf_sexp_consp(s); s = vf_sexp_cdr(s)){
    t = vf_sexp_car(s);
    if (dev_dpi != atoi(vf_sexp_get_cstring(vf_sexp_car(t))))
      continue;
    for (t = vf_sexp_cdr(t); vf_sexp_consp(t); t = vf_sexp_cdr(t)){
      if ((d = atoi(vf_sexp_get_cstring(vf_sexp_car(t)))) < 1)
	continue;
      if (d == mdpi)
	goto found;
      dpi_lo = (1.0 - v_default_resolution_accu) * d; 
      dpi_hi = (1.0 + v_default_resolution_accu) * d; 
      if ((dpi_lo <= mdpi) && (mdpi <= dpi_hi)){
	fixed_mdpi = d;
	goto found;
      }
    }
  }

found:
  if (vf_dbg_drv_texfonts == 1)
    printf(">> TeX resolution correction: %d dpi x %f = %d  ==>  %d\n", 
	   dev_dpi, mag, toint(mdpi), fixed_mdpi);

  return fixed_mdpi;
}


Glocal char*
vf_tex_search_file_tfm(char *filename, SEXP dirs, SEXP exts)
     /* NOTE: CALLER OF THIS FUNCTION *SHOULD* RELEASE RETURN VALUE. */
{
  char  *name, *e, *path; 
  char  target_name[1024];
  SEXP  s;

  if (vf_dbg_drv_texfonts == 1)
    printf(">> TeX file search tfm: %s\n", filename);

  /* "cmr10.400pk" ==> "cmr10"  */
  if ((name = vf_path_base_core(filename)) == NULL)
    return NULL;

  if (dirs == NULL)
    dirs = default_tfm_dirs;

  if (exts == NULL)
    exts = default_tfm_extensions;

  path = NULL;
  if (exts == NULL){
    path = vf_search_file(name, -1, NULL, TRUE, 
			  FSEARCH_FORMAT_TYPE_TFM, dirs, NULL, NULL);
  } else {
    for (s = exts; vf_sexp_consp(s); s = vf_sexp_cdr(s)){ 
      e = vf_sexp_get_cstring(vf_sexp_car(s));
      if (*e == '.')
	e = e+1;		/*  ".tfm"  ==> "tfm" */
      sprintf(target_name, "%s.%s", name, e);	/* "cmr10" ==> "cmr10.tfm" */
      if (vf_dbg_drv_texfonts == 1)
	printf(">> TeX file search tfm: Checking %s\n", target_name);
      path = vf_search_file(target_name, -1, NULL, TRUE, 
			    FSEARCH_FORMAT_TYPE_TFM, dirs, NULL, NULL);
      if (path != NULL)
	break;
    }
  }
  vf_free(name);
  return path;
}

Glocal char*
vf_tex_search_file_glyph(char *filename, int implicit, int format,
			 SEXP dirs, int dpi, double mag, SEXP exts)
{
  SEXP    s;
  char    *path, *corename, *e, target_name[1024];
  int     mdpi; 

  if (vf_dbg_drv_texfonts == 1){
    printf(">> TeX file search glyph: %s\n", filename);
    printf(">>   dirs: ");
    if (dirs != NULL)
      vf_sexp_pp(dirs);
    else 
      printf("null\n");
    printf(">>   exts: ");
    if (exts != NULL)
      vf_sexp_pp(exts);
    else 
      printf("null\n");
  }

  if (exts == NULL)
    return NULL;

  mdpi = vf_tex_fix_resolution(dpi, mag);
#if 0
  printf("*** %d %.14f  %.14f  %d\n", dpi, mag, dpi*mag, mdpi);
#endif

  /* "cmr10.400pk" ==> "cmr10" */
  if ((corename = vf_path_base_core(filename)) == NULL)
    return NULL;
  if (vf_dbg_drv_texfonts == 1)
    printf(">>   core: %s\n", corename);

  path = NULL;
  if (implicit == 1){
    for (s = exts; vf_sexp_consp(s); s = vf_sexp_cdr(s)){ 
      e = vf_sexp_get_cstring(vf_sexp_car(s));
      if (*e == '.')
	e = e+1;     /* ".pk" ==> "pk" */
      sprintf(target_name, "%s.%d%s", corename, mdpi, e);
      path = vf_search_file(target_name, mdpi, corename, TRUE, 
			    format, dirs, NULL, NULL);
      if (path != NULL)
    	break;
    }
  } else {
    path = vf_search_file(filename, mdpi, corename, TRUE, 
			  format, dirs, NULL, NULL);
  }

  vf_free(corename);
  return path;
}

Glocal char*
vf_tex_search_file_misc(char *filename, int implicit, int format,
			SEXP dirs, SEXP exts)
{
  SEXP    s;
  char    *path, *corename, *e, target_name[1024];

  if (vf_dbg_drv_texfonts == 1)
    printf(">>  TeX file search file: %s\n", filename);

  path = NULL;
  corename = NULL;
  e = NULL;
  if (implicit == 1){
    corename = vf_path_base_core(filename);  /* "cmr10.400pk" ==> "cmr10"  */
    for (s = exts; vf_sexp_consp(s); s = vf_sexp_cdr(s)){ 
      e = vf_sexp_get_cstring(vf_sexp_car(s));
      if (*e == '.')
	e = e+1;     /* ".vf" ==> "vf" */
      sprintf(target_name, "%s.%s", corename, e);
      path = vf_search_file(target_name, 0, NULL, 
			    TRUE, format, dirs, NULL, NULL);
      if (path != NULL)
    	break;
    }
  } else {
    path = vf_search_file(filename, 0, NULL, TRUE, format, dirs, NULL, NULL);
  }

  vf_free(corename);
  return path;
}



Glocal int
vf_tex_parse_open_style(char *s, int def_value)
{
  int  val;

  if (s == NULL)
    return def_value;

  if (vf_strcmp_ci(s, TEX_OPEN_STYLE_REQUIRE_STR) == 0){
    val = TEX_OPEN_STYLE_REQUIRE;
  } else if (vf_strcmp_ci(s, TEX_OPEN_STYLE_TRY_STR) == 0){
    val = TEX_OPEN_STYLE_TRY;
  } else if (vf_strcmp_ci(s, TEX_OPEN_STYLE_NONE_STR) == 0){
    val = TEX_OPEN_STYLE_NONE;
  } else {
    fprintf(stderr, 
	    "VFlib Warning: Unknown open style: %s.\n", s);
    val = def_value;
  }

  return val;
}


Glocal int
vf_tex_parse_glyph_style(char *s, int def_value)
{
  int  val;

  if (s == NULL)
    return def_value;

  if (vf_strcmp_ci(s, TEX_GLYPH_STYLE_EMPTY_STR) == 0){
    val = TEX_GLYPH_STYLE_EMPTY;
  } else if (vf_strcmp_ci(s, TEX_GLYPH_STYLE_FILL_STR) == 0){
    val = TEX_GLYPH_STYLE_FILL;
  } else {
    fprintf(stderr, "VFlib Warning: Unknown glyph stype: %s\n", s);
    val = def_value;
  }

  return val;
}





/*
 * Reading a Number from file
 */
Glocal unsigned long
vf_tex_read_uintn(FILE* fp, int size)
{
  unsigned long  v;

  v = 0L;
  while (size >= 1){
    v = v*256L + (unsigned long)getc(fp);
    --size;
  }
  return v;
}

Glocal long
vf_tex_read_intn(FILE* fp, int size)
{
  long           v;

  v = (long)getc(fp) & 0xffL;
  if (v & 0x80L)
    v = v - 256L;
  --size;
  while (size >= 1){
    v = v*256L + (unsigned long)getc(fp);
    --size;
  }

  return v;
}

Glocal void
vf_tex_skip_n(FILE* fp, int size)
{
  while (size > 0){
    (void)getc(fp);
    --size;
  }
}

Glocal unsigned long
vf_tex_get_uintn(unsigned char *p, int size)
{
  unsigned long  v;

  v = 0L;
  while (size >= 1){
    v = v*256L + (unsigned long) *(p++);
    --size;
  }

  return v;
}

Glocal long
vf_tex_get_intn(unsigned char *p, int size)
{
  long           v;

  v = (long)*(p++) & 0xffL;
  if (v & 0x80L)
    v = v - 256L;
  --size;
  while (size >= 1){
    v = v*256L + (unsigned long) *(p++);
    --size;
  }

  return v;
}



/*EOF*/
