/* vflibcap.h 
 * by Hirotsugu Kakugawa
 *
 *  Edition History
 *  18 Nov 1997  for VFlib Version 3.3  List-like syntax
 *   8 Jan 1998  for VFlib Version 3.4. Changed syntax.
 */
/*
 * Copyright (C) 1996-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */


#ifndef __VFLIB_VFLIBCAP_H__
#define __VFLIB_VFLIBCAP_H__

#include  "sexp.h"


/* validation of entry */
#define CAPABILITY_LIST          0  /* list, any length */
#define CAPABILITY_STRING        1  /* a string */
#define CAPABILITY_ALIST         2  /* alist */
#define CAPABILITY_VECTOR        3  /* list of two elements */
#define CAPABILITY_STRING_LIST0  4  /* a list of strings, including empty */
#define CAPABILITY_STRING_LIST1  5  /* a list of strings, at least one */
#define CAPABILITY_ESSENTIAL   100  /* essential capability */
#define CAPABILITY_OPTIONAL    101  /* optional capability */
struct s_capability_table {
  char  *cap;
  int    type;   /* LIST, LIST1, ALIST, or ALIST1, etc. */
  int    ess;    /* ESSENTIAL or OPTIONAL */ 
  SEXP  *val;    /* pointer to return value */
};
typedef struct s_capability_table   *CAPABILITY_TABLE;

/* vflibcap file interface */
#define VFLIBCAP_PARSED_OK         0
#define VFLIBCAP_PARSED_NOT_FOUND  1
#define VFLIBCAP_PARSED_ERROR      2

Glocal int    vf_cap_init(char *vflibcap_file);
Glocal SEXP   vf_cap_GetFontEntry(char *font_name);
Glocal int    vf_cap_GetParsedClassDefault(char *class_name, 
					   CAPABILITY_TABLE et,
					   SEXP_ALIST,SEXP_ALIST);
Glocal int    vf_cap_GetParsedFontEntry(SEXP entry, char *font_name,
					CAPABILITY_TABLE ct, 
					SEXP_ALIST,SEXP_ALIST);



#define VF_CAPE_VFLIBCAP_CLASS_DEFAULT_DEFINITION   "define-default"
#define VF_CAPE_VFLIBCAP_FONT_ENTRY_DEFINITION      "define-font"
#define VF_CAPE_VFLIBCAP_MACRO_DEFINITION           "define-macro"
#define VF_CAPE_VFLIBCAP_VARIABLE_MARK              "$"


#define VF_CAPE_VFLIB_DEFAULTS           "VFlib"
#define VF_CAPE_IMPLICIT_FONT_CLASSES    "implicit-font-classes"

#define VF_CAPE_FONT_CLASS               "font-class"
#define VF_CAPE_FONT_FILE                "font-file"
#define VF_CAPE_FONT_NAME                "font-name"
#define VF_CAPE_FONT_DIRECTORIES         "font-directories"
#define VF_CAPE_EXTENSIONS               "filename-extensions"
#define VF_CAPE_UNCOMPRESSER             "uncompression-programs"
#define VF_CAPE_COMPRESSION_EXT          "compression-extensions"
#define VF_CAPE_EXTENSION_HINT           "extension-hints"
#define VF_CAPE_VARIABLE_VALUES          "variable-values"
#define VF_CAPE_CODE_CONVERSION_FILES    "code-conversion-files"
#define VF_CAPE_DEBUG                    "debug"
#define VF_CAPE_KPATHSEA_SWITCH          "use-kpathsea"
#define VF_CAPE_KPATHSEA_MODE            "kpathsea-mode"
#define VF_CAPE_KPATHSEA_DPI             "kpathsea-dpi"
#define VF_CAPE_KPATHSEA_PROG_NAME       "kpathsea-program-name" 

#define VF_CAPE_CHARSET                  "character-set"
#define VF_CAPE_ENCODING                 "encoding"
#define VF_CAPE_FONT_CHARSET             "font-character-set"
#define VF_CAPE_FONT_ENCODING            "font-encoding"
#define VF_CAPE_PROPERTIES               "properties"
#define VF_CAPE_POINT_SIZE               "point-size"
#define VF_CAPE_PIXEL_SIZE               "pixel-size"
#define VF_CAPE_DPI                      "dpi"
#define VF_CAPE_DPI_X                    "dpi-x"
#define VF_CAPE_DPI_Y                    "dpi-y"
#define VF_CAPE_MAG                      "magnification"
#define VF_CAPE_ASPECT_RATIO             "aspect-ratio"     /* width/height */
#define VF_CAPE_SLANT_FACTOR             "slant-factor"     /* tan(angle) */
#define VF_CAPE_VECTOR_TO_BBX_UL         "vector-to-bbx-upper-left"
#define VF_CAPE_VECTOR_TO_NEXT           "vector-to-next-ref-point"
#define VF_CAPE_DIRECTION                "writing-direction"   /* H or V */

#endif  /*__VFLIB_VFLIBCAP_H__*/

/*EOF*/
