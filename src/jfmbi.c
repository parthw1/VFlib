/*
 * jfmbi.c - built-in JFM
 *
 *  9 Dec 1999  Added built-in JFM feature.
 */
/*
 * Copyright (C) 1999-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */

#include  "config.h"
#include  "with.h"

#include  <stdio.h>
#include  <stdlib.h>
#include  <ctype.h>
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif
#include  "VFlib-3_7.h"
#include  "VFsys.h"
#include  "metric.h"
#include  "jfmbi.h"



struct s_jfm_builtin_chartype_info {
  int  char_code;
  int  char_type;
};

static struct s_jfm_builtin_chartype_info 
jfm_builtin_chartype_info_table_h[] = {
  /* obtained by running mkbijfm.scm */
  {0x2122,  9},
  {0x2123,  9},
  {0x2124,  8},
  {0x2125,  8},
  {0x2126,  4},
  {0x2127,  4},
  {0x2128,  4},
  {0x2129,  5},
  {0x212a,  4},
  {0x212b,  2},
  {0x212c,  2},
  {0x212d,  4},
  {0x212e,  4},
  {0x212f,  5},
  {0x2130,  5},
  {0x2133,  5},
  {0x2135,  5},
  {0x2136,  3},
  {0x2137,  3},
  {0x2139,  3},
  {0x213e,  4},
  {0x2142,  4},
  {0x2143,  4},
  {0x2146,  4},
  {0x2147,  4},
  {0x2148,  5},
  {0x2149,  5},
  {0x214a,  6},
  {0x214b,  1},
  {0x214c,  6},
  {0x214d,  1},
  {0x214e,  6},
  {0x214f,  1},
  {0x2150,  6},
  {0x2151,  1},
  {0x2152,  6},
  {0x2153,  1},
  {0x2154,  6},
  {0x2155,  1},
  {0x2156,  6},
  {0x2157,  1},
  {0x2158,  6},
  {0x2159,  1},
  {0x215a,  6},
  {0x215b,  1},
  {0x2168,  3},
  {0x2169,  3},
  {0x216a,  3},
  {0x216b,  5},
  {0x216c,  5},
  {0x216d,  5},
  {0x2170,  3},
  {0x2171,  3},
  {0x2172,  3},
  {0x2178,  5},
  {0x2421,  3},
  {0x2423,  3},
  {0x2425,  3},
  {0x2426,  7},
  {0x2427,  3},
  {0x2429,  3},
  {0x242f,  7},
  {0x2430,  7},
  {0x2431,  7},
  {0x2432,  7},
  {0x2439,  7},
  {0x243a,  7},
  {0x2443,  3},
  {0x2463,  3},
  {0x2465,  3},
  {0x2467,  3},
  {0x246a,  7},
  {0x246e,  3},
  {0x2521,  3},
  {0x2522, 10},
  {0x2523,  3},
  {0x2524, 10},
  {0x2525,  3},
  {0x2526, 10},
  {0x2527,  3},
  {0x2529,  3},
  {0x252a, 10},
  {0x252f, 11},
  {0x2530, 11},
  {0x2531, 10},
  {0x2532, 10},
  {0x2535, 10},
  {0x2536, 10},
  {0x253d, 10},
  {0x253e, 10},
  {0x253f, 11},
  {0x2540, 11},
  {0x2541, 10},
  {0x2542, 10},
  {0x2543,  3},
  {0x2544, 10},
  {0x2545, 10},
  {0x2546,  7},
  {0x2547,  7},
  {0x2548, 10},
  {0x2549, 10},
  {0x254a, 10},
  {0x254e, 12},
  {0x2555, 10},
  {0x2556, 10},
  {0x2557, 10},
  {0x255f, 10},
  {0x2561, 10},
  {0x2563,  3},
  {0x2564,  7},
  {0x2565,  3},
  {0x2567,  3},
  {0x2569, 10},
  {0x256a, 10},
  {0x256e,  3},
  {0x256f, 10},
  {0x2572, 10},
  {0x2575,  3},
  {0x2576,  3},
  {0x2577,  3},
};

static struct s_jfm_builtin_chartype_info 
jfm_builtin_chartype_info_table_v[] = {
  /* obtained by running mkbijfm.scm */
  {0x2122,  2},
  {0x2123,  2},
  {0x2124,  1},
  {0x2125,  1},
  {0x2126,  7},
  {0x2129,  4},
  {0x212a,  4},
  {0x2133,  3},
  {0x2134,  3},
  {0x2135,  3},
  {0x2136,  3},
  {0x2137,  3},
  {0x2139,  3},
  {0x213d,  5},
  {0x213e,  7},
  {0x2142,  7},
  {0x2143,  7},
  {0x2144,  5},
  {0x2145,  5},
  {0x2146,  6},
  {0x2147,  8},
  {0x2148,  6},
  {0x2149,  8},
  {0x214a,  6},
  {0x214b,  8},
  {0x214c,  6},
  {0x214d,  8},
  {0x214e,  6},
  {0x214f,  8},
  {0x2150,  6},
  {0x2151,  8},
  {0x2152,  6},
  {0x2153,  8},
  {0x2154,  6},
  {0x2155,  8},
  {0x2156,  6},
  {0x2157,  8},
  {0x2158,  6},
  {0x2159,  8},
  {0x215a,  6},
  {0x215b,  8},
  {0x2421,  3},
  {0x2423,  3},
  {0x2425,  3},
  {0x2427,  3},
  {0x2429,  3},
  {0x2443,  3},
  {0x2463,  3},
  {0x2465,  3},
  {0x2467,  3},
  {0x246e,  3},
  {0x2521,  3},
  {0x2523,  3},
  {0x2525,  3},
  {0x2527,  3},
  {0x2529,  3},
  {0x2543,  3},
  {0x2563,  3},
  {0x2565,  3},
  {0x2567,  3},
  {0x256e,  3},
  {0x2575,  3},
  {0x2576,  3},
};

