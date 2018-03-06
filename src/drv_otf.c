/* Experimental.  
   No METRIC method. 
   No OUTLINE method. 
   Only BITMAP Method. */

/*
 * drv_otf - A font driver for OpenType fonts with FreeType2 library.
 * by Hirotsugu Kakugawa
 *
 * 10 Apr 2010  First implementation by FreeType 2, based on dtv_ttf.c.
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
 *    n - the number of fonts in a font file
 *    c - code mapping table information (ccv info)
 *    p - code mapping table information (non-ccv info)
 *    i - print char index
 *    m - print metrics
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
#include  <sys/param.h>
#include  "VFlib-3_7.h"
#include  "VFsys.h"
#include  "vflibcap.h"
#include  "bitmap.h"
#include  "cache.h"
#include  "fsearch.h"
#include  "str.h"
#include  "sexp.h"
#include  "ccv.h"


#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_TRUETYPE_IDS_H

#include  "otf.h"

#define DIRECTION_HORIZONTAL  0
#define DIRECTION_VERTICAL    1


Private SEXP_LIST    default_fontdirs;
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
Private SEXP_STRING  default_platform_id;
Private int          v_default_platform_id;
Private SEXP_STRING  default_encoding_id;
Private int          v_default_encoding_id;
Private SEXP_STRING  default_hinting;
Private int          v_default_hinting;
Private SEXP_ALIST   default_properties;
Private SEXP_ALIST   default_variables;
Private SEXP_STRING  default_debug_mode;
Private char        *env_debug_mode = NULL;
#define DEBUG_ENV_NAME   "VFLIB_OTF_DEBUG"


struct s_font_otf {
  int       release_on_close;
  int       otf_opened;
  FT_Face             ft2_face;
  FT_CharMap          ft2_charmap;
  char     *font_name;
  char     *file_path;
  double    point_size;
  double    pixel_size;
  double    dpi_x, dpi_y;
  double    aspect;
  double    mag;
  char      direction;
  int       font_number;
  int       platform_id;
  int       encoding_id;
  int       mapping_id;
  int       encoding_force;
  int       hinting;
  char     *charset_name;
  char     *encoding_name;
  SEXP      props;
  int       ccv_id;
};
typedef struct s_font_otf  *FONT_OTF;


struct s_id_name_tbl {
  int   id;
  char  *name;
};
typedef struct s_id_name_tbl  *ID_NAME_TBL;

 
struct s_fontbbx1 {
  double  w, h;
  double  xoff, yoff;
};
typedef struct s_fontbbx1  *FONTBBX1;
struct s_fontbbx2 {
  int     w, h;
  int     xoff, yoff;
};
typedef struct s_fontbbx2  *FONTBBX2;

#define MODE_METRIC1  1
#define MODE_BITMAP1  2
#define MODE_FONTBBX1 3
#define MODE_OUTLINE  4
#define MODE_METRIC2  5
#define MODE_FONTBBX2 6
#define MODE_BITMAP2  7

Private FT_Face *otf_open_method(char*,long,long,VF_FONT,FONT_OTF);
Private void     otf_close_method(FT_Face*,long,long,VF_FONT,FONT_OTF);

Private void* otf_get_xxx(int mode,
			  VF_FONT font, long code_point,
			  double mag_x, double mag_y, 
			  VF_METRIC1 metric1, VF_METRIC2 metric2,
			  FONTBBX1 bbx1, FONTBBX2 bbx2);

Private int         otf_create(VF_FONT,char*,char*,int,SEXP);
Private int         otf_close(VF_FONT);
Private int         otf_get_metric1(VF_FONT,long,VF_METRIC1,double,double);
Private int         otf_get_metric2(VF_FONT,long,VF_METRIC2,double,double);
Private int         otf_get_fontbbx1(VF_FONT font,double,double,
				     double*,double*,double*,double*);
Private int         otf_get_fontbbx2(VF_FONT font, double,double,
				     int*,int*,int*,int*);
Private VF_BITMAP   otf_get_bitmap1(VF_FONT,long,double,double);
Private VF_BITMAP   otf_get_bitmap2(VF_FONT,long,double,double);
Private VF_OUTLINE  otf_get_outline1(VF_FONT,long,double,double);
Private char       *otf_get_font_prop(VF_FONT,char*);

Private int    find_encoding_mapping(FONT_OTF);
Private int      get_id_from_platform_name(char*);
Private int      get_id_from_encoding_name(char*,int);
Private int         name2id(char*,ID_NAME_TBL,int,char*);
Private char*  conv_encoding_otf_to_vflib(int otf_enc, int plat);
Private char*  platform_id2name(int plat_id);
Private char*  encoding_id2name(int,int);
Private int    otf_debug(char);




static FT_Library   FreeType2_Library;
static int          Initialized_FreeType2 = 0;



Public int
VF_Init_Driver_OpenType(void)
{
  int    error;
  char  *p;
  struct s_capability_table  ct[20];
  int    z;

  z = 0;
  /* VF_CAPE_FONT_DIRECTORIES */
  ct[z].cap = VF_CAPE_FONT_DIRECTORIES;  ct[z].type = CAPABILITY_LIST;
  ct[z].ess = CAPABILITY_OPTIONAL;       ct[z++].val = &default_fontdirs;
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
  /* VF_CAPE_ASPECT_RATIO */
  ct[z].cap = VF_CAPE_ASPECT_RATIO;      ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;       ct[z++].val = &default_aspect;
  /* VF_CAPE_DIRECTION */
  ct[z].cap = VF_CAPE_DIRECTION;         ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;       ct[z++].val = &default_direction;
  /* VF_CAPE_OTF_PLATFORM_ID */
  ct[z].cap = VF_CAPE_OTF_PLATFORM_ID;   ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;       ct[z++].val = &default_platform_id;
  /* VF_CAPE_OTF_ENCODING_ID */
  ct[z].cap = VF_CAPE_OTF_ENCODING_ID;   ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;       ct[z++].val = &default_encoding_id;
  /* VF_CAPE_OTF_HINTING */
  ct[z].cap = VF_CAPE_OTF_HINTING;       ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;       ct[z++].val = &default_hinting;
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

  if (Initialized_FreeType2 == 0){
    error = FT_Init_FreeType(&FreeType2_Library);
    if (error){
      vf_error = VF_ERR_FREETYPE_INIT;
      return -1;
    }
    Initialized_FreeType2 = 1;
  }

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

  v_default_dpi_x  = VF_DEFAULT_DPI;
  v_default_dpi_y  = VF_DEFAULT_DPI;
  if (default_dpi != NULL)
    v_default_dpi_x = v_default_dpi_y = atof(vf_sexp_get_cstring(default_dpi));
  if (default_dpi_x != NULL)
    v_default_dpi_x = atof(vf_sexp_get_cstring(default_dpi_x));
  if (default_dpi_y != NULL)
    v_default_dpi_y = atof(vf_sexp_get_cstring(default_dpi_y));
  if (v_default_dpi_x < 0)
    v_default_dpi_x = VF_DEFAULT_DPI;
  if (v_default_dpi_y < 0)
    v_default_dpi_y = VF_DEFAULT_DPI;

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
      fprintf(stderr, "VFlib Warning: Unknown writing direction: %s\n", p);
      break;
    }
  }

  v_default_platform_id = TT_PLAT_ID_MICROSOFT;
  if (default_platform_id != NULL)
    v_default_platform_id
      = get_id_from_platform_name(vf_sexp_get_cstring(default_platform_id));

  v_default_encoding_id = TT_ENC_ID_ANY;
  if (default_encoding_id != NULL)
    v_default_encoding_id
      = get_id_from_encoding_name(vf_sexp_get_cstring(default_encoding_id),
				  v_default_platform_id);

  v_default_hinting = TRUE;
  if (default_hinting != NULL){
    v_default_hinting = vf_parse_bool(vf_sexp_get_cstring(default_hinting));
  }

  env_debug_mode = getenv(DEBUG_ENV_NAME);

  VF_InstallFontDriver(FONTCLASS_NAME, (DRIVER_FUNC_TYPE)otf_create);

  return 0;
}
  

