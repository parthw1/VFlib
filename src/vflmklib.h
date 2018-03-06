/* 
 * vflmklib.h 
 * by Hirotsugu Kakugawa
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

#ifndef  __VFLIBMKLIB_H__
#define __VFLIBMKLIB_H__

extern char*  copy_cmdline(int,char**);
extern void   banner(char*,char*,char*);
extern char*  x_strdup(char*);
extern char*  check_font_exist(char*,char**,int,int,char**);
extern void   check_argc(int);
extern int    map_need_tfm(char*);

#endif /* __VFLIBMKLIB_H__ */

