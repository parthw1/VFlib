/* vflibcap.c - a module for reading a vflibcap file.
 * by Hirotsugu Kakugawa
 *
 *  Edition History
 *  18 Nov 1997  for VFlib Version 3.4.  Lisp-like syntax.  
 *  10 Jun 1998  Added capability arg types CAPABILITY_STRING_LIST0
 *               and CAPABILITY_STRING_LIST1.
 *  24 Jun 1998  Changed to read environemnt variable value VFLIBCAP_PARAM_xxx
 *               as an s-exp.
 *  20 Jan 1999  Changed to vflibcap file searching.
 *               
 */
/*
 * Copyright (C) 1996-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */

#include  "config.h"
#include  <stdio.h>
#include  <stdlib.h>
#if defined(HAVE_STRING_H) || defined(STDC_HEADERS)
#  include  <string.h>
#else
#  include  <strings.h>
#endif
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif
#include  <ctype.h>
#ifdef HAVE_SYS_PARAM_H
#include  <sys/param.h>
#endif
#include  <fcntl.h>
#include  "VFlib-3_7.h"
#include  "VFsys.h"
#include  "vflibcap.h"
#include  "consts.h"
#include  "path.h"
#include  "vflpaths.h"
#include  "params.h"
#include  "str.h"
#include  "sexp.h"


Private char* find_vflibcap(char *name);
Private SEXP  get_entry(char *type, char *name, char *desc);
Private SEXP  get_entry2(FILE *fp, char *type, char *top_entry, char *name, 
			 char *desc, int depth, int *statp);
Private int   syntax_check_entry(SEXP entry);

Private int   parse_entry(SEXP entry, CAPABILITY_TABLE ct, char *name,
			  SEXP_ALIST, SEXP_ALIST, char *def_type);
Private SEXP  get_value_if_variable(SEXP val, SEXP_ALIST,SEXP_ALIST);
Private SEXP  get_variable_value(char *var, SEXP_ALIST,SEXP_ALIST);


Private char  *VFlibcapFile = NULL;


Glocal int
vf_cap_init(char *vflibcap_file)
{
  char    *s;

  if (vf_dbg_vfcap == 1)
    printf(">> Init vflibcap: arg=%s\n", 
	   (vflibcap_file==NULL) ? "<null>" : vflibcap_file);
  
  if (VFlibcapFile != NULL){
    vf_free(VFlibcapFile);
    VFlibcapFile = NULL;
  }

  /* If env var VFLIB_VFLIBCAP_PATH is defined, use its value as path name */
  if ((s = getenv(VF_ENV_VFLIBCAP_PATH)) != NULL){
    if (vf_dbg_vfcap == 1)
      printf(">>   Checking vflibcap %s  (%s %s)\n",
	     s, "given by environment variable", VF_ENV_VFLIBCAP_PATH);
    VFlibcapFile = find_vflibcap(s);
    if (VFlibcapFile == NULL)
      return -1;
    goto vflibcap_is_found;
  }

  if (vflibcap_file == NULL)
    vflibcap_file = VF_DEFAULT_VFLIBCAP_FILE;

  /* Search vflibcap in runtime dir */
  if ((VFlibcapFile = find_vflibcap(vflibcap_file)) != NULL)
    goto vflibcap_is_found;

  /* not found .. */
  if (vf_dbg_vfcap == 1)
    printf(">>  Not found\n");
  if (vf_error != VF_ERR_NO_MEMORY)
    vf_error = VF_ERR_NO_VFLIBCAP;
  return -1;

vflibcap_is_found:
  if (vf_dbg_vfcap == 1)
    printf(">> Found: %s\n", VFlibcapFile);
  return 0;
}

Private char*
find_vflibcap(char *name)
{
  char  *s, *p; 

  /* Check vflibcap file relative to current directory */
  if (vf_dbg_vfcap == 1)
    printf(">>   Checking vflibcap relative to current dir: %s\n", name);
  if (vf_path_file_read_ok(name) == TRUE){
    if ((s = vf_strdup(name)) == NULL){
      vf_error = VF_ERR_NO_MEMORY;
      return NULL;
    }
    return  s;
  }

  /* Next, Check vflibcap file in runtime dir */
  if (vf_dbg_vfcap == 1)
    printf(">>   Checking vflibcap in runtime dir: %s\n", name);
  p = vf_path_find_runtime_file(NULL, name, VF_ENV_VFLIBCAP_DIR);

  return p;
}



