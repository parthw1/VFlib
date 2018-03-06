/*
 * drv_ttf.c - A font driver for TrueType fonts with FreeType library.  
 * by Hirotsugu Kakugawa
 *
 *  6 Jan 1998  First implementation by FreeType 1.0.
 *  5 Feb 1998  VFlib 3.4  Changed API.
 * 20 Mar 1998  Added outline data extract routine.
 *  2 Jun 1998  Added 'hinting' capablity
 * 18 Jun 1998  Adopted FreeType 1.1
 * 21 Sep 1998  Fixed a bug that dumps core when a font is closed.
 *  9 Dec 1998  Adopted FreeType 1.2
 *  9 Dec 1998  Added debug flag control by environment variable.
 *              Added get_fontbbx1() and get_fontbbx2().
 * 20 Dec 1998  Added a feature to open/close font files dynamically on demand.
 * 28 Dec 1998  Fixed a bug in code for dynamic open/close font files.
 *  4 Mar 1999  Added ad-hoc solution to handle buggy JIS X 0212 fonts
 *              with empty Row 47 (e.g., Ricoh TrueTypeWorld ValueFont DX).
 *              Add "jisx0212-row47-empty-sjis" capability with "yes" value
 *              in vflibcap to use such fonts.
 * 29 Jul 1999  Fixed a bug in makeing outline data (refrence point).
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


#include  "freetype.h"
#include  "ttf.h"

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
#define DEBUG_ENV_NAME   "VFLIB_TTF_DEBUG"


struct s_font_ttf {
  int       release_on_close;
  int       ttf_opened;
  TT_Face             tt_face;
  TT_Instance         tt_instance;
  TT_CharMap          tt_charmap;
  TT_Glyph            tt_glyph;
  TT_Face_Properties  tt_fprops;
  TT_UShort           tt_upem;
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
  int       jisx0212_r47e_sjis;
  char     *charset_name;
  char     *encoding_name;
  SEXP      props;
  int       ccv_id;
};
typedef struct s_font_ttf  *FONT_TTF;


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

Private TT_Face *ttf_open_method(char*,long,long,VF_FONT,FONT_TTF);
Private void     ttf_close_method(TT_Face*,long,long,VF_FONT,FONT_TTF);

Private void* ttf_get_xxx(int mode, 
			  VF_FONT font, long code_point, 
			  double mag_x, double mag_y, 
			  VF_METRIC1 metric1, VF_METRIC2 metric2,
			  FONTBBX1 bbx1, FONTBBX2 bbx2);

Private int         ttf_create(VF_FONT,char*,char*,int,SEXP);
Private int         ttf_close(VF_FONT);
Private int         ttf_get_metric1(VF_FONT,long,VF_METRIC1,double,double);
Private int         ttf_get_metric2(VF_FONT,long,VF_METRIC2,double,double);
Private int         ttf_get_fontbbx1(VF_FONT font,double,double,
				     double*,double*,double*,double*);
Private int         ttf_get_fontbbx2(VF_FONT font, double,double,
				     int*,int*,int*,int*);
Private VF_BITMAP   ttf_get_bitmap1(VF_FONT,long,double,double);
Private VF_BITMAP   ttf_get_bitmap2(VF_FONT,long,double,double);
Private VF_OUTLINE  ttf_get_outline1(VF_FONT,long,double,double);
Private char       *ttf_get_font_prop(VF_FONT,char*);

Private VF_OUTLINE
  get_outline_ttf_to_vflib(FONT_TTF,TT_Outline*,
			   TT_Glyph_Metrics*, TT_Instance_Metrics*,
			   double ps, double mag_x, double mag_y, 
			   double dpix, double dpiy, double asp);
Private long   fix_jisx0212_row47_empty_sjis(long code_point);
Private int    find_encoding_mapping(FONT_TTF);
Private int      get_id_from_platform_name(char*);
Private int      get_id_from_encoding_name(char*,int);
Private int         name2id(char*,ID_NAME_TBL,int,char*);
Private char*  conv_encoding_ttf_to_vflib(int ttf_enc, int plat);
Private char*  platform_id2name(int plat_id);
Private char*  encoding_id2name(int,int);
Private int    ttf_debug(char);




static TT_Engine   FreeType_Engine;
static int         Initialized_FreeType = 0;



Public int
VF_Init_Driver_TrueType(void)
{
  TT_Error  error;
  char      *p;
  struct s_capability_table  ct[20];
  int  z;

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
  /* VF_CAPE_TTF_PLATFORM_ID */
  ct[z].cap = VF_CAPE_TTF_PLATFORM_ID;   ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;       ct[z++].val = &default_platform_id;
  /* VF_CAPE_TTF_ENCODING_ID */
  ct[z].cap = VF_CAPE_TTF_ENCODING_ID;   ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;       ct[z++].val = &default_encoding_id;
  /* VF_CAPE_TTF_HINTING */
  ct[z].cap = VF_CAPE_TTF_HINTING;       ct[z].type = CAPABILITY_STRING;
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

  if (Initialized_FreeType == 0){
    error = TT_Init_FreeType(&FreeType_Engine);
    if (error){
      vf_error = VF_ERR_FREETYPE_INIT;
      return -1;
    }
    Initialized_FreeType = 1;
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

  VF_InstallFontDriver(FONTCLASS_NAME, (DRIVER_FUNC_TYPE)ttf_create);

  return 0;
}
  

