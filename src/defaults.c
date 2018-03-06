/*
 * defaults.c - a module for reading VFlib defaults from a vflibcap file
 *
 */
/*
 * Copyright (C) 1996-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */

#include  "config.h"
#include  <stdio.h>
#include  <stdlib.h>
#include  <ctype.h>
#include  "VFlib-3_7.h"
#include  "VFsys.h"
#include  "vflibcap.h"
#include  "fsearch.h"
#include  "consts.h"
#include  "params.h"
#include  "sexp.h"
#include  "ccv.h"
#include  "str.h"
#include  "with.h"


Glocal SEXP_ALIST   vf_extension_hints       = NULL;
Glocal SEXP_LIST    vf_implicit_font_classes = NULL;
Glocal char        *vf_directory_delimiter   = VF_DIRECTORY_DELIMITER;

Private SEXP        cap_variables  = NULL;
Private SEXP        cap_unc        = NULL;
Private SEXP        cap_ccv        = NULL;
Private SEXP        cap_kps_switch = NULL;
Private SEXP        cap_kps_mode   = NULL;
Private SEXP        cap_kps_dpi    = NULL;
Private SEXP        cap_kps_prog   = NULL;


Glocal int
vf_defaults_init(void)
{
  SEXP     iter;
  int      z;
  int      kps_switch;
  char    *kps_mode;
  int      kps_dpi;
  char    *kps_prog;
  struct s_capability_table   ct[16];

  z = 0;
  /* VF_CAPE_IMPLICIT_FONT_CLASSES */
  ct[z].cap   = VF_CAPE_IMPLICIT_FONT_CLASSES; 
  ct[z].type  = CAPABILITY_LIST;
  ct[z].ess   = CAPABILITY_OPTIONAL;
  ct[z++].val = &vf_implicit_font_classes;
  /* VF_CAPE_EXTENSION_HINT */
  ct[z].cap   = VF_CAPE_EXTENSION_HINT; 
  ct[z].type  = CAPABILITY_ALIST;
  ct[z].ess   = CAPABILITY_OPTIONAL; 
  ct[z++].val = &vf_extension_hints;
  /* VF_CAPE_VARIABLE_VALUES */
  ct[z].cap   = VF_CAPE_VARIABLE_VALUES;
  ct[z].type  = CAPABILITY_ALIST;
  ct[z].ess   = CAPABILITY_OPTIONAL;
  ct[z++].val = &cap_variables;
  /* VF_CAPE_UNCOMPRESSER */
  ct[z].cap   = VF_CAPE_UNCOMPRESSER;
  ct[z].type  = CAPABILITY_ALIST;
  ct[z].ess   = CAPABILITY_OPTIONAL;
  ct[z++].val = &cap_unc;
  /* VF_CAPE_CODE_CONVERSION_FILES */
  ct[z].cap   = VF_CAPE_CODE_CONVERSION_FILES;
  ct[z].type  = CAPABILITY_STRING_LIST0;
  ct[z].ess   = CAPABILITY_OPTIONAL;
  ct[z++].val = &cap_ccv;
  /* VF_CAPE_KPATHSEA_SWITCH */
  ct[z].cap   = VF_CAPE_KPATHSEA_SWITCH;
  ct[z].type  = CAPABILITY_STRING;
  ct[z].ess   = CAPABILITY_OPTIONAL;
  ct[z++].val = &cap_kps_switch;
  /* VF_CAPE_KPATHSEA_MODE */
  ct[z].cap   = VF_CAPE_KPATHSEA_MODE;
  ct[z].type  = CAPABILITY_STRING;
  ct[z].ess   = CAPABILITY_OPTIONAL;
  ct[z++].val = &cap_kps_mode;
  /* VF_CAPE_KPATHSEA_DPI */
  ct[z].cap   = VF_CAPE_KPATHSEA_DPI;
  ct[z].type  = CAPABILITY_STRING;  
  ct[z].ess   = CAPABILITY_OPTIONAL; 
  ct[z++].val = &cap_kps_dpi;
  /* VF_CAPE_KPATHSEA_PROG_NAME */
  ct[z].cap   = VF_CAPE_KPATHSEA_PROG_NAME;
  ct[z].type  = CAPABILITY_STRING;  
  ct[z].ess   = CAPABILITY_OPTIONAL;
  ct[z++].val = &cap_kps_prog;
  /* end */
  ct[z].cap   = NULL;
  ct[z].type  = 0;
  ct[z].ess   = 0;
  ct[z++].val = NULL;

  if (vf_cap_GetParsedClassDefault(VF_CAPE_VFLIB_DEFAULTS, ct, NULL, NULL) 
      == VFLIBCAP_PARSED_ERROR){
    return -1;
  }
  
  (void) vf_add_uncompresser_alist(cap_unc);

  for (iter = cap_ccv;
       (iter != NULL) && (!vf_sexp_null(iter)); 
       iter = vf_sexp_cdr(iter)){
    if (vf_ccv_autoload(vf_sexp_get_cstring(vf_sexp_car(iter))) < 0){
      fprintf(stderr, "VFlib Warning: Code conversion file '%s' not found.\n", 
	      vf_sexp_get_cstring(vf_sexp_car(iter)));
    }
  }

  kps_switch = DEFAULT_KPS_SWITCH;
  if (cap_kps_switch != NULL)
    kps_switch = vf_parse_bool(vf_sexp_get_cstring(cap_kps_switch));

  kps_mode = DEFAULT_KPS_MODE;
  if (cap_kps_mode != NULL)
    kps_mode = vf_sexp_get_cstring(cap_kps_mode);

  kps_dpi = DEFAULT_KPS_DPI;
  if (cap_kps_dpi != NULL)
    kps_dpi = atoi(vf_sexp_get_cstring(cap_kps_dpi));

  kps_prog = DEFAULT_KPS_PROGRAM_NAME;
  if (cap_kps_prog != NULL)
    kps_prog = vf_sexp_get_cstring(cap_kps_prog);

  vf_kpathsea_init(kps_prog, kps_mode, kps_dpi, kps_switch);

  return 0;
}

/*EOF*/
