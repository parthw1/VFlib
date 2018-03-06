/*
 * drv_jtex.c - A font driver for ASCII Japanese TeX Kanji fonts.
 *
 *  4 Oct 1996  First version.
 * 17 Jan 1997  for VFlib 3.1
 * 26 Feb 1997  Added 'query_font_type'.
 *  4 Aug 1997  VFlib 3.3  Changed API.
 * 15 Feb 1998  VFlib 3.4  Changed API.
 * 21 Apr 1998  Added 'implicit-font-mapping-suffix' capability
 * 27 Jul 1998  Added a code to return some font properties by itself.
 * 07 Sep 1998  Added 'char-all' directive in adj file.
 * 24 Nov 1998  Added get_fontbbx1() and get_fontbbx2().
 * 20 Jan 1999  Changed to adj file searching.
 * 29 Jul 1999  Fixed bugs in jtex_met_adjustment().
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
#ifdef HAVE_SYS_PARAM_H
#include  <sys/param.h>
#endif
#include  <math.h>
#include  "VFlib-3_7.h"
#include  "VFsys.h"
#include  "vflibcap.h"
#include  "bitmap.h"
#include  "sexp.h"
#include  "str.h"
#include  "path.h"
#include  "vflpaths.h"
#include  "metric.h"
#include  "fsearch.h"
#include  "tfm.h"
#include  "jtex.h"

Private int vf_dbg_drv_ascii_jtex      = 0;
Private int vf_dbg_drv_ascii_jtex_dump = 0;
Private int vf_dbg_drv_ascii_jtex_adj  = 0;


#define KW_CHAR_TYPE             "char-type"
#define KW_CHAR_CODE             "char-code"
#define KW_CHAR_ALL              "char-all"

#define KW_SEMANTICS             "semantics"
#define KW_SEM_BITMAP               "bitmap-size"
#define KW_SEM_DESIGN               "design-size"
#define SEMANTICS_AUX_BITMAP        0
#define SEMANTICS_DESIGN_SIZE       1

#define KW_DIR                   "direction"
#define KW_DIR_HORIZONTAL           "horizontal"
#define KW_DIR_VERTICAL             "vertical"
#define DIR_HORIZONTAL              0
#define DIR_VERTICAL                1

#define KW_ROTATION              "rotation-semantics"
#define KW_ROT_PTEX                 "ptex"
#define KW_ROT_JISX0208             "jisx0208"
#define ROT_JISX0208                (1<<0)
#define ROT_PTEX                   ((1<<1) | ROT_JISX0208)



#define N_CC_CORR   64

struct s_met_adj_ct_corr {
  int     need_cc_corr;
  int     may_need_rotate;
  double  dx, dy;
};
struct s_met_adj_cc_corr {
  long    code_point;
  double  dx, dy;
};
typedef struct s_met_adj_cc_corr  *ADJ_CC_CORR;

struct s_met_adj_table {
  int   semantics;
  int   dir;
  int   rot_semantics;
  struct s_met_adj_ct_corr  ct_corr[JTEX_MAX_CHARTYPE];
  struct s_met_adj_ct_corr  ct_corr_all;
  int   n_cc_corr;
  struct s_met_adj_cc_corr *cc_corr;
};
typedef struct s_met_adj_table  *ADJ;


struct s_vert_rotation {
  int   code_point;
  int   angle;
  int   mirror;
  int   rot_semantics;
};
static struct s_vert_rotation VertCharInfo[] = {
  /* Code,  Rotation Angle, Mirror, Rotation in JISX0208 */
  { 0x213c, VF_BM_ROTATE_270, 1, ROT_JISX0208 }, /* Chou-On */
  { 0x213d, VF_BM_ROTATE_270, 0, ROT_PTEX },     /* Zenkaku-dash */
  { 0x213e, VF_BM_ROTATE_270, 0, ROT_PTEX },     /* Hyphen-dash */
  { 0x2141, VF_BM_ROTATE_270, 0, ROT_JISX0208 }, /* Nami-dash */
  { 0x2144, VF_BM_ROTATE_270, 0, ROT_JISX0208 }, /* Santen leader */
  { 0x2142, VF_BM_ROTATE_270, 0, ROT_PTEX },     /* Heikou */
  { 0x2143, VF_BM_ROTATE_270, 0, ROT_PTEX },     /* Tatesen */
  { 0x2145, VF_BM_ROTATE_270, 0, ROT_PTEX },     /* Niten leader */
  { 0x214a, VF_BM_ROTATE_270, 0, ROT_JISX0208 }, /* Syou-Kakko */
  { 0x214b, VF_BM_ROTATE_270, 0, ROT_JISX0208 }, /* Syou-Kakko */
  { 0x214c, VF_BM_ROTATE_270, 0, ROT_JISX0208 }, /* Kikkou-Kakko */
  { 0x214d, VF_BM_ROTATE_270, 0, ROT_JISX0208 }, /* Kikkou-Kakko */
  { 0x214e, VF_BM_ROTATE_270, 0, ROT_JISX0208 }, /* Kaku-Kakko */
  { 0x214f, VF_BM_ROTATE_270, 0, ROT_JISX0208 }, /* Kaku-Kakko */
  { 0x2150, VF_BM_ROTATE_270, 0, ROT_JISX0208 }, /* Chuu-Kakko */
  { 0x2151, VF_BM_ROTATE_270, 0, ROT_JISX0208 }, /* Chuu-Kakko */
  { 0x2152, VF_BM_ROTATE_270, 0, ROT_JISX0208 }, /* Yama-Kakko */
  { 0x2153, VF_BM_ROTATE_270, 0, ROT_JISX0208 }, /* Yama-Kakko */
  { 0x2154, VF_BM_ROTATE_270, 0, ROT_JISX0208 }, /* NijuuYama-Kakko */
  { 0x2155, VF_BM_ROTATE_270, 0, ROT_JISX0208 }, /* NijuuYama-Kakko */
  { 0x2156, VF_BM_ROTATE_270, 0, ROT_JISX0208 }, /* Kagi-Kakko */
  { 0x2157, VF_BM_ROTATE_270, 0, ROT_JISX0208 }, /* Kagi-Kakko */
  { 0x2158, VF_BM_ROTATE_270, 0, ROT_JISX0208 }, /* NijuuKagi-Kakko */
  { 0x2159, VF_BM_ROTATE_270, 0, ROT_JISX0208 }, /* NijuuKagi-Kakko */
  { 0x215a, VF_BM_ROTATE_270, 0, ROT_JISX0208 }, /* Sumitsuki-Kakko */
  { 0x215b, VF_BM_ROTATE_270, 0, ROT_JISX0208 }, /* Sumitsuki-Kakko */
  { 0x2161, VF_BM_ROTATE_270, 0, ROT_JISX0208 }, /* '=' sign */
  { -1, -1, -1, -1 }
};


struct s_font_jtex {
  char     *font_name;
  char     *kanji_font;
  int      kanji_font_id;
  double   kf_point_size;
  int      kf_pixel_size;
  double   kf_mag;
  char     *tfm_file;
  TFM      tfm;
  char     *adj_file;
  ADJ      adj;
  double   mode2_factor_x, mode2_factor_y;
  SEXP     props;
};
typedef struct s_font_jtex  *FONT_JTEX;


