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

#ifndef __VFLIB_JFMBI_H__
#define __VFLIB_JFMBI_H__

extern int         vf_tfm_builtin_jfm_chartype(long code_point, int dir_h);
extern VF_METRIC1  vf_tfm_builtin_jfm_metric(long code_point, 
					     VF_METRIC1 metric, 
					     int dir_h, double design_size);


#endif /*__VFLIB_JFMBI_H__*/
