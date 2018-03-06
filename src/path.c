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

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif
#if HAVE_STRING_H
# include <string.h>
#endif
#if HAVE_STRINGS_H
# include <strings.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#	include <sys/types.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#	include <sys/param.h>
#endif
#if HAVE_SYS_STAT_H
#  ifdef __linux__
#    define __USE_BSD
#  endif
#  include <sys/stat.h>
#endif

#ifdef WIN32 
#  include <io.h> 
#endif

#include "VFlib-3_7.h"
#include "VFsys.h"
#include "path.h"
#include "vflpaths.h"
#include "consts.h"
#include "str.h"


Private int   cons_path(char *path, int n, char *dir, char *file, char *ext);
Private char *vf_path_runtime_file2(char *root, char *subdir, char *file,
				    char *envname);



Glocal char*
vf_path_core_subst_ext(char *f, char *ext) 
     /* "/opt/font/cmr10.300pk" & "vf" => "cmr10.vf" */
     /* IMPORTANT: CALLER MUST RELEASE RETURNED DATA OBJECT */
{
  char  *b, *p, *e;

  if ((f == NULL) || (ext == NULL))
    return NULL;

  if ((b = vf_path_base(f)) == NULL){
    vf_error = VF_ERR_NO_MEMORY;
    return NULL;
  }
  if ((p = (char*)malloc(strlen(b) + 1 + strlen(ext) + 1)) == NULL){
    vf_error = VF_ERR_NO_MEMORY;
    return NULL;
  }
  strcpy(p, b);
  vf_free(b);

  if ((e = vf_index(p, '.')) != NULL)
    *e = '\0';
  if (*ext != '.')
    strcat(p, ".");
  strcat(p, ext);

  return p;
}


Glocal char*
vf_path_base(char *f)   
     /* "/opt/font/cmr10.300pk" => "cmr10.300pk" */
     /* IMPORTANT: CALLER MUST RELEASE RETURNED DATA OBJECT */
{
  char  *s, *b;
  int   dl; 

  dl = strlen(vf_directory_delimiter);
  b = f;
  for (s = f; *s != '\0'; s++){
    if (strncmp(s, vf_directory_delimiter, dl) == 0)
      b = &s[dl];
  }
  return vf_strdup(b); 
}


Glocal char*
vf_path_base_core(char *f)
     /* "/opt/font/cmr10.300pk" => "cmr10" */
     /* IMPORTANT: CALLER MUST RELEASE RETURNED DATA OBJECT */
{
  char  *p, *core;

  if ((core = vf_path_base(f)) == NULL)
    return NULL;
  if ((p = vf_index(core, '.')) != NULL)
    *p = '\0';
  return core;
}




Glocal int
vf_path_absolute(char *f)
{
  if (strncmp(f, vf_directory_delimiter,
	      strlen(vf_directory_delimiter)) == 0){
    return TRUE;
  }

#ifdef MSDOS
  if ((strlen(f) >= 3) 
      && isalpha(f[0]) && (f[1] == ':') 
      && ((f[2] == '/') || (f[2] == '\\'))){
    return TRUE;
  }
#endif /*MSDOS*/

  return FALSE;
}

Glocal int
vf_path_terminated_by_delim(char *f)
{
  int   dlen, index;

  dlen = strlen(vf_directory_delimiter);

  if ((index = strlen(f) - dlen) < 0)
    return FALSE;
  if (strcmp(&f[index], vf_directory_delimiter) != 0)
    return FALSE;

  return TRUE;  
}

Glocal int
vf_path_terminated_by_2delims(char *f)
{
  int   dlen, index;

  dlen = strlen(vf_directory_delimiter);

  if ((index = strlen(f) - 2*dlen) < 0)
    return FALSE;
  if (strncmp(&f[index],      vf_directory_delimiter, dlen) != 0)
    return FALSE;
  if (strncmp(&f[index+dlen], vf_directory_delimiter, dlen) != 0)
    return FALSE;

  return TRUE;  
}

Glocal void
vf_path_del_terminating_2delims(char *f)
{
  int   dlen;

  if (vf_path_terminated_by_2delims(f) == TRUE){
    dlen = strlen(vf_directory_delimiter);
    f[strlen(f)-2*dlen] = '\0';
  }
}

Glocal int
vf_path_cons_path(char *path, int n, char *dir, char *file)
{
  return cons_path(path, n, dir, file, NULL);
}

Glocal int
vf_path_cons_path2(char *path, int n, char *dir, char *file, char *ext)
{
  return cons_path(path, n, dir, file, ext);
}

