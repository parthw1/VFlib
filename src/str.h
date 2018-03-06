/*
 * str.h - string functions
 *
 */
/*
 * Copyright (C) 1996-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */


#ifndef __VFLIB_STR_H__
#define __VFLIB_STR_H__

extern char  *vf_strdup(char *s);
extern int    vf_strcmp_ci(char *s1, char *s2);
extern int    vf_strncmp_ci(char *s1, char *s2, int n);
extern char  *vf_index(char *s, char ch);
extern int    vf_index_i(char *s, char ch);
extern char  *vf_rindex(char *s, char ch);
extern int    vf_rindex_i(char *s, char ch);
extern int    vf_index_str_i(char *s, char *t);
extern char  *vf_index_str(char *s, char *t);
extern int    vf_rindex_str_i(char *s, char *t);
extern char  *vf_rindex_str(char *s, char *t);
extern int    vf_parse_bool(char *s);

#endif /*__VFLIB_STR_H__*/

/*EOF*/
