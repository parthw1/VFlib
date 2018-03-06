/*
 * cache.c - generic cache module 
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

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif
#include "VFlib-3_7.h"
#include "VFsys.h"
#include "cache.h"


/**
 ** CACHE (lru+hash)
 **/

Private void           *c_get_elem(VF_CACHE,void*,int);
Private void           c_del_elem(VF_CACHE,void*,int);
Private void           lru_move_top(VF_CACHE,VF_CACHE_ELEM);
Private void           lru_put_top(VF_CACHE,VF_CACHE_ELEM);
Private VF_CACHE_ELEM  lru_delete_tail(VF_CACHE);
Private void           lru_unlink_elem(VF_CACHE,VF_CACHE_ELEM);
Private int            c_hash(VF_CACHE,void*,int);
Private VF_CACHE_ELEM  c_hash_is_interned(VF_CACHE,void*,int);
Private void           c_hash_intern(VF_CACHE,VF_CACHE_ELEM,void*,int);
Private void           c_hash_unintern(VF_CACHE,VF_CACHE_ELEM);

/* vf_cache_create()
 *   --- Creates a cache object. 
 */
Glocal VF_CACHE
vf_cache_create(int cache_size, int hash_size, 
		void* (*load_func)(VF_CACHE,void*,int),
		void (*unload_func)(void*))
{
  int            i;
  VF_CACHE       cache;
  VF_CACHE_ELEM  celem;

  if (hash_size < 1)
    return NULL;
  ALLOC_IF_ERR(cache, struct s_vf_cache)
    return NULL;

  celem = NULL;
  if (cache_size < 0){
    cache->free_list = NULL;
  } else {
    ALLOCN_IF_ERR(celem, struct s_vf_cache_elem, cache_size){
      vf_free(cache);
      return NULL;
    }
    for (i = 0; i < cache_size; i++)
      celem[i].h_forw = &celem[i+1];
    celem[cache_size-1].h_forw = NULL;
    cache->free_list = &celem[0];
  }

  ALLOCN_IF_ERR(cache->hash_table, struct s_vf_cache_elem, hash_size){
    if (celem != NULL)
      vf_free(celem);
    vf_free(cache);
    return NULL;
  }

  cache->cache_size  = cache_size;
  cache->hash_size   = hash_size;
  cache->get         = c_get_elem;
  cache->del         = c_del_elem;
  cache->load_elem   = load_func;
  cache->unload_elem = unload_func;
  cache->lru_list.l_forw = &cache->lru_list; 
  cache->lru_list.l_back = &cache->lru_list;
  for (i = 0; i < hash_size; i++){
    cache->hash_table[i].h_forw = &cache->hash_table[i];
    cache->hash_table[i].h_back = &cache->hash_table[i];
  }
  return cache;
}

/* c_get_elem() 
 *   --- returns a elem. If not chached, reload it.
 */
Private void*
c_get_elem(VF_CACHE cache, void *key, int key_len)
{
  VF_CACHE_ELEM   ce;
  void            *key2;

  if ((ce = c_hash_is_interned(cache, key, key_len)) != NULL){
    lru_move_top(cache, ce);
    return (ce->object);
  }

  if ((ce = cache->free_list) == NULL){
    if (cache->cache_size > 0){
      if ((ce = lru_delete_tail(cache)) == NULL){
	fprintf(stderr, "Internal error in GET of CACHE object\n");
	abort();
      }
      c_hash_unintern(cache, ce);
      ce->h_forw = cache->free_list;
      cache->free_list = ce;
      if (cache->unload_elem != NULL) {
	(cache->unload_elem)(ce->object);
      } else if (ce->object != NULL) {
	vf_free(ce->object);
      }
      ce->object = NULL;
      if (ce->key != NULL)
	vf_free(ce->key);
    } else {
      ALLOC_IF_ERR(ce, struct s_vf_cache_elem)
	return NULL;
      ce->h_forw = NULL;
    }
  }
  cache->free_list = ce->h_forw;
  if ((key2 = malloc(key_len)) == NULL)
    return NULL;
  memcpy(key2, key, key_len);
  ce->object = (cache->load_elem)(cache, key, key_len);
  ce->key     = key2;
  ce->key_len = key_len;
  c_hash_intern(cache, ce, key2, key_len);
  lru_put_top(cache, ce);
  return (ce->object);
}