Private int
cons_path(char *path, int n, char *dir, char *file, char *ext)
{
  int  r;

  r = n;
  if (vf_path_absolute(file)){
    strncpy(path, file, n);
    if ((r -= strlen(file)) < 0)
      return -1;
    if (ext != NULL)
      strncat(path, ext, r);
    return 0;
  } else {
    strncpy(path, dir, r);
    if ((r -= strlen(dir)) < 0)
      return -1;
    if (vf_path_terminated_by_2delims(path) == TRUE)
      vf_path_del_terminating_2delims(path);
    strncat(path, vf_directory_delimiter, r);
    if ((r -= strlen(vf_directory_delimiter)) < 0)
      return -1;
    strncat(path, file, r);
    if ((r -= strlen(file)) < 0)
      return -1;
    if (ext != NULL)
      strncat(path, ext, r);
  }

  return 0;
}

Glocal int
vf_path_concat(char *path, int n, char *f)
{
  if (vf_path_terminated_by_2delims(path) == TRUE)
    vf_path_del_terminating_2delims(path);
  strncat(path, vf_directory_delimiter, n);
  strncat(path, f, n - strlen(path));

  return 0;
}




Glocal int
vf_path_file_read_ok(char *f)
{
#if HAVE_SYS_STAT_H
  struct stat  st;

  if (stat(f, &st) < 0)
    return FALSE;
  if ((st.st_mode & S_IFMT) != S_IFREG)
    return FALSE;
#endif

  if (access(f, R_OK) < 0)
    return FALSE;

  return TRUE;
}

Glocal int
vf_path_directory_read_ok(char *f)
{
#if HAVE_SYS_STAT_H
  struct stat  st;

  if (stat(f, &st) < 0)
    return FALSE;
  if ((st.st_mode & S_IFMT) != S_IFDIR)
    return FALSE;
#endif

  if (access(f, R_OK) < 0)
    return FALSE;

  return TRUE;
}


Glocal char*
vf_path_runtime_dir(char *subdir, char *envname)
     /* return a absolute directory name of SUBDIR under the default 
	runtime dir. Runtime dir can be overridden by ""
	It can be overridden by an env var ENVNAME if 
	it is defined. */
     /* IMPORTANT: CALLER MUST RELEASE RETURNED DATA OBJECT */
{
  char  *root, *dir, *s;
  int    sd_len, dir_len;

  if ((root = getenv(VF_ENV_DIR_RUNTIME_LIB)) == NULL)
    root = DIR_RUNTIME_LIB;

  if ((envname != NULL) && ((s = getenv(envname)) != NULL)){
    root = s;
    subdir = NULL;
  }

  if (subdir == NULL)
    sd_len = 0;
  else
    sd_len = strlen(subdir);
  dir_len = strlen(root) + strlen(vf_directory_delimiter) + sd_len + 4;
  ALLOCN_IF_ERR(dir, char, dir_len){
    return NULL;
  }

  if ((subdir == NULL) || (strcmp(subdir, "") == 0)){
    strncpy(dir, root, dir_len);
  } else {
    vf_path_cons_path(dir, dir_len, root, subdir);
  }

  return  dir;
}

Glocal char*
vf_path_find_runtime_file(char *subdir, char *file, char *envname)
     /* return a absolute directory name of SUBDIR under the default 
	runtime dir. Runtime dir can be overridden by ""
	It can be overridden by an env var ENVNAME if 
	it is defined. */
     /* IMPORTANT: CALLER MUST RELEASE RETURNED DATA OBJECT */
{
  char *root, *p;

  if ((root = getenv(VF_ENV_DIR_RUNTIME_SITE_LIB)) == NULL)
    root = DIR_RUNTIME_SITE_LIB;
  p = vf_path_runtime_file2(root, subdir, file, envname);
  if (p != NULL)
    return p;

  if ((root = getenv(VF_ENV_DIR_RUNTIME_LIB)) == NULL)
    root = DIR_RUNTIME_LIB;
  p = vf_path_runtime_file2(root, subdir, file, envname);
  return p;
}

Private char*
vf_path_runtime_file2(char *root, char *subdir, char *file, char *envname)
{
  char  *dir, *s;
  int    sd_len, dir_len;

  if ((envname != NULL) && ((s = getenv(envname)) != NULL)){
    root = s;
    subdir = NULL;
  }

  if (subdir == NULL)
    sd_len = 0;
  else
    sd_len = strlen(subdir);
  dir_len = strlen(root) + sd_len + strlen(file) 
            + 2 * strlen(vf_directory_delimiter) + 8;
  ALLOCN_IF_ERR(dir, char, dir_len){
    return NULL;
  }

  if ((subdir == NULL) || (strcmp(subdir, "") == 0)){
    vf_path_cons_path(dir, dir_len, root, file);
  } else {
    vf_path_cons_path(dir, dir_len, root, subdir);
    vf_path_concat(dir, dir_len, file);
  }
  if (vf_path_file_read_ok(dir) == FALSE){
    vf_free(dir);
    dir = NULL;
  }

  return  dir;
}


/*EOF*/
