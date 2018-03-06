/*
 * path.c  --- path string functions
 *
 */
/*
 * Copyright (C) 1996-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */


#ifndef __VFLIB_PATH_H__
#define __VFLIB_PATH_H__

extern char  *vf_path_core_subst_ext(char *f, char *ext);
extern char  *vf_path_base(char *f);
extern char  *vf_path_base_core(char *f);
extern int    vf_path_absolute(char *f);
extern int    vf_path_file_read_ok(char *f);
extern int    vf_path_directory_read_ok(char *f);
extern int    vf_path_terminated_by_delim(char *f);
extern int    vf_path_terminated_by_2delims(char *f);
extern void   vf_path_del_terminating_2delims(char *f);
extern int    vf_path_cons_path(char *path, int n, char *dir, char *file);
extern int    vf_path_cons_path2(char *path, int n, 
				 char *dir, char *file, char *ext);
extern int    vf_path_concat(char *path, int n, char *f);
extern char  *vf_path_runtime_dir(char *subdir, char *envname);
extern char  *vf_path_find_runtime_file(char *subdir, char *file,
					char *envname);

#endif /*__VFLIB_PATH_H__*/
