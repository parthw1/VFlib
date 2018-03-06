/*
 * ccv.c - a module for encoding & charset conversion 
 * by Hirotsugu Kakugawa
 *
 *  29 Jul 1997
 */
/*
 * Copyright (C) 1996-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */

#define CCV_STAT_LOADED     0
#define CCV_STAT_AUTOLOAD   1

#define CCV_ARG_TYPE_ARRAY         0
#define CCV_ARG_TYPE_RANDOM_ARRAY  1
#define CCV_ARG_TYPE_FUNC          2


struct s_ccv_random_array {
  int  *block_index;
  long *tbl;
};
typedef struct s_ccv_random_array  *CCV_RANDOM_ARRAY;

struct s_ccv_info {
  char *cs1_name;
  char **cs1_name_aliases;
  char *cs1_enc;
  char **cs1_enc_aliases;
  char *cs2_name;
  char **cs2_name_aliases;
  char *cs2_enc;
  char **cs2_enc_aliases;
  int  block_size;
  int  load_stat;
  long (*conv)(int,long);
  long arg;
  int  arg_type;
  int  c1min, c1max; 
  int  c2min, c2max;
  int  nblocks;
  char *file_name;
  char *file_path;
};
typedef struct s_ccv_info  *CCV_INFO;

Glocal int   vf_ccv_init(void);
Glocal int   vf_ccv_install_func(char *cs1_name, char *cs1_enc, 
				 char *cs2_name, char *cs2_enc,
				 long (*conv)(int,long));
Glocal int   vf_ccv_autoload(char *file_name);
Glocal int   vf_ccv_require(char *cs1_name, char *cs1_enc,
			 char *cs2_name, char *cs2_enc);
Glocal long  vf_ccv_conv(int ccvi_index, long code_point);

/*EOF*/