Private int
ttf_create(VF_FONT font, char *font_class, char *font_name, 
	   int implicit, SEXP entry)
{
  FONT_TTF   font_ttf;
  char       *font_file, *font_path, *p;
  int        val;
  SEXP       cap_fontdirs, cap_font, cap_point, cap_pixel;
  SEXP       cap_dpi, cap_dpi_x, cap_dpi_y, cap_mag, cap_aspect;
  SEXP       cap_font_number, cap_direction, cap_platform_id, cap_encoding_id;
  SEXP       cap_hinting, cap_mapping_id, cap_encoding_force;
  SEXP       cap_jisx0212_r47e_sjis;
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
  /* VF_CAPE_TTF_FONT_NUMBER */
  ct[z].cap = VF_CAPE_TTF_FONT_NUMBER;    ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;        ct[z++].val = &cap_font_number;
  /* VF_CAPE_TTF_PLATFORM_ID */
  ct[z].cap = VF_CAPE_TTF_PLATFORM_ID;    ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;        ct[z++].val = &cap_platform_id;
  /* VF_CAPE_TTF_ENCODING_ID */
  ct[z].cap = VF_CAPE_TTF_ENCODING_ID;    ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;        ct[z++].val = &cap_encoding_id;
  /* VF_CAPE_TTF_MAPPING_ID */
  ct[z].cap = VF_CAPE_TTF_MAPPING_ID;     ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;        ct[z++].val = &cap_mapping_id;
  /* VF_CAPE_TTF_HINTING */
  ct[z].cap = VF_CAPE_TTF_HINTING;        ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;        ct[z++].val = &cap_hinting;
  /* VF_CAPE_TTF_ENCODING_FORCE */
  ct[z].cap = VF_CAPE_TTF_ENCODING_FORCE; ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;        ct[z++].val = &cap_encoding_force;
  /* VF_CAPE_CHARSET */
  ct[z].cap = VF_CAPE_CHARSET;            ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;        ct[z++].val = &cap_charset;
  /* VF_CAPE_ENCODING */
  ct[z].cap = VF_CAPE_ENCODING;           ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;        ct[z++].val = &cap_encoding;
  /* VF_CAPE_TTF_JISX0212_R47E_SJIS */
  ct[z].cap = VF_CAPE_TTF_JISX0212_R47ES; ct[z].type = CAPABILITY_STRING;
  ct[z].ess = CAPABILITY_OPTIONAL;     ct[z++].val = &cap_jisx0212_r47e_sjis;
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
			       TRUE, FSEARCH_FORMAT_TYPE_TTF, 
			       cap_fontdirs, NULL, NULL);
  }
  if (font_path == NULL){
    font_path = vf_search_file(font_file, -1, NULL, 
			       TRUE, FSEARCH_FORMAT_TYPE_TTF, 
			       default_fontdirs, NULL, NULL);
  }
  if (font_path == NULL){
    vf_error = VF_ERR_NO_FONT_FILE;
    return -1;
  }

  if (ttf_debug('f')) 
    printf("VFlib TrueType: font file %s\n   ==> %s\n", font_file, font_path);

  font->font_type       = VF_FONT_TYPE_OUTLINE;
  font->get_metric1     = ttf_get_metric1;
  font->get_metric2     = ttf_get_metric2;
  font->get_fontbbx1    = ttf_get_fontbbx1;
  font->get_fontbbx2    = ttf_get_fontbbx2;
  font->get_bitmap1     = ttf_get_bitmap1;
  font->get_bitmap2     = ttf_get_bitmap2;
  font->get_outline     = ttf_get_outline1;
  font->get_font_prop   = ttf_get_font_prop;
  font->query_font_type = NULL;  /* Use font->font_type value. */
  font->close           = ttf_close;

  ALLOC_IF_ERR(font_ttf, struct s_font_ttf){
    vf_error = VF_ERR_NO_MEMORY;
    return -1;
  }
  font->private = font_ttf;

  font_ttf->release_on_close = 0;
  font_ttf->ttf_opened       = 0;
  font_ttf->font_name      = NULL;
  font_ttf->file_path      = font_path;
  font_ttf->point_size     = v_default_point_size;
  font_ttf->pixel_size     = v_default_pixel_size;
  font_ttf->mag            = 1.0;
  font_ttf->dpi_x          = v_default_dpi_x;
  font_ttf->dpi_y          = v_default_dpi_y;
  font_ttf->aspect         = v_default_aspect;
  font_ttf->direction      = v_default_direction;
  font_ttf->font_number    = -1;
  font_ttf->platform_id    = v_default_platform_id;
  font_ttf->encoding_id    = v_default_encoding_id;
  font_ttf->mapping_id     = TT_MAP_ID_SEARCH;
  font_ttf->encoding_force = -1;
  font_ttf->hinting        = v_default_hinting;
  font_ttf->charset_name   = NULL;
  font_ttf->encoding_name  = NULL;

  if (implicit == 0){
    if (cap_point != NULL)
      font_ttf->point_size = atof(vf_sexp_get_cstring(cap_point));
    if (cap_pixel != NULL)
      font_ttf->pixel_size = atof(vf_sexp_get_cstring(cap_pixel));
    if (cap_dpi != NULL)
      font_ttf->dpi_x = font_ttf->dpi_y = atof(vf_sexp_get_cstring(cap_dpi));
    if (cap_dpi_x != NULL)
      font_ttf->dpi_x = atof(vf_sexp_get_cstring(cap_dpi_x));
    if (cap_dpi_y != NULL)
      font_ttf->dpi_y = atof(vf_sexp_get_cstring(cap_dpi_y));
    if (cap_mag != NULL)
      font_ttf->mag = atof(vf_sexp_get_cstring(cap_mag));
    if (cap_aspect != NULL)
      font_ttf->aspect = atof(vf_sexp_get_cstring(cap_aspect));
    if (cap_direction != NULL){
      p = vf_sexp_get_cstring(cap_direction);
      switch (*p){
      case 'h': case 'H':
	font_ttf->direction = DIRECTION_HORIZONTAL;  break;
      case 'v': case 'V':
	font_ttf->direction = DIRECTION_VERTICAL;    break;
      default:
	fprintf(stderr, "VFlib Warning: Unknown writing direction: %s\n", p);
	break;
      }
    }
    if (cap_font_number != NULL)
      font_ttf->font_number = atoi(vf_sexp_get_cstring(cap_font_number));
    if (cap_platform_id != NULL)
      font_ttf->platform_id 
	= get_id_from_platform_name(vf_sexp_get_cstring(cap_platform_id));
    if (cap_encoding_id != NULL)
      font_ttf->encoding_id
	= get_id_from_encoding_name(vf_sexp_get_cstring(cap_encoding_id),
				    font_ttf->platform_id);
    if (cap_mapping_id != NULL)
      font_ttf->mapping_id = atoi(vf_sexp_get_cstring(cap_mapping_id));
    if (cap_encoding_force != NULL)
      font_ttf->encoding_force
	= get_id_from_encoding_name(vf_sexp_get_cstring(cap_encoding_force),
				    font_ttf->platform_id);
    if (cap_charset != NULL)
      font_ttf->charset_name = vf_strdup(vf_sexp_get_cstring(cap_charset));
    if (cap_encoding != NULL)
      font_ttf->encoding_name = vf_strdup(vf_sexp_get_cstring(cap_encoding));
    if (cap_props != NULL)
      font_ttf->props = cap_props;
    if (cap_hinting != NULL){
      font_ttf->hinting = vf_parse_bool(vf_sexp_get_cstring(cap_hinting));
    }
    if (cap_jisx0212_r47e_sjis != NULL){
      font_ttf->jisx0212_r47e_sjis 
	= vf_parse_bool(vf_sexp_get_cstring(cap_jisx0212_r47e_sjis));
    }
  }

  if ((font_ttf->font_name = vf_strdup(font_name)) == NULL){
    vf_error = VF_ERR_NO_MEMORY;
    goto Error;
  }

  if (vf_fm_OpenFileStreamApp(font_ttf->file_path,
			      font_ttf->font_number, -1, font, font_ttf,
			      (FM_OPEN_METHOD)ttf_open_method, 
			      (FM_CLOSE_METHOD)ttf_close_method, 
			      "FreeType") == NULL)
    goto Error;

  font_ttf->ccv_id = find_encoding_mapping(font_ttf);

  val = 0;

