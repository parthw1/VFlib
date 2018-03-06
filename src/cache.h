/*
 * cache.h - a header for cache module 
 * by Hirotsugu Kakugawa
 *   5 Aug 1996
 */
/*
 * Copyright (C) 1996-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */

#ifndef __VFLIB_CACHE_H__
#define __VFLIB_CACHE_H__

/* Cache Element */
typedef struct s_vf_cache_elem  *VF_CACHE_ELEM;
struct s_vf_cache_elem {
  void        *object;          /* cached object */
  void        *key;             /* element key */
  int         key_len;          /* key length */
  VF_CACHE_ELEM  l_forw, l_back; /* forw./backw. in LRU list */
  VF_CACHE_ELEM  h_forw, h_back; /* forw./backw. in hash table, free list*/
};
/* Cache */
typedef struct s_vf_cache  *VF_CACHE;
struct s_vf_cache {
  /* Public: common method */
  void    *(*get)(VF_CACHE,void*,int); 
  void    (*del)(VF_CACHE,void*,int); 
  /* Private: class dependent method */
  void    *(*load_elem)(VF_CACHE,void*,int);
  void    (*unload_elem)(void*);
  /* Private: internal data structure */
  int                  cache_size;
  int                  hash_size; 
  VF_CACHE_ELEM           hash_table;
  struct s_vf_cache_elem  lru_list;
  VF_CACHE_ELEM           free_list;
};
extern VF_CACHE vf_cache_create (int,int,
				 void *(*load_func)(VF_CACHE,void*,int),
				 void (*unload_func)(void*));

/** HASH **/
/* Hash Element */
typedef struct s_vf_hash_elem  *VF_HASH_ELEM;
struct s_vf_hash_elem {
  int         link_cnt;
  void        *object;          /* object */
  void        *key;             /* element key */
  int         key_len;          /* key length */
  VF_HASH_ELEM  h_forw, h_back; /* forw./backw. in hash table, free list*/
};
/* Hash */
typedef struct s_vf_hash  *VF_HASH;
struct s_vf_hash {
  /* Public: common method */
  void    *(*get)(VF_HASH,void*,int); 
  void    *(*put)(VF_HASH,void*,void*,int); 
  void    (*del)(VF_HASH,void*,int); 
  /* Private: internal data structure */
  int           hash_size; 
  VF_HASH_ELEM  table;
};
extern VF_HASH vf_hash_create (int);


/** TABLE **/
/* Table Element */
typedef struct s_vf_table_elem  *VF_TABLE_ELEM;
struct s_vf_table_elem {
  int         link_cnt;
  void        *object;     /* object */
  void        *key;        /* element key */
  int         key_len;     /* key length */
};
/* Table */
typedef struct s_vf_table  *VF_TABLE;
struct s_vf_table {
  /* Public: common method */
  int     (*put)(VF_TABLE,void*,void*,int); 
  int     (*put2)(VF_TABLE,void*,void*,int); 
  int     (*get_id_by_key)(VF_TABLE,void*,int); 
  int     (*get_id_by_obj)(VF_TABLE,void*); 
  void   *(*get_obj_by_id)(VF_TABLE,int); 
  void   *(*get_obj_by_key)(VF_TABLE,void*,int); 
  int     (*del_obj_by_id)(VF_TABLE,int); 
  int     (*del_obj_by_key)(VF_TABLE,void*,int); 
  int     (*link_by_id)(VF_TABLE,int); 
  int     (*unlink_by_id)(VF_TABLE,int); 
  int     (*get_nelements)(VF_TABLE); 
  /* Private: internal data */
  int            nelems;
  int            next_slot;
  int            table_size; 
  VF_TABLE_ELEM  table;
};
Glocal VF_TABLE  vf_table_create (void);

#endif  /* __VFLIB_CACHE_H__ */
			       
/*EOF*/
