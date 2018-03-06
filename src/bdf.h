/*
 * bdf.h - a header fiel for bdf interface
 * by Hirotsugu Kakugawa
 *
 */
/*
 * Copyright (C) 1996-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */

#ifndef __VFLIB_BDF_H__
#define __VFLIB_BDF_H__

#define FONTCLASS_NAME            "bdf"

#ifndef  BDF_DEFAULT_EXTENSIONS
#  define   BDF_DEFAULT_EXTENSIONS  ".bdf"
#endif

#define HAVE_FONT_ASCENT
#define HAVE_FONT_DESCENT

#define BDF_ENV_FONT_DIR          "VFLIB_BDF_FONTS"



struct s_bdf_char {
  long          code_point;
  long          f_offset;
  int           bbx_width, bbx_height;
  int           off_x, off_y;
  int           mv_x, mv_y;
  unsigned char *bitmap;
  int           raster;
};
typedef struct s_bdf_char  *BDF_CHAR; 

struct s_bdf {
  int     charset;
  char    *path_name;
  char    *uncompress;
  double  point_size;
  int     pixel_size;
  int     size;
  double  dpi_x, dpi_y;
  double  slant;
  int     font_bbx_width, font_bbx_height;
  int     font_bbx_xoff, font_bbx_yoff;
  int     ascent, descent;
  int         nchars;
  BDF_CHAR    char_table;
  long       *char_table_x;
  SEXP_ALIST     props;
};
typedef struct s_bdf  *BDF;

#include "sexp.h"

#endif /*__VFLIB_BDF_H__*/

/*EOF*/
