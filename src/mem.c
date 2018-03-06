/*
 * mem.c  --- memory manager
 *
 */
/*
 * Copyright (C) 1996-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif
#if HAVE_STRING_H
# include <string.h>
#endif
#if HAVE_STRINGS_H
# include <strings.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#	include <sys/types.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#	include <sys/param.h>
#endif
#include "VFlib-3_7.h"
#include "VFsys.h"


Glocal void
vf_free(void *p) 
{
  static  int  init = 0;
  static  void *xaddr = NULL;

  if (init == 0){
    init = 1;
    if (getenv("VFLIB_DEBUG_FREE") != NULL){
      sscanf(getenv("VFLIB_DEBUG_FREE"), "%p", &xaddr);
    }
  }

  if (p == NULL)
    return;

  if ((xaddr != NULL) && (p == xaddr)){
    fprintf(stderr, "VFlib error in free\n");
    abort();
  }

  free(p);
}


/*EOF*/
