/*
 * drv_ekan.c -  A font driver for eKanji format fonts.
 * by Hirotsugu Kakugawa
 *
 *  8 Dec 1999  First implementation.
 * 18 Oct 2001  Fixed memory leak.
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
#include  <math.h>
#include  "VFlib-3_7.h"
#include  "VFsys.h"
#include  "vflibcap.h"
#include  "ccv.h"
#include  "sexp.h"
#include  "str.h"
#include  "path.h"
#include  "fsearch.h"
#include  "bitmap.h"
#include  "ekan.h"

struct s_font_ek {
  char    *font_path;
  char    *font_name;
  char    *font_file;
  int      font_dot_size;
  double   point_size;
  int      pixel_size;
  double   dpi_x, dpi_y;
  double   mag;
  double   aspect;
  int      ccv_id;
  int      direction;
  double    vec_bbxul_x, vec_bbxul_y;
  double    vec_next_x, vec_next_y;
  long     nchars;
  int      mock_encoding; 
  long     mock_enc_arg; 
  SEXP_ALIST  props;
};
typedef struct s_font_ek  *FONT_EK;

Private SEXP_LIST    default_fontdirs;
Private SEXP_STRING  default_point_size;
Private double       v_default_point_size;
Private SEXP_STRING  default_pixel_size;
Private double       v_default_pixel_size;
Private SEXP_STRING  default_font_dot_size;
Private int            v_default_font_dot_size;
Private SEXP_STRING  default_dpi, default_dpi_x, default_dpi_y;
Private double         v_default_dpi_x, v_default_dpi_y;
Private SEXP_STRING  default_aspect;
Private double         v_default_aspect;
Private SEXP_STRING  default_direction;
Private int            v_default_direction;
Private SEXP_LIST    default_vec_bbxul;
Private double         v_default_vec_bbxul_x, v_default_vec_bbxul_y;
Private SEXP_LIST    default_vec_next;
Private double         v_default_vec_next_x, v_default_vec_next_y;
Private SEXP_ALIST   default_properties;
Private SEXP_ALIST   default_variables;
Private SEXP_STRING  default_debug_mode;
Private char         *env_debug_mode = NULL;
#define DEBUG_ENV_NAME   "VFLIB_EKANJI_DEBUG"


Private int          ek_create(VF_FONT font, char *font_class, 
			       char *font_name, int implicit, SEXP entry);
Private int          ek_close(VF_FONT font);
Private int          ek_get_metric1(VF_FONT font, long code_point,
				    VF_METRIC1 metric1, double,double);
Private int          ek_get_metric2(VF_FONT font, long code_point,
				    VF_METRIC2 metric2, double,double);
Private int          ek_get_fontbbx1(VF_FONT font,double,double,
				     double*,double*,double*,double*);
Private int          ek_get_fontbbx2(VF_FONT font, double,double,
				     int*,int*,int*,int*);
Private VF_BITMAP    ek_get_bitmap1(VF_FONT,long,double,double);
Private VF_BITMAP    ek_get_bitmap2(VF_FONT,long,double,double);
Private VF_OUTLINE   ek_get_outline1(VF_FONT,long,double,double);
Private char*        ek_get_font_prop(VF_FONT,char*);


Private int    ek_debug(char);
Private int    ek_debug2(char type, char *str);

Private VF_BITMAP  ek_file_read(FONT_EK font_ek, long cp);
Private void       ek_file_init(FONT_EK font_ek);




Glocal int
VF_Init_Driver_EKanji(void)
{
  int   z;
  char *p;
  SEXP  s1, s2;
  struct s_capability_table  ct[30];

  z = 0;
  /* VF_CAPE_FONT_DIRECTORIES */
  ct[z].cap = VF_CAPE_FONT_DIRECTORIES;  ct[z].type = CAPABILITY_STRING_LIST0;
  ct[z].ess = CAPABILITY_OPTIONAL;       ct[z++].val = &default_fontdirs;
  /* VF_CAPE_EK_FONT_DOT_SIZE */  
  ct[z].cap = VF_CAPE_EK_FONT_DOT_SIZE;  ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;       ct[z++].val = &default_font_dot_size;
  /* VF_CAPE_POINT_SIZE */
  ct[z].cap = VF_CAPE_POINT_SIZE;        ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;       ct[z++].val = &default_point_size;
  /* VF_CAPE_PIXEL_SIZE */
  ct[z].cap = VF_CAPE_PIXEL_SIZE;        ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;       ct[z++].val = &default_pixel_size;
  /* VF_CAPE_DPI */  
  ct[z].cap = VF_CAPE_DPI;               ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;       ct[z++].val = &default_dpi;
  /* VF_CAPE_DPI_X */
  ct[z].cap = VF_CAPE_DPI_X;             ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;       ct[z++].val = &default_dpi_x;
  /* VF_CAPE_DPI_Y */
  ct[z].cap = VF_CAPE_DPI_Y;             ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;       ct[z++].val = &default_dpi_y;
  /* VF_CAPE_DIRECTION */
  ct[z].cap = VF_CAPE_DIRECTION;         ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;       ct[z++].val = &default_direction;
  /* VF_CAPE_VECTOR_TO_BBX_UL */
  ct[z].cap = VF_CAPE_VECTOR_TO_BBX_UL;  ct[z].type = CAPABILITY_VECTOR;
  ct[z].ess = CAPABILITY_OPTIONAL;       ct[z++].val = &default_vec_bbxul;
  /* VF_CAPE_VECTOR_TO_NEXT */
  ct[z].cap = VF_CAPE_VECTOR_TO_NEXT;    ct[z].type = CAPABILITY_VECTOR;
  ct[z].ess = CAPABILITY_OPTIONAL;       ct[z++].val = &default_vec_next;
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

  if (vf_cap_GetParsedClassDefault(FONTCLASS_NAME, ct, NULL, NULL) 
      == VFLIBCAP_PARSED_ERROR)
    return -1;

  v_default_font_dot_size = DEFAULT_FONT_DOT_SIZE;
  if (default_font_dot_size != NULL)
    v_default_font_dot_size = atoi(vf_sexp_get_cstring(default_font_dot_size));

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

  v_default_dpi_x  = -1;
  v_default_dpi_y  = -1;
  if (default_dpi != NULL)
    v_default_dpi_x = v_default_dpi_y = atof(vf_sexp_get_cstring(default_dpi));
  if (default_dpi_x != NULL)
    v_default_dpi_x = atof(vf_sexp_get_cstring(default_dpi_x));
  if (default_dpi_y != NULL)
    v_default_dpi_y = atof(vf_sexp_get_cstring(default_dpi_y));

  v_default_aspect = 1.0;
  if (default_aspect != NULL)
    v_default_aspect = atof(vf_sexp_get_cstring(default_aspect));

  v_default_direction = DEFAULT_DIRECTION;
  if (default_direction != NULL){
    p = vf_sexp_get_cstring(default_direction);
    if ((*p == 'h') || (*p == 'H')){
      v_default_direction = DIRECTION_HORIZONTAL;
    } else if ((*p == 'v') || (*p == 'V')){
      v_default_direction = DIRECTION_VERTICAL;
    } else {
      fprintf(stderr, "VFlib warning: Unknown writing direction: %s\n", p);
    }
  }

  switch(v_default_direction){
  default:
  case DIRECTION_HORIZONTAL: 
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

  env_debug_mode = getenv(DEBUG_ENV_NAME);

  VF_InstallFontDriver(FONTCLASS_NAME, (DRIVER_FUNC_TYPE)ek_create);

  return 0;
}


