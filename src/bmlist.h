/*
 * bmlist.h - a header file for bmlist.c
 */
/*
 * Copyright (C) 1996-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */

#ifndef __VFLIB_BMLIST_H__
#define __VFLIB_BMLIST_H__

extern int       vf_bitmaplist_init(VF_BITMAPLIST);
extern int       vf_bitmaplist_put(VF_BITMAPLIST,VF_BITMAP,long,long);
extern VF_BITMAP vf_bitmaplist_compose(VF_BITMAPLIST);
extern int       vf_bitmaplist_finish(VF_BITMAPLIST);

#endif /** __VFLIB_BMLIST_H__*/


/*EOF*/
