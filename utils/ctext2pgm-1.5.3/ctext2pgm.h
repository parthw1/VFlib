/* ctext2pgm.h */ 

/*
 * Copyright (C) 1998-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */


/* Name of this program */
#define PROG_NAME  "ctext2pgm"
#define VERSION    "version 1.5.0, 13 June 1999"

/* Input encodings */
/* (codesets 2 and 3 are not supported for EUC encodings) */
#define ENC_DEFAULT     0
#define ENC_UNKNOWN     -1
#define ENC_CTEXT       0    /* Compound text */
#define ENC_ISO2022_JP  1    /* ISO-2022-JP */
#define ENC_ISO2022_KR  2    /* ISO-2022-KR */
#define ENC_ISO2022_CN  3    /* ISO-2022-CN */
#define ENC_ISO8859_1  11    /* ISO 8859-1, all chars are 1byte */
#define ENC_ISO8859_2  12    /* ISO 8859-2, all chars are 1byte */
#define ENC_ISO8859_3  13    /* ISO 8859-3, all chars are 1byte */
#define ENC_ISO8859_4  14    /* ISO 8859-4, all chars are 1byte */
#define ENC_ISO8859_5  15    /* ISO 8859-5, all chars are 1byte */
#define ENC_ISO8859_6  16    /* ISO 8859-6, all chars are 1byte */
#define ENC_ISO8859_7  17    /* ISO 8859-7, all chars are 1byte */
#define ENC_ISO8859_8  18    /* ISO 8859-8, all chars are 1byte */
#define ENC_ISO8859_9  19    /* ISO 8859-9, all chars are 1byte */
#define ENC_EUC_JP1    21    /* EUC Japan (JISX0201Roman for codeset 0) */ 
#define ENC_EUC_JP2    22    /* EUC Japan with ISO8859-1 for codeset 0 */
#define ENC_EUC_KR     23    /* EUC Korean */
#define ENC_EUC_CH_GB  24    /* EUC Chinese, GB2312 */
#define ENC_EUC_CH_CNS 25    /* EUC Chinese, CNS11643 */
#define ENC_SJIS       30    /* Shift JIS */

/* Writing directions in a line */
#define WDIR_DEFAULT   0
#define WDIR_L2R       0
#define WDIR_R2L       1

/* Charset types */
#define TYPEID_UNKNOWN -1
#define TYPEID94        1
#define TYPEID96        2
#define TYPEID94_2      3

/* Charsets */
#define CS_UNKNOWN        -1
#define CS_ASCII          (TYPEID94*256 + (4*16+2))
#define CS_ISO8859_1      (TYPEID96*256 + (4*16+1))
#define CS_ISO8859_2      (TYPEID96*256 + (4*16+2))
#define CS_ISO8859_3      (TYPEID96*256 + (4*16+3))
#define CS_ISO8859_4      (TYPEID96*256 + (4*16+4))
#define CS_ISO8859_5      (TYPEID96*256 + (4*16+12))
#define CS_ISO8859_6      (TYPEID96*256 + (4*16+7))
#define CS_ISO8859_7      (TYPEID96*256 + (4*16+6))
#define CS_ISO8859_8      (TYPEID96*256 + (4*16+8))
#define CS_ISO8859_9      (TYPEID96*256 + (4*16+13))
#define CS_JISX0201K      (TYPEID94*256 + (4*16+9))
#define CS_JISX0201R      (TYPEID94*256 + (4*16+10))
#define CS_JISX0208       (TYPEID94_2*256 + (4*16+2))
#define CS_JISX0212       (TYPEID94_2*256 + (4*16+4))
#define CS_VISCII         (TYPEID94_2*256 + (5*16+10))
#define CS_TIS620         (TYPEID94_2*256 + (5*16+4))
#define CS_KSC5601        (TYPEID94_2*256 + (4*16+3))
#define CS_GB2312         (TYPEID94_2*256 + (4*16+1))
#define CS_CNS11643_1     (TYPEID94_2*256 + (4*16+7))
#define CS_CNS11643_2     (TYPEID94_2*256 + (4*16+8))
#define CS_CNS11643_3     (TYPEID94_2*256 + (4*16+9))
#define CS_CNS11643_4     (TYPEID94_2*256 + (4*16+10))
#define CS_CNS11643_5     (TYPEID94_2*256 + (4*16+11))
#define CS_CNS11643_6     (TYPEID94_2*256 + (4*16+12))
#define CS_CNS11643_7     (TYPEID94_2*256 + (4*16+13))
#define CS_MULE_BIG5_L1     (TYPEID94_2*256 + (3*16+0))
#define CS_MULE_BIG5_L2     (TYPEID94_2*256 + (3*16+1))
#define CS_MULE_ARAB0       (TYPEID94*256 + (3*16+2))
#define CS_MULE_ARAB1       (TYPEID94*256 + (3*16+3))
#define CS_MULE_ARAB2       (TYPEID94*256 + (3*16+4))
#define CS_MULE_ETHIOPIC    (TYPEID94_2*256 + (3*16+3))
#define CS_MULE_VISCII_L    (TYPEID96*256 + (3*16+1))
#define CS_MULE_VISCII_U    (TYPEID96*256 + (3*16+2))

#define CSID(t,code)  ((t)*256+(code))

/* Font families */
#define FAM_DEFAULT   1
#define FAM_FIXED     0
#define FAM_TIMES     1
#define FAM_HELV      2
#define FAM_COUR      3

/* Font faces */
#define FACE_DEFAULT  0
#define FACE_NORMAL   0
#define FACE_BOLD     1
#define FACE_ITALIC   2

/* Status of s_font_info.font_id */
#define NOT_OPENED  -1
#define NOT_FOUND   -2

/* Font info structue */
struct s_font_info {
  int    pixel_size;
  int    charset_id;
  int    family_id;
  int    face_id;
  char   *font_name;
  int    font_id;   /* internal use only --- for opened VFlib font id */
};

/* Direction stack structue */
#define MAX_DIR_STACK         64
struct s_dir_stack_elem {
  int    dir;
  struct vf_s_bitmaplist  the_line_buff;
  int    h_pos_x;
  int    h_pos_y;
};


#define PR1(s1)        fprintf(stderr, s1);
#define PR2(s1,s2)     fprintf(stderr, s1, s2);
#define PR3(s1,s2,s3)  fprintf(stderr, s1, s2, s3);

/* Output formats */
#define OFORM_DEFAULT       0
#define OFORM_PGM_ASCII     0
#define OFORM_PGM_RAW       1
#define OFORM_PBM_ASCII     2
#define OFORM_PBM_RAW       3
#define OFORM_EPS          10
#define OFORM_ASCII_ART    20
#define OFORM_ASCII_ART_V  21
#define OFORM_NONE         99

/* Line typeset */
#define LINE_DEFAULT       0
#define LINE_FLUSH_LEFT    1
#define LINE_FLUSH_RIGHT   2
#define LINE_CENTER        3

/* Default values */
#define DEFAULT_VFLIBCAP         "vflibcap-ctext2pgm"
#define DEFAULT_PIXEL_SIZE       16
#define DEFAULT_BASELINESKIP     1.2
#define DEFAULT_MAG              1
#define DEFAULT_MARGIN           0
#define DEFAULT_SHRINK           1
#define DEFAULT_REVERSE          0
#define DEFAULT_LINE_POS         LINE_DEFAULT
#define DEFAULT_EPS_POINT_SIZE   12.0
#define DEFAULT_TAB_SKIP         6


/*EOF*/