Error:
  if (implicit == 0){   /* explicit font */
    vf_sexp_free4(&cap_fontdirs, &cap_font, &cap_point, &cap_pixel);
    vf_sexp_free3(&cap_dpi, &cap_dpi_x, &cap_dpi_y);
    vf_sexp_free3(&cap_mag, &cap_aspect, &cap_direction);
    vf_sexp_free3(&cap_platform_id, &cap_encoding_id, &cap_hinting);
    vf_sexp_free3(&cap_mapping_id, &cap_encoding_force, &cap_charset);
    vf_sexp_free1(&cap_encoding);
    vf_sexp_free1(&cap_jisx0212_r47e_sjis);
  }

  if ((val != 0) && (font_ttf != NULL)){
    vf_sexp_free1(&cap_props); 
    vf_free(font_ttf->font_name);
    vf_free(font_ttf->file_path);
    vf_free(font_ttf->charset_name);
    vf_free(font_ttf->encoding_name);
    vf_free(font_ttf); 
  }

  return val;
}


Private int
ttf_close(VF_FONT font)
{
  FONT_TTF  font_ttf;

  if ((font_ttf = (FONT_TTF)font->private) != NULL){
    vf_free(font_ttf->charset_name);
    vf_free(font_ttf->encoding_name);
    font_ttf->release_on_close = 1;
    /* `font_ttf' and `font_ttf->font_name' are
       released in ttf_close_method() */
  }
  return 0; 
}

Private TT_Face*
ttf_open_method(char *font_path, long fontnum, long iarg2,
		VF_FONT font, FONT_TTF font_ttf)
{
  TT_Error  error;

  if (font_ttf == NULL)
    return NULL;
  
  if (font_ttf->ttf_opened == 0){
    if (fontnum < 0){
      if (ttf_debug('f')) 
	printf("VFlib TrueType: TT_Open_Face %s\n", font_ttf->font_name);
      error = TT_Open_Face(FreeType_Engine, font_path,
			   &font_ttf->tt_face);
    } else {
      if (ttf_debug('f')) 
	printf("VFlib TrueType: TT_Open_Collection %s, %ld\n", 
	       font_ttf->font_name, fontnum);
      error = TT_Open_Collection(FreeType_Engine, font_path, 
				 fontnum, &font_ttf->tt_face);
    }
    if (error)
      return NULL;

    font_ttf->ttf_opened = 1;

    (void) TT_Get_Face_Properties(font_ttf->tt_face, &font_ttf->tt_fprops);
    font_ttf->tt_upem = font_ttf->tt_fprops.header->Units_Per_EM;
    
    if (ttf_debug('n')) 
      printf("VFlib TrueType: the number of embedded faces: %ld\n",
	     (long)font_ttf->tt_fprops.num_Faces);
    
    error = TT_New_Glyph(font_ttf->tt_face, &font_ttf->tt_glyph);
    if (error)
      return NULL;
    error = TT_New_Instance(font_ttf->tt_face, &font_ttf->tt_instance);
    if (error)
      return NULL;
  }

  return  &font_ttf->tt_face;
}

Private void
ttf_close_method(TT_Face *ttface, long iarg1, long iarg2, 
		 VF_FONT font, FONT_TTF font_ttf)
{
  if (font_ttf->release_on_close == 0){
    /* close temporality by the limitation of
       the number of simultaneously opened files */
    if (ttf_debug('f')) 
      printf("VFlib TrueType: TT_Flush_Face %s\n", font_ttf->font_name);
    TT_Flush_Face(font_ttf->tt_face);
  } else {
    /* after a font is closed */
    if (ttf_debug('f')) 
      printf("VFlib TrueType: TT_Close_Face %s\n", font_ttf->font_name);
    TT_Done_Glyph(font_ttf->tt_glyph);
    TT_Done_Instance(font_ttf->tt_instance);
    TT_Close_Face(font_ttf->tt_face);
    vf_free(font_ttf->font_name); 
    vf_free(font_ttf->file_path);
    vf_free(font_ttf);
  }
} 


Private int
ttf_get_metric1(VF_FONT font, long code_point, VF_METRIC1 metric, 
		double mag_x, double mag_y)
{
  if (ttf_get_xxx(MODE_METRIC1, font, code_point, mag_x, mag_y, 
		  metric, NULL, NULL, NULL) == NULL)
    return -1;
  return 0;
}

Private int
ttf_get_fontbbx1(VF_FONT font, double mag_x, double mag_y,
		 double *w_p, double *h_p, double *xoff_p, double *yoff_p)
{
  struct s_fontbbx1  bbx1;

  if (ttf_get_xxx(MODE_FONTBBX1, font, -1, mag_x, mag_y, 
		  NULL, NULL, &bbx1, NULL) == NULL)
    return -1;

  *w_p    = bbx1.w;
  *h_p    = bbx1.h; 
  *xoff_p = bbx1.xoff;
  *yoff_p = bbx1.yoff;
  return 0;
}

Private VF_BITMAP
ttf_get_bitmap1(VF_FONT font, long code_point,
		double mag_x, double mag_y)
{
  return (VF_BITMAP)ttf_get_xxx(MODE_BITMAP1, font, code_point, mag_x, mag_y, 
				NULL, NULL, NULL, NULL);
}


Private VF_OUTLINE
ttf_get_outline1(VF_FONT font, long code_point,
		 double mag_x, double mag_y)
{
  return (VF_OUTLINE)ttf_get_xxx(MODE_OUTLINE, font, code_point, mag_x, mag_y, 
				 NULL, NULL, NULL, NULL);
}


Private int
ttf_get_metric2(VF_FONT font, long code_point, VF_METRIC2 metric,
		double mag_x, double mag_y)
{
  if (ttf_get_xxx(MODE_METRIC2, font, code_point, mag_x, mag_y,
		  NULL, metric, NULL, NULL) == NULL)
    return -1;
  return 0;
}

Private int
ttf_get_fontbbx2(VF_FONT font, double mag_x, double mag_y,
		 int *w_p, int *h_p, int *xoff_p, int *yoff_p)
{
  struct s_fontbbx2  bbx2;

  if (ttf_get_xxx(MODE_FONTBBX2, font, -1, mag_x, mag_y, 
		  NULL, NULL, NULL, &bbx2) == NULL)
    return -1;

  *w_p    = bbx2.w;
  *h_p    = bbx2.h; 
  *xoff_p = bbx2.xoff;
  *yoff_p = bbx2.yoff;
  return 0;
}

Private VF_BITMAP
ttf_get_bitmap2(VF_FONT font, long code_point, 
		double mag_x, double mag_y)
{
  return (VF_BITMAP)ttf_get_xxx(MODE_BITMAP2,
				font, code_point, mag_x, mag_y, 
				NULL, NULL, NULL, NULL);
}


