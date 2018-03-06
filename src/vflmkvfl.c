/* 
 * vflmkvfl.c 
 * by Hirotsugu Kakugawa
 * - a vflibcap entry generator for VFlib default entry
 * - This program prints vflibcap entries to standard output.
 * - Useful for generating vflibcap for TeX DVI drivers
 *
 *  10 May 2001
 */
/*
 * Copyright (C) 2001-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */


#include "config.h"
#include "with.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif
#include <ctype.h>
#ifdef HAVE_SYS_PARAM_H
#  include  <sys/param.h>
#endif
#ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
#endif

#include  "VFlib-3_7.h"
#include  "VFsys.h"
#include  "vflibcap.h"
#include  "texfonts.h"
#include  "fsearch.h"
#include  "vflmklib.h"



void  gen_class_deafult(void);


char  *mode  = DEFAULT_KPS_MODE;
char  *dpi   = NULL;
int    dpi_i = DEFAULT_KPS_DPI;


#define NEHINTS    64
int   n_eh = 0; 
char  *ehint_ext[NEHINTS];
char  *ehint_cls[NEHINTS];

#define NIMPLS  64
int   n_impl = 0; 
char  *impl[NIMPLS];

#define NCCVS     256
int   n_ccv = 0; 
char  *ccvs[NCCVS];

int use_kpathsea = 0;
char *kps_prog = "xgdvi";


char *cmdline = NULL; 



int 
main(int argc, char **argv)
{
  int     i;
  int    xargc;
  char **xargv;

  dpi = malloc(256);
  sprintf(dpi, "%d", dpi_i);

  cmdline = copy_cmdline(argc, argv);

  n_eh = 0;
  for (i = 0; i < NEHINTS; i++){
    ehint_ext[i] = NULL;
    ehint_cls[i] = NULL;
  }
  n_impl = 0;
  for (i = 0; i < NIMPLS; i++){
    impl[i] = NULL;
  }
  n_ccv = 0;
  for (i = 0; i < NCCVS; i++){
    ccvs[i] = NULL;
  }


  xargc = argc; 
  xargv = argv;

  for (xargc--,xargv++; xargc > 0; xargc--,xargv++){
    if ((strcmp(*xargv, "--help") == 0)
	|| (strcmp(*xargv, "-help") == 0)){
      printf("vflmkvfl: generates vflibcap entries for TFM fonts\n");
      printf("Usage: vflmkvfl [options]\n");
      printf("Options\n");
      printf("  -k       : Use kpathsea for TeX file search\n");
      printf("  -r DPI   : Default device resolution\n");
      printf("  -n MODE  : Device mode name for kpathsea\n");
      printf("  -p PROG  : Application program name for kpathsea\n");
      printf("  -e EXT CLASS : extension hint\n");
      printf("  -i CLASS     : implicit font class\n");
      printf("  -c CCV   : CCV file\n");
      printf("Example: vflmkvfl -k -r 300 -n cx -p xgdvi\n");
      exit(0);

    } else if (strcmp(*xargv, "-k") == 0){
      /* kpathsea */
      use_kpathsea = 1;

    } else if (strcmp(*xargv, "-c") == 0){
      /* ccv */
      if (n_ccv == NCCVS){
	fprintf(stderr, "Too many CCV files\n");
	exit(1);
      }
      xargv++; xargc--;
      check_argc(xargc);
      ccvs[n_ccv++] = x_strdup(*xargv);

    } else if (strcmp(*xargv, "-r") == 0){
      xargv++; xargc--;
      check_argc(xargc);
      dpi = strdup(*xargv);

    } else if (strcmp(*xargv, "-n") == 0){
      /* mode name */
      xargv++; xargc--;
      check_argc(xargc);
      mode = x_strdup(*xargv);

    } else if (strcmp(*xargv, "-p") == 0){
      /* prog name */
      xargv++; xargc--;
      check_argc(xargc);
      kps_prog = x_strdup(*xargv);

    } else if (strcmp(*xargv, "-e") == 0){
      /* extension hint */
      if (n_eh == NEHINTS){
	fprintf(stderr, "Too many extension hints\n");
	exit(1);
      }
      xargv++; xargc--;
      ehint_ext[n_eh] = x_strdup(*xargv); 
      xargv++; xargc--;
      ehint_cls[n_eh] = x_strdup(*xargv); 
      n_eh++;

    } else if (strcmp(*xargv, "-i") == 0){
      /* implicit fonts */
      if (n_impl == NIMPLS){
	fprintf(stderr, "Too many implicit fonts\n");
	exit(1);
      }
      xargv++; xargc--;
      impl[n_impl] = x_strdup(*xargv); 
      n_impl++;

    } else {
      if (*xargv[0] == '-'){
	fprintf(stderr, "vflmkvfl: unknown option %s\n", *xargv);
	exit(1);
      }
      break;

    }
  }

  banner("VFlib defaults", "vflmkvfl", cmdline);

  gen_class_deafult();    

  return 0;
}


void
gen_class_deafult(void)
{
  int   i;

  printf("(%s %s", 
	 VF_CAPE_VFLIBCAP_CLASS_DEFAULT_DEFINITION, VF_CAPE_VFLIB_DEFAULTS);

  printf("\n  (%s", VF_CAPE_EXTENSION_HINT);
  for (i = 0; i < n_eh; i++){
    if ((i%4) == 0)
      printf("\n     ");
    printf(" (\"%s\" %s)", ehint_ext[i], ehint_cls[i]);
  }
  printf(")");

  printf("\n  (%s", VF_CAPE_IMPLICIT_FONT_CLASSES);
  for (i = 0; i < n_impl; i++){
    printf(" %s", impl[i]);
  }
  printf(")");  

  printf("\n  (%s %s)", VF_CAPE_UNCOMPRESSER, 
	 "(\".Z\" \"gzip -cd\") (\".gz\" \"gzip -cd\")");
  printf("\n  (%s", VF_CAPE_VARIABLE_VALUES);
  printf("\n      (TeX_USE_KPATHSEA     \"%s\")",
	 (use_kpathsea==1)?"Yes":"No");
  printf("\n      (TeX_DPI              \"%s\")", dpi);
  printf("\n      (TeX_KPATHSEA_MODE    \"%s\")", mode);
  printf("\n      (TeX_KPATHSEA_PROGRAM \"%s\")", kps_prog);
  printf(")");

  printf("\n  (%s $TeX_USE_KPATHSEA)",     VF_CAPE_KPATHSEA_SWITCH);
  printf("\n  (%s $TeX_KPATHSEA_MODE)",    VF_CAPE_KPATHSEA_MODE);
  printf("\n  (%s $TeX_DPI)",              VF_CAPE_KPATHSEA_DPI);
  printf("\n  (%s $TeX_KPATHSEA_PROGRAM)", VF_CAPE_KPATHSEA_PROG_NAME);

  printf("\n  (%s", VF_CAPE_CODE_CONVERSION_FILES);
  for (i = 0; i < n_ccv; i++){
    if ((i%3) == 0)
      printf("\n     ");
    printf(" \"%s\"", ccvs[i]);
  }
  printf(")");  

  printf(")");
  printf("\n");
  printf("\n");
}