Glocal int
vf_cap_GetParsedClassDefault(char *class_name, CAPABILITY_TABLE ct,
			     SEXP_ALIST varlist1, SEXP_ALIST varlist2)
{
  SEXP  entry;
  int   v, i;

  if (ct != NULL){
    for (i = 0; ct[i].cap != NULL; i++){
      if (ct[i].val != NULL)
	*(ct[i].val) = NULL;
    }
  }

  entry = get_entry(VF_CAPE_VFLIBCAP_CLASS_DEFAULT_DEFINITION,
		    class_name, "class default");
  if (entry == NULL)
    return VFLIBCAP_PARSED_NOT_FOUND;

  v = parse_entry(entry, ct, class_name, varlist1, varlist2,
		  VF_CAPE_VFLIBCAP_CLASS_DEFAULT_DEFINITION);

  vf_sexp_free(&entry);

  return v;
}

Glocal int
vf_cap_GetParsedFontEntry(SEXP entry, char *font_name, 
			  CAPABILITY_TABLE ct, 
			  SEXP_ALIST varlist1, SEXP_ALIST varlist2)
{
  int   v, i;

  if (ct != NULL){
    for (i = 0; ct[i].cap != NULL; i++){
      if (ct[i].val != NULL)
	*(ct[i].val) = NULL;
    }
  }

  if (entry == NULL)
    return -1;

  v = parse_entry(entry, ct, font_name, varlist1, varlist2,
		  VF_CAPE_VFLIBCAP_FONT_ENTRY_DEFINITION);

  return v;
}

Glocal SEXP
vf_cap_GetFontEntry(char *font_name)
{
  return get_entry(VF_CAPE_VFLIBCAP_FONT_ENTRY_DEFINITION,
		   font_name, "font entry");
}



#define GET_ENTRY_STATUS_OK                  0
#define GET_ENTRY_STATUS_LOOP                1
#define GET_ENTRY_STATUS_NOT_FOUND           2
#define GET_ENTRY_STATUS_NOT_FOUND_NEED_MSG  3

Private SEXP
get_entry(char *type, char *name, char *desc)
{
  int   stat;
  SEXP  entry;
  FILE  *fp;

  if ((fp = fopen(VFlibcapFile, FOPEN_RD_MODE_TEXT)) == NULL){
    vf_error = VF_ERR_NO_VFLIBCAP;
    if (vf_dbg_vfcap == 1)
      printf(">>Can't read vflibcap file: %s\n", VFlibcapFile);
    return NULL;
  }

  entry = get_entry2(fp, type, name, name, desc, 0, &stat);

  fclose(fp);

  if (stat == GET_ENTRY_STATUS_OK)
    return entry;

  vf_error = VF_ERR_NO_FONT_ENTRY;
  return NULL;
}