Private int
otf_create(VF_FONT font, char *font_class, char *font_name, 
	   int implicit, SEXP entry)
{
  FONT_OTF   font_otf;
  char       *font_file, *font_path, *p;
  int        val;
  SEXP       cap_fontdirs, cap_font, cap_point, cap_pixel;
  SEXP       cap_dpi, cap_dpi_x, cap_dpi_y, cap_mag, cap_aspect;
  SEXP       cap_font_number, cap_direction, cap_platform_id, cap_encoding_id;
  SEXP       cap_hinting, cap_mapping_id, cap_encoding_force;
  SEXP       cap_charset, cap_encoding, cap_props;
  struct s_capability_table  ct[30];
  int  z;

  z = 0;
  /* VF_CAPE_FONT_CLASS */
  ct[z].cap = VF_CAPE_FONT_CLASS;         ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_ESSENTIAL;       ct[z++].val = NULL;
  /* VF_CAPE_FONT_DIRECTORIES */
  ct[z].cap = VF_CAPE_FONT_DIRECTORIES;   ct[z].type = CAPABILITY_STRING_LIST1;
  ct[z].ess = CAPABILITY_OPTIONAL;        ct[z++].val = &cap_fontdirs;
  /* VF_CAPE_FONT_FILE */
  ct[z].cap = VF_CAPE_FONT_FILE;          ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;        ct[z++].val = &cap_font;
  /* VF_CAPE_POINT_SIZE */
  ct[z].cap = VF_CAPE_POINT_SIZE;         ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;        ct[z++].val = &cap_point;
  /* VF_CAPE_PIXEL_SIZE */
  ct[z].cap = VF_CAPE_PIXEL_SIZE;         ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;        ct[z++].val = &cap_pixel;
  /* VF_CAPE_DPI */
  ct[z].cap = VF_CAPE_DPI;                ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;        ct[z++].val = &cap_dpi;
  /* VF_CAPE_DPI_X */
  ct[z].cap = VF_CAPE_DPI_X;              ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;        ct[z++].val = &cap_dpi_x;
  /* VF_CAPE_DPI_Y */
  ct[z].cap = VF_CAPE_DPI_Y;              ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;        ct[z++].val = &cap_dpi_y;
  /* VF_CAPE_MAG */
  ct[z].cap = VF_CAPE_MAG;                ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;        ct[z++].val = &cap_mag;
  /* VF_CAPE_ASPECT_RATIO */
  ct[z].cap = VF_CAPE_ASPECT_RATIO;       ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;        ct[z++].val = &cap_aspect;
  /* VF_CAPE_DIRECTION */
  ct[z].cap = VF_CAPE_DIRECTION;          ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;        ct[z++].val = &cap_direction;
  /* VF_CAPE_OTF_FONT_NUMBER */
  ct[z].cap = VF_CAPE_OTF_FONT_NUMBER;    ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;        ct[z++].val = &cap_font_number;
  /* VF_CAPE_OTF_PLATFORM_ID */
  ct[z].cap = VF_CAPE_OTF_PLATFORM_ID;    ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;        ct[z++].val = &cap_platform_id;
  /* VF_CAPE_OTF_ENCODING_ID */
  ct[z].cap = VF_CAPE_OTF_ENCODING_ID;    ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;        ct[z++].val = &cap_encoding_id;
  /* VF_CAPE_OTF_MAPPING_ID */
  ct[z].cap = VF_CAPE_OTF_MAPPING_ID;     ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;        ct[z++].val = &cap_mapping_id;
  /* VF_CAPE_OTF_HINTING */
  ct[z].cap = VF_CAPE_OTF_HINTING;        ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;        ct[z++].val = &cap_hinting;
  /* VF_CAPE_OTF_ENCODING_FORCE */
  ct[z].cap = VF_CAPE_OTF_ENCODING_FORCE; ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;        ct[z++].val = &cap_encoding_force;
  /* VF_CAPE_CHARSET */
  ct[z].cap = VF_CAPE_CHARSET;            ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;        ct[z++].val = &cap_charset;
  /* VF_CAPE_ENCODING */
  ct[z].cap = VF_CAPE_ENCODING;           ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;        ct[z++].val = &cap_encoding;
  /* VF_CAPE_PROPERTIES */
  ct[z].cap = VF_CAPE_PROPERTIES;         ct[z].type = CAPABILITY_ALIST;
  ct[z].ess = CAPABILITY_OPTIONAL;        ct[z++].val = &cap_props;
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

  font_path = NULL;
  if ((implicit == 0) && (cap_fontdirs != NULL)){
    font_path = vf_search_file(font_file, -1, NULL, 
			       TRUE, FSEARCH_FORMAT_TYPE_OTF, 
			       cap_fontdirs, NULL, NULL);
  }
  if (font_path == NULL){
    font_path = vf_search_file(font_file, -1, NULL, 
			       TRUE, FSEARCH_FORMAT_TYPE_OTF, 
			       default_fontdirs, NULL, NULL);
  }
  if (font_path == NULL){
    vf_error = VF_ERR_NO_FONT_FILE;
    return -1;
  }

  if (otf_debug('f')) 
    printf("VFlib OpenType: font file %s\n   ==> %s\n", font_file, font_path);

  font->font_type       = VF_FONT_TYPE_OUTLINE;
  font->get_metric1     = otf_get_metric1;
  font->get_metric2     = otf_get_metric2;
  font->get_fontbbx1    = otf_get_fontbbx1;
  font->get_fontbbx2    = otf_get_fontbbx2;
  font->get_bitmap1     = otf_get_bitmap1;
  font->get_bitmap2     = otf_get_bitmap2;
  font->get_outline     = otf_get_outline1;
  font->get_font_prop   = otf_get_font_prop;
  font->query_font_type = NULL;  /* Use font->font_type value. */
  font->close           = otf_close;

  ALLOC_IF_ERR(font_otf, struct s_font_otf){
    vf_error = VF_ERR_NO_MEMORY;
    return -1;
  }
  font->private = font_otf;

  font_otf->release_on_close = 0;
  font_otf->otf_opened       = 0;
  font_otf->font_name      = NULL;
  font_otf->file_path      = font_path;
  font_otf->point_size     = v_default_point_size;
  font_otf->pixel_size     = v_default_pixel_size;
  font_otf->mag            = 1.0;
  font_otf->dpi_x          = v_default_dpi_x;
  font_otf->dpi_y          = v_default_dpi_y;
  font_otf->aspect         = v_default_aspect;
  font_otf->direction      = v_default_direction;
  font_otf->font_number    = -1;
  font_otf->platform_id    = v_default_platform_id;
  font_otf->encoding_id    = v_default_encoding_id;
  font_otf->mapping_id     = TT_MAP_ID_SEARCH;
  font_otf->encoding_force = -1;
  font_otf->hinting        = v_default_hinting;
  font_otf->charset_name   = NULL;
  font_otf->encoding_name  = NULL;

  if (implicit == 0){
    if (cap_point != NULL)
      font_otf->point_size = atof(vf_sexp_get_cstring(cap_point));
    if (cap_pixel != NULL)
      font_otf->pixel_size = atof(vf_sexp_get_cstring(cap_pixel));
    if (cap_dpi != NULL)
      font_otf->dpi_x = font_otf->dpi_y = atof(vf_sexp_get_cstring(cap_dpi));
    if (cap_dpi_x != NULL)
      font_otf->dpi_x = atof(vf_sexp_get_cstring(cap_dpi_x));
    if (cap_dpi_y != NULL)
      font_otf->dpi_y = atof(vf_sexp_get_cstring(cap_dpi_y));
    if (cap_mag != NULL)
      font_otf->mag = atof(vf_sexp_get_cstring(cap_mag));
    if (cap_aspect != NULL)
      font_otf->aspect = atof(vf_sexp_get_cstring(cap_aspect));
    if (cap_direction != NULL){
      p = vf_sexp_get_cstring(cap_direction);
      switch (*p){
      case 'h': case 'H':
	font_otf->direction = DIRECTION_HORIZONTAL;  break;
      case 'v': case 'V':
	font_otf->direction = DIRECTION_VERTICAL;    break;
      default:
	fprintf(stderr, "VFlib Warning: Unknown writing direction: %s\n", p);
	break;
      }
    }
    if (cap_font_number != NULL)
      font_otf->font_number = atoi(vf_sexp_get_cstring(cap_font_number));
    if (cap_platform_id != NULL)
      font_otf->platform_id 
	= get_id_from_platform_name(vf_sexp_get_cstring(cap_platform_id));
    if (cap_encoding_id != NULL)
      font_otf->encoding_id
	= get_id_from_encoding_name(vf_sexp_get_cstring(cap_encoding_id),
				    font_otf->platform_id);
    if (cap_mapping_id != NULL)
      font_otf->mapping_id = atoi(vf_sexp_get_cstring(cap_mapping_id));
    if (cap_encoding_force != NULL)
      font_otf->encoding_force
	= get_id_from_encoding_name(vf_sexp_get_cstring(cap_encoding_force),
				    font_otf->platform_id);
    if (cap_charset != NULL)
      font_otf->charset_name = vf_strdup(vf_sexp_get_cstring(cap_charset));
    if (cap_encoding != NULL)
      font_otf->encoding_name = vf_strdup(vf_sexp_get_cstring(cap_encoding));
    if (cap_props != NULL)
      font_otf->props = cap_props;
    if (cap_hinting != NULL){
      font_otf->hinting = vf_parse_bool(vf_sexp_get_cstring(cap_hinting));
    }
  }

  if ((font_otf->font_name = vf_strdup(font_name)) == NULL){
    vf_error = VF_ERR_NO_MEMORY;
    goto Error;
  }

  if (vf_fm_OpenFileStreamApp(font_otf->file_path,
			      font_otf->font_number, -1, font, font_otf,
			      (FM_OPEN_METHOD)  otf_open_method, 
			      (FM_CLOSE_METHOD) otf_close_method, 
			      "FreeType") == NULL)
    goto Error;

  font_otf->ccv_id = find_encoding_mapping(font_otf);

  val = 0;

Error:
  if (implicit == 0){   /* explicit font */
    vf_sexp_free4(&cap_fontdirs, &cap_font, &cap_point, &cap_pixel);
    vf_sexp_free3(&cap_dpi, &cap_dpi_x, &cap_dpi_y);
    vf_sexp_free3(&cap_mag, &cap_aspect, &cap_direction);
    vf_sexp_free3(&cap_platform_id, &cap_encoding_id, &cap_hinting);
    vf_sexp_free3(&cap_mapping_id, &cap_encoding_force, &cap_charset);
    vf_sexp_free1(&cap_encoding);
  }

  if ((val != 0) && (font_otf != NULL)){
    vf_sexp_free1(&cap_props); 
    vf_free(font_otf->font_name);
    vf_free(font_otf->file_path);
    vf_free(font_otf->charset_name);
    vf_free(font_otf->encoding_name);
    vf_free(font_otf); 
  }

  return val;
}


