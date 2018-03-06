/*
 * bijfmtest.c
 * by Hirotsugu Kakugawa
 */
/*
 * Copyright (C) 1996-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */

#include <stdio.h>
#include <stdlib.h>

struct s_chartype_info {
  int  char_code;
  int  char_type;
};

static struct s_chartype_info  chartype_info_table[] = {
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
  {0x2577,  3}
}; 


int
vf_tfm_builtin_jfm_chartype(long code_point)
{
  int   ct;
  int   n, hi, lo, m;

  n = sizeof(chartype_info_table) / sizeof(struct s_chartype_info);
    
  if ((code_point < chartype_info_table[0].char_code)
      || (chartype_info_table[n-1].char_code < code_point))
    return  0;

  lo = 0;
  hi = n - 1;
  while (lo < hi){
    m = (lo + hi) / 2;
    if (chartype_info_table[m].char_code == code_point)
      return chartype_info_table[m].char_type;
    if (code_point < chartype_info_table[m].char_code)
      hi = m-1;
    else 
      lo = m+1;
  }
  
  return 0;
}


int main(int argc, char **argv)
{
  long   d;
  int    ct;

  sscanf(argv[1], "%li", &d);

  ct = vf_tfm_builtin_jfm_chartype(d);
  printf("*** 0x%lx => %d\n", d, ct);
}
