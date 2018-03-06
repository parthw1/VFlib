/*
 * drv_ini.c - Call initialization functions of font drivers
 * by Hirotsugu Kakugawa
 *
 * 21 Jul 1998  
 * 11 Apr 2010
 *
 */
/*
 * Copyright (C) 1996-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "VFlib-3_7.h"
#include "VFsys.h"
#include "with.h"

extern int  VF_Init_Driver_BDF(void);
extern int  VF_Init_Driver_PCF(void);
extern int  VF_Init_Driver_HBF(void);
extern int  VF_Init_Driver_ZEIT(void);
extern int  VF_Init_Driver_JG(void);
extern int  VF_Init_Driver_EKanji(void);
extern int  VF_Init_Driver_Type1(void);
extern int  VF_Init_Driver_TrueType(void);
extern int  VF_Init_Driver_OpenType(void);
extern int  VF_Init_Driver_TeX(void);
extern int  VF_Init_Driver_GF(void);
extern int  VF_Init_Driver_PK(void);
extern int  VF_Init_Driver_TFM(void);
extern int  VF_Init_Driver_VF(void);
extern int  VF_Init_Driver_JTEX(void);
extern int  VF_Init_Driver_Try(void);
extern int  VF_Init_Driver_Comic(void);
extern int  VF_Init_Driver_Mojikmap(void);

struct drvtbl {
  int   (*func)();
  char   *name;
};

struct drvtbl  installed_drivers[] = {
#ifdef WITH_BDF
  { VF_Init_Driver_BDF, "BDF" },
#endif
#ifdef WITH_PCF
  { VF_Init_Driver_PCF, "PCF" }, 
#endif
#ifdef WITH_HBF
  { VF_Init_Driver_HBF, "HBF" }, 
#endif
#ifdef WITH_TRUETYPE
  { VF_Init_Driver_TrueType, "TrueType" },
#endif
#ifdef WITH_OPENTYPE
  { VF_Init_Driver_OpenType, "OpenType" },
#endif
#ifdef WITH_TYPE1
  { VF_Init_Driver_Type1, "Type1" },
#endif
#ifdef WITH_ZEIT
  { VF_Init_Driver_ZEIT, "Syotai Kurabu (Zeit)" },
#endif
#ifdef WITH_JG
  { VF_Init_Driver_JG, "JG" },
#endif
#ifdef WITH_EKANJI
  { VF_Init_Driver_EKanji, "EKanji" },
#endif
#ifdef WITH_TEXFONTS
  { VF_Init_Driver_TeX, "TeX Font Mapper" },
#ifdef WITH_GF
  { VF_Init_Driver_GF, "TeX GF" },
#endif
#ifdef WITH_PK
  { VF_Init_Driver_PK, "TeX PK" },
#endif
#ifdef WITH_TFM
  { VF_Init_Driver_TFM, "TeX TFM" },
#endif
#ifdef WITH_VF
  { VF_Init_Driver_VF, "TeX Virtual Font" },
#endif
#ifdef WITH_JTEX
  { VF_Init_Driver_JTEX, "ASCII Japanese TeX Kanji" },
#endif
#endif /*WITH_TEXFONTS*/
#ifdef WITH_TRY
  { VF_Init_Driver_Try, "Try" },
#endif
#ifdef WITH_COMIC
  { VF_Init_Driver_Comic, "Japanese Comic Composer" },
#endif
#ifdef WITH_MOJIKMAP
  { VF_Init_Driver_Mojikmap, "Mojikyo Mapper" },
#endif
  { NULL, NULL }
};

Glocal int 
vf_drv_init(void)
{
  int   i;

  for (i = 0; installed_drivers[i].func != NULL; i++){
    if ((*installed_drivers[i].func)() < 0){
      fprintf(stderr,
	      "VFlib warning: Failed to initialize a font driver: %s\n",
	      installed_drivers[i].name);
    }
  }

  return 0;
}