Private SEXP
get_entry2(FILE *fp, char *type, char *top_entry, char *name,
	   char *desc, int depth, int *statp)
{
  int   expanded, valid;
  SEXP  sexp, mdef, mhead, mtail, mlast, prev, s, next;

  *statp = GET_ENTRY_STATUS_OK;

  if (depth > 16){
    /* Nesting is too deep. 
       Possibly, the inheritance relation must have a loop. */
    fprintf(stderr, "VFlib: nesting of %s is too deep in vflibcap %s.\n",
	    top_entry, VFlibcapFile);
    *statp = GET_ENTRY_STATUS_LOOP;
    return NULL; 
  }

  rewind(fp);
  for (;;){
    if ((sexp = vf_sexp_read(fp)) == NULL)
      break;
    if (   vf_sexp_consp(sexp) 
	&& vf_sexp_stringp(vf_sexp_car(sexp))
	&& vf_sexp_consp(vf_sexp_cdr(sexp))
	&& vf_sexp_stringp(vf_sexp_cadr(sexp))  ){     /* (str str ...) */ 
      if ((strcmp(vf_sexp_get_cstring(vf_sexp_car(sexp)), type) == 0)
	  && (strcmp(vf_sexp_get_cstring(vf_sexp_cadr(sexp)), name) == 0)){
	if (vf_dbg_vfcap == 1)
	  printf (">>Found %s %s\n", desc, name);
	break;
      }
    }
    vf_sexp_free(&sexp);
  }

  if (sexp == NULL){
    if (vf_dbg_vfcap == 1)
      printf(">>Can't find %s '%s' in vflibcap %s\n", 
	     desc, name, VFlibcapFile);
    *statp = GET_ENTRY_STATUS_NOT_FOUND_NEED_MSG;
    return NULL;
  }

  /* inheritance by macros */
  for (;;){
    expanded = 0;
    prev = vf_sexp_cdr(sexp);
    s    = vf_sexp_cdr(prev);
    while (vf_sexp_consp(s)){
      if (vf_sexp_stringp(vf_sexp_car(s))){
	mdef = get_entry2(fp, VF_CAPE_VFLIBCAP_MACRO_DEFINITION,
			  top_entry, vf_sexp_get_cstring(vf_sexp_car(s)), 
			  "macro", depth+1, statp);
	if (mdef == NULL){
	  if (*statp == GET_ENTRY_STATUS_NOT_FOUND_NEED_MSG){
	    fprintf(stderr, "VFlib: macro '%s' is undefined in vflibcap %s\n",
		    vf_sexp_get_cstring(vf_sexp_car(s)), VFlibcapFile);
	    *statp = GET_ENTRY_STATUS_NOT_FOUND;
	  }
	  return NULL;
	}
	/* expand macro */
	mhead = mtail = vf_sexp_cddr(mdef);
	while (vf_sexp_consp(vf_sexp_cdr(mtail)))
	  mtail = vf_sexp_cdr(mtail);
	mlast = vf_sexp_cdr(mtail);
	next = vf_sexp_cdr(s);
	vf_sexp_rplacd(prev, mhead);
	vf_sexp_rplacd(mtail, next);
	/* release garbage */
	vf_sexp_cdr(mdef)->t.cons.cdr = NULL; 
	vf_sexp_free(&mdef);
	vf_sexp_free(&mlast);
	s->t.cons.cdr = NULL;
	vf_sexp_free(&s);
	expanded = 1;
	break;
      }
      prev = s;
      s = vf_sexp_cdr(s);
    }
    if (expanded == 0)
      break;
  }    

  valid = syntax_check_entry(sexp);
  if (!valid){
    fprintf(stderr, "VFlib: Syntax error in vflibcap '%s' for entry '%s'.\n",
	    VFlibcapFile, name);
    *statp = GET_ENTRY_STATUS_NOT_FOUND;    
    vf_sexp_free(&sexp);
    return NULL;
  }
      
  return sexp;
}

Private int
syntax_check_entry(SEXP entry)
{
  char  *str;
#if 0
  SEXP  s, t, u, sa;
#endif

  if (   !vf_sexp_listp(entry)
      || (vf_sexp_length(entry) < 2)
      || !vf_sexp_stringp(vf_sexp_car(entry))
      || !vf_sexp_stringp(vf_sexp_cadr(entry)) )
    return FALSE;

  str = vf_sexp_get_cstring(vf_sexp_car(entry));
  if (   (strcmp(str, VF_CAPE_VFLIBCAP_FONT_ENTRY_DEFINITION) != 0)
      && (strcmp(str, VF_CAPE_VFLIBCAP_CLASS_DEFAULT_DEFINITION) != 0)
      && (strcmp(str, VF_CAPE_VFLIBCAP_MACRO_DEFINITION) != 0) )
    return FALSE;

#if 0
  for (s = vf_sexp_cddr(entry); vf_sexp_consp(s); s = vf_sexp_cdr(s)){
    sa = vf_sexp_car(s);
    if (   !vf_sexp_listp(sa)
	|| !vf_sexp_stringp(vf_sexp_car(sa))
	|| (vf_sexp_length(sa) < 2)  )
      return FALSE;
    t = vf_sexp_cdr(sa); 
    if (vf_sexp_stringp(vf_sexp_car(t))){
      /* all elements must be strings */
      for (t = vf_sexp_cdr(t); vf_sexp_consp(t); t = vf_sexp_cdr(t)){
	if (!vf_sexp_stringp(vf_sexp_car(t)))
	  return FALSE;
      }
    } else {
      /* must be an alist */
      for ( ; vf_sexp_consp(t); t = vf_sexp_cdr(t)){
	if (!vf_sexp_listp(vf_sexp_car(t))
	    || (vf_sexp_length(vf_sexp_car(t)) <= 1))
	  return FALSE;
	for (u = vf_sexp_car(t); !vf_sexp_null(u); u = vf_sexp_cdr(u)){
	  if (!vf_sexp_stringp(vf_sexp_car(u)))
	    return FALSE;
	}
      }
    }
  }
  if (!vf_sexp_null(s))
    return FALSE;
#endif

  return TRUE;
}