Private void*
ttf_get_xxx(int mode, 
	    VF_FONT font, long code_point, 
	    double mag_x, double mag_y,
	    VF_METRIC1 metric1, VF_METRIC2 metric2,
	    FONTBBX1 fontbbx1, FONTBBX2 fontbbx2)
{
  FONT_TTF   font_ttf;
  VF_BITMAP  bm;
  VF_OUTLINE ol;
  void      *val;
  double     ps = 0.0, mx, my, asp, aspd; 
  double dpix = 0.0, dpiy = 0.0; 
  long       cp;
  int        chindex;
  int        load_flag;
  TT_Raster_Map       tt_bitmap;
  TT_Glyph_Metrics    tt_metrics;
  TT_Instance_Metrics tt_imetrics;
  TT_Outline        tt_outline;
  TT_Short          xoff, yoff;
  TT_Error          error;

  error = 0;

  if ((font_ttf = (FONT_TTF)font->private) == NULL){
    fprintf(stderr, "VFlib: Internal error in ttf_get_xxx()\n");
    abort();
  }

  mx = mag_x * font_ttf->mag * font->mag_x;
  my = mag_y * font_ttf->mag * font->mag_y;
  asp = v_default_aspect * font_ttf->aspect * (mx / my);

  if (   (mode == MODE_METRIC1) 
      || (mode == MODE_FONTBBX1) 
      || (mode == MODE_BITMAP1) 
      || (mode == MODE_OUTLINE) ){
    if (((dpix = font->dpi_x) <= 0) || ((dpiy = font->dpi_y) <= 0)){
      dpix = font_ttf->dpi_x;
      dpiy = font_ttf->dpi_y;
    }
    if ((ps = font->point_size) < 0)
      ps = font_ttf->point_size;
    ps = ps * my;
  } else if (   (mode == MODE_METRIC2) 
	     || (mode == MODE_FONTBBX2) 
	     || (mode == MODE_BITMAP2) ){
    if ((ps = font->pixel_size) < 0)
      ps = font_ttf->pixel_size;
    ps = ps * my;
    dpix = POINTS_PER_INCH;
    dpiy = POINTS_PER_INCH;
  }

  vf_fm_OpenFileStreamApp(font_ttf->file_path,
			  font_ttf->font_number, -1, font, font_ttf,
			  (FM_OPEN_METHOD)ttf_open_method, 
			  (FM_CLOSE_METHOD)ttf_close_method, "FreeType");

  if (mode != MODE_OUTLINE){
    TT_Set_Instance_Resolutions(font_ttf->tt_instance, dpix, dpiy);
    error = TT_Set_Instance_PointSize(font_ttf->tt_instance, ps);
  } else {
    TT_Set_Instance_Resolutions(font_ttf->tt_instance, 
				96*4, 96*4); /*XXX THESE ARE AD-HOC VALUES!!*/
    error = TT_Set_Instance_PointSize(font_ttf->tt_instance, 12);
  } 
  if (error){
    vf_error = VF_ERR_NO_GLYPH;
    return NULL;
  }


  if ((mode == MODE_FONTBBX1) || (mode == MODE_FONTBBX2)){
    chindex = -1;
  } else {
    cp = code_point;
    if (font_ttf->ccv_id >= 0)
      cp = vf_ccv_conv(font_ttf->ccv_id, code_point);
    
    if (ttf_debug('c')) 
      printf("VFlib TrueType: CCV  0x%lx => 0x%lx\n", code_point, cp);

    if (cp < 0){
      vf_error = VF_ERR_ILL_CODE_POINT;
      return NULL;
    }

    if (font_ttf->jisx0212_r47e_sjis == TRUE){
      cp = fix_jisx0212_row47_empty_sjis(cp);
    }

    chindex = TT_Char_Index(font_ttf->tt_charmap, (int)cp);
    if (chindex <= 0){
      vf_error = VF_ERR_ILL_CODE_POINT;
      return NULL;
    }
    if (ttf_debug('i')) 
      printf("VFlib TrueType: charindex 0x%x  (code point 0x%lx)\n", 
	     chindex, code_point);

    if (font_ttf->hinting == TRUE)
      load_flag = TTLOAD_SCALE_GLYPH | TTLOAD_HINT_GLYPH;
    else 
      load_flag = TTLOAD_SCALE_GLYPH;
    error = TT_Load_Glyph(font_ttf->tt_instance, font_ttf->tt_glyph, 
			  chindex, load_flag);
    if (error){
      vf_error = VF_ERR_NO_GLYPH;
      return NULL;
    }
    (void) TT_Get_Glyph_Metrics(font_ttf->tt_glyph, &tt_metrics);
  }

  (void) TT_Get_Instance_Metrics(font_ttf->tt_instance, &tt_imetrics);

  if (ttf_debug('m')){
    printf("VFlib TrueType: Metrics\n");
    printf("  in Header   upem: %d, xMin: %d, yMin:%d, xMax:%d, yMax:%d\n",
	   font_ttf->tt_fprops.header->Units_Per_EM,
	   font_ttf->tt_fprops.header->xMin, 
	   font_ttf->tt_fprops.header->yMin, 
	   font_ttf->tt_fprops.header->xMax, 
	   font_ttf->tt_fprops.header->yMax);
    printf("  tt_metrics.bbx  xMin:%ld, yMin:%ld, xMax:%ld, yMax:%ld\n",
	   tt_metrics.bbox.xMin, tt_metrics.bbox.yMin, 
	   tt_metrics.bbox.xMax, tt_metrics.bbox.yMax);
    printf("  tt_imetrics  x_ppem:%d, y_ppem:%d, pointSize:%.4f, upem:%d\n",
	   tt_imetrics.x_ppem, tt_imetrics.y_ppem, 
	   (double)tt_imetrics.pointSize/64.0, font_ttf->tt_upem);
  }

  val = NULL;
  if ((mode == MODE_BITMAP1) || (mode == MODE_BITMAP2)){
    tt_bitmap.width = (tt_metrics.bbox.xMax-tt_metrics.bbox.xMin)*asp/64 + 1;
    tt_bitmap.rows  = (tt_metrics.bbox.yMax-tt_metrics.bbox.yMin)/64 + 1;
    tt_bitmap.cols  = (tt_bitmap.width + 7) >> 3;
    tt_bitmap.size  = tt_bitmap.cols * tt_bitmap.rows;
    tt_bitmap.bitmap = (void *) malloc(tt_bitmap.size);
    tt_bitmap.flow  = TT_Flow_Down;
    (void)memclr((char*) tt_bitmap.bitmap, tt_bitmap.size);
    xoff = -tt_metrics.bbox.xMin;
    yoff = -tt_metrics.bbox.yMin;
    aspd = 1.0 - asp;
    if (aspd < 0)
      aspd = 0.0 - aspd;
    if (aspd < 1.0e-6){
      error = TT_Get_Glyph_Bitmap(font_ttf->tt_glyph, &tt_bitmap, xoff, yoff);
      if (error)
	return NULL;
    } else {
      TT_Matrix  mat;
      mat.xx = asp * (1<<16);  mat.xy = 0;
      mat.yx = 0;              mat.yy = 1 * (1<<16);
      TT_Get_Glyph_Outline(font_ttf->tt_glyph, &tt_outline);
      TT_Transform_Outline(&tt_outline, &mat);
      TT_Translate_Outline(&tt_outline, 
			   -asp*tt_metrics.bbox.xMin, -tt_metrics.bbox.yMin);
      error = TT_Get_Outline_Bitmap(FreeType_Engine, &tt_outline, &tt_bitmap);
      TT_Done_Outline(&tt_outline);
    }
    ALLOC_IF_ERR(bm, struct vf_s_bitmap){
      vf_error = VF_ERR_NO_MEMORY;
      return NULL;
    }
    bm->bbx_width  = tt_bitmap.width;
    bm->bbx_height = tt_bitmap.rows;
    bm->raster     = tt_bitmap.cols;
    bm->bitmap     = (unsigned char*)tt_bitmap.bitmap;
    bm->off_x      = tt_metrics.bearingX * asp / 64;
    bm->off_y      = tt_metrics.bbox.yMax / 64;
    bm->mv_x       = tt_metrics.advance * asp / 64;
    bm->mv_y       = 0;
    val = (void*) bm;
    
  } else if (mode == MODE_METRIC1){
    double  xppem = 64.0 * (double)tt_imetrics.x_ppem;
    double  yppem = 64.0 * (double)tt_imetrics.y_ppem;
    double  pt  = (double)tt_imetrics.pointSize / 64.0;
    double  ptx = (double)tt_imetrics.pointSize * asp / 64.0;
    if (metric1 != NULL){
      metric1->bbx_width 
	= (double)((tt_metrics.bbox.xMax - tt_metrics.bbox.xMin) / xppem)*ptx;
      metric1->bbx_height 
	= (double)((tt_metrics.bbox.yMax - tt_metrics.bbox.yMin) / yppem)*pt;
      metric1->off_x = (double)(tt_metrics.bearingX  / xppem) * ptx;
      metric1->off_y = (double)(tt_metrics.bbox.yMax / yppem) * pt;
      metric1->mv_x = (double)(tt_metrics.advance / xppem) * ptx; /*XXX*/
      metric1->mv_y = 0.0 * pt;                                   /*XXX*/
    }
    val = (void*) metric1;

  } else if (mode == MODE_METRIC2){
    if (metric2 != NULL){
      metric2->bbx_width 
	= toint((double)(tt_metrics.bbox.xMax-tt_metrics.bbox.xMin)*asp /64.0);
      metric2->bbx_height 
	= toint((double)(tt_metrics.bbox.yMax-tt_metrics.bbox.yMin) /64.0);
      metric2->off_x = toint((double)tt_metrics.bearingX * asp / 64.0);
      metric2->off_y = toint((double)tt_metrics.bbox.yMax / 64.0);
      metric2->mv_x  = toint((double)tt_metrics.advance * asp / 64.0);
      metric2->mv_y  = 0.0;
    }
    val = (void*) metric2;

  } else if (mode == MODE_OUTLINE){
    error = TT_Get_Glyph_Outline(font_ttf->tt_glyph, &tt_outline);
    if (error)
      return NULL;
    ol = get_outline_ttf_to_vflib(font_ttf, 
				  &tt_outline, &tt_metrics, &tt_imetrics,
				  ps, mag_x, mag_y, dpix, dpiy, asp);
    TT_Done_Outline(&tt_outline);
    val = (void*) ol;

  } else if (mode == MODE_FONTBBX1){
    long  xmax = font_ttf->tt_fprops.header->xMax;
    long  xmin = font_ttf->tt_fprops.header->xMin;
    long  ymax = font_ttf->tt_fprops.header->yMax;
    long  ymin = font_ttf->tt_fprops.header->yMin;
    if (fontbbx1 != NULL){
      fontbbx1->w = (double)(xmax - xmin) * ps * asp / font_ttf->tt_upem;
      fontbbx1->h = (double)(ymax - ymin) * ps       / font_ttf->tt_upem;
      fontbbx1->xoff = (double) xmin * ps  * asp / font_ttf->tt_upem;
      fontbbx1->yoff = (double) ymin * ps        / font_ttf->tt_upem;
    }
    val = (void*) fontbbx1;

  } else if (mode == MODE_FONTBBX2){
    long  xmax = font_ttf->tt_fprops.header->xMax;
    long  xmin = font_ttf->tt_fprops.header->xMin;
    long  ymax = font_ttf->tt_fprops.header->yMax;
    long  ymin = font_ttf->tt_fprops.header->yMin;
    if (fontbbx2 != NULL){
      fontbbx2->w = (double)(xmax - xmin) * ps * asp / font_ttf->tt_upem;
      fontbbx2->h = (double)(ymax - ymin) * ps       / font_ttf->tt_upem;
      fontbbx2->xoff = (double) xmin * ps * asp / font_ttf->tt_upem;
      fontbbx2->yoff = (double) ymin * ps       / font_ttf->tt_upem;
    }
    val = (void*) fontbbx2;

  } else {
    fprintf(stderr, "VFlib: Internal error in ttf_get_xxx1()\n");
    fprintf(stderr, "Unknown mode: %d\n", mode);
    abort();
  }

  return val;
}

