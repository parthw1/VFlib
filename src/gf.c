/*
 * gf.c - TeX GF format font fonts loader.
 *
 * 28 Sep 1996  First version.
 * 30 Jan 1998  VFlib 3.4  Changed API.
 * 16 Sep 1999  Changed not to use TFM
 */
/*
 * Copyright (C) 1996-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */

Private GF_GLYPH  gf_loader(VF_CACHE,FILE*);
Private int       gf_read_glyph(FILE*,VF_BITMAP);

Private unsigned char   bit_table[] = {
  0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };


Private GF_GLYPH
GF_CacheLoader(VF_CACHE c, char *font_path, int l)
{
  FILE      *fp;
  UINT1      pre, id;
  UINT4      k;

  if (font_path == NULL)
    return NULL;
  if ((fp = vf_fm_OpenBinaryFileStream(font_path)) == NULL)
    return NULL;

  pre = READ_UINT1(fp);
  if (pre != GF_PRE)
    return NULL;
  id = READ_UINT1(fp);
  if (id != GF_ID)
    return NULL;
  k = READ_UINT1(fp);
  SKIP_N(fp, k);

  return gf_loader(c, fp);
}

Private void
GF_CacheDisposer(GF_GLYPH go)
{
  int   nc, i;

  if (go != NULL){
    if (go->bm_table != NULL){
      nc = go->code_max - go->code_min + 1;
      for (i = 0; i < nc; i++)
	vf_free(go->bm_table[i].bitmap);
      vf_free(go->bm_table);
    }
    vf_free(go);
  }
}



Private GF_GLYPH
gf_loader(VF_CACHE c, FILE* fp)
{
  GF_GLYPH    go;
  VF_BITMAP   bm;
  UINT1       instr, d;
  UINT4       ds, check_sum, hppp, vppp;
  INT4        min_m, max_m, min_n, max_n;
  INT4        w;
  UINT4       code;
  double      dx, dy;
  long        ptr_post, ptr_p, ptr, optr, gptr;
  int         bc, ec, nchars, i;

  go = NULL;
  nchars = -1;

  /* seek to post_post instr. */
  fseek(fp, -1, SEEK_END);
  while ((d = READ_UINT1(fp)) == 223)
    fseek(fp, -2, SEEK_CUR);
  if (d != GF_ID){
    vf_error = VF_ERR_ILL_FONT_FILE;
    goto ErrExit;
  }
  fseek(fp, -6, SEEK_CUR);

  /* check if the code is post_post */
  if (READ_UINT1(fp) != GF_POST_POST){
    vf_error = VF_ERR_ILL_FONT_FILE;
    goto ErrExit;
  }

  /* read pointer to post instr. */
  if ((ptr_post = READ_UINT4(fp)) == -1){
    vf_error = VF_ERR_ILL_FONT_FILE;
    goto ErrExit;
  }

  /* goto post instr. and read it */
  fseek(fp, ptr_post, SEEK_SET);
  if (READ_UINT1(fp) != GF_POST){
    vf_error = VF_ERR_ILL_FONT_FILE;
    goto ErrExit;
  }
  ptr_p     = READ_UINT4(fp);
  ds        = READ_UINT4(fp);
  check_sum = READ_UINT4(fp);
  hppp      = READ_UINT4(fp);
  vppp      = READ_UINT4(fp);
  min_m     = READ_INT4(fp);
  max_m     = READ_INT4(fp);
  min_n     = READ_INT4(fp);
  max_n     = READ_INT4(fp);

  gptr = ftell(fp);

#if 0
  /* read min & max char code */
  bc = 256;
  ec = -1;
  for (;;){
    instr = READ_UINT1(fp);
    if (instr == GF_POST_POST){
      break;
    } else if (instr == GF_CHAR_LOC){
      code = READ_UINT1(fp);
      (void)SKIP_N(fp, 16);
    } else if (instr == GF_CHAR_LOC0){
      code = READ_UINT1(fp);
      (void)SKIP_N(fp, 9);
    } else {
      vf_error = VF_ERR_ILL_FONT_FILE;
      goto ErrExit;
    }
    if (code < bc)
      bc = code;
    if (code > ec)
      ec = code;
  }
#else
  bc = 0;
  ec = 255;
#endif

  nchars = ec - bc + 1;
  ALLOC_IF_ERR(go, struct s_gf_glyph){
    vf_error = VF_ERR_NO_MEMORY;
    goto ErrExit;
  }
  ALLOCN_IF_ERR(go->bm_table, struct vf_s_bitmap, nchars){
    vf_error = VF_ERR_NO_MEMORY;
    goto ErrExit;
  }

  for (i = 0; i < nchars; i++)
    go->bm_table[i].bitmap = NULL;
  go->ds   = (double)ds/(1<<20);
  go->hppp = (double)hppp/(1<<16);
  go->vppp = (double)vppp/(1<<16);
  go->font_bbx_w = max_m - min_m;
  go->font_bbx_h = max_n - min_n;
  go->font_bbx_xoff = min_m;
  go->font_bbx_yoff = min_n;
  go->code_min = bc;
  go->code_max = ec;

  /* read glyph */
#if 0
  fseek(fp, gptr, SEEK_SET);
#endif
  for (;;){
    if ((instr = READ_UINT1(fp)) == GF_POST_POST)
      break;
    switch ((int)instr){
    case GF_CHAR_LOC:
      code = READ_UINT1(fp);
      dx   = (double)READ_INT4(fp)/(double)(1<<16);
      dy   = (double)READ_INT4(fp)/(double)(1<<16);
      w    = READ_INT4(fp);
      ptr  = READ_INT4(fp);
      break;
    case GF_CHAR_LOC0:
      code = READ_UINT1(fp);
      dx   = (double)READ_INT1(fp);
      dy   = (double)0;
      w    = READ_INT4(fp);
      ptr  = READ_INT4(fp);
      break;
    default:
      vf_error = VF_ERR_ILL_FONT_FILE;
      goto ErrExit;
    }
    optr = ftell(fp);
    fseek(fp, ptr, SEEK_SET);
    bm = &go->bm_table[code - bc];
    if (gf_read_glyph(fp, bm) < 0)
      goto ErrExit;
    bm->mv_x = dx;
    bm->mv_y = dy;
    fseek(fp, optr, SEEK_SET);
  }
  return go;
  
ErrExit:
printf("*ERROR\n");
  if (go != NULL){
    if (go->bm_table != NULL){
      for (i = 0; i < nchars; i++)
	vf_free(go->bm_table[i].bitmap);
    }
    vf_free(go->bm_table);
  }
  vf_free(go);
  return NULL;
}

