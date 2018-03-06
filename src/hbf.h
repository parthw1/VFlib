/*
 * hbf.c - a header file for hbf.c 
 * by Hirotsugu Kakugawa
 *
 */
/*
 * Copyright (C) 1997-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */


#define FONTCLASS_NAME        "hbf"

#ifndef  HBF_DEFAULT_EXTENSIONS
#  define   HBF_DEFAULT_EXTENSIONS  ".hbf"
#endif

#define HBF_ENV_FONT_DIR          "VFLIB_HBF_FONTS"

struct s_hbf_char {
  int           bbx_width, bbx_height;
  int           off_x, off_y;
  int           mv_x, mv_y;
  unsigned char *bitmap;
  int           raster;
};
typedef struct s_hbf_char  *HBF_CHAR; 

struct s_hbf {
  int     charset;
  char    *path_name;
  char    *uncompress;
  int     nchars;
  int     byte2_ranges;
  int     *byte2_range_start;
  int     *byte2_range_end;
  int     n_byte2;
  int     byte2_index[256];
  int     code_ranges;
  long    *code_range_start;
  long    *code_range_end;
  char    **code_range_bitmap_file_paths;
  char    **code_range_bitmap_uncompresser;
  unsigned char ***code_range_bitmaps;
  long    *code_range_offset;
  double  point_size;
  int     pixel_size;
  int     size;
  double  dpi_x, dpi_y;
  double  slant;
  int     ascent, descent;
  int     font_bbx_width, font_bbx_height;
  int     font_bbx_xoff, font_bbx_yoff;
  SEXP_ALIST  props;
};
typedef struct s_hbf  *HBF;

Private int       HBF_Init(void);
Private int       HBF_Open(char*,SEXP);
Private void      HBF_Close(int);
Private HBF_CHAR  HBF_GetBitmap(int,long);
Private HBF_CHAR  HBF_GetHBFChar(HBF,long);
Private char     *HBF_GetProp(HBF,char*);

/*EOF*/