Private long
fix_jisx0212_row47_empty_sjis(long code_point)
{
  unsigned int   c1, c2;
  int            row_offset, cell_offset, adjust;

  if (code_point < 256)
    return code_point;

  c1 = code_point / 0x100;
  c2 = code_point % 0x100;

  /* SJIS->JIS */
  if (c2 < 159)
    adjust = 1;
  else
    adjust = 0;
  row_offset  = (c1 < 160) ? 112 : 176;
  cell_offset = (adjust == 1) ? ((c2 > 127) ? 32 : 31) : 126;
  c1 = ((c1 - row_offset) << 1) - adjust;
  c2 -= cell_offset;

  /* Fix */
  if (c1 >= (47 + 0x20))
    c1++;

  /* JIS -> SJIS */
  row_offset  = (c1 < 95) ? 112 : 176;
  cell_offset = (c1 % 2) ? ((c2 > 95) ? 32 : 31) : 126;
  c1 = ((c1 + 1) >> 1) + row_offset;
  c2 += cell_offset;

  return c1*0x100 + c2;
}


/*
 * The following routine is snarfed from VF_Ftype.c
 * in VFlib 2.23 by Mr Matsuda.    
 *                                           --- H.Kakugawa
 */
Private VF_OUTLINE
get_outline_ttf_to_vflib(FONT_TTF font_ttf, 
			 TT_Outline *tt_outline, 
			 TT_Glyph_Metrics *tt_metrics,
			 TT_Instance_Metrics *tt_imetrics,
			 double ps, double mag_x, double mag_y, 
			 double dpix, double dpiy, double asp)
{
  int              vfsize, ct, pt, sp, ep, p1, p2, p3;
  int              maxw, maxh, bbx;
  double           x1, y1, x2, y2, x3, y3, f, fupem,  mmm;
  VF_OUTLINE_ELEM  token, *vfdata, *vfp;

  mmm = 4*4;   /*ad-hoc*/

  maxw = (tt_metrics->bbox.xMax - tt_metrics->bbox.xMin + 1);
  maxh = (tt_metrics->bbox.yMax - tt_metrics->bbox.yMin + 1); 
  if ((bbx = maxw) < maxh)
    bbx = maxh;

#if 0
  printf("*** %d %d %d   %d   %d %d %d %d\n",
	 maxw, maxh, bbx, font_ttf->tt_upem,
	 font_ttf->tt_fprops.header->xMin, font_ttf->tt_fprops.header->yMin, 
	 font_ttf->tt_fprops.header->xMax, font_ttf->tt_fprops.header->yMax);
#endif
#if 0
  printf("*** %.3f  %.3f %.3f  %ld %ld %ld %ld\n",
	 tt_metrics->advance/64.0, 
	 tt_metrics->bearingX/64.0, tt_metrics->bearingY/64.0,
	 tt_metrics->bbox.xMax/64, tt_metrics->bbox.xMin/64,
	 tt_metrics->bbox.yMax/64, tt_metrics->bbox.yMin/64);
  printf("  %.3f %.3f\n",
	 (double)tt_imetrics->x_ppem / font_ttf->tt_upem,
	 (double)tt_imetrics->y_ppem / font_ttf->tt_upem);
#endif

  vfsize = VF_OL_OUTLINE_HEADER_SIZE_TYPE0;
  for (ct = pt = 0; ct < tt_outline->n_contours; ct++){ 
    token = 0L;
    sp = pt;
    ep = tt_outline->contours[ct];  
    for (; pt <= ep; pt++) {
      p1 = pt;
      p2 = (p1 < ep)? (p1 + 1) : sp;
#if 0
      printf(" (%ld %ld)\n", 
	     tt_outline->points.x[p1], tt_outline->points.y[p1]);
#endif
      /*XXX 1.0    [] => 1.1        XXX*/
      /*XXX conEnds[] => contors[]  XXX*/
      /*XXX contours  => n_contours XXX*/
      /*XXX xCoord    => points.x   XXX*/
      /*XXX yCoord    => points.y   XXX*/
      /*XXX flag[]    => flags      XXX*/
      if (tt_outline->flags[p1] & tt_outline->flags[p2] & TTF_OL_ONCURVE){
	/* Line */
        if (token != VF_OL_INSTR_LINE){
          token = VF_OL_INSTR_LINE;
          vfsize++;
        }
        vfsize += 1;
      } else {
        if (tt_outline->flags[p2] & TTF_OL_ONCURVE) 
	  continue;
        /* spline */
        if (token != VF_OL_INSTR_BEZ){
          token = VF_OL_INSTR_BEZ;
          vfsize++;
        }
        vfsize += 3;
      }
    }
  }
  vfsize++;

  if ((vfdata = (VF_OUTLINE)malloc(vfsize*sizeof(long))) == (VF_OUTLINE)NULL)
    return NULL;

  f     = (double)VF_OL_COORD_RANGE / (double)(bbx * mmm);
  fupem = (double)VF_OL_COORD_RANGE / (double)bbx;
  vfdata[VF_OL_HEADER_INDEX_HEADER_TYPE] = VF_OL_OUTLINE_HEADER_TYPE0; 
  vfdata[VF_OL_HEADER_INDEX_DATA_SIZE]   = vfsize; 
  vfdata[VF_OL_HEADER_INDEX_DPI_X]       = VF_OL_HEADER_ENCODE(dpix); 
  vfdata[VF_OL_HEADER_INDEX_DPI_Y]       = VF_OL_HEADER_ENCODE(dpiy); 
  vfdata[VF_OL_HEADER_INDEX_POINT_SIZE]  = VF_OL_HEADER_ENCODE(ps); 
  vfdata[VF_OL_HEADER_INDEX_EM]          = ceil(fupem * font_ttf->tt_upem);
  vfdata[VF_OL_HEADER_INDEX_MAX_X] 
    = toint(f * (tt_metrics->bbox.xMax - tt_metrics->bbox.xMin)) + 1;
  vfdata[VF_OL_HEADER_INDEX_MAX_Y] 
    = toint(f * (tt_metrics->bbox.yMax - tt_metrics->bbox.yMin)) + 1;
  /* ... horizintal direction only. B-( */
  vfdata[VF_OL_HEADER_INDEX_REF_X] = toint(f * (tt_metrics->bearingX));
  vfdata[VF_OL_HEADER_INDEX_REF_Y] = toint(f * tt_metrics->bbox.yMax);
  vfdata[VF_OL_HEADER_INDEX_MV_X]  = toint(f * (tt_metrics->advance));
  vfdata[VF_OL_HEADER_INDEX_MV_Y]  = toint(f * 0);

#define ConvX(x) (long)(VF_OL_COORD_OFFSET + f * ((x)-tt_metrics->bbox.xMin))
#define ConvY(y) (long)(VF_OL_COORD_OFFSET + f * (tt_metrics->bbox.yMax-(y)))

  vfp = &vfdata[VF_OL_OUTLINE_HEADER_SIZE_TYPE0];
  for (ct = pt = 0; ct < tt_outline->n_contours; ct++){
    token = (VF_OL_INSTR_TOKEN | VF_OL_INSTR_CWCURV);
    sp = pt;
    ep = tt_outline->contours[ct];
    for (; pt <= ep; pt++){
      p1 = pt;
      p2 = (p1 < ep) ? (p1 + 1) : sp;
      p3 = (p2 < ep) ? (p2 + 1) : sp;
      if (tt_outline->flags[p1] & tt_outline->flags[p2] & TTF_OL_ONCURVE){
	/* Line */
        if (token != (VF_OL_INSTR_TOKEN | VF_OL_INSTR_LINE)) {
          if (token == (VF_OL_INSTR_TOKEN | VF_OL_INSTR_CWCURV)){
            *(vfp++) = token | VF_OL_INSTR_LINE;
          } else {
            *(vfp++) = VF_OL_INSTR_TOKEN | VF_OL_INSTR_LINE;
          }
          token = VF_OL_INSTR_TOKEN | VF_OL_INSTR_LINE;
        }
        x1 = tt_outline->points[p1].x;
        y1 = tt_outline->points[p1].y;
        *(vfp++) = VF_OL_MAKE_XY(ConvX(x1), ConvY(y1));
      } else {
        if (tt_outline->flags[p2] & TTF_OL_ONCURVE)
	  continue;
        /* spline */
        if (token != (VF_OL_INSTR_TOKEN | VF_OL_INSTR_BEZ)){
          if (token == (VF_OL_INSTR_TOKEN | VF_OL_INSTR_CWCURV)){
            *(vfp++) = token | VF_OL_INSTR_BEZ;
          } else {
            *(vfp++) = VF_OL_INSTR_TOKEN | VF_OL_INSTR_BEZ;
          }
          token = VF_OL_INSTR_TOKEN | VF_OL_INSTR_BEZ;
        }
        if (tt_outline->flags[p1] & TTF_OL_ONCURVE){
          x1 = tt_outline->points[p1].x;
          y1 = tt_outline->points[p1].y;
        } else {
          x1 = (tt_outline->points[p1].x + tt_outline->points[p2].x) / 2.0;
          y1 = (tt_outline->points[p1].y + tt_outline->points[p2].y) / 2.0;
        }
        x2 = tt_outline->points[p2].x;
        y2 = tt_outline->points[p2].y;
        if (tt_outline->flags[p3] & TTF_OL_ONCURVE){
          x3 = tt_outline->points[p3].x;
          y3 = tt_outline->points[p3].y;
        } else {
          x3 = (tt_outline->points[p2].x + tt_outline->points[p3].x) / 2.0;
          y3 = (tt_outline->points[p2].y + tt_outline->points[p3].y) / 2.0;
        }
        *(vfp++) = VF_OL_MAKE_XY(ConvX(x1), ConvY(y1));
        *(vfp++) = VF_OL_MAKE_XY(ConvX((x1 + 2.0 * x2) / 3.0),
				 ConvY((y1 + 2.0 * y2) / 3.0));
        *(vfp++) = VF_OL_MAKE_XY(ConvX((2.0 * x2 + x3) / 3.0),
				 ConvY((2.0 * y2 + y3) / 3.0));
      }
    }
  }
  *(vfp++) = 0L;

  if (vfdata[VF_OL_OUTLINE_HEADER_SIZE_TYPE0] != 0L)
    vfdata[VF_OL_OUTLINE_HEADER_SIZE_TYPE0] |= VF_OL_INSTR_CHAR;

  return (VF_OUTLINE)vfdata;
}



