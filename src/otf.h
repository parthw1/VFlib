/*
 * otf.h - a heder file for OpenType driver with FreeType 2.
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

#ifndef __VFLIB_OTF_H__

#define __VFLIB_OTF_H__


#define FONTCLASS_NAME                "opentype"

#define VF_CAPE_OTF_FONT_NUMBER       "font-number"
#define VF_CAPE_OTF_PLATFORM_ID       "platform-id"
#define VF_CAPE_OTF_ENCODING_ID       "encoding-id"
#define VF_CAPE_OTF_MAPPING_ID        "mapping-id"
#define VF_CAPE_OTF_ENCODING_FORCE    "encoding-force"
#define VF_CAPE_OTF_HINTING           "hinting"

#define OTF_ENV_FONT_DIR            "VFLIB_OTF_FONTS"

#ifndef  DEFAULT_EXTENSIONS
#  define   DEFAULT_EXTENSIONS      ".otf"
#endif

#define   POINTS_PER_INCH          72.27

#define   DEFAULT_POINT_SIZE       12.0
#define   DEFAULT_PIXEL_SIZE       24
#define   DEFAULT_DIRECTION        'H'  /* horizontal */



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


#endif /*__VFLIB_OTF_H__*/

/*EOF*/
