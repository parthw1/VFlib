/*
 * VFsys.h - misc definitions for internals of VFlib
 * 
 */
/*
 * Copyright (C) 1996-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */

#ifndef __VFLIB_METRIC_H__
#define __VFLIB_METRIC_H__

extern VF_METRIC1 vf_alloc_metric1(void);
extern VF_METRIC2 vf_alloc_metric2(void);
extern void       vf_metric1_to_metric2(VF_METRIC1,double,VF_METRIC2);

extern void  vf_dump_metric1(VF_METRIC1);
extern void  vf_dump_metric2(VF_METRIC2);

#endif
/*EOF*/