Private int
ek_create(VF_FONT font, char *font_class, 
	  char *font_name, int implicit, SEXP entry)
{
  FONT_EK  font_ek;
  FILE     *fp;
  int      val, len;
  char    *path_name, *font_file, *p;
  SEXP     cap_fontdirs, cap_font_dot_size, cap_font, cap_point, cap_pixel;
  SEXP     cap_dpi, cap_dpi_x, cap_dpi_y, cap_mag, cap_aspect;
  SEXP     cap_charset, cap_encoding, cap_font_charset, cap_font_encoding;
  SEXP     cap_props, cap_direction, cap_vec_bbxul, cap_vec_next;
  SEXP     cap_mock_enc;
  SEXP     fontdirs, s1, s2;
  char    *charset, *encoding, *font_charset, *font_encoding;
  int      z;
  struct s_capability_table  ct[30];

  z = 0;
  /* VF_CAPE_FONT_CLASS */
  ct[z].cap = VF_CAPE_FONT_CLASS;       ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_ESSENTIAL;     ct[z++].val = NULL;
  /* VF_CAPE_EK_FONT_DOT_SIZE */  
  ct[z].cap = VF_CAPE_EK_FONT_DOT_SIZE; ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;      ct[z++].val = &cap_font_dot_size;
  /* VF_CAPE_EK_MOCK_FONT_ENC */  
  ct[z].cap = VF_CAPE_EK_MOCK_FONT_ENC; ct[z].type = CAPABILITY_STRING_LIST0;
  ct[z].ess = CAPABILITY_OPTIONAL;      ct[z++].val = &cap_mock_enc;
  /* VF_CAPE_FONT_DIRECTORIES */
  ct[z].cap = VF_CAPE_FONT_DIRECTORIES; ct[z].type = CAPABILITY_STRING_LIST0;
  ct[z].ess = CAPABILITY_OPTIONAL;      ct[z++].val = &cap_fontdirs;
  /* VF_CAPE_DIRECTION */
  ct[z].cap = VF_CAPE_DIRECTION;        ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;      ct[z++].val = &cap_direction;
  /* VF_CAPE_VECTOR_TO_BBX_UL */
  ct[z].cap = VF_CAPE_VECTOR_TO_BBX_UL; ct[z].type = CAPABILITY_VECTOR;
  ct[z].ess = CAPABILITY_OPTIONAL;      ct[z++].val = &cap_vec_bbxul;
  /* VF_CAPE_VECTOR_TO_NEXT */
  ct[z].cap = VF_CAPE_VECTOR_TO_NEXT;   ct[z].type = CAPABILITY_VECTOR;
  ct[z].ess = CAPABILITY_OPTIONAL;      ct[z++].val = &cap_vec_next;
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
  /* VF_CAPE_CHARSET */
  ct[z].cap = VF_CAPE_CHARSET;         ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;     ct[z++].val = &cap_charset;
  /* VF_CAPE_ENCODING */
  ct[z].cap = VF_CAPE_ENCODING;        ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;     ct[z++].val = &cap_encoding;
  /* VF_CAPE_FONT_CHARSET */
  ct[z].cap = VF_CAPE_FONT_CHARSET;    ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;     ct[z++].val = &cap_font_charset;
  /* VF_CAPE_FONT_ENCODING */
  ct[z].cap = VF_CAPE_FONT_ENCODING;   ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;     ct[z++].val = &cap_font_encoding;
  /* VF_CAPE_PROPERTIES */
  ct[z].cap = VF_CAPE_PROPERTIES;      ct[z].type = CAPABILITY_ALIST;
  ct[z].ess = CAPABILITY_OPTIONAL;     ct[z++].val = &cap_props;
  /* end */
  ct[z].cap = NULL; ct[z].type = 0; ct[z].ess = 0; ct[z++].val  = NULL;


  val = -1;
  font_file = font_name;
  fontdirs = default_fontdirs;
  font_ek = NULL;

  if (implicit == 1){   /* implicit font */
    font_file = font_name;
  } else {              /* explicit font */
    if (vf_cap_GetParsedFontEntry(entry, font_name, ct, 
				  default_variables, NULL) 
	== VFLIBCAP_PARSED_ERROR)
      return -1;
    if (cap_fontdirs != NULL)
      fontdirs = cap_fontdirs;
    if (cap_font != NULL){
      font_file = vf_sexp_get_cstring(cap_font);
    } else {
      /* Use font name as font file name if font file name is not given. */
      font_file = font_name;
    }
  }

  if (ek_debug('f'))
    printf("eKanji font file to open %s\n", font_file);
  path_name = vf_search_file(font_file, -1, NULL, FALSE, -1, 
			     fontdirs, NULL, NULL);
  if (path_name == NULL){
    if (ek_debug('f'))
      printf("eKanji font file not found\n");
    vf_error = VF_ERR_NO_FONT_FILE;
    goto End; 
  }
  if (ek_debug('f'))
    printf("eKanji Font File: %s ==> %s\n", font_file, path_name);
  if ((fp = vf_fm_OpenBinaryFileStream(path_name)) == NULL){
    vf_error = VF_ERR_NO_FONT_FILE;
    goto End; 
  }

  font->font_type       = VF_FONT_TYPE_BITMAP;
  font->get_metric1     = ek_get_metric1;
  font->get_metric2     = ek_get_metric2;
  font->get_fontbbx1    = ek_get_fontbbx1;
  font->get_fontbbx2    = ek_get_fontbbx2;
  font->get_bitmap1     = ek_get_bitmap1;
  font->get_bitmap2     = ek_get_bitmap2;
  font->get_outline     = ek_get_outline1;
  font->get_font_prop   = ek_get_font_prop;
  font->query_font_type = NULL;
  font->close           = ek_close;

  ALLOC_IF_ERR(font_ek, struct s_font_ek){
    vf_error = VF_ERR_NO_MEMORY;
    goto End; 
  }

  font_ek->font_path     = path_name;
  font_ek->font_name     = NULL; 
  font_ek->font_file     = NULL; 
  font_ek->font_dot_size = v_default_font_dot_size; 
  font_ek->mock_encoding = MOCK_FONT_ENC_RAW; 
  font_ek->mock_enc_arg  = 0; 
  font_ek->point_size    = v_default_point_size;
  font_ek->pixel_size    = v_default_pixel_size;
  font_ek->dpi_x         = v_default_dpi_x;
  font_ek->dpi_y         = v_default_dpi_y;
  font_ek->mag           = 1.0;
  font_ek->aspect        = v_default_aspect;
  font_ek->ccv_id        = -1;
  font_ek->direction     = v_default_direction;
  font_ek->vec_bbxul_x   = v_default_vec_bbxul_x;
  font_ek->vec_bbxul_y   = v_default_vec_bbxul_y;
  font_ek->vec_next_x    = v_default_vec_next_x;
  font_ek->vec_next_y    = v_default_vec_next_y;
  font_ek->props         = NULL;

  charset       = NULL;
  encoding      = NULL;
  font_charset  = NULL;
  font_encoding = NULL;

  if (implicit == 0){
    if (cap_font_dot_size != NULL)
      font_ek->font_dot_size = atoi(vf_sexp_get_cstring(cap_font_dot_size));
    if (cap_mock_enc != NULL){
      len = vf_sexp_length(cap_mock_enc);
      if (len > 0){
	p = vf_sexp_get_cstring(vf_sexp_car(cap_mock_enc));
	if (vf_strcmp_ci(p, CAPE_MOCK_FONT_ENC_RAW) == 0){
	  font_ek->mock_encoding = MOCK_FONT_ENC_RAW;
	} else if (vf_strcmp_ci(p, CAPE_MOCK_FONT_ENC_SUBBLOCKS_94X94) == 0){
	  font_ek->mock_encoding = MOCK_FONT_ENC_SUBBLOCKS_94X94;
	} else if (vf_strcmp_ci(p, CAPE_MOCK_FONT_ENC_SUBBLOCKS_94X60) == 0){
	  font_ek->mock_encoding = MOCK_FONT_ENC_SUBBLOCKS_94X60;
	} else if (vf_strcmp_ci(p, CAPE_MOCK_FONT_ENC_WITH_OFFSET) == 0){
	  font_ek->mock_encoding = MOCK_FONT_ENC_WITH_OFFSET;
	} else {
	  fprintf(stderr, 
		  "VFlib warning: unknown keyword `%s' in capability %s\n",
		  p, VF_CAPE_EK_MOCK_FONT_ENC);
	}
      }
      if (len > 1){
	p = vf_sexp_get_cstring(vf_sexp_cadr(cap_mock_enc));
	if (*p == '-'){ 
	  sscanf(p+1, "%li", &font_ek->mock_enc_arg);
	  font_ek->mock_enc_arg = -font_ek->mock_enc_arg;
	} else if (*p == '+'){ 
	  sscanf(p+1, "%li", &font_ek->mock_enc_arg);
	} else {
	  sscanf(p, "%li", &font_ek->mock_enc_arg);
	}
      }
    }
    if (cap_point != NULL)
      font_ek->point_size = atof(vf_sexp_get_cstring(cap_point));
    if (cap_pixel != NULL)
      font_ek->pixel_size = atof(vf_sexp_get_cstring(cap_pixel));
    if (cap_dpi != NULL)
      font_ek->dpi_x = font_ek->dpi_y = atof(vf_sexp_get_cstring(cap_dpi));
    if (cap_dpi_x != NULL)
      font_ek->dpi_x = atof(vf_sexp_get_cstring(cap_dpi_x));
    if (cap_dpi_y != NULL)
      font_ek->dpi_y = atof(vf_sexp_get_cstring(cap_dpi_y));
    if (cap_mag != NULL)
      font_ek->mag = atof(vf_sexp_get_cstring(cap_mag));
    if (cap_aspect != NULL)
      font_ek->aspect = atof(vf_sexp_get_cstring(cap_aspect));
    if (cap_charset != NULL)
      charset = vf_sexp_get_cstring(cap_charset);
    if (cap_encoding != NULL)
      encoding = vf_sexp_get_cstring(cap_encoding);
    if (cap_encoding != NULL)
      font_encoding = vf_sexp_get_cstring(cap_encoding);
    if (cap_font_charset != NULL)
      font_charset = vf_sexp_get_cstring(cap_font_charset);
    if (cap_font_encoding != NULL)
      font_encoding = vf_sexp_get_cstring(cap_font_encoding);
    if (cap_direction != NULL){
      p = vf_sexp_get_cstring(cap_direction);
      if ((*p == 'h') || (*p == 'H')){
	font_ek->direction = DIRECTION_HORIZONTAL;
      } else if ((*p == 'v') || (*p == 'V')){
	font_ek->direction = DIRECTION_VERTICAL;
      } else {
	fprintf(stderr, "VFlib warning: Unknown writing direction: %s\n", p);
      }
    }
    switch(font_ek->direction){
    default:
    case DIRECTION_HORIZONTAL: 
      font_ek->vec_bbxul_x = v_default_vec_bbxul_x;
      font_ek->vec_bbxul_y = v_default_vec_bbxul_y;
      font_ek->vec_next_x  = v_default_vec_next_x;
      font_ek->vec_next_y  = v_default_vec_next_y;
      break;
    case DIRECTION_VERTICAL:
      font_ek->vec_bbxul_x = v_default_vec_bbxul_x;
      font_ek->vec_bbxul_y = v_default_vec_bbxul_y;
      font_ek->vec_next_x  = v_default_vec_next_x;
      font_ek->vec_next_y  = v_default_vec_next_y;
      break;
    }
    if (cap_vec_bbxul != NULL){
      s1 = vf_sexp_car(cap_vec_bbxul);
      s2 = vf_sexp_cadr(cap_vec_bbxul);
      font_ek->vec_bbxul_x = atof(vf_sexp_get_cstring(s1));
      font_ek->vec_bbxul_y = atof(vf_sexp_get_cstring(s2));
    }
    if (default_vec_next != NULL){
      s1 = vf_sexp_car(default_vec_next);
      s2 = vf_sexp_cadr(default_vec_next);
      font_ek->vec_next_x = atof(vf_sexp_get_cstring(s1));
      font_ek->vec_next_y = atof(vf_sexp_get_cstring(s2));
    }
    if (cap_props != NULL)
      font_ek->props = cap_props;
  }

  if ((font_ek->font_file = vf_strdup(font_file)) == NULL){
    vf_error = VF_ERR_NO_MEMORY;
    goto End;
  }
  if ((font_ek->font_name = vf_strdup(font_name)) == NULL){
    vf_error = VF_ERR_NO_MEMORY;
    goto End;
  }

  if (ek_debug('c')){
    printf("VFlib EK: cs=%s, enc=%s, font_cs=%s, font_enc=%s\n",
	   charset, encoding, font_charset, font_encoding);
  }

  font_ek->ccv_id = -1;
  if ((charset != NULL) && (encoding != NULL)
      && (font_charset != NULL) && (font_encoding != NULL)){
    font_ek->ccv_id 
      = vf_ccv_require(charset, encoding, font_charset, font_encoding);
  } else {
    ;  /* need warning ? */
  }
  if (ek_debug('c')) 
    printf("VFlib EK: ccv id  %d\n", font_ek->ccv_id);

  font->private = font_ek;
  val = 0;

End:
  ek_file_init(font_ek);
  if (implicit == 0){ /* explicit font */
    vf_sexp_free2(&cap_fontdirs, &cap_font_dot_size);
    vf_sexp_free3(&cap_font, &cap_point, &cap_pixel);
    vf_sexp_free3(&cap_dpi, &cap_dpi_x, &cap_dpi_y);
    vf_sexp_free2(&cap_mag, &cap_aspect);
    vf_sexp_free2(&cap_charset, &cap_encoding);
    vf_sexp_free2(&cap_font_charset, &cap_font_encoding);
    vf_sexp_free3(&cap_direction, &cap_vec_bbxul, &cap_vec_next);
    vf_sexp_free1(&cap_mock_enc);
    if (val < 0)
      vf_sexp_free1(&cap_props);
  }
  if (val < 0){
    if (font_ek != NULL){
      vf_free(font_ek->font_path); 
      vf_free(font_ek->font_name); 
      vf_free(font_ek->font_file); 
    }
    vf_free(font_ek); 
  }

  return val;
}


