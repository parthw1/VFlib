/*
 * param.c - handling vflibcap parameters
 * by Hirotsugu Kakugawa
 *
 * Edition History
 *  22 Mar 1997  First implementation
 *   9 Jan 1998  For VFlib 3.4.
 */
/*
 * Copyright (C) 1997-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#	include <sys/param.h>
#endif
#include <ctype.h>

#include "VFlib-3_7.h"
#include "VFsys.h"
#include "params.h"
#include "sexp.h"



/* Default values given at VF_Init(). */
Private SEXP_ALIST   parameters_args    = NULL;

/* Default values given in vflibcap. */
Private SEXP_ALIST   parameters_default = NULL;


/*
 * Parse default values given at VF_Init(). 
 */
Glocal int
vf_params_init(char *params)
{
  if (parameters_args != NULL)
    vf_sexp_free(&parameters_args);

  if (params == NULL)
    return 0;

  if ((parameters_args = vf_sexp_cstring2alist(params)) == NULL)
    return -1;
  if (vf_dbg_parameters == 1){
    printf(">> Parameter by VF_Init(): ");
    vf_sexp_pp(parameters_args);
  }
  
  return 0;
}


/*
 * Parse default values given in vflibcap. 
 */
Glocal int
vf_params_default(SEXP_ALIST params)
{
  if ((params != NULL) && !vf_sexp_alistp(params)){
    fprintf(stderr, "VFlib Warning: %s: ", 
	    "variable value list in vflibcap must be an alist. ignored.\n");
    vf_sexp_pp(params);
    return -1;
  }

  if (parameters_default != NULL)
    vf_sexp_free(&parameters_default);

  if (params == NULL)
    return 0;

  parameters_default = params;

  if (vf_dbg_parameters == 1){
    printf(">> Parameters in vflibcap: ");
    vf_sexp_pp(params);
  }

  return 0;
}


/*
 * Lookup default values 
 */
Glocal SEXP
vf_params_lookup(char *param)
{
  SEXP  as, v;

  /* First, check values given at VF_Init(). */
  if (parameters_args != NULL){
    if ((as = vf_sexp_assoc(param, parameters_args)) != NULL){
      v = vf_sexp_cdr(as);
      if (vf_dbg_parameters == 1){
	printf(">> Parameter lookup (#1): %s ==> ", param);
	vf_sexp_pp(v);
      }
      return vf_sexp_copy(v);
    }
  }

  /* Next, check values given in vflibcap. */
  if ((parameters_default != NULL)
      && ((as = vf_sexp_assoc(param, parameters_default)) != NULL)){
    v = vf_sexp_cdr(as);
    if (vf_dbg_parameters == 1){
      printf(">> Parameter lookup (#2): %s ==> ", param);
      vf_sexp_pp(v);
    }
    return vf_sexp_copy(v);
  }

  return NULL;
}


/*EOF*/
