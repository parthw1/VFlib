/*
 * fsearch.h - search a file 
 *
 */
/*
 * Copyright (C) 1996-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */

#ifndef __VFLIB_FSEARCH_H__
#define __VFLIB_FSEARCH_H__

#include "VFsys.h"
#include "sexp.h"

#ifndef WIN32
#  include "vflpaths.h"
#endif

#define   DEFAULT_KPS_SWITCH         FALSE
#define   DEFAULT_KPS_MODE           "cx"
#define   DEFAULT_KPS_DPI            300
#define   DEFAULT_KPS_PROGRAM_PATH   VFLSERVER_PATH
#define   DEFAULT_KPS_PROGRAM_NAME   "vflserver"

#define   FSEARCH_FORMAT_TYPE_GF         1
#define   FSEARCH_FORMAT_TYPE_PK         2
#define   FSEARCH_FORMAT_TYPE_VF         3  /* searches vf and ofm */
#define   FSEARCH_FORMAT_TYPE_TFM        4  /* searches tfm and ofm */
#define   FSEARCH_FORMAT_TYPE_OFM        5  /* searches ofm only */
#define   FSEARCH_FORMAT_TYPE_OVF        6  /* searches ovf only */
#define   FSEARCH_FORMAT_TYPE_TTF       10
#define   FSEARCH_FORMAT_TYPE_TYPE1     11
#define   FSEARCH_FORMAT_TYPE_TYPE42    12
#define   FSEARCH_FORMAT_TYPE_AFM       13
#define   FSEARCH_FORMAT_TYPE_PSHEADER  14
#define   FSEARCH_FORMAT_TYPE_OTF       15


extern void  vf_kpathsea_init(char *prog, char *mode, int dpi, int sw);

extern int   vf_add_uncompresser_alist(SEXP);
extern FILE* vf_open_uncompress_stream(char*,char*);
extern int   vf_close_uncompress_stream(FILE*);

extern char* vf_search_file(char *name, int opt_arg1, char *opt_arg2,
			    int use_kpathsea, int kpathsea_file_format,
			    SEXP_LIST dir_list, 
			    SEXP_LIST compressed_ext_list, 
			    char **p_uncomp_prog);
extern char* vf_find_file_in_directory(char *name, char *dir);
extern int   vf_tex_make_glyph(int type, char *font_name, int dpi, double mag);

#endif /*__VFLIB_FSEARCH_H__*/