Private int
parse_entry(SEXP entry, CAPABILITY_TABLE ct, char *name,
	    SEXP_ALIST varlist1, SEXP_ALIST varlist2, 
	    char *def_type)
{
  SEXP  sexp, scap = NULL, sval, cap_val, v, vvv;
  int   val, f, i;
  char  *typename;

  if (vf_dbg_vfcap == 1)
    printf(">>  Parsing vflibcap entry: (%s %s ...)\n", def_type, name);

  if (entry == NULL)
    return VFLIBCAP_PARSED_OK;

  if (ct == NULL){
    if (vf_sexp_length(entry) == 2)
      return VFLIBCAP_PARSED_OK;
    else if (vf_sexp_length(entry) < 2)
      return VFLIBCAP_PARSED_ERROR;
    for (sexp = vf_sexp_cddr(entry); 
	 !vf_sexp_null(sexp); 
	 sexp = vf_sexp_cdr(sexp)){
      scap = vf_sexp_caar(sexp);
      fprintf(stderr, "VFlib Warning: %s: %s\n",
	      "undefined capability in vflibcap",
	      vf_sexp_get_cstring(scap));
    }
    return VFLIBCAP_PARSED_ERROR;
  }

  if (vf_sexp_length(entry) <= 1)
    return VFLIBCAP_PARSED_ERROR;

  val = VFLIBCAP_PARSED_OK;

  /* Check capabilities in a vflibcap entry */
  for (sexp = vf_sexp_cddr(entry);
       vf_sexp_consp(sexp);
       sexp = vf_sexp_cdr(sexp)){
    if (!vf_sexp_consp(vf_sexp_car(sexp)))
      continue;
    scap = vf_sexp_caar(sexp);
    sval = vf_sexp_cdar(sexp);
    for (i = 0; ct[i].cap != NULL; i++){
      if (strcmp(vf_sexp_get_cstring(scap), ct[i].cap) == 0)
	break;
    }
    if (ct[i].cap == NULL){
      /* the capability given vflibcap is not defined. */
      fprintf(stderr, "VFlib Warning: %s '%s' (%s %s ...)\n",
	      "Undefined capability in vflibcap",
	      vf_sexp_get_cstring(scap), def_type, name);
    } else {
      if ((ct[i].val != NULL) && (*(ct[i].val) != NULL)){
	fprintf(stderr, "VFlib Warning: %s '%s' in vflibcap (%s %s ...)\n",
		"multiple definition of a capability",
		vf_sexp_get_cstring(scap), def_type, name);
	vf_sexp_free(ct[i].val);
	*(ct[i].val) = NULL;
      }
      /* variable expantion. */
      if ((strcmp(VF_CAPE_VFLIB_DEFAULTS, name) == 0)
	  && (strcmp(ct[i].cap, VF_CAPE_VARIABLE_VALUES) == 0))
	v = vf_sexp_copy(sval);
      else 
	v = get_value_if_variable(sval, varlist1, varlist2);
      if (v == NULL)
	continue;
      /* Check the value type. */
      cap_val = NULL;
      switch (ct[i].type){
      case CAPABILITY_LIST:
	typename = "a list";
	if (vf_sexp_listp(v))
	  cap_val = vf_sexp_copy(v);
	break;
      case CAPABILITY_STRING:
	typename = "a string";
	if (vf_sexp_listp(v) && (vf_sexp_length(v) == 1) 
	     && (vf_sexp_stringp(vf_sexp_car(v))))
	  cap_val = vf_sexp_copy(vf_sexp_car(v));
	break;
      case CAPABILITY_ALIST:
	typename = "an alist";
	if (vf_sexp_alistp(v))
	  cap_val = vf_sexp_copy(v);
	break;
      case CAPABILITY_VECTOR:
	typename = "a vector";
	if (vf_sexp_listp(v) && (vf_sexp_length(v) == 2)
	    && (vf_sexp_stringp(vf_sexp_car(v))) 
	    && (vf_sexp_stringp(vf_sexp_cadr(v))))
	  cap_val = vf_sexp_copy(v);
	break;
      case CAPABILITY_STRING_LIST0:
	typename = "a list of strings (including none)";
	if ((vf_sexp_listp(v) && (vf_sexp_length(v) >= 0))){
	  f = 1;
	  for (vvv = v; vf_sexp_consp(vvv); vvv = vf_sexp_cdr(vvv)){
	    if (!vf_sexp_stringp(vf_sexp_car(vvv))){
	      f = 0;
	      break;
	    }
	  }
	  if (f == 1)
	    cap_val = vf_sexp_copy(v);
	}
	break;
      case CAPABILITY_STRING_LIST1:
	typename = "list of strings (at least one)";
	if ((vf_sexp_listp(v) && (vf_sexp_length(v) >= 1))){
	  f = 1;
	  for (vvv = v; vf_sexp_consp(vvv); vvv = vf_sexp_cdr(vvv)){
	    if (!vf_sexp_stringp(vf_sexp_car(vvv))){
	      f = 0;
	      break;
	    }
	  }
	  if (f == 1)
	    cap_val = vf_sexp_copy(v);
	}
	break;
      default:
	fprintf(stderr, 
		"VFlib internal error: cannot happen in %s\n",
		"parse_entry()");
	abort();
	break;
      }
      vf_sexp_free(&v);
      if (cap_val != NULL){ 
	if ((strcmp(VF_CAPE_VFLIB_DEFAULTS, name) == 0)
	    && (strcmp(ct[i].cap, VF_CAPE_VARIABLE_VALUES) == 0)){
	  (void) vf_params_default(vf_sexp_copy(cap_val));
	}
	if (ct[i].val != NULL)
	  *(ct[i].val) = cap_val;
	else
	  vf_sexp_free(&cap_val);
      } else {   /* type error */
	fprintf(stderr, 
		"VFlib: %s '%s' in vflibcap - %s is expected. (%s %s ...)\n",
		"type mismatch for capability",
		vf_sexp_get_cstring(scap), typename, def_type, name);
	val = VFLIBCAP_PARSED_ERROR;
      }
    }
  }

  /* Check if essential capabilities are defined. */
  for (i = 0; ct[i].cap != NULL; i++){
    if (ct[i].ess != CAPABILITY_ESSENTIAL)
      continue;
    for (sexp = vf_sexp_cddr(entry); 
	 !vf_sexp_null(sexp); 
	 sexp = vf_sexp_cdr(sexp)){
      scap = vf_sexp_caar(sexp);
      if (strcmp(vf_sexp_get_cstring(scap), ct[i].cap) == 0)
	break;
    }
    if (vf_sexp_null(sexp)){
      /* the essential capability is not given in vflibcap entry. */
      fprintf(stderr, "VFlib Error: %s '%s' %s: (%s %s ... )\n",
	      "essential capability", vf_sexp_get_cstring(scap), 
	      "is not given in vflibcap", def_type, name);
      val = VFLIBCAP_PARSED_ERROR;
    }
  }
  
  if (vf_dbg_vfcap == 1){
    for (i = 0; ct[i].cap != NULL; i++){
      if ((ct[i].val != NULL) && (*(ct[i].val) != NULL)){
	printf("** %s: ", ct[i].cap);
	vf_sexp_pp(*(ct[i].val));
      }
    }
  }

  return val;
}