Private int
gf_read_glyph(FILE* fp, VF_BITMAP bm)
{
  long           m, n;
  int            paint_sw;
  int            instr;
  INT4           min_m, max_m, min_n, max_n, del_m, del_n;
  long           w, h, d;
  int            m_b, k;
  unsigned char  *ptr;

  switch (READ_UINT1(fp)){
  case GF_BOC:
    SKIP_N(fp, 4);
    SKIP_N(fp, 4);
    min_m = READ_INT4(fp);
    max_m = READ_INT4(fp);
    min_n = READ_INT4(fp);
    max_n = READ_INT4(fp);
    break;
  case GF_BOC1:
    SKIP_N(fp, 1);
    del_m = (INT4)READ_UINT1(fp);
    max_m = (INT4)READ_UINT1(fp);
    del_n = (INT4)READ_UINT1(fp);
    max_n = (INT4)READ_UINT1(fp);
    min_m = max_m - del_m;
    min_n = max_n - del_n;
    break;
  default:
    return -1;
  }

  w = max_m - min_m + 1;
  h = max_n - min_n + 1;
  if ((w < 0) || (h < 0)){
    vf_error = VF_ERR_ILL_FONT_FILE;
    return -1;
  }

  if ((bm->bitmap = (unsigned char*)malloc(h*((w+7)/8))) == NULL){
    vf_error = VF_ERR_NO_MEMORY;
    return -1;
  }
  memclr(bm->bitmap, h*((w+7)/8));
  bm->raster     = (w+7)/8;
  bm->bbx_width  = w;
  bm->bbx_height = h;
  bm->off_x      = -min_m;
  bm->off_y      = max_n;
#if 0
  bm->mv_x       = -min_m;
  bm->mv_y       = max_n;
#endif

  m        = min_m;
  n        = max_n;
  paint_sw = 0;
  while ((instr = (int)READ_UINT1(fp)) != GF_EOC){
    if (instr == GF_PAINT_0){
      paint_sw = 1 - paint_sw;
    } else if ((GF_NEW_ROW_0 <= instr) && (instr <= GF_NEW_ROW_164)){
      m        = min_m + (instr - GF_NEW_ROW_0);
      n        = n - 1;
      paint_sw = 1;
    } else if ((GF_PAINT_1 <= instr) && (instr <= GF_PAINT_63)){
      d = (instr - GF_PAINT_1 + 1);
      goto Paint;
    } else {
      switch ((int)instr){
      case GF_PAINT1:
      case GF_PAINT2:
      case GF_PAINT3:
	d = (UINT4)READ_UINTN(fp, (instr - GF_PAINT1 + 1));
Paint:
	if (paint_sw == 0){
	  m = m + d;
	} else {
	  ptr = &bm->bitmap[(max_n - n) * bm->raster + (m - min_m)/8];
	  m_b = (m - min_m) % 8;
	  while (d > 0){
	    *ptr |= bit_table[m_b];
	    m++;
	    if (++m_b >= 8){
	      m_b = 0;
	      ++ptr;
	    }
	    d--;
	  }
	}
	paint_sw = 1 - paint_sw;
	break;
      case GF_SKIP0:
	m = min_m;
	n = n - 1;
	paint_sw = 0;
	break;
      case GF_SKIP1:
      case GF_SKIP2:
      case GF_SKIP3:
	m = min_m;
	n = n - (UINT4)READ_UINTN(fp, (instr - GF_SKIP1 + 1)) - 1;
	paint_sw = 0;
	break;
      case GF_XXX1:
      case GF_XXX2:
      case GF_XXX3:
      case GF_XXX4:
	k = READ_UINTN(fp, instr - GF_XXX1 + 1);
	SKIP_N(fp, k);
	break;
      case GF_YYY:
	SKIP_N(fp, 4);
	break;
      case GF_NO_OP:
	break;
      default:
	vf_free(bm->bitmap);
	bm->bitmap = NULL;
	vf_error = VF_ERR_ILL_FONT_FILE;
	return -1;
      }
    }
  }

  return 0;
}


/*EOF*/
