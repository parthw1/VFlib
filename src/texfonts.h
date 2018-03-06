/*
 * texfonts.h - a header file for texfonts.c
 * by Hirotsugu Kakugawa
 *
 *   5 Mar 1997  VFlib 3.1.4
 *   1 Apr 1997  VFlib 3.2    Long capability names
 */
/*
 * Copyright (C) 1996-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */

#ifndef __VFLIB_TEXFONTS_H__
#define __VFLIB_TEXFONTS_H__

#define FONTCLASS_NAME_GF           "gf"
#define FONTCLASS_NAME_PK           "pk"
#define FONTCLASS_NAME_TFM          "tfm"
#define FONTCLASS_NAME_VF           "vf"
#define FONTCLASS_NAME_TeX          "TeX"

#define DEFAULT_EXTENSIONS_GF       "gf"     /* e.g., "gf, GF" */
#define DEFAULT_EXTENSIONS_PK       "pk"     /* e.g., "pk, PK" */
#define DEFAULT_EXTENSIONS_TFM      "tfm"    /* e.g., "tfm, TFM" */
#define DEFAULT_EXTENSIONS_VF       "vf"     /* e.g., "vf, VF" */

#define TEX_FILE_FORMAT_TYPE_TFM    0
#define TEX_FILE_FORMAT_TYPE_GF     1
#define TEX_FILE_FORMAT_TYPE_PK     2
#define TEX_FILE_FORMAT_TYPE_VF     3

#define VF_CAPE_TEX_TFM_DIRECTORIES    "tfm-directories"
#define VF_CAPE_TEX_TFM_EXTENSIONS     "tfm-filename-extensions"
#define VF_CAPE_MAKE_MISSING_GLYPH     "make-missing-glyph"
#define VF_CAPE_RESOLUTION_CORR        "resolution-corrections"
#define VF_CAPE_RESOLUTION_ACCU        "resolution-accuracy"

#define VF_CAPE_TEX_FONT_MAPPING       "font-mapping"
#define TEX_FONT_MAPPING_PTSIZE           "point-size-from-tfm" 
#define TEX_FONT_MAPPING_MAG_ADJ          "magnification-adjustment" 

#define VF_CAPE_TEX_GLYPH_STYLE        "glyph-style"  /* for vf, tfm */
#define TEX_GLYPH_STYLE_DEFAULT           1
#define TEX_GLYPH_STYLE_EMPTY_STR         "empty"
#define TEX_GLYPH_STYLE_EMPTY             0
#define TEX_GLYPH_STYLE_FILL_STR          "fill"
#define TEX_GLYPH_STYLE_FILL              1

#define VF_CAPE_TEX_OPEN_STYLE         "open-style"   /* for vf */
#define TEX_OPEN_STYLE_DEFAULT            0
#define TEX_OPEN_STYLE_REQUIRE_STR        "require"
#define TEX_OPEN_STYLE_REQUIRE            0
#define TEX_OPEN_STYLE_TRY_STR            "try"
#define TEX_OPEN_STYLE_TRY                1
#define TEX_OPEN_STYLE_NONE_STR           "none"
#define TEX_OPEN_STYLE_NONE               2

#define DEFAULT_DPI                300
#define DEFAULT_RESOLUTION_ACCU    0.03

#define VFLIB_ENV_ASCII_JTEX_DIR    "VFLIB_ASCII_JTEX_DIRECTORY"

#if 0
#define TEX_ENV_FONT_DIR         "VFLIB_TEX_FONTS"
#define GF_ENV_FONT_DIR          "VFLIB_GF_FONTS"
#define PK_ENV_FONT_DIR          "VFLIB_PK_FONTS"
#define VF_ENV_FONT_DIR          "VFLIB_VF_FONTS"
#define TFM_ENV_FONT_DIR         "VFLIB_TFM_FONTS"
#endif


Glocal int           vf_dbg_drv_texfonts;
Glocal SEXP_ALIST    vf_tex_default_properties;
Glocal SEXP_ALIST    vf_tex_default_variables;



/* IMPORTANT ASSUMPTION:
 *   char:  at least 8 bits
 *   int:   at least 16 bits
 *   long:  at least 32 bits
 */
typedef  char           INT1;
typedef  unsigned char  UINT1;
typedef  int            INT2;
typedef  unsigned int   UINT2;
typedef  long           INT3;
typedef  unsigned long  UINT3;
typedef  long           INT4;
typedef  unsigned long  UINT4;

/*
 * TFM structure 
 */

