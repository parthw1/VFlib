/*
 * pcf.h - a header file for pcf.c
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

#ifndef __VFLIB_PCF_H__
#define __VFLIB_PCF_H__

#define FONTCLASS_NAME            "pcf"

#ifndef  PCF_DEFAULT_EXTENSIONS
#  define   PCF_DEFAULT_EXTENSIONS  ".pcf"
#endif

#define PCF_ENV_FONT_DIR          "VFLIB_PCF_FONTS"


typedef char  INT1;
typedef int   INT2;
typedef long  INT4;
typedef long  CARD4;

struct s_pcf_table {
  CARD4  type;
  CARD4  format;
  CARD4  size;
  CARD4  offset;
};
typedef struct s_pcf_table  *PCF_TABLE;

#define PCF_MSB_FIRST  0
#define PCF_LSB_FIRST  1

#define PCF_FILE_VERSION       0x70636601     /* `p', `c', `f', 1 */
#define PCF_FORMAT_MASK        0xffffff00
#define PCF_FORMAT_MATCH(x,y)  (((x)&PCF_FORMAT_MASK)==((y)&PCF_FORMAT_MASK))

#define PCF_DEFAULT_FORMAT     0x00000000
#define PCF_INKBOUNDS          0x00000200
#define PCF_ACCEL_W_INKBOUNDS  0x00000100
#define PCF_COMPRESSED_METRICS 0x00000100

#define PCF_GLYPH_PAD_MASK     (3<<0)
#define PCF_BYTE_MASK          (1<<2)
#define PCF_BIT_MASK           (1<<3)
#define PCF_SCAN_UNIT_MASK     (3<<4)
#define PCF_GLYPH_PAD_INDEX(f) ((f) & PCF_GLYPH_PAD_MASK)
#define PCF_GLYPH_PAD(f)       (1 << PCF_GLYPH_PAD_INDEX(f))
#define PCF_SCAN_UNIT_INDEX(f) (((f)&PCF_SCAN_UNIT_MASK) >> 4)
#define PCF_SCAN_UNIT(f)       (1 << PCF_SCAN_UNIT_INDEX(f))

#define PCF_BYTE_ORDER(f)  (((f)&PCF_BYTE_MASK) ? PCF_MSB_FIRST:PCF_LSB_FIRST)
#define PCF_BIT_ORDER(f)   (((f)&PCF_BIT_MASK)  ? PCF_MSB_FIRST:PCF_LSB_FIRST)
#define PCF_FORMAT_BITS(f) ((f) & (PCF_GLYPH_PAD_MASK | PCF_BYTE_MASK \
                                   | PCF_BIT_MASK | PCF_SCAN_UNIT_MASK))
#define PCF_SIZE_TO_INDEX(s)   (((s)==4) ? 2 : (((s)==2) ? 1 : 0))
#define PCF_INDEX_TO_SIZE(b)   (1<<b)

#define PCF_PROPERTIES		    (1<<0)
#define PCF_ACCELERATORS	    (1<<1)
#define PCF_METRICS		    (1<<2)
#define PCF_BITMAPS		    (1<<3)
#define PCF_INK_METRICS		    (1<<4)
#define	PCF_BDF_ENCODINGS	    (1<<5)
#define PCF_SWIDTHS		    (1<<6)
#define PCF_GLYPH_NAMES		    (1<<7)
#define PCF_BDF_ACCELERATORS	    (1<<8)

#define PCF_GLYPHPADOPTIONS         4


struct s_pcf_char {
  int    leftSideBearing;
  int    rightSideBearing;
  int    characterWidth;
  int    ascent;
  int    descent;
  int    attributes;
  int           bbx_width, bbx_height;
  int           off_x, off_y;
  int           mv_x, mv_y;
  unsigned char *bitmap;
  int           raster;
};
typedef struct s_pcf_char  *PCF_CHAR; 

struct s_pcf {
  char    *path_name;
  char    *uncompress;
  double  point_size;
  int     pixel_size;
  int     size;
  double  dpi_x, dpi_y;
  double  slant;
  int     charset;
  int     firstCol, lastCol, firstRow, lastRow;
  int     defaultCh;
  int     ascent, descent;
  int     fontAscent, fontDescent;
  int     font_bbx_width, font_bbx_height;
  int     font_bbx_xoff, font_bbx_yoff;
  int           nchars;
  PCF_CHAR      char_table;
  unsigned char*bitmap_block;
  int           nencodings;
  INT2         *encoding;
  SEXP_ALIST    props;
};
typedef struct s_pcf  *PCF;

#include "sexp.h"

#endif /*__VFLIB_PCF_H__*/

/*EOF*/