Private SEXP_STRING  default_map_suffix     = NULL;
Private SEXP_LIST    default_tfm_dirs       = NULL;
Private SEXP_LIST    default_tfm_extensions = NULL;
Private SEXP_ALIST   default_properties     = NULL;
Private SEXP_ALIST   default_variables      = NULL;
Private SEXP_STRING  default_debug_mode     = NULL;


Private int        jtex_create(VF_FONT,char*,char*,int,SEXP);
Private int        jtex_close(VF_FONT);
Private int        jtex_get_metric1(VF_FONT,long,VF_METRIC1,double,double);
Private int        jtex_get_metric2(VF_FONT,long,VF_METRIC2,double,double);
Private int        jtex_get_fontbbx1(VF_FONT,double,double,
				     double*,double*,double*,double*);
Private int        jtex_get_fontbbx2(VF_FONT,double,double, 
				     int*,int*,int*,int*);
Private VF_BITMAP  jtex_get_bitmap1(VF_FONT,long,double,double);
Private VF_BITMAP  jtex_get_bitmap2(VF_FONT,long,double,double);
Private VF_OUTLINE jtex_get_outline(VF_FONT,long,double,double);
Private int        jtex_query_font_type(VF_FONT,long);
Private char      *jtex_get_font_prop(VF_FONT,char*);
Private VF_BITMAP  jtex_rotate_bitmap(VF_BITMAP bm0, long code_point, 
				      FONT_JTEX font_jtex, int *need_free_p);
Private int        jtex_met_adjustment(VF_BITMAP,VF_METRIC2,long,
				       VF_FONT,FONT_JTEX,double,double);
Private ADJ        jtex_read_met_adjustment_file(FONT_JTEX,char*);
Private void       jtex_free_adj(ADJ adj);
Private void       jtex_met_cc_adjustment(ADJ adj, long code_point,
					  int char_type, 
					  double *fxp, double *fyp);
Private ADJ        realloc_cc_corr(ADJ adj);
Private char      *find_adj(char *fname);
Private void       release_font_jtex(FONT_JTEX);




Glocal int
VF_Init_Driver_JTEX(void)
{
  struct s_capability_table  ct[10];
  int  z;
  
  z = 0;
  /* VF_CAPE_JTEX_MAP_SUFIX */
  ct[z].cap = VF_CAPE_JTEX_MAP_SUFIX;      ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;         ct[z++].val = &default_map_suffix;
  /* VF_CAPE_TEX_TFM_DIRECTORIES */
  ct[z].cap = VF_CAPE_TEX_TFM_DIRECTORIES; ct[z].type = CAPABILITY_LIST;
  ct[z].ess = CAPABILITY_OPTIONAL;         ct[z++].val = &default_tfm_dirs;
  /* VF_CAPE_TEX_TFM_EXTENSIONS */
  ct[z].cap = VF_CAPE_TEX_TFM_EXTENSIONS;  ct[z].type = CAPABILITY_LIST;
  ct[z].ess = CAPABILITY_OPTIONAL;      ct[z++].val = &default_tfm_extensions;
  /* VF_CAPE_PROPERTIES */
  ct[z].cap = VF_CAPE_PROPERTIES;          ct[z].type = CAPABILITY_ALIST;
  ct[z].ess = CAPABILITY_OPTIONAL;          ct[z++].val = &default_properties;
  /* VF_CAPE_VARIABLE_VALUES */
  ct[z].cap = VF_CAPE_VARIABLE_VALUES;      ct[z].type = CAPABILITY_ALIST;
  ct[z].ess = CAPABILITY_OPTIONAL;          ct[z++].val = &default_variables;
  /* VF_CAPE_DEBUG */
  ct[z].cap = VF_CAPE_DEBUG;                ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;          ct[z++].val = &default_debug_mode;
  /* end */
  ct[z].cap = NULL; ct[z].type = 0; ct[z].ess = 0; ct[z++].val = NULL;


  if (vf_cap_GetParsedClassDefault(FONTCLASS_NAME_JTEX, ct, NULL, NULL) 
      == VFLIBCAP_PARSED_ERROR)
    return -1;

  if (vf_tex_init() < 0)
    return -1;

  if (getenv("VFLIB_DEBUG_ASCII_JTEX") != NULL)
    vf_dbg_drv_ascii_jtex = 1;

  if (getenv("VFLIB_DEBUG_ASCII_JTEX_DUMP") != NULL)
    vf_dbg_drv_ascii_jtex_dump = 1;

  if (getenv("VFLIB_DEBUG_ASCII_JTEX_ADJ") != NULL)
    vf_dbg_drv_ascii_jtex_adj = 1;

  VF_InstallFontDriver(FONTCLASS_NAME_JTEX, (DRIVER_FUNC_TYPE)jtex_create);

  return 0;
}