Private SEXP
get_value_if_variable(SEXP val,
		      SEXP_ALIST varlist1, SEXP_ALIST varlist2)
{
  char  *strval, *varname;

  if (vf_sexp_listp(val) && (vf_sexp_length(val) == 1)
      && vf_sexp_stringp(vf_sexp_car(val))
      && (strncmp(vf_sexp_get_cstring(vf_sexp_car(val)),
		  VF_CAPE_VFLIBCAP_VARIABLE_MARK, 
		  strlen(VF_CAPE_VFLIBCAP_VARIABLE_MARK)) == 0) ){
    /* variable reference */
    strval = vf_sexp_get_cstring(vf_sexp_car(val));
    varname = &strval[strlen(VF_CAPE_VFLIBCAP_VARIABLE_MARK)];
    val = get_variable_value(varname, varlist1, varlist2);
    if ((val != NULL) && (vf_dbg_vfcap == 1)){
      printf(">>  vflibcap parameterlization: %s => ", varname);
      vf_sexp_pp(val);
    }
  } else {
    val = vf_sexp_copy(val);
  }

  return val;
}

Private SEXP
get_variable_value(char *var, 
		   SEXP_ALIST varlist1, SEXP_ALIST varlist2)
{
  char  *strval, *envname; 
  SEXP  v, as;

  /* check an environment variable */
  envname 
    = (char*)malloc(strlen(VF_ENV_VFLIBCAP_PARAM_PREFIX) + strlen(var) + 1);
  if (envname != NULL){
    sprintf(envname, "%s%s", VF_ENV_VFLIBCAP_PARAM_PREFIX, var);
    if ((strval = getenv(envname)) != NULL){
      v = vf_sexp_read_from_string_stream(strval);
      if (vf_dbg_vfcap == 1){
	printf(">>  Variable (by env var) '%s' = ", var);
	vf_sexp_pp(v);
      }
      return v;
    }
  }
  vf_free(envname);
  
  /* check parameters given at class default */
  if (varlist1 != NULL){
    if ((as = vf_sexp_assoc(var, varlist1)) != NULL){
      v = vf_sexp_cdr(as);
      if (vf_dbg_vfcap == 1){
	printf(">>  Variable (by default value) '%s' = ", var);
	vf_sexp_pp(v);
      }
      return vf_sexp_copy(v);
    }
  }
  if (varlist2 != NULL){
    if ((as = vf_sexp_assoc(var, varlist2)) != NULL){
      v = vf_sexp_cdr(as);
      if (vf_dbg_vfcap == 1){
	printf(">>  Variable (by default value) '%s' = ", var);
	vf_sexp_pp(v);
      }
      return vf_sexp_copy(v);
    }
  }

  /* check parameters given at VF_Init() or at VFlib default in vflibcap */
  if ((v = vf_params_lookup(var)) != NULL){
    if (vf_dbg_vfcap == 1){
      printf(">>  Variable (by VF_Init() or VFlib default) '%s' = ", var);
      vf_sexp_pp(v);
    }
    return v;
  }

  fprintf(stderr, "VFlib warning: Undefined variable: '%s'\n", var); 
  return NULL;
}




