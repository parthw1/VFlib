/*
 * jg.h - a header file for JG font driver in drv_zeit.c 
 * by Hirotsugu Kakugawa
 */
/*
 * Copyright (C) 1996-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */

#ifndef __VFLIB_JG_H__
#define __VFLIB_JG_H__

#define FONTCLASS_NAME       "jg"

#ifndef  DEFAULT_EXTENSIONS
#  define   DEFAULT_EXTENSIONS  ".fn, .FN"
#endif

#define JG_ENV_FONT_DIR      "VFLIB_JG_FONTS"


struct s_jg_header {
  char    *font_path;
  int     nchars;
  long    *ol_offset;
  long    *ol_size;
  int     base;
};
typedef struct s_jg_header *JG_HEADER;

struct s_jg {
  int     nfiles;
  struct s_jg_header  **headers;
};
typedef struct s_jg  *JG;


#define JG_MAX_VALUE         0x0fff
#define JG_MAX_XY            0x07ff
#define JG_XY_MASK           0x07ff
#define JG_CMD_MASK          0x0800

#define JG_CODE_SIZE0        0x0582
#define JG_CODE_SIZE1        0x0bc0
#define JG_CODE_SIZE2        0x0d96
/*#define JG_CODE_SIZE2      0x0d3d*/
#define THRESHOLD_SIZE       0x1000
#define EMPTY_PTR            0xffffffffL

#define   DEFAULT_TO_REF_PT_H  0.86
#define   DEFAULT_TO_REF_PT_V  -0.5


Private int         JG_Init(void);
Private int         JG_Open(char*);
Private void        JG_Close(int);
Private VF_OUTLINE  JG_ReadOutline(int,int,double,double);

#endif /*__VFLIB_JG_H__*/

/*EOF*/