Private int
jtex_create(VF_FONT font, char *font_class, 
	    char *font_name, int implicit, SEXP entry)
{
  FONT_JTEX   font_jtex;
  double      point_size, dpi, dpix = 0.0, dpiy = 0.0;
  int         pixel_size;
  char        *mapped_font, *name_core, *suffix, *tfm_path, *p;
  SEXP        entry2, cap_kanji, cap_kf_point, cap_kf_pixel, cap_kf_mag;
  SEXP        cap_tfm, cap_adj, cap_props;
  SEXP        tfm_ext;
  struct s_capability_table  ct[10];
  int  z;

  z = 0;
  /* VF_CAPE_FONT_CLASS */
  ct[z].cap = VF_CAPE_FONT_CLASS;           ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_ESSENTIAL;         ct[z++].val = NULL;
  /* VF_CAPE_JTEX_KANJI_FONT */
  ct[z].cap = VF_CAPE_JTEX_KANJI_FONT;      ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;          ct[z++].val = &cap_kanji;
  /* VF_CAPE_JTEX_KF_POINT_SIZE */
  ct[z].cap = VF_CAPE_JTEX_KF_POINT_SIZE;   ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;          ct[z++].val = &cap_kf_point;
  /* VF_CAPE_JTEX_KF_PIXEL_SIZE */
  ct[z].cap = VF_CAPE_JTEX_KF_PIXEL_SIZE;   ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;          ct[z++].val = &cap_kf_pixel;
  /* VF_CAPE_JTEX_KF_MAG */
  ct[z].cap = VF_CAPE_JTEX_KF_MAG;          ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;          ct[z++].val = &cap_kf_mag;
  /* VF_CAPE_JTEX_TFM_FILE */
  ct[z].cap = VF_CAPE_JTEX_TFM_FILE;        ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;          ct[z++].val = &cap_tfm;
  /* VF_CAPE_JTEX_ADJUSTMENT_FILE */
  ct[z].cap = VF_CAPE_JTEX_ADJUSTMENT_FILE; ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;          ct[z++].val = &cap_adj;
  /* VF_CAPE_PROPERTIES */
  ct[z].cap = VF_CAPE_PROPERTIES;           ct[z].type = CAPABILITY_ALIST;
  ct[z].ess = CAPABILITY_OPTIONAL;          ct[z++].val = &cap_props;
  /* end */
  ct[z].cap = NULL; ct[z].type = 0; ct[z].ess = 0; ct[z++].val = NULL;


  font_jtex = NULL;
  mapped_font = NULL;
  name_core = NULL;
  tfm_path = NULL;
  entry2 = NULL;

  if (vf_dbg_drv_ascii_jtex == 1){
    printf(">>VFlib ascii jtex: open request %s\n", font_name);
  }

  if (implicit == 1){
    /* font name mapping: 
     *   requested font: "min10.400pk" or "min10.pk"
     *    ==>  "min10XXX", where XXX is a suffix (e.g., ".pk")
     */
    suffix = "";
    if ((default_map_suffix != NULL) 
	&& (vf_sexp_get_cstring(default_map_suffix) != NULL)){
      suffix = vf_sexp_get_cstring(default_map_suffix);
    }
    if ((name_core = vf_path_base_core(font_name)) == NULL)
      return -1;
    ALLOCN_IF_ERR(mapped_font, char, strlen(name_core)+strlen(suffix)+1){
      vf_free(name_core);
      return -1;
    }
    sprintf(mapped_font, "%s%s", name_core, suffix);
    if (vf_dbg_drv_ascii_jtex == 1){
      printf(">>VFlib ascii jtex font mapping:  %s ==> %s\n", 
	     font_name, mapped_font);
    }
    if (strcmp(mapped_font, font_name) == 0){        /* a loop */
      vf_free(name_core);
      vf_free(mapped_font);
      return -1;
    }

    /* Parse font name.  Formats of file names that this routine supports:
     *   "cmr10.300XX" - A "cmr10" font for 300 dpi.
     *   "cmr10.XX"    - A "cmr10" font. Dpi value is default value.
     *   "cmr10"       -   ditto.
     * ("XX" can be any string such as "pk", "gf", and "tfm".)
     */
    if (font->mode == 1){
      p = vf_index(font_name, '.');
      dpi = font->dpi_y;
      if ((p != NULL) && (isdigit((int)*(p+1)))){ /* "cmr10.300gf" */
	if (dpi < 0)
	  dpi = (double)atoi(p+1);
      } else {			/* "cmr10" or "cmr10.gf" */
	if (dpi < 0)
	  dpi = (double)vf_tex_default_dpi();
      }
      dpix = dpi * (double)font->dpi_x / (double)font->dpi_y;
      dpiy = dpi;
    }

    if ((entry2 = vf_cap_GetFontEntry(mapped_font)) == NULL){
      vf_error = VF_ERR_NO_FONT_ENTRY;
      vf_free(name_core);
      vf_free(mapped_font);
      return -1;
    }
    if (vf_cap_GetParsedFontEntry(entry2, mapped_font, ct, 
				  default_variables, NULL)
	== VFLIBCAP_PARSED_ERROR){
      vf_sexp_free(&entry2);
      vf_free(name_core);
      vf_free(mapped_font);
      return -1;
    }
  } else {
    if (vf_cap_GetParsedFontEntry(entry, font_name, ct, 
				  default_variables, NULL)
	== VFLIBCAP_PARSED_ERROR){
      return -1;
    }
    dpi = font->dpi_y;
    if (dpi < 0)
      dpi = (double)vf_tex_default_dpi();
    dpix = dpi * (double)font->dpi_x / (double)font->dpi_y;
    dpiy = dpi;
  }

  font->font_type       = VF_FONT_TYPE_UNDEF; /* call jtex_query_font_type() */
  font->get_metric1     = jtex_get_metric1;
  font->get_metric2     = jtex_get_metric2;
  font->get_fontbbx1    = jtex_get_fontbbx1;
  font->get_fontbbx2    = jtex_get_fontbbx2;
  font->get_bitmap1     = jtex_get_bitmap1;
  font->get_bitmap2     = jtex_get_bitmap2;
  font->get_outline     = jtex_get_outline;
  font->get_font_prop   = jtex_get_font_prop;
  font->query_font_type = jtex_query_font_type;
  font->close           = jtex_close;

  ALLOC_IF_ERR(font_jtex, struct s_font_jtex){
    vf_error = VF_ERR_NO_MEMORY;
    vf_sexp_free(&entry2);
    vf_free(name_core);
    vf_free(mapped_font);
    return -1;
  }

  font_jtex->font_name      = vf_strdup(font_name);
  font_jtex->kanji_font     = NULL;
  font_jtex->kanji_font_id  = -1;
  font_jtex->kf_point_size  = -1;
  font_jtex->kf_pixel_size  = -1;
  font_jtex->kf_mag         = 1.0;
  font_jtex->tfm_file       = NULL;
  font_jtex->tfm            = NULL;
  font_jtex->adj_file       = NULL;
  font_jtex->adj            = NULL;
  font_jtex->mode2_factor_x = -1;
  font_jtex->mode2_factor_y = -1;
  font_jtex->props          = NULL;

  if (default_tfm_extensions == NULL)
    default_tfm_extensions = vf_sexp_cstring2list(DEFAULT_EXTENSIONS_TFM);
  if (cap_kanji != NULL)
    font_jtex->kanji_font = vf_strdup(vf_sexp_get_cstring(cap_kanji));
  else 
    font_jtex->kanji_font = vf_path_base_core(font_name);
  if (font_jtex->kanji_font == NULL){
    vf_error = VF_ERR_NO_AUX_FONT_NAME;
    vf_sexp_free(&entry2);
    vf_free(name_core);
    vf_free(mapped_font);
    vf_free(font_jtex);
    return -1;
  }
  if (cap_kf_point != NULL)
    font_jtex->kf_point_size = atof(vf_sexp_get_cstring(cap_kf_point));
  if (cap_kf_pixel != NULL)
    font_jtex->kf_pixel_size = atoi(vf_sexp_get_cstring(cap_kf_pixel));
  if (cap_kf_mag != NULL)
    font_jtex->kf_mag = atof(vf_sexp_get_cstring(cap_kf_mag));
  if (cap_tfm != NULL)
    font_jtex->tfm_file = vf_strdup(vf_sexp_get_cstring(cap_tfm));
  else
    font_jtex->tfm_file = vf_path_base_core(font_name);
  if (cap_adj != NULL)
    font_jtex->adj_file = vf_strdup(vf_sexp_get_cstring(cap_adj));
  font_jtex->props = cap_props;

  if (vf_dbg_drv_ascii_jtex == 1){
    printf(">>VFlib ascii jtex font name:  %s\n",   font_jtex->font_name);
    printf(">>VFlib ascii jtex kanji font: %s\n",   font_jtex->kanji_font);
    printf(">>VFlib ascii jtex tfm file:   %s\n",   font_jtex->tfm_file);
    printf(">>VFlib ascii jtex adj file:   %s\n",   font_jtex->adj_file);
    printf(">>VFlib ascii jtex kf point:   %.3f\n", font_jtex->kf_point_size);
    printf(">>VFlib ascii jtex kf pixel:   %d\n",   font_jtex->kf_pixel_size);
    printf(">>VFlib ascii jtex kf mag:     %.3f\n", font_jtex->kf_mag);
  }

  /* Open aux kanji font */
  if (font->mode == 1){
    if ((point_size = font->point_size) < 0)
      point_size = font_jtex->kf_point_size;
    font_jtex->kanji_font_id
      = VF_OpenFont1(font_jtex->kanji_font, dpix, dpiy, point_size, 
		     font->mag_x * font_jtex->kf_mag, 
		     font->mag_y * font_jtex->kf_mag);
  } else if (font->mode == 2){
    if ((pixel_size = font->pixel_size) < 0)
      pixel_size = font_jtex->kf_pixel_size;
    font_jtex->kanji_font_id 
      = VF_OpenFont2(font_jtex->kanji_font, pixel_size,
		     font->mag_x * font_jtex->kf_mag,
		     font->mag_y * font_jtex->kf_mag);
  }
  if (vf_dbg_drv_ascii_jtex == 1)
    printf(">>VFlib ascii jtex:  Aux Kanji font ID: %d\n",
	   font_jtex->kanji_font_id);

  if (font_jtex->kanji_font_id < 0){
    vf_error = VF_ERR_NO_FONT_ENTRY;
    goto Error;
  }

  /* Search TFM */
  tfm_ext = (cap_tfm != NULL) ? NULL : default_tfm_extensions;
  tfm_path = vf_tex_search_file_tfm(font_jtex->tfm_file,
				    default_tfm_dirs, tfm_ext);
  if (vf_dbg_drv_ascii_jtex == 1)
    printf(">>VFlib ascii jtex:  TFM path: %s ==> %s\n",
	   font_jtex->tfm_file, (tfm_path==NULL)?"(not found)":tfm_path);
  if (tfm_path == NULL)
    goto Error;
  if ((font_jtex->tfm = vf_tfm_open(tfm_path)) == NULL)
    goto Error;

  /* Check if it is a JFM (Japanese Font Metric) format */
  if (font_jtex->tfm->type != METRIC_TYPE_JFM){
    vf_error = VF_ERR_NOT_JFM;
    goto Error;
  }

  /* Read ADJ file */
  if (cap_adj != NULL){
    font_jtex->adj_file = vf_strdup(vf_sexp_get_cstring(cap_adj));
    if (font_jtex->adj_file != NULL)
      font_jtex->adj 
	= jtex_read_met_adjustment_file(font_jtex, font_jtex->adj_file);
    if (font_jtex->adj == NULL){
      fprintf(stderr, "VFlib warning: Can't read: %s\n", font_jtex->adj_file);
    }
  } else {
    fprintf(stderr, "VFlib warning: no ADJ file.\n");
  }

  if (font->mode == 2){
    p = VF_GetFontProp(font_jtex->kanji_font_id, "PIXEL_SIZE");
    if (p != NULL){
      font_jtex->mode2_factor_x
	= font->mag_x * atof(p) / font_jtex->tfm->design_size;
      font_jtex->mode2_factor_y
	= font->mag_y * atof(p) / font_jtex->tfm->design_size;
#if 0
      printf("*** pixel_size:%.3f  design_size: %.3f\n", 
	     atof(p), font_jtex->tfm->design_size);
      printf("*** factor_x:%.3f  factor_y: %.3f\n", 
	     font_jtex->mode2_factor_x, font_jtex->mode2_factor_y);
#endif
    }
  }    

  font->private = font_jtex;

  vf_free(mapped_font);
  vf_free(name_core);
  vf_free(tfm_path);
  if (implicit == 0){
    vf_sexp_free4(&cap_kanji, &cap_kf_point, &cap_kf_pixel, &cap_kf_mag);
    vf_sexp_free2(&cap_tfm, &cap_adj);
  }
  vf_sexp_free(&entry2);
  return 0;


Error:
  release_font_jtex(font_jtex);
  vf_free(mapped_font);
  vf_free(name_core);
  vf_free(tfm_path);
  if (implicit == 0){
    vf_sexp_free4(&cap_kanji, &cap_kf_point, &cap_kf_pixel, &cap_kf_mag);
    vf_sexp_free2(&cap_tfm, &cap_adj);
    vf_sexp_free(&cap_props);
  }
  vf_sexp_free(&entry2);
  return -1;
}