Glocal int
vf_tfm_builtin_jfm_chartype(long code_point, int dir_h)
{
  int   n, nh, nv, hi, lo, m;
  struct s_jfm_builtin_chartype_info  *tbl;

  nh = sizeof(jfm_builtin_chartype_info_table_h)
       / sizeof(struct s_jfm_builtin_chartype_info);
  nv = sizeof(jfm_builtin_chartype_info_table_v)
       / sizeof(struct s_jfm_builtin_chartype_info);

  if (dir_h == 1){
    n = nh;
    tbl = jfm_builtin_chartype_info_table_h;
  } else {
    n = nv;
    tbl = jfm_builtin_chartype_info_table_v;
  }
    
  if ((code_point < tbl[0].char_code) || (tbl[n-1].char_code < code_point))
    return  0;

  lo = 0;
  hi = n - 1;
  while (lo < hi){
    m = (lo + hi) / 2;
    if (tbl[m].char_code == code_point)
      return  tbl[m].char_type;
    if (code_point < tbl[m].char_code)
      hi = m-1;
    else 
      lo = m+1;
  }
  
  return 0;
}


struct s_jfm_builtin_metrics_info {
  int  char_type;
  double   wd, ht, dp;
};

static struct s_jfm_builtin_metrics_info  
jfm_builtin_metrics_info_table_h[] = {
  /* obtained by running mkbijfm2.scm */
  { 0, 0.962216, 0.777588, 0.138855},
  { 1, 0.504013, 0.777588, 0.138855},
  { 2, 0.353665, 0.777588, 0.138855},
  { 3, 0.747434, 0.777588, 0.138855},
  { 4, 0.353665, 0.777588, 0.138855},
  { 5, 0.504013, 0.777588, 0.138855},
  { 6, 0.504013, 0.777588, 0.138855},
  { 7, 0.962216, 0.777588, 0.138855},
  { 8, 0.353665, 0.777588, 0.138855},
  { 9, 0.504013, 0.777588, 0.138855},
  {10, 0.962216, 0.777588, 0.138855},
  {11, 0.962216, 0.777588, 0.138855},
  {12, 0.962216, 0.777588, 0.138855}
};

static struct s_jfm_builtin_metrics_info  
jfm_builtin_metrics_info_table_v[] = {
  /* obtained by running mkbijfm2.scm */
  { 0, 0.962216, 0.458221, 0.458221},
  { 1, 0.481108, 0.458221, 0.458221},
  { 2, 0.481108, 0.458221, 0.458221},
  { 3, 0.747434, 0.458221, 0.458221},
  { 4, 0.962216, 0.458221, 0.458221},
  { 5, 0.962216, 0.458221, 0.458221},
  { 6, 0.481108, 0.458221, 0.458221},
  { 7, 0.481108, 0.458221, 0.458221},
  { 8, 0.481108, 0.458221, 0.458221},
};


Glocal VF_METRIC1
vf_tfm_builtin_jfm_metric(long code_point, VF_METRIC1 metric, 
			  int  dir_h, double design_size)
{
  int      ct;
  double   w, h, d;
  struct s_jfm_builtin_metrics_info  *tbl;

  if ((ct = vf_tfm_builtin_jfm_chartype(code_point, dir_h)) < 0)
    return NULL;

  if (dir_h == 1){
    tbl = jfm_builtin_metrics_info_table_h;
  } else {
    tbl = jfm_builtin_metrics_info_table_v;
  }
  
  if (metric == NULL){
    if ((metric = vf_alloc_metric1()) == NULL)
      return NULL;
  }

  w = design_size * tbl[ct].wd;
  h = design_size * tbl[ct].ht;
  d = design_size * tbl[ct].dp;

  if (dir_h == 1){
    metric->bbx_width  = w;
    metric->bbx_height = h + d;
    metric->off_x = 0;
    metric->off_y = h;
    metric->mv_x = w;
    metric->mv_y = 0;
  } else {
    metric->bbx_width  = h + d;
    metric->bbx_height = w;
    metric->off_x = -d;
    metric->off_y = 0;
    metric->mv_x = 0;
    metric->mv_y = -w;
  }

  return  metric;
}

/*EOF*/
