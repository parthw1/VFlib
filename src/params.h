/*
 * params.h - a header file for params.c
 * by Hirotsugu Kakugawa
 *
 * Edition History
 *  22 Mar 1997  First implementation
 */
/*
 * Copyright (C) 1997-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */

#ifndef __VFLIB_PARAMS_H__
#define __VFLIB_PARAMS_H__

#include "sexp.h"

Glocal int   vf_params_init(char*);
Glocal int   vf_params_default(SEXP_ALIST);
Glocal SEXP  vf_params_lookup(char*);

#endif  /* __VFLIB_PARAMS_H__ */

/*EOF*/