Private void
release_font_jtex(FONT_JTEX font_jtex)
{
  if (font_jtex != NULL){
    vf_free(font_jtex->font_name);
    vf_free(font_jtex->kanji_font);
    if (font_jtex->kanji_font_id >= 0)
      VF_CloseFont(font_jtex->kanji_font_id);
    vf_free(font_jtex->tfm_file);
    vf_tfm_free(font_jtex->tfm);
    vf_free(font_jtex->adj_file); 
    jtex_free_adj(font_jtex->adj);
    vf_sexp_free1(&font_jtex->props);
    vf_free(font_jtex); 
  }
}


Private int
jtex_close(VF_FONT font)
{
  FONT_JTEX    font_jtex;

  if ((font_jtex = (FONT_JTEX)font->private) == NULL){
    fprintf(stderr, "VFlib: internal error in jtex_close()\n");
    abort();
  }

  release_font_jtex(font_jtex);
  return 0; 
}


Private int
jtex_get_metric1(VF_FONT font, long code_point, VF_METRIC1 metric, 
		 double mag_x, double mag_y)
{
  FONT_JTEX    font_jtex;
  double       ds, dr;

  if (  (metric == NULL)
      || ((font_jtex = (FONT_JTEX)font->private) == NULL) ){
    fprintf(stderr, "VFlib internal error in jtex_get_metric1()\n");
    abort();
  }

  if (vf_tfm_metric(font_jtex->tfm, code_point, metric) == NULL)
    return -1;

  ds = font_jtex->tfm->design_size;
  dr = 1.0;
  if (font->point_size > 0)
    dr = font->point_size / ds;

  metric->bbx_width  *= mag_x * font->mag_x;
  metric->bbx_height *= mag_y * font->mag_y;
  metric->off_x      *= mag_x * font->mag_x;
  metric->off_y      *= mag_y * font->mag_y;
  metric->mv_x       *= mag_x * font->mag_x * dr;
  metric->mv_y       *= mag_y * font->mag_y * dr;

  return 0;
}

Private int
jtex_get_fontbbx1(VF_FONT font, double mag_x, double mag_y,
		 double *w_p, double *h_p, double *xoff_p, double *yoff_p)
{
  FONT_JTEX    font_jtex;

  if ((font_jtex = (FONT_JTEX)font->private) == NULL){
    fprintf(stderr, "VFlib internal error in jtex_get_fontbbx1()\n");
    abort();
  }

  *w_p    = font_jtex->tfm->font_bbx_w * mag_x * font->mag_x;
  *h_p    = font_jtex->tfm->font_bbx_h * mag_y * font->mag_y;
  *xoff_p = font_jtex->tfm->font_bbx_xoff * mag_x * font->mag_x;
  *yoff_p = font_jtex->tfm->font_bbx_yoff * mag_y * font->mag_y;

  return 0;
}