/* c_del_elem() 
 *   --- delete an elem.
 */
Private void
c_del_elem(VF_CACHE cache, void *key, int key_len)
{
  VF_CACHE_ELEM   ce;

  if ((ce = c_hash_is_interned(cache, key, key_len)) == NULL)
    return;
  c_hash_unintern(cache, ce);
  lru_unlink_elem(cache, ce);
  if (cache->unload_elem != NULL)
    (cache->unload_elem)(ce->object);
  else if (ce->object != NULL)
    vf_free(ce->object);
  ce->object = NULL;

  if (cache->cache_size > 0){
    ce->h_forw = cache->free_list;
    cache->free_list = ce;
  } else {
    vf_free(ce);
  }
}

Private void
lru_unlink_elem(VF_CACHE cache, VF_CACHE_ELEM ce) 
{
  VF_CACHE_ELEM  ce_b, ce_f;

  ce_b = ce->l_back;
  ce_f = ce->l_forw;
  ce_b->l_forw = ce_f;
  ce_f->l_back = ce_b;
}

/* lru_put_top()
 *   --- puts an ELEM at the head of LRU list. 
 *       The ELEM must not be in LRU list.
 */
Private void 
lru_put_top(VF_CACHE cache, VF_CACHE_ELEM ce)
{
  VF_CACHE_ELEM  ce_f;

  ce_f           = cache->lru_list.l_forw;
  ce->l_forw     = ce_f;
  ce_f->l_back   = ce;
  ce->l_back             = &cache->lru_list;
  cache->lru_list.l_forw = ce;
}

/* lru_move_top() 
 *   --- moves an ELEM at the top of LRU list.
 *       ELEM must be in LRU list.
 */
Private void
lru_move_top(VF_CACHE cache, VF_CACHE_ELEM ce)
{
  lru_unlink_elem(cache, ce);
  lru_put_top(cache, ce);
}

Private VF_CACHE_ELEM
lru_delete_tail(VF_CACHE cache)  /* NOTE: There must be at least one 
				    ELEM in LRU list */
{
  VF_CACHE_ELEM  ce;

  if ((ce = cache->lru_list.l_back) == &cache->lru_list)
    return NULL;
  lru_unlink_elem(cache, ce);
  return ce;
}

Private int
c_hash(VF_CACHE cache, void *key, int key_len)
{
  char          *p;
  int           i;
  unsigned int  h;

  h = 0;
  for (i = 0, p = key; i < key_len; i++, p++)
    h = (h + (unsigned int)*p) % cache->hash_size;
  return  h;
} 

Private VF_CACHE_ELEM
c_hash_is_interned(VF_CACHE cache, void *key, int key_len)
{
  int            h;
  VF_CACHE_ELEM  ce, ce0;

  h = c_hash(cache, key, key_len);
  ce0 = &cache->hash_table[h]; 
  for (ce = ce0->h_forw; ce != ce0; ce = ce->h_forw){
    if ((ce->key_len == key_len) && (memcmp(ce->key, key, key_len) == 0)){
      if (ce != ce0->h_forw){
	c_hash_unintern(cache, ce);
	c_hash_intern(cache, ce, NULL, h);  /* MAGIC */
      }
      return ce;
    }
  }
  return NULL;
}

Private void
c_hash_intern (VF_CACHE cache, VF_CACHE_ELEM ce, void *key, int key_len)
{
  int         h;
  VF_CACHE_ELEM  ce1;

  if (key == NULL)  /* MAGIC */
    h = key_len;  
  else
    h = c_hash(cache, key, key_len);
  ce1                         = cache->hash_table[h].h_forw;
  cache->hash_table[h].h_forw = ce;
  ce->h_forw                  = ce1;
  ce1->h_back                 = ce;
  ce->h_back                  = &cache->hash_table[h];
}