Private int
otf_close(VF_FONT font)
{
  FONT_OTF  font_otf;

  if ((font_otf = (FONT_OTF)font->private) != NULL){
    vf_free(font_otf->charset_name);
    vf_free(font_otf->encoding_name);
    font_otf->release_on_close = 1;
    /* `font_otf' and `font_otf->font_name' are
       released in otf_close_method() */
  }
  return 0; 
}

Private FT_Face*
otf_open_method(char *font_path, 
		long fontnum, 
		long iarg2,
		VF_FONT font, 
		FONT_OTF font_otf)
{
  int  fn; 
  int  error;

  if (font_otf == NULL)
    return NULL;
  
  if (font_otf->otf_opened == 0){
    if ((fn = fontnum) < 0) {
      fn = 0;
    }
    if (otf_debug('f')) 
      printf("VFlib OpenType: FT_Open_Face %s\n", font_otf->font_name);
    error = FT_New_Face(FreeType2_Library, font_path, fn, &font_otf->ft2_face);
    if (error)
      return NULL;
    font_otf->otf_opened = 1;
#if 0
    (void) TT_Get_Face_Properties(font_otf->ft2_face, &font_otf->ft2_fprops);
    font_otf->tt_upem = font_otf->tt_fprops.header->Units_Per_EM;
    if (otf_debug('n')) 
      printf("VFlib OpenType: the number of embedded faces: %ld\n",
	     (long)font_otf->ft2_fprops.num_Faces);
#endif
  }

  return  &font_otf->ft2_face;
}

