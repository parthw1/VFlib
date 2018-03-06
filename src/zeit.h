/*
 * zeit.h - a header file for a font driver in drv_zeit.c
 * by Hirotsugu Kakugawa
 */
/*
 * Copyright (C) 1996-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */

#ifndef  __VFLIB_ZEIT_H__
#define  __VFLIB_ZEIT_H__

#define FONTCLASS_NAME       "zeit"

#ifndef  DEFAULT_EXTENSIONS
#  define   DEFAULT_EXTENSIONS  ".vf, .VF"
#endif

#define ZEIT_ENV_FONT_DIR    "VFLIB_ZEIT_FONTS"


struct s_zeit {
  char    *path_name1;
  char    *path_name2;
  long    *ol_offset1;
  long    *ol_size1;
  long    *ol_offset2;
  long    *ol_size2;
};
typedef struct s_zeit  *ZEIT;


#define ZEIT_NCHARS          0x1142
#define ZEIT_HEADER_SIZE     (2+4*ZEIT_NCHARS)
#define THRESHOLD_SIZE       0x1000
#define ZEIT_MAX_VALUE       0x0400

#define   DEFAULT_TO_REF_PT_H  0.86
#define   DEFAULT_TO_REF_PT_V  -0.5


Private int         ZEIT_Init(void);
Private int         ZEIT_Open(char*);
Private void        ZEIT_Close(int);
Private VF_OUTLINE  ZEIT_ReadOutline(int,int,double,double);

#endif /*__VFLIB_ZEIT_H__*/

/*EOF*/