#ifdef  DEBUG
int vf_error = 0;
int vf_dbg_vfcap = 1; 
int vf_dbg_font_search = 0; 
int vf_dbg_parameters = 0; 
int vf_ccv_autoload = 0; 

int
main(argc, argv)
     int argc;
     char **argv;
{
#if 0
  int    i;
  SEXP   entry, iter;
  SEXP   val;

  if (argc < 3){
    printf("Usage: ./a.out VFLIBCAP ENTRY [CAP1 CAP2 ...]\n");
    exit(1);
  }
  VF_CAP_Init(argv[1], NULL);
  if ((entry = VF_CAP_GetFontEntry(argv[2])) != NULL){
    vf_sexp_pp(entry);
  } else {
    printf("not found\n");
  }
  for (i = 3; i < argc; i++){
      printf("\"%s\": \n", argv[i]);
    for (iter = VF_CAP_PropValueHead(entry, argv[i]);
	 iter != NULL;  
	 iter = VF_CAP_PropValueNext(iter)){
      val = VF_CAP_PropValueGet(iter);
      printf("    ");
      vf_sexp_pp(val);
    }
    vf_sexp_free(&entry);
  }

#else
  int    i;
  SEXP   fontclass, fontdirs, extensions, pixel, point;
  struct s_capability_table  ct[] = {
    {"font-class",  CAPABILITY_STRING, CAPABILITY_ESSENTIAL, &fontclass}, 
    {"directories", CAPABILITY_LIST,   CAPABILITY_OPTIONAL,  &fontdirs}, 
    {"extensions",  CAPABILITY_LIST,   CAPABILITY_OPTIONAL,  &extensions}, 
    {"pixel-size",  CAPABILITY_STRING, CAPABILITY_OPTIONAL,  &pixel}, 
    {"point-size",  CAPABILITY_STRING, CAPABILITY_OPTIONAL,  &point}, 
    {NULL, 0, 0, NULL}
  };

  if (argc < 3){
    printf("Usage: ./a.out VFLIBCAP ENTRY\n");
    exit(1);
  }
  VF_CAP_Init(argv[1], NULL);
  if (VF_CAP_GetParsedFontEntry(argv[2], ct) < 0){
    printf("not found\n");
    exit(0);
  }
  for (i = 0; ct[i].cap != NULL; i++){
    if (*(ct[i].val) != NULL){
      printf("\"%s\": \n", ct[i].cap);
      printf("    ");
      vf_sexp_pp(*(ct[i].val));
    }
    vf_sexp_free(ct[i].val);
  }
#endif

  return 0;
}

char*
VF_CAP_GetEntry(char *entry)
{
  return NULL;
}
char*
VF_CAP_GetString(char *entry)
{
  return NULL;
}
#endif

/*EOF*/

