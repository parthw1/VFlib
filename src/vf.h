/*
 * vf.h - A vf (virtual font) interface
 * by Hirotsugu Kakugawa
 *
 * 30 Jan 1997  First implementation.
 *  7 Aug 1997  VFlib 3.3  Changed API.
 *  2 Feb 1998  VFlib 3.4
 *
 */
/*
 * Copyright (C) 1996-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */


#ifndef __VFLIB_VF_H__
#define __VFLIB_VF_H__


#ifndef VF_CACHE_SIZE
#  define VF_CACHE_SIZE  16
#endif
#ifndef VF_HASH_SIZE
#  define VF_HASH_SIZE   7
#endif


struct s_vf_char_packet {
  UINT4         pl;
  UINT4         cc;
  UINT4         tfm;
  unsigned char *dvi;
};
typedef struct s_vf_char_packet  *VF_CHAR_PACKET;
struct s_vf_char_packet_tbl {
  int                npackets;
  VF_CHAR_PACKET     packets;
};  
typedef struct s_vf_char_packet_tbl  *VF_CHAR_PACKET_TBL;

struct s_vf_subfont {
  UINT4         k;
  UINT4         s;
  UINT4         d;
  UINT4         a;
  UINT4         l;
  char          *n;
  int           font_id;  /* font id in VFlib */
  struct s_vf_subfont *next;
};
typedef struct s_vf_subfont  *VF_SUBFONT;


struct s_vf {
  char        *vf_path;
  UINT4       cs;
  UINT4       ds; 
  double      design_size;
  double      point_size;
  double      dpi_x, dpi_y;
  double      mag_x, mag_y;
  /* TFM */
  char        *tfm_path;
  TFM         tfm;
  /* subfotns */
  struct s_vf_subfont  *subfonts;
  int                  subfonts_opened;
  int                  default_subfont;
  /* file offset to character packets (offset in vf file) */
  long                 offs_char_packet;
};
typedef struct s_vf  *VF;


struct s_vf_cache_key {
  char    *font_path;
  TFM     tfm;
  long    offs_char_packet;
};
typedef struct s_vf_cache_key  *VF_CACHE_KEY;


#define  VFINST_ID_BYTE           202
#define  VFINST_CP_SHORT_CHAR0      0
#define  VFINST_CP_SHORT_CHAR241  241
#define  VFINST_CP_LONG_CHAR      242

#define  VFINST_SETCHAR0        0
#define  VFINST_SETCHAR127    127
#define  VFINST_SET1          128
#define  VFINST_SET2          129
#define  VFINST_SET3          130
#define  VFINST_SET4          131
#define  VFINST_SETRULE       132
#define  VFINST_PUT1          133
#define  VFINST_PUT2          134
#define  VFINST_PUT3          135
#define  VFINST_PUT4          136
#define  VFINST_PUTRULE       137
#define  VFINST_NOP           138
#define  VFINST_PUSH          141
#define  VFINST_POP           142
#define  VFINST_RIGHT1        143
#define  VFINST_RIGHT2        144
#define  VFINST_RIGHT3        145
#define  VFINST_RIGHT4        146
#define  VFINST_W0            147
#define  VFINST_W1            148
#define  VFINST_W2            149
#define  VFINST_W3            150
#define  VFINST_W4            151
#define  VFINST_X0            152
#define  VFINST_X1            153
#define  VFINST_X2            154
#define  VFINST_X3            155
#define  VFINST_X4            156
#define  VFINST_DOWN1         157
#define  VFINST_DOWN2         158
#define  VFINST_DOWN3         159
#define  VFINST_DOWN4         160
#define  VFINST_Y0            161
#define  VFINST_Y1            162
#define  VFINST_Y2            163
#define  VFINST_Y3            164
#define  VFINST_Y4            165
#define  VFINST_Z0            166
#define  VFINST_Z1            167
#define  VFINST_Z2            168
#define  VFINST_Z3            169
#define  VFINST_Z4            170
#define  VFINST_FNTNUM0       171
#define  VFINST_FNTNUM63      234
#define  VFINST_FNT1          235
#define  VFINST_FNT2          236
#define  VFINST_FNT3          237
#define  VFINST_FNT4          238
#define  VFINST_XXX1          239
#define  VFINST_XXX2          240
#define  VFINST_XXX3          241
#define  VFINST_XXX4          242
#define  VFINST_FNTDEF1       243
#define  VFINST_FNTDEF2       244
#define  VFINST_FNTDEF3       245
#define  VFINST_FNTDEF4       246
#define  VFINST_PRE           247
#define  VFINST_POST          248


#endif /*__VFLIB_VF_H__*/

/*EOF*/