Private VF_BITMAP
jtex_get_bitmap1(VF_FONT font, long code_point,
		 double mag_x, double mag_y)
{
  FONT_JTEX     font_jtex;
  VF_BITMAP     bm_kanji, bm;
  int           need_free;
  double        dpi_x, dpi_y, mx, my;
  struct vf_s_metric1  met1; 
  struct vf_s_metric2  met2; 

  if ((font_jtex = (FONT_JTEX)font->private) == NULL){
    fprintf(stderr, "VFlib: Internal error in jtex_get_bitmap1().\n");
    abort();
  }

  if (font_jtex->kanji_font_id < 0){
    vf_error = VF_ERR_ILL_CODE_POINT;
    return NULL;
  }

  bm_kanji = VF_GetBitmap1(font_jtex->kanji_font_id, code_point, mag_x, mag_y);
  if (bm_kanji == NULL){
#if 1
    bm_kanji = vf_alloc_bitmap(1, 1);
#else
    return NULL;
#endif
  }

  bm = jtex_rotate_bitmap(bm_kanji, code_point, font_jtex, &need_free);
  if (bm == NULL){
    VF_FreeBitmap(bm_kanji);
    return NULL;
  }

  if (jtex_get_metric1(font, code_point, &met1, mag_x, mag_y) < 0){
    VF_FreeBitmap(bm_kanji);
    if (need_free == 1)
      VF_FreeBitmap(bm);
    return NULL;
  }

  if (((dpi_x = font->dpi_x) < 0) || ((dpi_y = font->dpi_y) < 0)){
    dpi_x = vf_tex_default_dpi();
    dpi_y = vf_tex_default_dpi();
  }
  mx = dpi_x/72.27;
  my = dpi_y/72.27;

  met2.bbx_width  = bm->bbx_width;
  met2.bbx_height = bm->bbx_height;
  met2.off_x      = bm->off_x;
  met2.off_y      = bm->off_y;
  met2.mv_x       = toint(met1.mv_x * mx);
  met2.mv_y       = toint(met1.mv_y * my);

  if (vf_dbg_drv_ascii_jtex_dump == 1){
    printf(">>* ascii_jtex: get bitmap 1\n");
    printf("  CharCode: 0x%04lx  (char-type: %d)\n", 
	   code_point, vf_tfm_jfm_chartype(font_jtex->tfm, (UINT4)code_point));
    printf(">>  metric 1\n");
    vf_dump_metric1(&met1);
    printf(">>  metric 2\n");
    vf_dump_metric2(&met2);
    printf(">>  bitmap (orig)\n");
    VF_DumpBitmap(bm);
  }

  jtex_met_adjustment(bm, &met2, code_point, font, font_jtex, mag_x, mag_y);

  if (vf_dbg_drv_ascii_jtex_dump == 1){
    printf(">>  bitmap (adj)\n");
    VF_DumpBitmap(bm);
  }

  if (need_free == 1){
    VF_FreeBitmap(bm_kanji);  /* bm is a new object, we don't need bm_kanji */
  } else {
    ; /*empty*/               /* bm and bm_kanji points to the same object.*/
  }

  return bm;
}


Private VF_OUTLINE
jtex_get_outline(VF_FONT font, long code_point,
		 double mag_x, double mag_y)
{
  FONT_JTEX     font_jtex;

  if ((font_jtex = (FONT_JTEX)font->private) == NULL){
    fprintf(stderr, "VFlib: Internal error in jtex_get_outline().\n");
    abort();
  }

  if (font_jtex->kanji_font_id < 0){
    vf_error = VF_ERR_ILL_CODE_POINT;
    return NULL;
  }

  /**
  ** Metric correction is not supported. Sorry.
  **/

  return  VF_GetOutline(font_jtex->kanji_font_id, code_point, mag_x, mag_y);
}


Private int
jtex_get_metric2(VF_FONT font, long code_point, VF_METRIC2 metric,
		 double mag_x, double mag_y)
{
  FONT_JTEX   font_jtex;
  TFM         tfm;
  struct vf_s_metric1  met1;

  if (   (metric == NULL)
      || ((font_jtex = (FONT_JTEX)font->private) == NULL)
      || ((tfm = font_jtex->tfm) == NULL) ){
    fprintf(stderr, "VFlib internal error in htex_get_metric2()\n");
    abort();
  }

  if (vf_tfm_metric(tfm, code_point, &met1) == NULL)
    return -1;

  metric->bbx_width  = toint(met1.bbx_width  * mag_x * font->mag_x);
  metric->bbx_height = toint(met1.bbx_height * mag_y * font->mag_y);
  metric->off_x      = toint(met1.off_x * mag_x * font->mag_x);
  metric->off_y      = toint(met1.off_y * mag_y * font->mag_y);
  metric->mv_x       = toint(met1.mv_x  * mag_x * font->mag_x);
  metric->mv_y       = toint(met1.mv_y  * mag_y * font->mag_y);

  return 0;
}


Private int
jtex_get_fontbbx2(VF_FONT font, double mag_x, double mag_y,
		  int *w_p, int *h_p, int *xoff_p, int *yoff_p)
{
  FONT_JTEX    font_jtex;

  if ((font_jtex = (FONT_JTEX)font->private) == NULL){
    fprintf(stderr, "VFlib internal error in jtex_get_fontbbx2()\n");
    abort();
  }

  *w_p    = toint(font_jtex->tfm->font_bbx_w * mag_x * font->mag_x);
  *h_p    = toint(font_jtex->tfm->font_bbx_h * mag_y * font->mag_y);
  *xoff_p = toint(font_jtex->tfm->font_bbx_xoff * mag_x * font->mag_x);
  *yoff_p = toint(font_jtex->tfm->font_bbx_yoff * mag_y * font->mag_y);

  return 0;
}

Private VF_BITMAP
jtex_get_bitmap2(VF_FONT font, long code_point, 
		 double mag_x, double mag_y)
{
  FONT_JTEX     font_jtex;
  VF_BITMAP     bm_kanji, bm;
  int           need_free;
  struct vf_s_metric1  met1; 
  struct vf_s_metric2  met2; 

  if ((font_jtex = (FONT_JTEX)font->private) == NULL){
    fprintf(stderr, "VFlib: Internal error in jtex_get_bitmap2().\n");
    abort();
  }

  if (font_jtex->kanji_font_id < 0){
    vf_error = VF_ERR_ILL_CODE_POINT;
    return NULL;
  }

  bm_kanji = VF_GetBitmap2(font_jtex->kanji_font_id, code_point, mag_x, mag_y);
  if (bm_kanji == NULL){
#if 1
    bm_kanji = vf_alloc_bitmap(1, 1);
#else
    return NULL;
#endif
  }

  bm = jtex_rotate_bitmap(bm_kanji, code_point, font_jtex, &need_free);
  if (bm == NULL){
    VF_FreeBitmap(bm_kanji);
    return NULL;
  }

  if (vf_tfm_metric(font_jtex->tfm, code_point, &met1) == NULL){
    VF_FreeBitmap(bm_kanji);
    if (need_free == 1)
      VF_FreeBitmap(bm);
    return NULL;
  }

  met2.bbx_width  = bm->bbx_width;
  met2.bbx_height = bm->bbx_height;
  met2.off_x      = bm->off_x;
  met2.off_y      = bm->off_y;
  if ((font_jtex->mode2_factor_x > 0) && (font_jtex->mode2_factor_x > 0)){
    met2.mv_x     = met1.mv_x * font_jtex->mode2_factor_x;
    met2.mv_y     = met1.mv_y * font_jtex->mode2_factor_y;
  } else {
    met2.mv_x     = bm->mv_x;
    met2.mv_y     = bm->mv_y;
  }

  if (vf_dbg_drv_ascii_jtex_dump == 1){
    printf(">>* ascii_jtex: get bitmap 2\n");
    printf(">>  metric\n");
    vf_dump_metric2(&met2);
    printf(">>  bitmap (orig)\n");
    VF_DumpBitmap(bm);
  }

  jtex_met_adjustment(bm, &met2, code_point, font, font_jtex, mag_x, mag_y);

  if (vf_dbg_drv_ascii_jtex_dump == 1){
    printf(">>  bitmap (adj)\n");
    VF_DumpBitmap(bm);
  }

  if (need_free == 1){
    VF_FreeBitmap(bm_kanji);  /* bm is a new object, we don't need bm_kanji */
  } else {
    ; /*empty*/               /* bm and bm_kanji points to the same object.*/
  }

  return bm;
}