Private void
c_hash_unintern(VF_CACHE cache, VF_CACHE_ELEM ce)
{
  VF_CACHE_ELEM  ce_b, ce_f;

  ce_b         = ce->h_back;
  ce_f         = ce->h_forw;
  ce_b->h_forw = ce_f;
  ce_f->h_back = ce_b;
}



/**
 ** HASH TABLE
 **
 ** 1. A hash table object is created by the following function:
 **  FUNC: vf_hash_create(int hash_size)
 **   --- Caller must specify the size of hash table.  This hash object 
 **       uses `chaining' to store data objects: the hash table can store
 **       any number of data objects (more than hash_size).
 ** 2. A data object is stored in hash table by the PUT method.
 **  FUNC: (HASH_OBJ->put)(HASH_OBJ, DATA_OBJ, KEY, KEY_LENGTH)
 **   --- A data object, DATA_OBJ, is stored with specifying its
 **       KEY and KEY_LENGTH, the length (in byte) of the KEY.
 **       This method does not return any value. If the same data object
 **       in the sense of KEY and KEY_LENGTH exists in the HASH_OBJ,
 **       the object is not newly interned and link count is increased.
 ** 3. Stored data object is extracted by the GET method.
 **  FUNC: (TABLE_OBJ->get)(HASH, KEY, KEY_ID)
 **   --- This extracts a data object whose key and key length matches 
 **       KEY and KEY_LENGTH.  If NULL is returned, it implies that 
 **       such data is not interned.
 ** 4. Stored data object can be deleted from the hash table by DEL method.
 **  FUNC: (TABLE_OBJ->del)(HASH_OBJ, KEY, KEY_ID)
 **   --- This delets a data object whose key and key length matches 
 **       KEY and KEY_LENGTH.  If its link count is more than one,
 **       the link count is decremented by one and the object is not
 **       deleted.
 **/
Private void*    h_hash_put_object(VF_HASH,void*,void*,int);
Private void*    h_hash_get_object(VF_HASH,void*,int);
Private void     h_hash_del_object(VF_HASH,void*,int);
Private int        h_hash(VF_HASH,void*,int);
Private void       h_hash_intern(VF_HASH,VF_HASH_ELEM,void*,int);
Private void       h_hash_unintern(VF_HASH,VF_HASH_ELEM);

/* vf_hash_create()
 *   --- Creates a hash table object. 
 */
Glocal VF_HASH
vf_hash_create(int hash_size)
{
  int           i;
  VF_HASH       hash;

  if ((hash_size < 1)
      || ((hash = (VF_HASH)calloc(1, sizeof(struct s_vf_hash))) == NULL))
    return NULL;
  hash->table = (VF_HASH_ELEM)calloc(hash_size, sizeof(struct s_vf_hash_elem));
  if (hash->table == NULL){
    vf_free(hash);
    return NULL;
  }

  hash->hash_size = hash_size;
  hash->put       = h_hash_put_object;
  hash->get       = h_hash_get_object;
  hash->del       = h_hash_del_object;
  for (i = 0; i < hash_size; i++){
    hash->table[i].h_forw = &hash->table[i];
    hash->table[i].h_back = &hash->table[i];
  }
  return hash;
}

Private void*
h_hash_get_object(VF_HASH hash, void *key, int key_len)
{
  int           h;
  VF_HASH_ELEM  he, he0;

  h = h_hash(hash, key, key_len);
  he0 = &hash->table[h]; 
  for (he = he0->h_forw; he != he0; he = he->h_forw)
    if ((he->key_len == key_len) && (memcmp(he->key, key, key_len) == 0)){
      if (he != he0->h_forw){  /* move top if it is not top */
	h_hash_unintern(hash, he);
	h_hash_intern(hash, he, NULL, h);  /* MAGIC */
      }
      return he->object;
    }
  return NULL;
}

Private void*
h_hash_put_object(VF_HASH hash, void* object, void *key, int key_len)
{
  VF_HASH_ELEM   he;
  void           *key2;

  if ((he = h_hash_get_object(hash, key, key_len)) != NULL){
    ++he->link_cnt;
    return he->object;
  }

  he = (VF_HASH_ELEM)calloc(1, sizeof(struct s_vf_hash_elem));
  key2 = calloc(1, key_len);
  if ((he == NULL) || (key2 == NULL))
    return NULL;
  memcpy(key2, key, key_len);
  he->link_cnt = 1;
  he->object   = object;
  he->key      = key2;
  he->key_len  = key_len;
  h_hash_intern(hash, he, key2, key_len);
  return he->object;
}

