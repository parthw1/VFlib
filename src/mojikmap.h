/*
 * mojikmap.h - a header file for drv_mojikmap.c
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

#define   FONTCLASS_NAME_MOJIKMAP    "mojikyo-mapper"

#define VF_CAPE_SUBFONT_NAME         "subfont-name-format"
#define VF_CAPE_DIV_SCHEME           "division-scheme"
#define VF_CAPE_TTF_SUBFONT_ENC      "truetype-subfont-encoding"

#define TYPE1_NSUBS    24

#define  DIVISION_SCHEME_TTF         0
#define  DIVISION_SCHEME_TYPE1       1
#define  DEFAULT_DIVISION_SCHEME     DIVISION_SCHEME_TTF

#define  DEFAULT_SUBFONT_NAME_TTF   "Mojik%d.ttf"
#define  DEFAULT_SUBFONT_NAME_TYPE1 "mo%dm%02d.pfb"

#if   (DEFAULT_DIVISION_SCHEME == DIVISION_SCHEME_TTF)
#  define  DEFAULT_SUBFONT_NAME   DEFAULT_SUBFONT_NAME_TTF
#elif (DEFAULT_DIVISION_SCHEME == DIVISION_SCHEME_TYPE1)
#  define  DEFAULT_SUBFONT_NAME   DEFAULT_SUBFONT_NAME_TYPE1
#endif

#define  TTF_SUBFONT_ENC_UNICODE     0
#define  TTF_SUBFONT_ENC_JIS         1
#define  DEFAULT_TTF_SUBFONT_ENC    TTF_SUBFONT_ENC_UNICODE

/*EOF*/