Private void
otf_close_method(FT_Face *otface, 
		 long iarg1, 
		 long iarg2, 
		 VF_FONT font, 
		 FONT_OTF font_otf)
{
  if (font_otf->release_on_close == 0){
    /* close temporality by the limitation of
       the number of simultaneously opened files */
#if 0
    if (otf_debug('f')) 
      printf("VFlib OpenType: TT_Flush_Face %s\n", font_otf->font_name);
    FT_Flush_Face(font_otf->ft2_face);
#endif
  } else {
    /* after a font is closed */
    if (otf_debug('f')) 
      printf("VFlib OpenType: FT_Done_Face %s\n", font_otf->font_name);
    FT_Done_Face(font_otf->ft2_face);
    vf_free(font_otf->font_name); 
    vf_free(font_otf->file_path);
    vf_free(font_otf);
  }
} 


Private int
otf_get_metric1(VF_FONT font, long code_point, VF_METRIC1 metric, 
		double mag_x, double mag_y)
{
  if (otf_get_xxx(MODE_METRIC1, font, code_point, mag_x, mag_y, 
		  metric, NULL, NULL, NULL) == NULL)
    return -1;
  return 0;
}

Private int
otf_get_fontbbx1(VF_FONT font, double mag_x, double mag_y,
		 double *w_p, double *h_p, double *xoff_p, double *yoff_p)
{
  struct s_fontbbx1  bbx1;

  if (otf_get_xxx(MODE_FONTBBX1, font, -1, mag_x, mag_y, 
		  NULL, NULL, &bbx1, NULL) == NULL)
    return -1;

  *w_p    = bbx1.w;
  *h_p    = bbx1.h; 
  *xoff_p = bbx1.xoff;
  *yoff_p = bbx1.yoff;
  return 0;
}

Private VF_BITMAP
otf_get_bitmap1(VF_FONT font, long code_point,
		double mag_x, double mag_y)
{
  return (VF_BITMAP)otf_get_xxx(MODE_BITMAP1, font, code_point, mag_x, mag_y, 
				NULL, NULL, NULL, NULL);
}


Private VF_OUTLINE
otf_get_outline1(VF_FONT font, long code_point,
		 double mag_x, double mag_y)
{
  return (VF_OUTLINE)otf_get_xxx(MODE_OUTLINE, font, code_point, mag_x, mag_y, 
				 NULL, NULL, NULL, NULL);
}


Private int
otf_get_metric2(VF_FONT font, long code_point, VF_METRIC2 metric,
		double mag_x, double mag_y)
{
  if (otf_get_xxx(MODE_METRIC2, font, code_point, mag_x, mag_y,
		  NULL, metric, NULL, NULL) == NULL)
    return -1;
  return 0;
}

Private int
otf_get_fontbbx2(VF_FONT font, double mag_x, double mag_y,
		 int *w_p, int *h_p, int *xoff_p, int *yoff_p)
{
  struct s_fontbbx2  bbx2;

  if (otf_get_xxx(MODE_FONTBBX2, font, -1, mag_x, mag_y, 
		  NULL, NULL, NULL, &bbx2) == NULL)
    return -1;

  *w_p    = bbx2.w;
  *h_p    = bbx2.h; 
  *xoff_p = bbx2.xoff;
  *yoff_p = bbx2.yoff;
  return 0;
}

Private VF_BITMAP
otf_get_bitmap2(VF_FONT font, long code_point, 
		double mag_x, double mag_y)
{
  return (VF_BITMAP)otf_get_xxx(MODE_BITMAP2,
				font, code_point, mag_x, mag_y, 
				NULL, NULL, NULL, NULL);
}


static void oft_dump_bitmap_freetype2(FT_Bitmap bm);