Private int
ek_close(VF_FONT font)
{
  FONT_EK  font_ek;

  font_ek = (FONT_EK)font->private;
  if (font_ek->props != NULL)
    vf_sexp_free1(&font_ek->props);
  vf_free(font_ek->font_path); 
  vf_free(font_ek->font_name); 
  vf_free(font_ek->font_file); 
  vf_free(font_ek); 

  return 0; 
}




Private void
mag_mode_1(FONT_EK font_ek, VF_FONT font, 
	   double mag_x, double mag_y,
	   double *ret_point_size,
	   double *ret_bbx_w, double *ret_bbx_h, 
	   double *ret_mag_x, double *ret_mag_y, 
	   double *ret_dpix, double *ret_dpiy)
{
  double  mx, my, dpix, dpiy, ps, asp;

  if ((ps = font->point_size) < 0)
    if ((ps = font_ek->point_size) < 0)
      ps = v_default_point_size;

  if (ret_point_size != NULL)
    *ret_point_size = ps;

  asp = (v_default_aspect * font_ek->aspect);

  mx = mag_x * font_ek->mag * font->mag_x * asp;
  my = mag_y * font_ek->mag * font->mag_y;

  if (ret_mag_x != NULL)
    *ret_mag_x = mx;
  if (ret_mag_y != NULL)
    *ret_mag_y = my;

  if ((font->dpi_x > 0) && (font->dpi_y > 0)){
    dpix = font->dpi_x;
    dpiy = font->dpi_y;
  } else if ((font_ek->dpi_x > 0) && (font_ek->dpi_y > 0)){
    dpix = font_ek->dpi_x;
    dpiy = font_ek->dpi_y;
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
  printf("*** %.3f %.3f %.3f\n", mag_x, font_ek->mag, font->mag_x);
  printf("    %.3f %.3f %.3f\n", mag_y, font_ek->mag, font->mag_y);
  printf("    dpix=%.3f  font_dpi_x=%.3f\n", dpix, font_dpi_x);
  printf("    dpiy=%.3f  font_dpi_y=%.3f\n", dpiy, font_dpi_y);
  printf("    asp=%.3f\n", asp);
  printf("    mx=%.3f, my=%.3f\n", mx, my);
  if (ret_bbx_w != NULL)
    printf("    W=%.3f  H=%.3f\n", *ret_bbx_w, *ret_bbx_h);
#endif

}


Private int
ek_get_metric1(VF_FONT font, long code_point, VF_METRIC1 metric, 
	       double mag_x, double mag_y)
{
  FONT_EK    font_ek;
  double     bbx_w, bbx_h, dpix, dpiy;
  long       cp;

  if (   ((font_ek = (FONT_EK)font->private) == NULL)
      || (metric == NULL)){
    fprintf(stderr, "VFlib internal error: in ek_get_metric1()\n");
    abort();
  }

  cp = code_point;
  if (font_ek->ccv_id >= 0){
    cp = vf_ccv_conv(font_ek->ccv_id, code_point);
    if (ek_debug('C')) 
      printf("VFlib EK: CCV  0x%lx => 0x%lx\n", code_point, cp);
  }
  if (cp < 0)
    return -1;

  mag_mode_1(font_ek, font, mag_x, mag_y, 
	     NULL, &bbx_w, &bbx_h, NULL, NULL, &dpix, &dpiy);

  metric->bbx_width  = bbx_w;
  metric->bbx_height = bbx_h;
  metric->off_x = bbx_w * font_ek->vec_bbxul_x;
  metric->off_y = bbx_h * font_ek->vec_bbxul_y;
  metric->mv_x  = bbx_w * font_ek->vec_next_x;
  metric->mv_y  = bbx_h * font_ek->vec_next_y;

  return 0;
}

Private int
ek_get_fontbbx1(VF_FONT font, double mag_x, double mag_y,
		  double *w_p, double *h_p, double *xoff_p, double *yoff_p)
{
  FONT_EK    font_ek;
  double     bbx_w, bbx_h, dpix, dpiy;

  if ((font_ek = (FONT_EK)font->private) == NULL){
    fprintf(stderr, "VFlib internal error: in ek_get_fontbbx1()\n");
    abort();
  }

  mag_mode_1(font_ek, font, mag_x, mag_y, 
	     NULL, &bbx_w, &bbx_h, NULL, NULL, &dpix, &dpiy);

  *w_p = bbx_w;
  *h_p = bbx_h;
  *xoff_p = bbx_w * font_ek->vec_bbxul_x;
  *yoff_p = bbx_h * (1.0 - font_ek->vec_bbxul_y);

  return 0;
}

Private VF_BITMAP
ek_get_bitmap1(VF_FONT font, long code_point,
	       double mag_x, double mag_y)
{
  FONT_EK      font_ek;
  VF_BITMAP    bm0, bm;
  double       bbx_w, bbx_h;
  long         cp;

  if ((font_ek = (FONT_EK)font->private) == NULL){
    fprintf(stderr, "VFlib internal error: in ek_get_bitmap1()\n");
    abort();
  }

  cp = code_point;
  if (font_ek->ccv_id >= 0){
    cp = vf_ccv_conv(font_ek->ccv_id, code_point);
    if (ek_debug('C')) 
      printf("VFlib EK: CCV  0x%lx => 0x%lx\n", code_point, cp);
  }
  if (cp < 0)
    return NULL;

  if ((bm0 = ek_file_read(font_ek, cp)) == NULL)
    return NULL;
  /* note: bm0 should not be released*/

  mag_mode_1(font_ek, font, mag_x, mag_y, 
	     NULL, &bbx_w, &bbx_h, NULL, NULL, NULL, NULL);

  bm = VF_MakeScaledBitmap(bm0, 
			   bbx_w/(double)font_ek->font_dot_size, 
			   bbx_h/(double)font_ek->font_dot_size);

  bm->off_x = toint(bm->bbx_width  * font_ek->vec_bbxul_x);
  bm->off_y = toint(bm->bbx_height * font_ek->vec_bbxul_y);
  bm->mv_x  = toint(bm->bbx_width  * font_ek->vec_next_x);
  bm->mv_y  = toint(bm->bbx_height * font_ek->vec_next_y);
  
  return  bm;
}


Private VF_OUTLINE
ek_get_outline1(VF_FONT font, long code_point,
		  double mag_x, double mag_y)
{
  FONT_EK       font_ek;
  long          cp;
  VF_BITMAP     bm;
  VF_OUTLINE    ol;
  double        ps, dpi_x, dpi_y, bbx_w, bbx_h;

  if ((font_ek = (FONT_EK)font->private) == NULL){
    fprintf(stderr, "VFlib internal error: in ek_get_metric1()\n");
    abort();
  }

  cp = code_point;
  if (font_ek->ccv_id >= 0){
    cp = vf_ccv_conv(font_ek->ccv_id, code_point);
    if (ek_debug('C')) 
      printf("VFlib EK: CCV  0x%lx => 0x%lx\n", code_point, cp);
  }
  if (cp < 0)
    return NULL;

  if ((bm = ek_get_bitmap1(font, cp, mag_x, mag_y)) == NULL)
    return NULL;

  mag_mode_1(font_ek, font, mag_x, mag_y, 
	     &ps, &bbx_w, &bbx_h, NULL, NULL, &dpi_x, &dpi_y);
  
  ol = vf_bitmap_to_outline(bm, bbx_w, bbx_h, dpi_x, dpi_y, ps, 1, 1);
  VF_FreeBitmap(bm);

  return ol;
}


Private void
mag_mode_2(FONT_EK font_ek, VF_FONT font, 
	   double mag_x, double mag_y,
	   double *ret_pixel_size,
	   double *ret_magx, double *ret_magy,
	   double *ret_bbx_w, double *ret_bbx_h)
{
  int     ps;
  double  mx, my, asp;

  if ((ps = font->pixel_size) < 0)
    if ((ps = font_ek->pixel_size) < 0)
      ps = v_default_pixel_size;

  asp = (v_default_aspect * font_ek->aspect);

  mx = mag_x * font_ek->mag * font->mag_x * asp;
  my = mag_y * font_ek->mag * font->mag_y;

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
ek_get_metric2(VF_FONT font, long code_point, VF_METRIC2 metric, 
		 double mag_x, double mag_y)
{
  FONT_EK    font_ek;
  double     bbx_w, bbx_h;
  long       cp;

  if (   ((font_ek = (FONT_EK)font->private) == NULL)
      || (metric == NULL)){
    fprintf(stderr, "VFlib internal error: in ek_get_metric2()\n");
    abort();
  }

  cp = code_point;
  if (font_ek->ccv_id >= 0){
    cp = vf_ccv_conv(font_ek->ccv_id, code_point);
    if (ek_debug('C')) 
      printf("VFlib EK: CCV  0x%lx => 0x%lx\n", code_point, cp);
  }
  if (cp < 0)
    return 0;

  mag_mode_2(font_ek, font, mag_x, mag_y, NULL, NULL, NULL, &bbx_w, &bbx_h);

  metric->bbx_width  = (int)ceil(bbx_w);
  metric->bbx_height = (int)ceil(bbx_h);
  metric->off_x = toint(bbx_w * font_ek->vec_bbxul_x);
  metric->off_y = toint(bbx_h * font_ek->vec_bbxul_y);
  metric->mv_x  = toint(bbx_w * font_ek->vec_next_x);
  metric->mv_y  = toint(bbx_h * font_ek->vec_next_y);

  return 0;
}


Private int
ek_get_fontbbx2(VF_FONT font, double mag_x, double mag_y,
		  int *w_p, int *h_p, int *xoff_p, int *yoff_p)
{
  FONT_EK    font_ek;
  double     bbx_w, bbx_h;

  if ((font_ek = (FONT_EK)font->private) == NULL){
    fprintf(stderr, "VFlib internal error: in ek_get_fontbbx2()\n");
    abort();
  }

  mag_mode_2(font_ek, font, mag_x, mag_y, NULL, NULL, NULL, &bbx_w, &bbx_h);

  *w_p  = toint(bbx_w);
  *h_p  = toint(bbx_h);
  *xoff_p = toint(bbx_w * font_ek->vec_bbxul_x);
  *yoff_p = toint(bbx_w * (font_ek->vec_bbxul_y - 1.0));

  return 0;
}


Private VF_BITMAP
ek_get_bitmap2(VF_FONT font, long code_point, 
		 double mag_x, double mag_y)
{
  FONT_EK     font_ek;
  VF_BITMAP   bm0, bm;
  double      bbx_w, bbx_h, mx, my;
  long        cp;

  if ((font_ek = (FONT_EK)font->private) == NULL){
    fprintf(stderr, "VFlib internal error: in ek_get_bitmap2()\n");
    abort();
  }

  cp = code_point;
  if (font_ek->ccv_id >= 0){
    cp = vf_ccv_conv(font_ek->ccv_id, code_point);
    if (ek_debug('C')) 
      printf("VFlib EK: CCV  0x%lx => 0x%lx\n", code_point, cp);
  }
  if (cp < 0)
    return NULL;

  if ((bm0 = ek_file_read(font_ek, cp)) == NULL)
    return NULL;
  /* note: bm0 should not be released*/

  mag_mode_2(font_ek, font, mag_x, mag_y, NULL, &mx, &my, &bbx_w, &bbx_h);

  bm = VF_MakeScaledBitmap(bm0, 
			   bbx_w/(double)font_ek->font_dot_size, 
			   bbx_h/(double)font_ek->font_dot_size);

  bm->off_x = toint(bm->bbx_width  * font_ek->vec_bbxul_x);
  bm->off_y = toint(bm->bbx_height * font_ek->vec_bbxul_y);
  bm->mv_x  = toint(bm->bbx_width  * font_ek->vec_next_x);
  bm->mv_y  = toint(bm->bbx_height * font_ek->vec_next_y);

  return bm;
}


Private char*
ek_get_font_prop(VF_FONT font, char *prop_name)
     /* CALLER MUST RELEASE RETURNED STRING */
{
  FONT_EK   font_ek;
  char        str[512];
  double      ps, dpix, dpiy;
  SEXP        v;

  if ((font_ek = (FONT_EK)font->private) == NULL){
    fprintf(stderr, "VFlib: Internal error in ek_get_font_prop()\n");
    abort();
  }
  
  if ((v = vf_sexp_assoc(prop_name, font_ek->props)) != NULL){
    return vf_strdup(vf_sexp_get_cstring(vf_sexp_cadr(v)));
  } else if ((v = vf_sexp_assoc(prop_name, default_properties)) != NULL){
    return vf_strdup(vf_sexp_get_cstring(vf_sexp_cadr(v)));
  }

  if (font->mode == 1){
    mag_mode_1(font_ek, font, 1.0, 1.0,
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
    mag_mode_2(font_ek, font, 1.0, 1.0, &ps, NULL, NULL, NULL, NULL);
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




Private void
ek_file_init(FONT_EK font_ek)
{
  FILE  *fp; 
  long   len;
  int    bw;

  if (font_ek == NULL)
    return;

  font_ek->nchars = 0;  

  if (font_ek->font_dot_size <= 0)
    return;

  if ((fp = vf_fm_OpenBinaryFileStream(font_ek->font_path)) == NULL)
    return;
  
  fseek(fp, 0, SEEK_END);
  len = ftell(fp);

  bw = (font_ek->font_dot_size + 7) / 8;
  font_ek->nchars = len / (font_ek->font_dot_size * bw);

  if (ek_debug('f')){
    printf("Ekanji File: %s, size: %ld\n", font_ek->font_file, len);
    printf("  Dot size: %d, nchars: %ld\n", 
	   font_ek->font_dot_size, font_ek->nchars);
  }
}


Private VF_BITMAP
ek_file_read(FONT_EK font_ek, long cp)
     /* CALLER SHOULD NOT RELEASE RETURNED OBJECT */
{
  int                 y;
  long                m0, m1;
  FILE               *fp;
  static VF_BITMAP    bmbuff = NULL;
  static int          bw = -1;

  if (font_ek->font_dot_size <= 0)
    return NULL;

  switch (font_ek->mock_encoding){
  case MOCK_FONT_ENC_RAW:
    break;
  case MOCK_FONT_ENC_SUBBLOCKS_94X94:
    m0 = cp / 0x100;
    m1 = cp % 0x100;
    if ((m0 < 0x21) || (0x7e < m0) || (m1 < 0x21) || (0x7e < m1)){
      cp = -1;
    } else {
      m0 -= 0x21;
      m1 -= 0x21;
      cp = m0 * 94 + m1 + font_ek->mock_enc_arg * 94 * 94 + 1;
    }
    break;
  case MOCK_FONT_ENC_SUBBLOCKS_94X60:
    m0 = cp / 0x100;
    m1 = cp % 0x100;
    if ((m0 < 0x30) || ((0x4d < m0)&&(m0 < 0x50)) || (0x6d < m0) 
	|| (m1 < 0x21) || (0x7e < m1)){
      cp = -1;
    } else {
      if (m0 >= 0x50)
	m0 -= 2;
      m0 -= 0x30;
      m1 -= 0x21;
      cp = m0 * 94 + m1 + font_ek->mock_enc_arg*94*60 + 1;
    }
    break;
  case MOCK_FONT_ENC_WITH_OFFSET:
#if 0
    printf("** 0x%lx, %ld     0x%lx, %ld  %d, 0x%lx\n", 
	   cp, cp, cp + font_ek->mock_enc_arg,cp + font_ek->mock_enc_arg,
	   font_ek->mock_enc_arg, font_ek->mock_enc_arg);
#endif
    cp = cp + font_ek->mock_enc_arg;
    break;
  default:
    fprintf(stderr, "VFlib internal error: Cannot happen. ek_file_read()\n");
    abort();
  }

  if ((cp < 0) || (font_ek->nchars <= 0) || (font_ek->nchars < cp)){
    vf_error = VF_ERR_ILL_CODE_POINT;
    return NULL;
  }

  if ((bmbuff == NULL) || (bw != font_ek->font_dot_size)){
    if (bmbuff != NULL){
      vf_free_bitmap(bmbuff);
      bmbuff = NULL;
      bw = -1;
    }
    bw = (font_ek->font_dot_size + 7) / 8;
    if ((bmbuff = vf_alloc_bitmap(font_ek->font_dot_size,
				  font_ek->font_dot_size)) == NULL){
      bw = -1;
      vf_error = VF_ERR_NO_MEMORY;
      return NULL;
    }
  }
  
  if ((fp = vf_fm_OpenBinaryFileStream(font_ek->font_path)) == NULL)
    return NULL;

  fseek(fp, font_ek->font_dot_size * bw * (cp-1), SEEK_SET);
  for (y = 0; y < font_ek->font_dot_size; y++){
    fread(&bmbuff->bitmap[y * bmbuff->raster], bw, sizeof(unsigned char), fp);
  }

  return  bmbuff;
}



Private int
ek_debug(char type)
{
  int   v;
  char  *p0;

  v = FALSE;
  if (env_debug_mode != NULL){
    if ((v = ek_debug2(type, env_debug_mode)) == TRUE)
      return TRUE;
  }

  if (default_debug_mode == NULL)
    return FALSE;
  if ((p0 = vf_sexp_get_cstring(default_debug_mode)) == NULL)
    return FALSE;
  return ek_debug2(type, p0);
}

Private int
ek_debug2(char type, char *p0)
{
  char  *p;

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
