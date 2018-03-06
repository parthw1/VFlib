/* sexp.h - a header for vfsexp.c
 * by Hirotsugu Kakugawa
 *
 *  Edition History
 *  7 Jan 1998  First implementation
 */
/*
 * Copyright (C) 1996-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */

#ifndef __VFLIB_SEXP_H__
#define __VFLIB_SEXP_H__

typedef struct  s_sexp   *SEXP;

struct s_sexp_cons {
  SEXP   car;
  SEXP   cdr;
};

struct s_sexp {
  int    tag;                  /* data type */
  union {
    struct s_sexp_cons  cons;  /* cons */
    char                *str;  /* string or symbol */
  } t;
};


/* Data type.
 * String and Symbol are the same type except 
 * they are printed different ways by pretty-printer. */
#define VF_SEXP_TAG_NIL        0
#define VF_SEXP_TAG_CONS       1
#define VF_SEXP_TAG_STRING     2
#define VF_SEXP_TAG_SYMBOL     3
#define VF_SEXP_TAG_RELEASED 255

#define SEXP_LIST     SEXP
#define SEXP_ALIST    SEXP
#define SEXP_STRING   SEXP


extern SEXP   vf_sexp_cons(SEXP s1, SEXP s2);
extern SEXP   vf_sexp_car(SEXP s);
extern SEXP   vf_sexp_cdr(SEXP s);
extern SEXP   vf_sexp_caar(SEXP s);
extern SEXP   vf_sexp_cadr(SEXP s);
extern SEXP   vf_sexp_cdar(SEXP s);
extern SEXP   vf_sexp_cddr(SEXP s);
extern SEXP   vf_sexp_caddr(SEXP s);
extern void   vf_sexp_rplaca(SEXP s, SEXP val);
extern void   vf_sexp_rplacd(SEXP s, SEXP val);
extern int    vf_sexp_atom(SEXP s);
extern int    vf_sexp_null(SEXP s);
extern int    vf_sexp_consp(SEXP s);
extern int    vf_sexp_stringp(SEXP s);
extern char*  vf_sexp_get_cstring(SEXP s);
extern int    vf_sexp_listp(SEXP s);
extern int    vf_sexp_alistp(SEXP s);
extern int    vf_sexp_length(SEXP s);
extern int    vf_sexp_member(char *key, SEXP s);
extern SEXP   vf_sexp_assoc(char *key, SEXP s);
extern SEXP   vf_sexp_alist_put(char *key, char *val, SEXP_ALIST alist);
extern SEXP   vf_sexp_copy(SEXP s);
extern SEXP   vf_sexp_list1(SEXP s);
extern SEXP   vf_sexp_list2(SEXP s1, SEXP s2);
extern void   vf_sexp_nconc(SEXP s1, SEXP s2);
extern SEXP   vf_sexp_empty_list(void);
extern SEXP   vf_sexp_getf(SEXP s, char *key); 

extern SEXP   vf_sexp_cstring2string(char *str);
extern SEXP   vf_sexp_cstring2list(char *str);
extern SEXP   vf_sexp_cstring2alist(char *str);

extern SEXP   vf_sexp_read(FILE *fp);
extern SEXP   vf_sexp_read_from_file_stream(FILE *fp);
extern SEXP   vf_sexp_read_from_string_stream(char *str);

extern void   vf_sexp_pp(SEXP s);
extern void   vf_sexp_pp_fp(SEXP s, FILE* fp);
extern void   vf_sexp_pp_entry(SEXP s);
extern void   vf_sexp_pp_entry_fp(SEXP s, FILE* fp);

extern void   vf_sexp_free(SEXP *s);
extern void   vf_sexp_free1(SEXP *s1);
extern void   vf_sexp_free2(SEXP *s1, SEXP *s2);
extern void   vf_sexp_free3(SEXP *s1, SEXP *s2, SEXP *s3);
extern void   vf_sexp_free4(SEXP *s1, SEXP *s2, SEXP *s3, SEXP *s4);

#endif /*__VFLIB_SEXP_H__*/