Private void*
otf_get_xxx(int mode, 
	    VF_FONT font, long code_point, 
	    double mag_x, double mag_y,
	    VF_METRIC1 metric1, VF_METRIC2 metric2,
	    FONTBBX1 fontbbx1, FONTBBX2 fontbbx2)
{
  FONT_OTF   font_otf;
  VF_BITMAP  bm;
  void      *val;
  double     ps = 0.0, mx, my, asp, aspd; 
  double dpix = 0.0, dpiy = 0.0; 
  long       cp;
  int        chindex;
  int        bmX, bmY;
  int        error;

  error = 0;

  if ((font_otf = (FONT_OTF)font->private) == NULL){
    fprintf(stderr, "VFlib: Internal error in otf_get_xxx()\n");
    abort();
  }

  mx = mag_x * font_otf->mag * font->mag_x;
  my = mag_y * font_otf->mag * font->mag_y;
  asp = v_default_aspect * font_otf->aspect * (mx / my);

  if (   (mode == MODE_METRIC1) 
      || (mode == MODE_FONTBBX1) 
      || (mode == MODE_BITMAP1) 
      || (mode == MODE_OUTLINE) ){
    if (((dpix = font->dpi_x) <= 0) || ((dpiy = font->dpi_y) <= 0)){
      dpix = font_otf->dpi_x;
      dpiy = font_otf->dpi_y;
    }
    if ((ps = font->point_size) < 0)
      ps = font_otf->point_size;
    ps = ps * my;
  } else if (   (mode == MODE_METRIC2) 
	     || (mode == MODE_FONTBBX2) 
	     || (mode == MODE_BITMAP2) ){
    if ((ps = font->pixel_size) < 0)
      ps = font_otf->pixel_size;
    ps = ps * my;
    dpix = POINTS_PER_INCH;
    dpiy = POINTS_PER_INCH;
  }

  vf_fm_OpenFileStreamApp(font_otf->file_path,
			  font_otf->font_number, -1, font, font_otf,
			  (FM_OPEN_METHOD) otf_open_method, 
			  (FM_CLOSE_METHOD)otf_close_method, 
			  "FreeType2");

  if (mode != MODE_OUTLINE){
    error = FT_Set_Char_Size(font_otf->ft2_face, 0, ps*64, dpix, dpiy);
  } else {
    error = FT_Set_Char_Size(font_otf->ft2_face, 0, 12*64, 96, 96);
  } 
  if (error){
    vf_error = VF_ERR_NO_GLYPH;
    return NULL;
  }


  if ((mode == MODE_FONTBBX1) || (mode == MODE_FONTBBX2)){
    chindex = -1;
  } else {
    cp = code_point;
    if (font_otf->ccv_id >= 0)
      cp = vf_ccv_conv(font_otf->ccv_id, code_point);
    
    if (otf_debug('c')) 
      printf("VFlib OpenType: CCV  0x%lx => 0x%lx\n", code_point, cp);

    if (cp < 0){
      vf_error = VF_ERR_ILL_CODE_POINT;
      return NULL;
    }

    chindex = FT_Get_Char_Index(font_otf->ft2_face, (int)cp);
    if (chindex <= 0){
      vf_error = VF_ERR_ILL_CODE_POINT;
      return NULL;
    }
    if (otf_debug('i')) 
      printf("VFlib OpenType: charindex 0x%x  (code point 0x%lx)\n", 
	     chindex, code_point);

    error = FT_Load_Glyph(font_otf->ft2_face, chindex, FT_LOAD_DEFAULT);
    if (error){
      vf_error = VF_ERR_NO_GLYPH;
      return NULL;
    }
    /*(void) TT_Get_Glyph_Metrics(font_otf->tt_glyph, &tt_metrics);*/
  }
  /*(void) TT_Get_Instance_Metrics(font_otf->tt_instance, &tt_imetrics);*/

#if 0 
  if (otf_debug('m')){
    printf("VFlib OpenType: Metrics\n");
    printf("  in Header   upem: %d, xMin: %d, yMin:%d, xMax:%d, yMax:%d\n",
	   font_otf->tt_fprops.header->Units_Per_EM,
	   font_otf->tt_fprops.header->xMin, 
	   font_otf->tt_fprops.header->yMin, 
	   font_otf->tt_fprops.header->xMax, 
	   font_otf->tt_fprops.header->yMax);
    printf("  tt_metrics.bbx  xMin:%ld, yMin:%ld, xMax:%ld, yMax:%ld\n",
	   tt_metrics.bbox.xMin, tt_metrics.bbox.yMin, 
	   tt_metrics.bbox.xMax, tt_metrics.bbox.yMax);
    printf("  tt_imetrics  x_ppem:%d, y_ppem:%d, pointSize:%.4f, upem:%d\n",
	   tt_imetrics.x_ppem, tt_imetrics.y_ppem, 
	   (double)tt_imetrics.pointSize/64.0, font_otf->tt_upem);
  }
#endif

  val = NULL;
  if ((mode == MODE_BITMAP1) || (mode == MODE_BITMAP2)){
    aspd = 1.0 - asp;
    if (aspd < 0)
      aspd = 0.0 - aspd;
    if (aspd < 1.0e-6){
      error = FT_Render_Glyph(font_otf->ft2_face->glyph, FT_RENDER_MODE_MONO);
      if (error)
	return NULL;
    } else {
      FT_Matrix  mat;
      mat.xx = asp * (1<<16);  mat.xy = 0;
      mat.yx = 0;              mat.yy = 1 * (1<<16);
      FT_Glyph_Transform(font_otf->ft2_face->glyph, &mat, NULL); 
      error = FT_Render_Glyph(font_otf->ft2_face->glyph, FT_RENDER_MODE_MONO);
    }

    ALLOC_IF_ERR(bm, struct vf_s_bitmap){
      vf_error = VF_ERR_NO_MEMORY;
      return NULL;
    }
    bm->bbx_width  = font_otf->ft2_face->glyph->bitmap.width;
    bm->bbx_height = font_otf->ft2_face->glyph->bitmap.rows;
    bm->raster     = font_otf->ft2_face->glyph->bitmap.pitch;
    bm->bitmap     = malloc(((bm->bbx_width+7)/8) * bm->bbx_height);
    memclr(bm->bitmap,      ((bm->bbx_width+7)/8) * bm->bbx_height);
    bm->off_x      = -font_otf->ft2_face->glyph->bitmap_left;
    bm->off_y      =  font_otf->ft2_face->glyph->bitmap_top;
    bm->mv_x       = font_otf->ft2_face->glyph->advance.x/64;
    bm->mv_y       = 0;
    for (bmY = 0; 
	 bmY < font_otf->ft2_face->glyph->bitmap.rows; 
	 bmY++) {
      for (bmX = 0; 
	   bmX < (font_otf->ft2_face->glyph->bitmap.width+7)/8;
	   bmX++) {
	bm->bitmap[bm->raster*bmY + bmX] 
	  = font_otf->ft2_face->glyph->bitmap.buffer
            [font_otf->ft2_face->glyph->bitmap.pitch*bmY + bmX]; 
      }
    }
#if 0
    oft_dump_bitmap_freetype2(font_otf->ft2_face->glyph->bitmap);  /*XXX*/
#endif
#if 0
    vf_dump_bitmap(bm);                                            /*XXX*/
#endif
    val = (void*) bm;
    
  } else if (mode == MODE_METRIC1){  
    if (metric1 != NULL){
      metric1->bbx_width  = 0;    /* orz */
      metric1->bbx_height = 0;
      metric1->off_x = 0;
      metric1->off_y = 0;
      metric1->mv_x  = 0;
      metric1->mv_y  = 0;
    }
    val = (void*) metric1;

  } else if (mode == MODE_METRIC2){
    if (metric2 != NULL){
      metric2->bbx_width  = 0;    /* orz */
      metric2->bbx_height = 0;
      metric2->off_x = 0;
      metric2->off_y = 0;
      metric2->mv_x  = 0;
      metric2->mv_y  = 0;
    }
    val = (void*) metric2;

  } else if (mode == MODE_OUTLINE){
    val = (void*) NULL;

  } else if (mode == MODE_FONTBBX1){
    if (fontbbx1 != NULL){
      fontbbx1->w = 0;    /* orz */
      fontbbx1->h = 0;
      fontbbx1->xoff = 0;
      fontbbx1->yoff = 0;
    }
    val = (void*) fontbbx1;

  } else if (mode == MODE_FONTBBX2){
    if (fontbbx2 != NULL){
      fontbbx2->w = 0;   /* orz */
      fontbbx2->h = 0; 
      fontbbx2->xoff = 0; 
      fontbbx2->yoff = 0; 
    }
    val = (void*) fontbbx2;

  } else {
    fprintf(stderr, "VFlib: Internal error in otf_get_xxx1()\n");
    fprintf(stderr, "Unknown mode: %d\n", mode);
    abort();
  }

  return val;
}

