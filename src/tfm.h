/*
 * tfm.h - TFM files interface
 */
/*
 * Copyright (C) 1996-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */

#ifndef __VFLIB_TFM_H__
#define __VFLIB_TFM_H__

#include "texfonts.h" 


extern int         vf_tfm_init(void);
extern TFM         vf_tfm_open(char *tfm_path);
extern int         vf_tfm_jfm_chartype(TFM tfm, UINT4 code);
extern VF_METRIC1  vf_tfm_metric(TFM tfm, UINT4 code, VF_METRIC1 metric);
extern void        vf_tfm_free(TFM tfm);

#endif /*__VFLIB_TFM_H__*/


/*EOF*/
