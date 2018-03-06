/* 
 * vfldrvs.c - print a list of installed font drivers
 * by Hirotsugu Kakugawa
 *
 *  22 Jul 1998   
 *
 */
/*
 * Copyright (C) 1996-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */


/*
 * Usage:  vfldrvs
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#if defined(HAVE_STRING_H) || defined(STDC_HEADERS)
#  include  <string.h>
#else
#  include  <strings.h>
#endif
#include "VFlib-3_7.h"


int
main(int argc, char **argv)
{
  int     i;
  char    *vfcap, **drv_list;

  vfcap = NULL;
  --argc; argv++;
  while (argc > 0){
    if ((argc >= 2) && (strcmp(argv[0], "-v") == 0)){
      --argc; argv++;
      vfcap = argv[0];
      --argc; argv++;
    } else {
      break;
    }
  }

  if ((VF_Init(vfcap, NULL) < 0) 
      || ((drv_list = VF_FontDriverList()) == NULL))
    exit(1);

  for (i = 0; drv_list[i] != NULL; i++)
    printf("%s\n", drv_list[i]);

  return 0;
}