static void oft_dump_bitmap_freetype2(FT_Bitmap bm)
{
  int bmX, bmY;

  for (bmY = 0; bmY < bm.rows; bmY++) {
    for (bmX = 0; bmX < bm.width; bmX++) {
      if ((bm.buffer[bm.pitch*bmY+(bmX/8)] & (1<<(7-bmX%8))) != 0) {
	printf("*"); 
      } else {
	printf(" "); 
      }
    }
    printf("\n"); 
  }
}



Private char*
otf_get_font_prop(VF_FONT font, char *prop_name)
{ /* CALLER MUST RELEASE RETURNED STRING LATER */
  SEXP       v;
  FONT_OTF   font_otf;
  char       str[1024];
  double     dpix, dpiy, p;

  if ((font_otf = (FONT_OTF)font->private) == NULL){
    fprintf(stderr, "VFlib: internal error in otf_get_font_prop()\n");
    abort();
  }

  if ((v = vf_sexp_assoc(prop_name, font_otf->props)) != NULL){
    return vf_strdup(vf_sexp_get_cstring(vf_sexp_cadr(v)));
  } else if ((v = vf_sexp_assoc(prop_name, default_properties)) != NULL){
    return vf_strdup(vf_sexp_get_cstring(vf_sexp_cadr(v)));
  }

  if (font->mode == 1){
    if ((dpix = font->dpi_x) < 0)
      if ((dpix = font_otf->dpi_x) < 0)
	dpix = v_default_dpi_x; 
    if ((dpiy = font->dpi_y) < 0)
      if ((dpiy = font_otf->dpi_y) < 0)
	dpiy = v_default_dpi_y; 
    if ((p = font->point_size) < 0)
      if ((p = font_otf->point_size) < 0)
	p = v_default_point_size;
    p = p * font->mag_y * font_otf->mag;
    if (strcmp(prop_name, "POINT_SIZE") == 0){  
      snprintf(str, sizeof(str), "%d", toint(p * 10.0)); 
      return vf_strdup(str);
    } else if (strcmp(prop_name, "PIXEL_SIZE") == 0){
      snprintf(str, sizeof(str), "%d", toint(p * dpiy / POINTS_PER_INCH));
      return vf_strdup(str);
    } else if (strcmp(prop_name, "RESOLUTION_X") == 0){
      snprintf(str, sizeof(str), "%d", toint(dpix)); 
      return vf_strdup(str);
    } else if (strcmp(prop_name, "RESOLUTION_Y") == 0){
      snprintf(str, sizeof(str), "%d", toint(dpiy)); 
      return vf_strdup(str);
    }

  } else if (font->mode == 2){
    if ((p = font->pixel_size) < 0)
      if ((p = font_otf->pixel_size) < 0)
	p = v_default_pixel_size;
    p = p * font->mag_y * font_otf->mag;
    if (strcmp(prop_name, "POINT_SIZE") == 0){  
      snprintf(str, sizeof(str), 
	       "%d", toint(p * 10.0 * POINTS_PER_INCH / VF_DEFAULT_DPI)); 
    } else if (strcmp(prop_name, "PIXEL_SIZE") == 0){
      snprintf(str, sizeof(str),
	       "%d", toint(p));
      return vf_strdup(str);
    } else if (strcmp(prop_name, "RESOLUTION_X") == 0){
      snprintf(str, sizeof(str), 
	       "%d", toint(VF_DEFAULT_DPI)); 
      return vf_strdup(str);
    } else if (strcmp(prop_name, "RESOLUTION_Y") == 0){
      snprintf(str, sizeof(str), 
	       "%d", toint(VF_DEFAULT_DPI)); 
      return vf_strdup(str);
    }
  }
    
#if 0
  if (strcmp(prop_name, "FONT_ASCENT") == 0){
    ;
  } else if (strcmp(prop_name, "FONT_DESCENT") == 0){
    ;
  }
#endif

  return NULL;
}