Private void
h_hash_del_object(VF_HASH hash, void *key, int key_len)
{
  VF_HASH_ELEM  he;

  if ((he = h_hash_get_object(hash, key, key_len)) == NULL)
    return;  /* not interned */

  if (--he->link_cnt > 0)
    return;

  h_hash_unintern(hash, he);
  if (he->key != NULL)
    vf_free(he->key);
  vf_free(he);
}

Private void
h_hash_intern (VF_HASH hash, VF_HASH_ELEM he, void *key, int key_len)
{
  int          h;
  VF_HASH_ELEM he1;

  if (key == NULL)  /* MAGIC */
    h = key_len;  
  else
    h = h_hash(hash, key, key_len);
  he1                   = hash->table[h].h_forw;
  hash->table[h].h_forw = he;
  he->h_forw            = he1;
  he1->h_back           = he;
  he->h_back            = &hash->table[h];
}

Private void
h_hash_unintern(VF_HASH hash, VF_HASH_ELEM he)
{
  VF_HASH_ELEM  he_b, he_f;

  he_b         = he->h_back;
  he_f         = he->h_forw;
  he_b->h_forw = he_f;
  he_f->h_back = he_b;
}

Private int
h_hash(VF_HASH hash, void *key, int key_len)
{
  char          *p;
  int           i;
  unsigned int  h;

  h = 0;
  for (i = 0, p = key; i < key_len; i++, p++)
    h = h + (unsigned int)*p;
  return (h % hash->hash_size);
} 


/**
 ** TABLE
 **
 ** 1. A table object is created by the following function:
 **  FUNC: vf_table_create(void)
 **   --- Table size is need not be specified.  It autoatically 
 **       and dynammically allocates memory for table memory.  
 ** 2. A table object stores data object.  
 **  FUNC: (TABLE_OBJ->put_obj)(TABLE_OBJ, DATA_OBJ, KEY, KEY_LENGTH)
 **   --- A data object, DATA_OBJ, is stored with specifying its
 **       KEY and KEY_LENGTH, the length (in byte) of the KEY.
 **       This method returns an ID (a non-negative integer) for the 
 **       DATA_OBJ.  If -1 is returned, some error occured internnaly.
 **       ID is used to extract DATA_OBJ.  If the same data object
 **       in the sense of KEY and KEY_LENGTH exists in the TABLE,
 **       the object is not newly interned and link count is increased.
 ** 3. Stored data object is extracted by two ways: by ID and by KEY.
 ** 3.1 Data extraction by ID.
 **  FUNC: (TABLE_OBJ->get_obj_by_id)(TABLE_OBJ, ID)
 **   --- Extract a data object whose id is ID.  If NULL is returned,
 **       ID is wrong (i.e., such data is not interned).
 ** 3.2 Data extraction by KEY.
 **  FUNC: (TABLE_OBJ->get_obj_by_key)(TABLE_OBJ, KEY, KEY_LENGTH)
 **   --- Extract a data object whose key and key length are KEY 
 **       and KEY_LENGTH.  If NULL is returned, such data is not interned.
 ** 4. Stored data object can be deleted from the table by two ways:
 **   by ID and by KEY.
 ** 4.1 Data deletion by ID.
 **  FUNC: (TABLE_OBJ->del_obj_by_id)(TABLE_OBJ, ID)
 **   --- Delete a data object from the TABLE whose id is ID. Precisely,
 **       it decreases the link count of the data item.  If it is
 **       zero, the data object is deleted from the TABLE.
 ** 4.2 Data deletion by KEY.
 **  FUNC: (TABLE_OBJ->del_obj_by_key)(TABLE_OBJ, KEY, KEY_LENGTH)
 **   --- Delete a data object whose key and key length are KEY 
 **       and KEY_LENGTH.  Precisely, it decreases the link count 
 **       of the data item.  If it is
 **       zero, the data object is deleted from the TABLE.
 ** 5. Obtaining data ID 
 ** 5.1 Obtain data ID by KEY and KEY_LENGTH.
 **  FUNC: (TABLE_OBJ->get_id_by_key)(TABLE_OBJ, KEY, KEY_LENGTH)
 **   --- Return an ID whose key and key length are KEY and KEY_LENGTH.
 ** 5.2 Obtain data ID by DATA
 **  FUNC: (TABLE_OBJ->get_id_by_obj)(TABLE_OBJ, DATA)
 **   --- Return an ID for the DATA.
 ** 6. Incrementing link count.
 **  FUNC: (TABLE_OBJ->link_by_id)(TABLE_OBJ, ID)
 **   --- Increment link count of an entry ID.
 ** 7. Decrementing link count.
 **  FUNC: (TABLE_OBJ->unlink_by_id)(TABLE_OBJ, ID)
 **   --- Decrement link count of an entry ID.  If link count becomes zero,
 **       the entry is deleted from the table. 
 ** 8. The number of elements in the TABLE object can be checked.
 **  FUNC: (TABLE_OBJ->get_nelements)(TABLE_OBJ)
 **   --- Return the number of elements in the TABLE.
 **
 **/