#define METRIC_TYPE_TFM         0  /* Traditional TFM by Prof. Knuth */
#define METRIC_TYPE_OFM         1  /* Omega Font Metric */
#define METRIC_TYPE_JFM         2 /* Japanese Font Metric by ASCII Co. */
#define METRIC_TYPE_JFM_AUX_H     0     /* Horizontal */
#define METRIC_TYPE_JFM_AUX_V     1     /* Vertical */

typedef struct s_tfm  *TFM;
struct s_tfm {
  /* Font Info */
  int             type;         /* METRIC_TYPE_xxx */
  int             type_aux;     /* METRIC_TYPE_AUX_xxx */
  UINT4           cs;
  /* Metrics */
  UINT4           ds; 
  double          design_size;
  double          slant;
  unsigned int    begin_char, end_char;
  INT4            *width, *height, *depth;
  unsigned int    *ct_kcode, *ct_ctype;   /* JFM only */
  int             nt;                     /* JFM only */
  /* Font bounding box */
  double          font_bbx_w, font_bbx_h;
  double          font_bbx_xoff, font_bbx_yoff;
};



#define READ_INT1(fp)    (INT1)vf_tex_read_intn((fp), 1)
#define READ_UINT1(fp)   (UINT1)vf_tex_read_uintn((fp), 1)
#define READ_INT2(fp)    (INT2)vf_tex_read_intn((fp), 2)
#define READ_UINT2(fp)   (UINT2)vf_tex_read_uintn((fp), 2)
#define READ_INT3(fp)    (INT3)vf_tex_read_intn((fp), 3)
#define READ_UINT3(fp)   (UINT3)vf_tex_read_uintn((fp), 3)
#define READ_INT4(fp)    (INT4)vf_tex_read_intn((fp), 4)
#define READ_UINT4(fp)   (UINT4)vf_tex_read_uintn((fp), 4)
#define READ_INTN(fp,n)  (INT4)vf_tex_read_intn((fp), (n))
#define READ_UINTN(fp,n) (UINT4)vf_tex_read_uintn((fp), (n))
#define SKIP_N(fp,k)     vf_tex_skip_n((fp), (k))

Glocal long           vf_tex_read_intn(FILE*,int);
Glocal unsigned long  vf_tex_read_uintn(FILE*,int);
Glocal void           vf_tex_skip_n(FILE*,int);


#define GET_INT1(p)      (INT1)vf_tex_get_intn((p), 1)
#define GET_UINT1(p)     (UINT1)vf_tex_get_uintn((p), 1)
#define GET_INT2(p)      (INT2)vf_tex_get_intn((p), 2)
#define GET_UINT2(p)     (UINT2)vf_tex_get_uintn((p), 2)
#define GET_INT3(p)      (INT3)vf_tex_get_intn((p), 3)
#define GET_UINT3(p)     (UINT3)vf_tex_get_uintn((p), 3)
#define GET_INT4(p)      (INT4)vf_tex_get_intn((p), 4)
#define GET_UINT4(p)     (UINT4)vf_tex_get_uintn((p), 4)
#define GET_INTN(p,n)    (INT4)vf_tex_get_intn((p), (n))
#define GET_UINTN(p,n)   (UINT4)vf_tex_get_uintn((p), (n))

Glocal long           vf_tex_get_intn(unsigned char*,int);
Glocal unsigned long  vf_tex_get_uintn(unsigned char*,int);



Glocal int         vf_tex_init(void);
Glocal int         vf_tex_default_dpi(void);
Glocal int         vf_tex_fix_resolution(int dev_dpi, double mag);
Glocal int         vf_tex_parse_open_style(char *s, int def_value);
Glocal int         vf_tex_parse_glyph_style(char *s, int def_value);
Glocal void        vf_texfont_parse_font_name(char*,char*,double,double,
					      char*,int,int*,int*,double*);

Glocal int   vf_tex_syntax_check_font_mapping(SEXP);
Glocal int   vf_tex_try_map_and_open_font(VF_FONT font, char *font_name, 
					  SEXP font_mapping,
					  double tfm_design_size,
					  SEXP tfm_dirs, SEXP tfm_extensions,
					  double opt_mag);

Glocal char* vf_tex_search_file_tfm(char *filename, SEXP dirs, SEXP exts);
Glocal char* vf_tex_search_file_glyph(char *filename, int implicit, int format,
				      SEXP dirs, int dpi, double mag, 
				      SEXP exts);
Glocal char* vf_tex_search_file_misc(char *filename, int implicit, int format,
				     SEXP dirs, SEXP exts);

#endif /*__VFLIB_TEXFONTS_H__*/


/*EOF*/