Private int
find_encoding_mapping(FONT_OTF font_otf)
{
  int               ccv_id, map, ntables;
  char              *internal_enc, *cs;
  unsigned short    platform_id, encoding_id;
  int               error;

  if (otf_debug('p')){
    printf("VFlib OpenType: Searching platform:%d (%s), encoding:%d (%s))\n", 
	   font_otf->platform_id,
	   platform_id2name(font_otf->platform_id), 
	   font_otf->encoding_id,
	   encoding_id2name(font_otf->platform_id,font_otf->encoding_id));
  }

  /* Find mapping table number to be used in OpenType font file. */
  map = font_otf->mapping_id;
  ntables = font_otf->ft2_face->num_charmaps;
  if (map != TT_MAP_ID_SEARCH){
    if (ntables == 0){
      fprintf(stderr, "VFlib OpenType warning: No mapping tables: %s\n",
	      font_otf->font_name);
      return -1;
    } else if (ntables < 0){
      fprintf(stderr, "VFlib OpenType Internal error: #charmaps %s\n",
	      font_otf->font_name);
      return -1;
    } else if ((map < 0) || (ntables <= map)){
      map = 0;
      fprintf(stderr, "VFlib OpenType warning: Illegal mapping table ID.");
      fprintf(stderr, "Use mapping table #0.\n");
    }
    platform_id = font_otf->ft2_face->charmaps[map]->platform_id;
    encoding_id = font_otf->ft2_face->charmaps[map]->encoding_id;
    if (otf_debug('p')) 
      printf("VFlib OpenType:  Platform,Encoding=(%s,%s)\n", 
	     platform_id2name(platform_id), 
	     encoding_id2name(platform_id, encoding_id));
  } else {
    platform_id = 0;
    encoding_id = 0;
    for (map = 0; map < ntables; map++){
      platform_id = font_otf->ft2_face->charmaps[map]->platform_id;
      encoding_id = font_otf->ft2_face->charmaps[map]->encoding_id;
      if (otf_debug('p'))
	printf("VFlib OpenType:   mapping #%d: Platform: %s, Encoding:%s\n", 
	       map, 
	       platform_id2name(platform_id), 
	       encoding_id2name(platform_id, encoding_id));
      if (   ((font_otf->platform_id == TT_PLAT_ID_ANY)
	      || (platform_id == font_otf->platform_id))
	  && ((font_otf->encoding_id == TT_ENC_ID_ANY) 
	      || (encoding_id == font_otf->encoding_id)) ){
	break;
      }
      font_otf->mapping_id = 0;
    }
    if (map == ntables){
      fprintf(stderr, "VFlib OpenType: Mapping table not found.\n");
      return -1;
    }
  }

  font_otf->mapping_id = map;
  if (otf_debug('p'))
    printf("VFlib OpenType: Use mapping table #%d (encoding#%d)\n", 
	   font_otf->mapping_id, encoding_id);
  
  error = FT_Set_Charmap(font_otf->ft2_face, 
			 font_otf->ft2_face->charmaps[font_otf->mapping_id]);
  if (error) {
      fprintf(stderr, "VFlib OpenType: FT_Set_Charmap() failed.\n");
    exit(1);
  }

  if (font_otf->encoding_force < 0){
    internal_enc = conv_encoding_otf_to_vflib(encoding_id, platform_id);
  } else {
    internal_enc = conv_encoding_otf_to_vflib(font_otf->encoding_force,
					      platform_id);
    if (otf_debug('p')){
      printf("VFlib OpenType: Encoding force: %s ==> %s\n", 
	     encoding_id2name(platform_id, encoding_id),
	     encoding_id2name(platform_id, font_otf->encoding_force));
    }
  }

  /* ad-hoc */
  cs = font_otf->charset_name; 
  if ((internal_enc != NULL) && (strcmp(internal_enc, "UNICODE") == 0))
    cs = "UNICODE";

  ccv_id = vf_ccv_require(font_otf->charset_name, font_otf->encoding_name,
			  cs, internal_enc);

  if (otf_debug('p') || otf_debug('c'))
    printf("VFlib OpenType: CCV ID = %d\n", ccv_id);

  return ccv_id;
}


Private struct s_id_name_tbl  PlatformNameTable[] = {
  {TT_PLAT_ID_ANY,       "any"},       {TT_PLAT_ID_ANY,       "?"},
  {TT_PLAT_ID_ANY,       "*"},         {TT_PLAT_ID_APPLE,     "apple"},
  {TT_PLAT_ID_MACINTOSH, "macintosh"}, {TT_PLAT_ID_MACINTOSH, "mac"},
  {TT_PLAT_ID_ISO,       "iso"},       {TT_PLAT_ID_ISO,       "ascii"},
  {TT_PLAT_ID_MICROSOFT, "windows"},   {TT_PLAT_ID_MICROSOFT, "microsoft"},
  {TT_PLAT_ID_MICROSOFT, "ms"},        {-1, NULL}}; 

Private struct s_id_name_tbl  EncodingNameTableISO[] = {
  {TT_ENC_ID_ANY,        "any"},
  {TT_ENC_ID_ANY,        "?"},
  {TT_ENC_ID_ANY,        "*"},          
  {TT_ENC_ID_ISO_ASCII,             "ascii"},   
  {TT_ENC_ID_ISO_10646,             "iso10464"},
  {TT_ENC_ID_ISO_8859_1,            "iso8859-1"},
  {-1, NULL}}; 
Private struct s_id_name_tbl  EncodingNameTableMS[] = {
  {TT_ENC_ID_ANY,        "any"},        
  {TT_ENC_ID_ANY,        "?"},
  {TT_ENC_ID_ANY,        "*"},
  {TT_ENC_ID_MS_SYMBOL,     "symbol"},
  {TT_ENC_ID_MS_UNICODE,    "unicode"},
  {TT_ENC_ID_MS_SHIFT_JIS,  "shift-jis"},
  {TT_ENC_ID_MS_SHIFT_JIS,  "sjis"},    
  {TT_ENC_ID_MS_SHIFT_JIS,  "ms-kanji"},
  {TT_ENC_ID_MS_BIG5,       "big5"},   
  {TT_ENC_ID_MS_RPC,        "rpc"},
  {TT_ENC_ID_MS_WANSUNG,    "wansung"},
  {TT_ENC_ID_MS_JOHAB,      "johab"},
  {-1, NULL}}; 
Private struct s_id_name_tbl  EncodingNameTableApple[] = {
  {TT_ENC_ID_ANY,        "any"},
  {TT_ENC_ID_ANY,        "?"},
  {TT_ENC_ID_ANY,        "*"},          
  {TT_ENC_ID_APPLE_DEFAULT,       "default"},   
  {TT_ENC_ID_APPLE_UNICODE_1_1,   "unicode1.1"},
  {TT_ENC_ID_APPLE_UNICODE_2_0,   "unicode2.0"},
  {TT_ENC_ID_APPLE_ISO_10646,     "iso10464"},
  {-1, NULL}}; 