Private char*
jtex_get_font_prop(VF_FONT font, char *prop_name)
{
  FONT_JTEX  font_jtex;
  double     dpi_x, dpi_y;
  SEXP       v;
  char       *r;

  if ((font_jtex = (FONT_JTEX)font->private) == NULL){
    fprintf(stderr, "VFlib: Internal error in jtex_get_font_prop().\n");
    abort();
  }
  if (font_jtex->kanji_font_id < 0){
    return NULL;
  }

  if ((v = vf_sexp_assoc(prop_name, font_jtex->props)) != NULL){
    if ((r = vf_strdup(vf_sexp_get_cstring(vf_sexp_cadr(v)))) == NULL){
      vf_error = VF_ERR_NO_MEMORY;
      return NULL;
    }
    return r;
  } else if ((v = vf_sexp_assoc(prop_name, default_properties)) != NULL){
    if ((r = vf_strdup(vf_sexp_get_cstring(vf_sexp_cadr(v)))) == NULL){
      vf_error = VF_ERR_NO_MEMORY;
      return NULL;
    }
    return r;
  }

  if (((dpi_x = font->dpi_x)<=0) || ((dpi_y = font->dpi_y)<=0)){
    dpi_x = vf_tex_default_dpi();
    dpi_y = vf_tex_default_dpi();
  }

#if 0
  if (font->mode == 1){
    if ((ps = font->point_size) < 0)
      ps = font_jtex->tfm->design_size;
    ps = ps * font->mag_y;
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
	sprintf(str, "%d", toint(font_jtex->tfm->design_size 
				 * 10.0 * font->mag_y));
	return vf_strdup(str);
      }
      ps = ps * font->mag_y; 
      sprintf(str, "%d", toint(ps * 10.0 * 72.27 / dpi_y));
      return vf_strdup(str);
    } else if (strcmp(prop_name, "PIXEL_SIZE") == 0){
      if ((ps = font->pixel_size) < 0)
	ps = font_jtex->tfm->design_size * dpi_y / 72.27;
      ps = ps * font->mag_y;
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
#endif

  return VF_GetFontProp(font_jtex->kanji_font_id, prop_name);
}


Private int
jtex_query_font_type(VF_FONT font, long code_point)
{
  FONT_JTEX  font_jtex;

  if ((font_jtex = (FONT_JTEX)font->private) == NULL){
    fprintf(stderr, "VFlib: Internal error in jtex_get_font_prop().\n");
    abort();
  }

  return VF_QueryFontType(font_jtex->kanji_font_id, code_point);
}



Private VF_BITMAP
jtex_rotate_bitmap(VF_BITMAP bm0, long code_point, 
		   FONT_JTEX font_jtex, int *need_free_p)
{
  VF_BITMAP  bm, bm_mirror;
  ADJ        adj;
  int        ctype, i;

  *need_free_p = 0;
  if (((adj = font_jtex->adj) == NULL)
      || (font_jtex->tfm->type_aux != METRIC_TYPE_JFM_AUX_V))
    return bm0;

  ctype = vf_tfm_jfm_chartype(font_jtex->tfm, (UINT4)code_point);
  if (adj->ct_corr[ctype].may_need_rotate == FALSE)
    return bm0;

  for (i = 0; VertCharInfo[i].code_point > 0; i++){
    if ((VertCharInfo[i].code_point == code_point)
	&& ((VertCharInfo[i].rot_semantics & adj->rot_semantics) != 0)){
      if ((bm = VF_RotatedBitmap(bm0, VertCharInfo[i].angle)) == NULL)
	return NULL;
      if (VertCharInfo[i].mirror == 0){
	*need_free_p = 1;
	return bm;
      }
      if ((bm_mirror = VF_ReflectedBitmap(bm, 1, 0)) == NULL){
	VF_FreeBitmap(bm);
	return NULL;
      }
      VF_FreeBitmap(bm);
      *need_free_p = 1;
      return bm_mirror;
    }
  }

  *need_free_p = 0;
  return bm0;
}



Private int
jtex_met_adjustment(VF_BITMAP bm, VF_METRIC2 met2, long code_point,
		    VF_FONT font, FONT_JTEX font_jtex, 
		    double mag_x, double mag_y)
{
  ADJ        adj;
  int        ctype, dx, dy;
  double     dpix, dpiy, fdx, fdy, ds, dr; 

  bm->mv_x = met2->mv_x; 
  bm->mv_y = met2->mv_y; 

  if ((adj = font_jtex->adj) == NULL){
    if (vf_dbg_drv_ascii_jtex_adj == 1)
      printf(">>VFlib ascii jtex ADJ: none\n");
    return 0;
  }

  ctype = vf_tfm_jfm_chartype(font_jtex->tfm, (UINT4)code_point);
  fdx = adj->ct_corr[ctype].dx + adj->ct_corr_all.dx;
  fdy = adj->ct_corr[ctype].dy + adj->ct_corr_all.dy;
  if (adj->ct_corr[ctype].need_cc_corr)
    jtex_met_cc_adjustment(adj, code_point, ctype, &fdx, &fdy);

  ds = font_jtex->tfm->design_size;
  dr = 1.0;
  if (font->point_size > 0)
    dr = font->point_size / ds;
  
  dx = 0; 
  dy = 0; 

  switch (font->mode){
  case 1:
    if (adj->semantics == SEMANTICS_AUX_BITMAP){
      dx = met2->bbx_width  * fdx * mag_x * font->mag_x;
      dy = met2->bbx_height * fdy * mag_y * font->mag_y;
    } else if (adj->semantics == SEMANTICS_DESIGN_SIZE){
      if (((dpix = font->dpi_x) < 0) || ((dpiy = font->dpi_y) < 0)){
	dpix = vf_tex_default_dpi();
	dpiy = vf_tex_default_dpi();
      }
      dx = (dpix/72.27) * ds * dr * fdx * mag_x * font->mag_x;
      dy = (dpiy/72.27) * ds * dr * fdy * mag_y * font->mag_y;
    }
    break;
  case 2:
  default:
    if (adj->semantics == SEMANTICS_AUX_BITMAP){
      dx = met2->bbx_width  * fdx * mag_x * font->mag_x;
      dy = met2->bbx_height * fdy * mag_y * font->mag_y;
    } else if (adj->semantics == SEMANTICS_DESIGN_SIZE){
      dx = font_jtex->mode2_factor_x * ds * dr * fdx * mag_x * font->mag_x;
      dy = font_jtex->mode2_factor_y * ds * dr * fdy * mag_y * font->mag_y;
    }
    break;
  }
  bm->off_x += dx;
  bm->off_y += dy;

  if (vf_dbg_drv_ascii_jtex_adj == 1){
    printf(">>VFlib ascii jtex ADJ 0x%x CharType %d %.3f,%.3f BBX[%d,%d]\n",
	   (int)code_point, ctype, 
	   adj->ct_corr[ctype].dx, adj->ct_corr[ctype].dy, 
	   met2->bbx_width, met2->bbx_height);
  }

  return 0;
}

