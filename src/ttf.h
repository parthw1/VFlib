/*
 * ttf.h - a heder file for TrueType driver with FreeType.
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

#ifndef __VFLIB_TTF_H__

#define __VFLIB_TTF_H__


#define FONTCLASS_NAME                "truetype"

#define VF_CAPE_TTF_FONT_NUMBER       "font-number"
#define VF_CAPE_TTF_PLATFORM_ID       "platform-id"
#define VF_CAPE_TTF_ENCODING_ID       "encoding-id"
#define VF_CAPE_TTF_MAPPING_ID        "mapping-id"
#define VF_CAPE_TTF_ENCODING_FORCE    "encoding-force"
#define VF_CAPE_TTF_HINTING           "hinting"
#define VF_CAPE_TTF_JISX0212_R47ES    "jisx0212-row47-empty-sjis"

#define TTF_ENV_FONT_DIR            "VFLIB_TTF_FONTS"

#ifndef  DEFAULT_EXTENSIONS
#  define   DEFAULT_EXTENSIONS      ".ttf, .ttc"
#endif

#define   POINTS_PER_INCH          72.27

#define   DEFAULT_POINT_SIZE       12.0
#define   DEFAULT_PIXEL_SIZE       24
#define   DEFAULT_DIRECTION        'H'  /* horizontal */



#define TTF_OL_ONCURVE                0x01
#define TTF_OL_X_BYTE                 0x02
#define TTF_OL_Y_BYTE                 0x04
#define TTF_OL_REPEAT_FLAGS           0x08
#define TTF_OL_SHORT_X_IS_POSITIVE    0x10
#define TTF_OL_NEXT_X_IS_ZERO         0x10
#define TTF_OL_SHORT_Y_IS_POSITIVE    0x20
#define TTF_OL_NEXT_Y_IS_ZERO         0x20

#define TTF_OL_COMP_ARG1_AND_2_ARE_WORDS      (1<<0)
#define TTF_OL_COMP_ARGS_ARE_XY_VALUES        (1<<1)
#define TTF_OL_COMP_ROUND_XY_TO_GRID          (1<<2)
#define TTF_OL_COMP_WE_HAVE_A_SCALE           (1<<3)
#define TTF_OL_COMP_RESERVED                  (1<<4)
#define TTF_OL_COMP_MORE_COMPONENTS           (1<<5)
#define TTF_OL_COMP_WE_HAVE_AN_X_AND_Y_SCALE  (1<<6)
#define TTF_OL_COMP_WE_HAVE_A_TWO_BY_TWO      (1<<7)
#define TTF_OL_COMP_WE_HAVE_INSTRUCTIONS      (1<<8)
#define TTF_OL_COMP_USE_MY_METRICS            (1<<9)

#define TT_MAP_ID_SEARCH      -1

#define TT_PLAT_ID_ANY         -1
#define TT_PLAT_ID_APPLE       0
#define TT_PLAT_ID_MACINTOSH   1
#define TT_PLAT_ID_ISO         2
#define TT_PLAT_ID_MICROSOFT   3

#define TT_ENC_ID_ANY                     -1
#define TT_ENC_ID_ISO_ASCII                0
#define TT_ENC_ID_ISO_10646                1                
#define TT_ENC_ID_ISO_8859_1               2

#define TT_ENC_ID_MS_SYMBOL                0
#define TT_ENC_ID_MS_UNICODE               1
#define TT_ENC_ID_MS_SHIFT_JIS             2
#define TT_ENC_ID_MS_BIG5                  3
#define TT_ENC_ID_MS_RPC                   4
#define TT_ENC_ID_MS_WANSUNG               5
#define TT_ENC_ID_MS_JOHAB                 6   
#define TT_ENC_ID_APPLE_DEFAULT            0
#define TT_ENC_ID_APPLE_UNICODE_1_1        1
#define TT_ENC_ID_APPLE_ISO_10646          2
#define TT_ENC_ID_APPLE_UNICODE_2_0        3
#define TT_ENC_ID_MAC_ROMAN                0
#define TT_ENC_ID_MAC_JAPANESE             1
#define TT_ENC_ID_MAC_TRADITIONAL_CHINESE  2
#define TT_ENC_ID_MAC_KOREAN               3
#define TT_ENC_ID_MAC_ARABIC               4
#define TT_ENC_ID_MAC_HEBREW               5
#define TT_ENC_ID_MAC_GREEK                6
#define TT_ENC_ID_MAC_RUSSIAN              7
#define TT_ENC_ID_MAC_RSYMBOL              8
#define TT_ENC_ID_MAC_DEVANAGARI           9
#define TT_ENC_ID_MAC_GURMUKHI             10
#define TT_ENC_ID_MAC_GUJARATI             11
#define TT_ENC_ID_MAC_ORIYA                12
#define TT_ENC_ID_MAC_BENGALI              13
#define TT_ENC_ID_MAC_TAMIL                14
#define TT_ENC_ID_MAC_TELUGU               15
#define TT_ENC_ID_MAC_KANNADA              16
#define TT_ENC_ID_MAC_MALAYALAM            17
#define TT_ENC_ID_MAC_SINHALESE            18
#define TT_ENC_ID_MAC_BURMESE              19
#define TT_ENC_ID_MAC_KHMER                20
#define TT_ENC_ID_MAC_THAI                 21
#define TT_ENC_ID_MAC_LAOTIAN              22
#define TT_ENC_ID_MAC_GEORGIAN             23
#define TT_ENC_ID_MAC_ARMENIAN             24
#define TT_ENC_ID_MAC_MALDIVIAN            25
#define TT_ENC_ID_MAC_SIMPLIFIED_CHINESE   25
#define TT_ENC_ID_MAC_TIBETAN              26
#define TT_ENC_ID_MAC_MONGOLIAN            27
#define TT_ENC_ID_MAC_GEEZ                 28
#define TT_ENC_ID_MAC_SLAVIC               29
#define TT_ENC_ID_MAC_VIETNAMESE           30
#define TT_ENC_ID_MAC_SINDHI               31
#define TT_ENC_ID_MAC_UNINTERP             32


/* 
 * TTF outlune
 */
typedef unsigned char   ttf_byte;
typedef char            ttf_char;
typedef unsigned short  ttf_ushort;
typedef short           ttf_short;
typedef unsigned long   ttf_ulong;
typedef long            ttf_long;
typedef long            ttf_fixed;
typedef short           ttf_funit;
typedef short           ttf_fword;
typedef unsigned short  ttf_ufword;
typedef short           ttf_f2dot14;

typedef struct {
  ttf_ufword advanceWidth;
  ttf_fword  lsb;
} ttf_hor_metrics;
typedef struct {
  ttf_ushort advanceHeight;
  ttf_short  tsb;
} ttf_ver_metrics;

typedef struct
{
  int            n_cts;
  int            xMin, yMin, xMax, yMax;
  ttf_hor_metrics  h_met;
  ttf_ver_metrics  v_met;
  int            n_pts;
  unsigned int   *end_points;
  int            n_instructions;
  unsigned char  *instructions;
  unsigned int   *flags;
  int            *xlist;
  int            *ylist;
} ttf_outline;
typedef ttf_outline *TTF_OUTLINE;


#endif /*__VFLIB_TTF_H__*/

/*EOF*/