Private struct s_id_name_tbl  EncodingNameTableMac[] = {
  {TT_ENC_ID_ANY,        "any"},
  {TT_ENC_ID_ANY,        "?"},
  {TT_ENC_ID_ANY,        "*"},          
  {TT_ENC_ID_MAC_ROMAN,               "roman"},   
  {TT_ENC_ID_MAC_JAPANESE,            "japanese"},
  {TT_ENC_ID_MAC_TRADITIONAL_CHINESE, "traditional-chinese"},
  {TT_ENC_ID_MAC_KOREAN,              "korean"},
  {TT_ENC_ID_MAC_ARABIC,              "arabic"},
  {TT_ENC_ID_MAC_HEBREW,              "hebrew"},
  {TT_ENC_ID_MAC_GREEK,               "greek"},
  {TT_ENC_ID_MAC_RUSSIAN,             "russian"},
  {-1, NULL}}; 
Private struct s_id_name_tbl  *EncodingNameTableTable[] = {
  EncodingNameTableApple,
  EncodingNameTableMac,
  EncodingNameTableISO,
  EncodingNameTableMS,
  NULL
};

Private int 
get_id_from_platform_name(char *name)
{
  return name2id(name, PlatformNameTable, TT_PLAT_ID_ANY, "platform name");
}

Private int 
get_id_from_encoding_name(char *name, int platform)
{
  struct s_id_name_tbl *tbl;

  tbl = EncodingNameTableTable[platform];
  return name2id(name, tbl, TT_ENC_ID_ANY, "encoding name");
}

Private int 
name2id(char *name, ID_NAME_TBL tbl, int value_default, char *desc)
{
  int         id, t;
  char        *p;

  for (p = name; *p != '\0'; p++){
    if (!isspace((int)*p))
      break;
  }
  if (*p == '\0'){
    id = value_default;
  } else if (isdigit((int)*p)){
    id = atoi(p);
  } else {
    id = value_default;
    for (t = 0; tbl[t].name != NULL; t++){
      if (vf_strncmp_ci(name, tbl[t].name, strlen(tbl[t].name)) == 0)
	return tbl[t].id;
    }
    fprintf(stderr, "VFlib warning: in vflibcap - No such %s: %s\n", 
	    desc, name);
  }
  return id;
}


struct s_otf_encoding_tbl {
  int  otf_enc;
  char *str_name;
};
Private struct s_otf_encoding_tbl  otf_encoding_tbl_apple[] = {
  {-10000,                   NULL}
};
Private struct s_otf_encoding_tbl  otf_encoding_tbl_mac[] = {
  {-10000,                   NULL}
};
Private struct s_otf_encoding_tbl  otf_encoding_tbl_iso[] = {
  {-10000,                   NULL}
};
Private struct s_otf_encoding_tbl  otf_encoding_tbl_ms[] = {
  {TT_ENC_ID_MS_SYMBOL,      "Symbol"}, 
  {TT_ENC_ID_MS_UNICODE,     "UNICODE"},
  {TT_ENC_ID_MS_SHIFT_JIS,   "SJIS"}, 
  {TT_ENC_ID_MS_BIG5,        "BIG5"},   
  {TT_ENC_ID_MS_RPC,         "RPC"}, 
  {TT_ENC_ID_MS_WANSUNG,     "WANSUNG"}, 
  {TT_ENC_ID_MS_JOHAB,       "JOHAB"}, 
  {TT_ENC_ID_ANY,            NULL},       
  {-10000,                   NULL}
};

Private struct s_otf_encoding_tbl  *otf_encoding_tbltbl[] = {
  otf_encoding_tbl_apple,
  otf_encoding_tbl_mac,
  otf_encoding_tbl_iso,
  otf_encoding_tbl_ms,
  NULL
};

Private char*
conv_encoding_otf_to_vflib(int otf_enc, int platform)
{
  int   i;
  struct s_otf_encoding_tbl  *tbl;

  tbl = otf_encoding_tbltbl[platform];
  for (i = 0; tbl[i].otf_enc >= -10; i++){
    if (tbl[i].otf_enc == otf_enc)
      return tbl[i].str_name;
  }
  return NULL;
}



Private char*  platform_id2name(int);
Private char *otf_platform_name[] = {
  "Apple", "Macintosh", "ISO", "Microsoft",  NULL };

Private char *otf_encoding_name_apple[] = {
  "Apple" "Unicode 1.1", "IS10646", "Unicode 2.0", NULL };
Private char *otf_encoding_name_mac[] = {
  "Roman",      "Japanese",  "Chinese",   "Koran",    "Arabic", 
  "Hebrew",     "Greek",     "Russian",   "RSymbol",  "Devanagari",
  "Gurmukhi",   "Gujarati",  "Oriya",     "Bengali",   "Tamil",
  "Telugu",     "Kannada",   "Malayalam", "Singalese", "Burmese",
  "Khmer",      "Thai",      "Laotian",   "Georgian",  "Armenian",
  "Maldivian",  "Tibetan",   "Mongolian", "Geez",      "Slavic", 
  "Vietnamese", "Sindhi",    "Uninterp",  NULL };
Private char *otf_encoding_name_iso[] = {
  "7-bit ASCII", "ISO 10646", "ISO 8859-1", NULL};
Private char *otf_encoding_name_ms[] = {
  "Symbol", "Unicode", "Shift JIS", "Big 5", 
  "RPC",    "WanSung", "Johab",     NULL };
Private char **otf_encoding_table[] = {
  otf_encoding_name_apple,
  otf_encoding_name_mac, 
  otf_encoding_name_iso, 
  otf_encoding_name_ms, 
};

Private char*
platform_id2name(int plat_id)
{
  int  j;
  char *s;

  for (j = 0; otf_platform_name[j] != NULL; j++){
    if (j == plat_id)
      break;
  }
  if ((s = otf_platform_name[j]) == NULL)
    return "?";
  return s;
}

Private char*
encoding_id2name(int plat_id, int enc_id)
{
  int  j;
  char *s;

  for (j = 0; otf_encoding_table[plat_id][j] != NULL; j++){
    if (j == enc_id)
      break;
  }
  if ((s = otf_encoding_table[plat_id][j]) == NULL)
    return "?";
  return s;
}



Private int  otf_debug2(char type, char *str);

Private int
otf_debug(char type)
{
  int    v;
  char  *p0;

  v = FALSE;
  if (env_debug_mode != NULL){
    if ((v = otf_debug2(type, env_debug_mode)) == TRUE)
      return TRUE;
  }

  if (default_debug_mode == NULL)
    return FALSE;
  if ((p0 = vf_sexp_get_cstring(default_debug_mode)) == NULL)
    return FALSE;
  return otf_debug2(type, p0);
}

Private int
otf_debug2(char type, char *p0)
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