Private void
jtex_met_cc_adjustment(ADJ adj, long code_point, int char_type, 
		       double *fxp, double *fyp)
{
  int  i, last;

  last = adj->n_cc_corr - 1;
  adj->cc_corr[last].code_point = code_point;
  adj->cc_corr[last].dx = 0.0;
  adj->cc_corr[last].dy = 0.0;

  for (i = 0; adj->cc_corr[i].code_point != code_point; i++)
    ;
  
  if (i != last){
    if (vf_dbg_drv_ascii_jtex_adj == 1)
      printf(">>VFlib: ascii jtex:   cc adj: 0x%04lx  %.3f %.3f\n",
	     code_point, adj->cc_corr[i].dx, adj->cc_corr[i].dy);
    /* Override! */
    *fxp = adj->cc_corr[i].dx;
    *fyp = adj->cc_corr[i].dy;
  }
}


Private ADJ
jtex_read_met_adjustment_file(FONT_JTEX font_jtex, char* file_name)
{
  FILE    *fp;
  ADJ     adj;
  SEXP    s, s0, s1, s2, s3;
  int     ct, i_cc, i;
  long    cc;
  char    *path, *key;

  if (file_name == NULL){
    vf_error = VF_ERR_CANT_OPEN;
    return NULL;
  }

  path = find_adj(file_name);
  if (path == NULL){
    vf_error = VF_ERR_CANT_OPEN;
    return NULL;
  }

  if ((fp = vf_fm_OpenTextFileStream(path)) == NULL){
    vf_free(path);
    vf_error = VF_ERR_CANT_OPEN;
    return NULL;
  }

  ALLOC_IF_ERR(adj, struct s_met_adj_table){
    vf_error = VF_ERR_NO_MEMORY;
    vf_free(path);
    return NULL;
  }

  adj->semantics = -1;
  adj->dir = -1;
  adj->rot_semantics = -1;
  for (ct = 0; ct < JTEX_MAX_CHARTYPE; ct++){
    adj->ct_corr[ct].need_cc_corr = FALSE;
    adj->ct_corr[ct].may_need_rotate = FALSE;
    adj->ct_corr[ct].dx = 0.0;
    adj->ct_corr[ct].dy = 0.0;
  }
  adj->ct_corr_all.dx = 0.0;
  adj->ct_corr_all.dy = 0.0;

  for (i = 0; VertCharInfo[i].code_point > 0; i++){
    ct = vf_tfm_jfm_chartype(font_jtex->tfm, 
			     (UINT4)VertCharInfo[i].code_point);
    adj->ct_corr[ct].may_need_rotate = TRUE;
  }

  adj->n_cc_corr = 0;
  if (realloc_cc_corr(adj) == NULL){
    vf_free(adj);
    vf_free(path);
    return NULL;
  }

  i_cc = 0;
  s = NULL;

  for (;;){
    vf_sexp_free(&s);
    if ((s = vf_sexp_read(fp)) == NULL)
      break;

    if (vf_dbg_drv_ascii_jtex_adj == 1){
      printf(">>VFlib ascii jtex ADJ: ");
      vf_sexp_pp(s);
    }

    if (!vf_sexp_listp(s)){
      fprintf(stderr, "VFlib warning: Illegal ADJ file format in %s: ", path);
      vf_sexp_pp(s);
      continue;
    }
    s0 = vf_sexp_car(s);
    if (!vf_sexp_stringp(s0)){
      fprintf(stderr, "VFlib warning: Illegal ADJ file format in %s: ", path);
      vf_sexp_pp(s);
      continue;
    }

    key = vf_sexp_get_cstring(s0);

    if (vf_strcmp_ci(key, KW_CHAR_ALL) == 0){
      if (vf_sexp_length(s) != 3){
	fprintf(stderr, "VFlib warning: Illegal ADJ file format in %s: ",
		path);
	vf_sexp_pp(s);
	continue;
      }
      s1 = vf_sexp_cadr(s);
      s2 = vf_sexp_caddr(s);
      if (!vf_sexp_stringp(s1)||!vf_sexp_stringp(s2)){
	fprintf(stderr, "VFlib warning: Illegal ADJ file format in %s: ",
		path);
	vf_sexp_pp(s);
	continue;
      }
      adj->ct_corr_all.dx = atof(vf_sexp_get_cstring(s1));
      adj->ct_corr_all.dy = atof(vf_sexp_get_cstring(s2));
      if (vf_dbg_drv_ascii_jtex_adj == 1){
	printf(">>  %s: Correction: %.3f,%.3f\n",
	       KW_CHAR_ALL, adj->ct_corr_all.dx, adj->ct_corr_all.dy);
      }

    } else if (vf_strcmp_ci(key, KW_CHAR_TYPE) == 0){
      if (vf_sexp_length(s) != 4){
	fprintf(stderr, "VFlib warning: Illegal ADJ file format in %s: ",
		path);
	vf_sexp_pp(s);
	continue;
      }
      s1 = vf_sexp_cadr(s);
      s2 = vf_sexp_caddr(s);
      s3 = vf_sexp_caddr(vf_sexp_cdr(s));
      if (!vf_sexp_stringp(s1)||!vf_sexp_stringp(s2)||!vf_sexp_stringp(s2)){
	fprintf(stderr, "VFlib warning: Illegal ADJ file format in %s: ",
		path);
	vf_sexp_pp(s);
	continue;
      }
      ct = atoi(vf_sexp_get_cstring(s1));
      if ((ct < 0) || (JTEX_MAX_CHARTYPE <= ct)){
	fprintf(stderr, "VFlib warning: %s %d out of range in %s.\n", 
		KW_CHAR_TYPE, ct, path);
	continue;
      }
      adj->ct_corr[ct].dx = atof(vf_sexp_get_cstring(s2));
      adj->ct_corr[ct].dy = atof(vf_sexp_get_cstring(s3));
      if (vf_dbg_drv_ascii_jtex_adj == 1){
	printf(">>  %s CharType: %02d, Correction: %.3f,%.3f\n",
	       KW_CHAR_TYPE, ct, adj->ct_corr[ct].dx, adj->ct_corr[ct].dy);
      }

    } else if (vf_strcmp_ci(key, KW_CHAR_CODE) == 0){
      if (vf_sexp_length(s) != 4){
	fprintf(stderr, "VFlib warning: Illegal ADJ file format in %s: ",
		path);
	vf_sexp_pp(s);
	continue;
      }
      s1 = vf_sexp_cadr(s);
      s2 = vf_sexp_caddr(s);
      s3 = vf_sexp_caddr(vf_sexp_cdr(s));
      if (!vf_sexp_stringp(s1)||!vf_sexp_stringp(s2)||!vf_sexp_stringp(s2)){
	fprintf(stderr, "VFlib warning: Illegal ADJ file format in %s: ",
		path);
	vf_sexp_pp(s);
	continue;
      }
      cc = atoi(vf_sexp_get_cstring(s1));
      if (cc < 0){
	fprintf(stderr, "VFlib warning: %s %d out of range in %s.\n", 
		KW_CHAR_CODE, ct, path);
	continue;
      }
      if (i_cc >= (adj->n_cc_corr-1))
	realloc_cc_corr(adj);
      sscanf(vf_sexp_get_cstring(s1), "%li", &cc);
      adj->cc_corr[i_cc].code_point = cc;
      adj->cc_corr[i_cc].dx         = atof(vf_sexp_get_cstring(s2));
      adj->cc_corr[i_cc].dy         = atof(vf_sexp_get_cstring(s3));
      if (vf_dbg_drv_ascii_jtex_adj == 1){
	printf(">>  %s  Code: 0x%lx, Correection: %.3f,%.3f\n", 
	       KW_CHAR_CODE, cc,
	       adj->cc_corr[i_cc].dx, adj->cc_corr[i_cc].dy);
      }
      i_cc++;
      ct = vf_tfm_jfm_chartype(font_jtex->tfm, (UINT4)cc);
      adj->ct_corr[ct].need_cc_corr = TRUE;

    } else if (vf_strcmp_ci(key, KW_SEMANTICS) == 0){
      if (vf_sexp_length(s) != 2){
	fprintf(stderr, "VFlib warning: Illegal ADJ file format in %s: ", 
		path);
	vf_sexp_pp(s);
	continue;
      }
      s1 = vf_sexp_cadr(s);
      if (!vf_sexp_stringp(s1)){
	fprintf(stderr, "VFlib warning: Illegal ADJ file format in %s: ",
		path);
	vf_sexp_pp(s);
	continue;
      }
      if (vf_strcmp_ci(vf_sexp_get_cstring(s1), KW_SEM_BITMAP) == 0){
	adj->semantics = SEMANTICS_AUX_BITMAP;
      } else if (vf_strcmp_ci(vf_sexp_get_cstring(s1), KW_SEM_DESIGN) == 0){
	adj->semantics = SEMANTICS_DESIGN_SIZE;
      } else {
	fprintf(stderr, "VFlib warning: Unknown semantics name %s in %s.\n", 
		vf_sexp_get_cstring(s1), path);
      }

    } else if (vf_strcmp_ci(key, KW_DIR) == 0){
      if (vf_sexp_length(s) != 2){
	fprintf(stderr, "VFlib warning: Illegal ADJ file format in %s: ", 
		path);
	vf_sexp_pp(s);
	continue;
      }
      s1 = vf_sexp_cadr(s);
      if (!vf_sexp_stringp(s1)){
	fprintf(stderr, "VFlib warning: Illegal ADJ file format in %s: ",
		path);
	vf_sexp_pp(s);
	continue;
      }
      if (vf_strcmp_ci(vf_sexp_get_cstring(s1), KW_DIR_HORIZONTAL) == 0){
	adj->dir = DIR_HORIZONTAL;
      } else if (vf_strcmp_ci(vf_sexp_get_cstring(s1), KW_DIR_VERTICAL) == 0){
	adj->dir = DIR_VERTICAL;
      } else {
	fprintf(stderr, "VFlib warning: Unknown direction name %s in %s.\n", 
		vf_sexp_get_cstring(s1), path);
	fprintf(stderr, "Use \"%s\" or \"%s\"\n", 
		KW_DIR_HORIZONTAL, KW_DIR_VERTICAL);
      }

    } else if (vf_strcmp_ci(key, KW_ROTATION) == 0){
      if (vf_sexp_length(s) != 2){
	fprintf(stderr, "VFlib warning: Illegal ADJ file format in %s: ", 
		path);
	vf_sexp_pp(s);
	continue;
      }
      s1 = vf_sexp_cadr(s);
      if (!vf_sexp_stringp(s1)){
	fprintf(stderr, "VFlib warning: Illegal ADJ file format in %s: ",
		path);
	vf_sexp_pp(s);
	continue;
      }
      if (vf_strcmp_ci(vf_sexp_get_cstring(s1), KW_ROT_PTEX) == 0){
	adj->rot_semantics = ROT_PTEX;
      } else if (vf_strcmp_ci(vf_sexp_get_cstring(s1), KW_ROT_JISX0208) == 0){
	adj->rot_semantics = ROT_JISX0208;
      } else {
	fprintf(stderr, "VFlib warning: Unknown keyword %s in %s.\n", 
		vf_sexp_get_cstring(s1), path);
	fprintf(stderr, "Use \"%s\" or \"%s\"\n", 
		KW_ROT_PTEX, KW_ROT_JISX0208);
      }

    } else {
      fprintf(stderr, "VFlib warning: Illegal ADJ file format in %s: ", path);
      vf_sexp_pp(s);
      continue;
    }
  }

  /* default values */
  if (adj->semantics == -1)
    adj->semantics = SEMANTICS_AUX_BITMAP;
  if (adj->dir == -1)
    adj->dir = DIR_HORIZONTAL;
  if (adj->rot_semantics == -1)
    adj->rot_semantics = ROT_PTEX;

  vf_free(path);

  return adj;
}

