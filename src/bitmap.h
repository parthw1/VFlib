/*
 * bitmap.h - a header file for bitmap
 *
 */
/*
 * Copyright (C) 1996-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */

#ifndef __VFLIB_BITMAP_H__
#define __VFLIB_BITMAP_H__

extern VF_BITMAP  vf_alloc_bitmap(int,int);
extern VF_BITMAP  vf_alloc_bitmap_with_metric1(VF_METRIC1,double,double);
extern VF_BITMAP  vf_alloc_bitmap_with_metric2(VF_METRIC2);
extern void       vf_free_bitmap(VF_BITMAP);
extern void       vf_dump_bitmap(VF_BITMAP);

#endif /*__VFLIB_BITMAP_H__*/


/*EOF*/
