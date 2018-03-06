/*
 * vflpp.c - prettyprint a vflibcap file
 * by Hirotsugu Kakugawa
 *
 *   22 Jul 1998  First implementation
 */
/*
 * Copyright (C) 1998-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#if HAVE_STDARG_H
#  include <stdarg.h>
#else
#  include <varargs.h>
#endif

#include "VFlib-3_7.h"
#include "VFsys.h"
#include "vflibcap.h"
#include "sexp.h"

#define PROG_NAME  "vflpp"

void  usage(void);


int
main(int argc, char **argv)
{
  FILE   *fp_in;
  int    need_close;
  SEXP   s;

  /* get input stream */
  need_close = 0;
  if (argc >= 2){
    if ((fp_in = fopen(argv[1], FOPEN_RD_MODE_TEXT)) == NULL){
      fprintf(stderr, "Cannot open: %s\n", argv[1]);
      exit(1);
    }
    need_close = 1;
  } else {
    fp_in = stdin;
  }

  /* prettyprint */
  while ((s = vf_sexp_read(fp_in)) != NULL){
    if (   vf_sexp_listp(s)
	&& (vf_sexp_length(s) >= 2)
	&& (vf_sexp_stringp(vf_sexp_car(s)))
	&& (vf_sexp_stringp(vf_sexp_cadr(s)))
	&& (   (strcmp(vf_sexp_get_cstring(vf_sexp_car(s)), 
		       VF_CAPE_VFLIBCAP_CLASS_DEFAULT_DEFINITION) == 0)
	    || (strcmp(vf_sexp_get_cstring(vf_sexp_car(s)), 
		       VF_CAPE_VFLIBCAP_FONT_ENTRY_DEFINITION) == 0)
	    || (strcmp(vf_sexp_get_cstring(vf_sexp_car(s)), 
		       VF_CAPE_VFLIBCAP_MACRO_DEFINITION) == 0))){
      vf_sexp_pp_entry(s);
    } else {
      vf_sexp_pp(s);
    }
    printf("\n");
    vf_sexp_free1(&s);
  }

  /* close input stream */
  if (need_close == 1){
    fclose(fp_in);
  }

  return 0;
}