Private void
jtex_free_adj(ADJ adj)
{
  if (adj != NULL){
    vf_free(adj->cc_corr);
    vf_free(adj);
  }
}

Private ADJ
realloc_cc_corr(ADJ adj)
{
  ADJ_CC_CORR  new_cc_corr;
  int          new_size, i;

  new_size = 2 * adj->n_cc_corr;
  if (new_size <= 0)
    new_size = N_CC_CORR;

  ALLOCN_IF_ERR(new_cc_corr, struct s_met_adj_cc_corr, new_size){
    vf_error = VF_ERR_NO_MEMORY;
    return NULL;
  }

  for (i = 0; i < adj->n_cc_corr; i++){
    new_cc_corr[i].code_point = adj->cc_corr[i].code_point;
    new_cc_corr[i].dx         = adj->cc_corr[i].dx;
    new_cc_corr[i].dy         = adj->cc_corr[i].dy;
  }
  for (i = adj->n_cc_corr; i < new_size; i++){
    new_cc_corr[i].code_point = -1L;
    new_cc_corr[i].dx         = 0.0;
    new_cc_corr[i].dy         = 0.0;
  }

  vf_free(adj->cc_corr);
  adj->n_cc_corr = new_size;
  adj->cc_corr   = new_cc_corr;

  return adj;
}


Private char*
find_adj(char *fname)
{
  char  *path; 

  if (fname == NULL)
    return NULL;

  if (vf_dbg_drv_ascii_jtex_adj == 1)
    printf(">>VFlib ascii jtex:   Searching ADJ file: %s\n", fname);

  path = NULL;
  if (vf_path_absolute(fname)){
    if (vf_path_file_read_ok(fname)){
      if ((path = vf_strdup(fname)) == NULL)
	vf_error = VF_ERR_NO_MEMORY;
    }
  } else {
    path = vf_path_find_runtime_file("ascii-jtex", fname, 
				     VFLIB_ENV_ASCII_JTEX_DIR);
  }

  if (vf_dbg_drv_ascii_jtex_adj == 1){
    if (path != NULL)
      printf(">>      --- found: %s\n", path);
    else 
      printf(">>      --- not found\n");
  }

  return path;
}



/*EOF*/