Private int  table_put_obj(VF_TABLE,void*,void*,int);
Private int  table_put_obj2(VF_TABLE,void*,void*,int);
Private int  table_get_id_by_key(VF_TABLE,void*,int);
Private int  table_get_id_by_obj(VF_TABLE,void*);
Private void *table_get_obj_by_id(VF_TABLE,int);
Private void *table_get_obj_by_key(VF_TABLE,void*,int);
Private int  table_del_obj_by_id(VF_TABLE,int);
Private int  table_del_obj_by_key(VF_TABLE,void*,int);
Private int  table_link_by_id(VF_TABLE,int);
Private int  table_unlink_by_id(VF_TABLE,int);
Private int  table_get_nelements(VF_TABLE);
#ifndef VF_INIT_TABLE_SIZE  
#  define  VF_INIT_TABLE_SIZE   16
#endif/*VF_INIT_TABLE_SIZE*/

/* vf_table_create()
 *   --- Creates a table object. 
 */
Glocal VF_TABLE
vf_table_create(void)
{
  VF_TABLE   table;

  if (VF_INIT_TABLE_SIZE < 1){
    fprintf(stderr, "Internal error: Initial # of elems for TABLE\n");
    abort();
  }
  ALLOC_IF_ERR(table, struct s_vf_table)
    return NULL;
  table->put             = table_put_obj;
  table->put2            = table_put_obj2;
  table->get_id_by_key   = table_get_id_by_key;
  table->get_id_by_obj   = table_get_id_by_obj;
  table->get_obj_by_id   = table_get_obj_by_id;
  table->get_obj_by_key  = table_get_obj_by_key;
  table->del_obj_by_id   = table_del_obj_by_id;
  table->del_obj_by_key  = table_del_obj_by_key;
  table->link_by_id      = table_link_by_id;
  table->unlink_by_id    = table_unlink_by_id;
  table->get_nelements   = table_get_nelements;
  table->nelems     = 0;
  table->next_slot  = 0;
  table->table_size = 0;
  table->table      = NULL;
  return table;
}

Private int
table_put_obj(VF_TABLE table, void *object, void *key, int key_len)
{
  int             id;
  VF_TABLE_ELEM   te;

  if ((id = table_get_id_by_key(table, key, key_len)) >= 0){
    te = &table->table[id]; 
    ++te->link_cnt;
    return id;
  }

  return table_put_obj2(table, object, key, key_len);
}

