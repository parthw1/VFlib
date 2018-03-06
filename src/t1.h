/*
 * t1.h - a heder file for Type 1 driver with t1lib.
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

#ifndef __VFLIB_T1_H__

#define __VFLIB_T1_H__


#define FONTCLASS_NAME                "type1"

#define VF_CAPE_TYPE1_LOG_LEVEL          "log-level"
#define VF_CAPE_TYPE1_AFM_DIRECTORIES    "afm-directories"
#define VF_CAPE_TYPE1_ENC_DIRECTORIES    "encoding-vector-directories"
#define VF_CAPE_TYPE1_ENC_VECT           "encoding-vector"
#define VF_CAPE_TYPE1_TFM                "tfm"

#define TTF_ENV_FONT_DIR            "VFLIB_TYPE1_FONTS"

#ifndef  DEFAULT_EXTENSIONS
#  define   DEFAULT_EXTENSIONS      ".pfa, .pfb"
#endif


#define   TYPE1_DEFAULT_DPI        72
#define   TYPE1_POINTS_PER_INCH    72

#define   DEFAULT_POINT_SIZE       12.0
#define   DEFAULT_PIXEL_SIZE       24


#endif /*__VFLIB_T1_H__*/

/*EOF*/