Private char*
ttf_get_font_prop(VF_FONT font, char *prop_name)
{ /* CALLER MUST RELEASE RETURNED STRING LATER */
  SEXP       v;
  FONT_TTF   font_ttf;
  char       str[512];
  double     dpix, dpiy, p;

  if ((font_ttf = (FONT_TTF)font->private) == NULL){
    fprintf(stderr, "VFlib: internal error in ttf_get_font_prop()\n");
    abort();
  }

  if ((v = vf_sexp_assoc(prop_name, font_ttf->props)) != NULL){
    return vf_strdup(vf_sexp_get_cstring(vf_sexp_cadr(v)));
  } else if ((v = vf_sexp_assoc(prop_name, default_properties)) != NULL){
    return vf_strdup(vf_sexp_get_cstring(vf_sexp_cadr(v)));
  }

  if (font->mode == 1){
    if ((dpix = font->dpi_x) < 0)
      if ((dpix = font_ttf->dpi_x) < 0)
	dpix = v_default_dpi_x; 
    if ((dpiy = font->dpi_y) < 0)
      if ((dpiy = font_ttf->dpi_y) < 0)
	dpiy = v_default_dpi_y; 
    if ((p = font->point_size) < 0)
      if ((p = font_ttf->point_size) < 0)
	p = v_default_point_size;
    p = p * font->mag_y * font_ttf->mag;
    if (strcmp(prop_name, "POINT_SIZE") == 0){  
      sprintf(str, "%d", toint(p * 10.0)); 
      return vf_strdup(str);
    } else if (strcmp(prop_name, "PIXEL_SIZE") == 0){
      sprintf(str, "%d", toint(p * dpiy / POINTS_PER_INCH));
      return vf_strdup(str);
    } else if (strcmp(prop_name, "RESOLUTION_X") == 0){
      sprintf(str, "%d", toint(dpix)); 
      return vf_strdup(str);
    } else if (strcmp(prop_name, "RESOLUTION_Y") == 0){
      sprintf(str, "%d", toint(dpiy)); 
      return vf_strdup(str);
    }

  } else if (font->mode == 2){
    if ((p = font->pixel_size) < 0)
      if ((p = font_ttf->pixel_size) < 0)
	p = v_default_pixel_size;
    p = p * font->mag_y * font_ttf->mag;
    if (strcmp(prop_name, "POINT_SIZE") == 0){  
      sprintf(str, "%d", toint(p * 10.0 * POINTS_PER_INCH / VF_DEFAULT_DPI)); 
    } else if (strcmp(prop_name, "PIXEL_SIZE") == 0){
      sprintf(str, "%d", toint(p));
      return vf_strdup(str);
    } else if (strcmp(prop_name, "RESOLUTION_X") == 0){
      sprintf(str, "%d", toint(VF_DEFAULT_DPI)); 
      return vf_strdup(str);
    } else if (strcmp(prop_name, "RESOLUTION_Y") == 0){
      sprintf(str, "%d", toint(VF_DEFAULT_DPI)); 
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
find_encoding_mapping(FONT_TTF font_ttf)
{
  int               ccv_id, map, ntables;
  char              *internal_enc, *cs;
  unsigned short    platform_id, encoding_id;
  TT_Error          error;

  if (ttf_debug('p')){
    printf("VFlib TrueType: Searching platform:%d (%s), encoding:%d (%s))\n", 
	   font_ttf->platform_id,
	   platform_id2name(font_ttf->platform_id), 
	   font_ttf->encoding_id,
	   encoding_id2name(font_ttf->platform_id,font_ttf->encoding_id));
  }

  /* Find mapping table number to be used in TrueType font file. */
  map = font_ttf->mapping_id;
  ntables = TT_Get_CharMap_Count(font_ttf->tt_face);
  if (map != TT_MAP_ID_SEARCH){
    if (ntables == 0){
      fprintf(stderr, "VFlib TrueType warning: No mapping tables: %s\n",
	      font_ttf->font_name);
      return -1;
    } else if (ntables < 0){
      fprintf(stderr, "VFlib TrueType Internal error: CharMap_Count()%s\n",
	      font_ttf->font_name);
      return -1;
    } else if ((map < 0) || (ntables <= map)){
      map = 0;
      fprintf(stderr, "VFlib TrueType warning: Illegal mapping table ID.");
      fprintf(stderr, "Use mapping table #0.\n");
    }
    error = TT_Get_CharMap_ID(font_ttf->tt_face, map, 
			      &platform_id, &encoding_id);
    if (error)
      return -1;
    if (ttf_debug('p')) 
      printf("VFlib TrueType:  Platform,Encoding=(%s,%s)\n", 
	     platform_id2name(platform_id), 
	     encoding_id2name(platform_id, encoding_id));
  } else {
    for (map = 0; map < ntables; map++){
      error = TT_Get_CharMap_ID(font_ttf->tt_face, map, 
				&platform_id, &encoding_id);
      if (ttf_debug('p'))
	printf("VFlib TrueType:   mapping #%d: Platform: %s, Encoding:%s\n", 
	       map, 
	       platform_id2name(platform_id), 
	       encoding_id2name(platform_id, encoding_id));
      if (   ((font_ttf->platform_id == TT_PLAT_ID_ANY)
	      || (platform_id == font_ttf->platform_id))
	  && ((font_ttf->encoding_id == TT_ENC_ID_ANY) 
	      || (encoding_id == font_ttf->encoding_id)) ){
	break;
      }
      font_ttf->mapping_id = 0;
    }
    if (map == ntables){
      fprintf(stderr, "VFlib TrueType: Mapping table not found.\n");
      return -1;
    }
  }

  font_ttf->mapping_id = map;

  if (ttf_debug('p'))
    printf("VFlib TrueType: Use mapping table #%d (encoding#%d)\n", 
	   font_ttf->mapping_id, encoding_id);
  
  error = TT_Get_CharMap(font_ttf->tt_face, font_ttf->mapping_id, 
			 &font_ttf->tt_charmap);
  if (error)
    return -1;

  if (font_ttf->encoding_force < 0){
    internal_enc = conv_encoding_ttf_to_vflib(encoding_id, platform_id);
  } else {
    internal_enc = conv_encoding_ttf_to_vflib(font_ttf->encoding_force,
					      platform_id);
    if (ttf_debug('p')){
      printf("VFlib TrueType: Encoding force: %s ==> %s\n", 
	     encoding_id2name(platform_id, encoding_id),
	     encoding_id2name(platform_id, font_ttf->encoding_force));
    }
  }

  /* ad-hoc */
  cs = font_ttf->charset_name; 
  if ((internal_enc != NULL) && (strcmp(internal_enc, "UNICODE") == 0))
    cs = "UNICODE";

  ccv_id = vf_ccv_require(font_ttf->charset_name, font_ttf->encoding_name,
			  cs, internal_enc);

  if (ttf_debug('p') || ttf_debug('c'))
    printf("VFlib TrueType: CCV ID = %d\n", ccv_id);

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


struct s_ttf_encoding_tbl {
  int  ttf_enc;
  char *str_name;
};
Private struct s_ttf_encoding_tbl  ttf_encoding_tbl_apple[] = {
  {-10000,                   NULL}
};
Private struct s_ttf_encoding_tbl  ttf_encoding_tbl_mac[] = {
  {-10000,                   NULL}
};
Private struct s_ttf_encoding_tbl  ttf_encoding_tbl_iso[] = {
  {-10000,                   NULL}
};
Private struct s_ttf_encoding_tbl  ttf_encoding_tbl_ms[] = {
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

Private struct s_ttf_encoding_tbl  *ttf_encoding_tbltbl[] = {
  ttf_encoding_tbl_apple,
  ttf_encoding_tbl_mac,
  ttf_encoding_tbl_iso,
  ttf_encoding_tbl_ms,
  NULL
};

Private char*
conv_encoding_ttf_to_vflib(int ttf_enc, int platform)
{
  int   i;
  struct s_ttf_encoding_tbl  *tbl;

  tbl = ttf_encoding_tbltbl[platform];
  for (i = 0; tbl[i].ttf_enc >= -10; i++){
    if (tbl[i].ttf_enc == ttf_enc)
      return tbl[i].str_name;
  }
  return NULL;
}



Private char*  platform_id2name(int);
Private char *ttf_platform_name[] = {
  "Apple", "Macintosh", "ISO", "Microsoft",  NULL };

Private char *ttf_encoding_name_apple[] = {
  "Apple" "Unicode 1.1", "IS10646", "Unicode 2.0", NULL };
Private char *ttf_encoding_name_mac[] = {
  "Roman",      "Japanese",  "Chinese",   "Koran",    "Arabic", 
  "Hebrew",     "Greek",     "Russian",   "RSymbol",  "Devanagari",
  "Gurmukhi",   "Gujarati",  "Oriya",     "Bengali",   "Tamil",
  "Telugu",     "Kannada",   "Malayalam", "Singalese", "Burmese",
  "Khmer",      "Thai",      "Laotian",   "Georgian",  "Armenian",
  "Maldivian",  "Tibetan",   "Mongolian", "Geez",      "Slavic", 
  "Vietnamese", "Sindhi",    "Uninterp",  NULL };
Private char *ttf_encoding_name_iso[] = {
  "7-bit ASCII", "ISO 10646", "ISO 8859-1", NULL};
Private char *ttf_encoding_name_ms[] = {
  "Symbol", "Unicode", "Shift JIS", "Big 5", 
  "RPC",    "WanSung", "Johab",     NULL };
Private char **ttf_encoding_table[] = {
  ttf_encoding_name_apple,
  ttf_encoding_name_mac, 
  ttf_encoding_name_iso, 
  ttf_encoding_name_ms, 
};

Private char*
platform_id2name(int plat_id)
{
  int  j;
  char *s;

  for (j = 0; ttf_platform_name[j] != NULL; j++){
    if (j == plat_id)
      break;
  }
  if ((s = ttf_platform_name[j]) == NULL)
    return "?";
  return s;
}

Private char*
encoding_id2name(int plat_id, int enc_id)
{
  int  j;
  char *s;

  for (j = 0; ttf_encoding_table[plat_id][j] != NULL; j++){
    if (j == enc_id)
      break;
  }
  if ((s = ttf_encoding_table[plat_id][j]) == NULL)
    return "?";
  return s;
}



Private int  ttf_debug2(char type, char *str);

Private int
ttf_debug(char type)
{
  int    v;
  char  *p0;

  v = FALSE;
  if (env_debug_mode != NULL){
    if ((v = ttf_debug2(type, env_debug_mode)) == TRUE)
      return TRUE;
  }

  if (default_debug_mode == NULL)
    return FALSE;
  if ((p0 = vf_sexp_get_cstring(default_debug_mode)) == NULL)
    return FALSE;
  return ttf_debug2(type, p0);
}

Private int
ttf_debug2(char type, char *p0)
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