Private int
table_put_obj2(VF_TABLE table, void *object, void *key, int key_len)
{
  int             id, idz, new_table_size, i;
  VF_TABLE_ELEM   new_table;
  void            *key2;

  if (table->nelems == table->table_size){   /* realloc */
    if (table->table_size == 0)
      new_table_size = VF_INIT_TABLE_SIZE;
    else 
      new_table_size = 2 * table->table_size;
    ALLOCN_IF_ERR(new_table, struct s_vf_table_elem, new_table_size){
      return -1;
    }
    for (i = 0; i < table->table_size; i++){
      new_table[i].link_cnt = table->table[i].link_cnt;
      new_table[i].object   = table->table[i].object;
      new_table[i].key      = table->table[i].key;
      new_table[i].key_len  = table->table[i].key_len;
    }
    for (i = table->table_size; i < new_table_size; i++){
      new_table[i].link_cnt = 0;
      new_table[i].object   = NULL;
      new_table[i].key      = NULL;
      new_table[i].key_len  = 0;
    }
    table->next_slot  = table->table_size;     /* possibly, free slot */
    table->table_size = new_table_size;
    if (table->table != NULL)
      vf_free(table->table);
    table->table      = new_table;
  }

  id = idz = table->next_slot;
  do {
    if ((table->table[id].object == NULL) && (table->table[id].key == NULL)
	&& (table->table[id].key_len == 0)){
      if ((key2 = malloc(key_len)) == NULL)
	return -1;
      memcpy(key2, key, key_len);
      table->table[id].link_cnt = 1;
      table->table[id].object   = object;
      table->table[id].key      = key2;
      table->table[id].key_len  = key_len;
      table->next_slot = (id + 1) % table->table_size;
      ++table->nelems;
      return id;
    }
    id = (id + 1) % table->table_size;
  } while (id != idz);

  fprintf(stderr, "Cannot happen in table_put_obj()\n");
  abort();
  return -1;
}

Private int
table_get_id_by_key(VF_TABLE table, void *key, int key_len)
{
  int            id;
  VF_TABLE_ELEM  te;

  for (id = 0; id < table->table_size; id++){
    te = &table->table[id]; 
    if ((te->object == NULL) && (te->key == NULL) && (te->key_len == 0))
      continue;
    if ((te->key_len == key_len) && (memcmp(te->key, key, key_len) == 0))
      return id;
  }
  return -1;
}

Private int
table_get_id_by_obj(VF_TABLE table, void *obj)
{
  int            id;
  VF_TABLE_ELEM  te;

  if (obj == NULL)
    return -1;
  for (id = 0; id < table->table_size; id++){
    te = &table->table[id]; 
    if (te->object == obj)
      return id;
  }
  return -1;
}

Private void*
table_get_obj_by_id(VF_TABLE table, int id)
{
  if ((id < 0) && (table->table_size <= id))
    return NULL;
  if (table->table == NULL)
    return NULL;
  return table->table[id].object;
}

Private void*
table_get_obj_by_key(VF_TABLE table, void *key, int key_len)
{
  int  id;

  if ((id = table_get_id_by_key(table, key, key_len)) < 0)
    return NULL;
  return table->table[id].object;
}

Private int
table_del_obj_by_id(VF_TABLE table, int id)
{
  --table->table[id].link_cnt;
  if (table->table[id].link_cnt > 0)
    return table->table[id].link_cnt;

  --table->nelems;
  if (table->table[id].key != NULL)
    vf_free(table->table[id].key);
  table->table[id].object   = NULL;
  table->table[id].key      = NULL;
  table->table[id].key_len  = 0;
  table->table[id].link_cnt = 0;

  return table->table[id].link_cnt;
}

Private int
table_del_obj_by_key(VF_TABLE table, void *key, int key_len)
{
  int            id;

  if ((id = table_get_id_by_key(table, key, key_len)) < 0)
    return -1;
  return table_del_obj_by_id(table, id);
}

Private int
table_link_by_id(VF_TABLE table, int id)
{
  table->table[id].link_cnt++;
  return table->table[id].link_cnt;
}

Private int
table_unlink_by_id(VF_TABLE table, int id)
{
  --table->table[id].link_cnt;

  if (table->table[id].link_cnt <= 0){
    --table->nelems;
    if (table->table[id].key != NULL)
      vf_free(table->table[id].key);
    table->table[id].object   = NULL;
    table->table[id].key      = NULL;
    table->table[id].key_len  = 0;
    table->table[id].link_cnt = 0;
    return 0;
  }

  return table->table[id].link_cnt;
}

Private int
table_get_nelements(VF_TABLE table)
{
  return table->nelems;
}

/*EOF*/
