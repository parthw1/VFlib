/*
 * pk.c - TeX PK format font fonts loader.
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


Private PK_GLYPH  pk_loader(VF_CACHE,FILE*);
Private int       pk_read_14(FILE*,int,int,UINT4,VF_BITMAP,long);
Private int       pk_read_n14(FILE*,int,int,UINT4,VF_BITMAP,long);
Private long      pk_read_packed_number(long*,FILE*,int);
Private void      pk_read_nyble_init(int);
Private int       pk_read_nyble(FILE*);

Private unsigned char   bit_table[] = {
  0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };



Private PK_GLYPH
PK_CacheLoader(VF_CACHE c, char *font_path, int l)
{
  FILE      *fp;
  UINT1      pre, id;

  if (font_path == NULL)
    return NULL;
  if ((fp = vf_fm_OpenBinaryFileStream(font_path)) == NULL)
    return NULL;

  pre = READ_UINT1(fp);
  if (pre != PK_PRE)
    return NULL;

  id = READ_UINT1(fp);
  if (id != PK_ID)
    return NULL;

  return pk_loader(c, fp);
}

Private void
PK_CacheDisposer(PK_GLYPH go)
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


Private PK_GLYPH
pk_loader(VF_CACHE c, FILE* fp)
{
  PK_GLYPH      go;
  UINT1         instr;
  UINT4         ds, check_sum, hppp, vppp, k;
  unsigned int  flag, dny_f, bw, ess, size;
  UINT4         cc, tfm, dx, dy, dm, w, h, rs; 
  INT4          hoff, voff, mv_x, mv_y;
  long          gptr;
  int           bc, ec, nchars, index, i;

  k         = READ_UINT1(fp);
  SKIP_N(fp, k);
  ds        = READ_UINT4(fp);
  check_sum = READ_UINT4(fp);
  hppp      = READ_UINT4(fp);
  vppp      = READ_UINT4(fp);

  gptr = ftell(fp);

#if 0
  /* read min & max char code */
  bc = 256;
  ec = -1;
  for (;;){
    instr = READ_UINT1(fp);
    if (instr == PK_POST)
      break; 
    switch ((int) instr){
    case PK_XXX1:  k = (UINT4)READ_UINT1(fp); SKIP_N(fp, k); break;
    case PK_XXX2:  k = (UINT4)READ_UINT2(fp); SKIP_N(fp, k); break;
    case PK_XXX3:  k = (UINT4)READ_UINT3(fp); SKIP_N(fp, k); break;
    case PK_XXX4:  k = (UINT4)READ_UINT4(fp); SKIP_N(fp, k); break;
    case PK_YYY:   SKIP_N(fp, 4); break;
    case PK_NO_OP: break;
    default:
      size  = instr & 0x3; instr >>= 2;
      ess   = instr & 0x1; 
      if (ess == 0){                          /* short */
	rs = (UINT4)(size*256) + (UINT4)READ_UINT1(fp);
	cc   = (UINT4)READ_UINT1(fp);  
      } else if ((ess == 1) && (size != 3)){  /* extended short */
	rs = (UINT4)(size*65536) + (UINT4)READ_UINT2(fp);
	cc   = (UINT4)READ_UINT1(fp);  
      } else {                                /* standard */
	rs   = READ_UINT4(fp);
	cc   = (UINT4)READ_UINT4(fp);  
      }
      SKIP_N(fp, rs);
      if (cc < bc)
	bc = cc;
      if (cc > ec)
	ec = cc;
      break;
    }
  }

#else
  bc = 0;
  ec = 255;
#endif

  nchars = ec - bc + 1;
  ALLOC_IF_ERR(go, struct s_pk_glyph)
    return NULL;
  ALLOCN_IF_ERR(go->bm_table, struct vf_s_bitmap, nchars){
    vf_free(go);
    return NULL;
  }
  for (i = 0; i < nchars; i++)
    go->bm_table[i].bitmap = NULL;
  go->ds   = (double)ds/(1<<20);
  go->hppp = (double)hppp/(1<<16);
  go->vppp = (double)vppp/(1<<16);
  go->font_bbx_w = 0;
  go->font_bbx_h = 0;
  go->font_bbx_xoff = 0;
  go->font_bbx_yoff = 0;
  go->code_min = bc;
  go->code_max = ec;

  /* read glyphs */
  fseek(fp, gptr, SEEK_SET);
  for (;;){
    if ((instr = READ_UINT1(fp)) == PK_POST)
      break;
    switch ((int)instr){
    case PK_XXX1:  k = (UINT4)READ_UINT1(fp); SKIP_N(fp, k); break;
    case PK_XXX2:  k = (UINT4)READ_UINT2(fp); SKIP_N(fp, k); break;
    case PK_XXX3:  k = (UINT4)READ_UINT3(fp); SKIP_N(fp, k); break;
    case PK_XXX4:  k = (UINT4)READ_UINT4(fp); SKIP_N(fp, k); break;
    case PK_YYY:   SKIP_N(fp, 4); break;
    case PK_NO_OP: break;
    default:
      flag = instr;
      size  = flag % 0x04;  flag = flag >> 2;
      ess   = flag % 0x02;  flag = flag >> 1;
      bw    = flag % 0x02;  flag = flag >> 1;
      dny_f = flag % 0x10;
      if (ess == 0){                          /* short */
	rs = (UINT4)(size*256) + (UINT4)READ_UINT1(fp) - (UINT4)8; 
	cc   = (UINT4)READ_UINT1(fp);   tfm  = (UINT4)READ_UINT3(fp);
	dm   = (UINT4)READ_UINT1(fp);
	w    = (UINT4)READ_UINT1(fp);   h    = (UINT4)READ_UINT1(fp);
	hoff = (INT4)READ_INT1(fp);     voff = (INT4)READ_INT1(fp);
	mv_x = dm;
	mv_y = 0;
      } else if ((ess == 1) && (size != 3)){  /* extended short */
	rs = (UINT4)(size*65536) + (UINT4)READ_UINT2(fp) - (UINT4)13; 
	cc   = (UINT4)READ_UINT1(fp);   tfm  = (UINT4)READ_UINT3(fp);
	dm   = (UINT4)READ_UINT2(fp);
	w    = (UINT4)READ_UINT2(fp);   h    = (UINT4)READ_UINT2(fp);
	hoff = (INT4)READ_INT2(fp);     voff = (INT4)READ_INT2(fp);
	mv_x = dm;
	mv_y = 0;
      } else {                                /* standard */
	rs   = READ_UINT4(fp) - (UINT4)28;
	cc   = READ_UINT4(fp);          tfm  = READ_UINT4(fp);
	dx   = READ_UINT4(fp);	        dy   = READ_UINT4(fp);
	w    = READ_UINT4(fp);	        h    = READ_UINT4(fp);
	hoff = READ_INT4(fp);	        voff = READ_INT4(fp);
	mv_x = (double)dx/(double)(1<<16);
	mv_y = (double)dy/(double)(1<<16);
      }
      if ((cc < go->code_min) || (go->code_max < cc)){
	vf_error = VF_ERR_ILL_CODE_POINT;
	goto Error;
      }
      index = cc - go->code_min;
      go->bm_table[index].bbx_width  = w;
      go->bm_table[index].bbx_height = h;
      go->bm_table[index].raster = (w+7)/8;
      go->bm_table[index].off_x  = -hoff;
      go->bm_table[index].off_y  = voff;
      go->bm_table[index].mv_x   = mv_x;
      go->bm_table[index].mv_y   = mv_y;
      go->bm_table[index].bitmap = (unsigned char*)malloc(h*((w+7)/8));
      if (go->bm_table[index].bitmap == NULL){
	vf_error = VF_ERR_NO_MEMORY;
	goto Error;
      }
      memclr(go->bm_table[index].bitmap, h*((w+7)/8));
      if (dny_f == 14){
	if (pk_read_14(fp, dny_f, bw, rs, &(go->bm_table[index]), cc) < 0){
	  vf_error = VF_ERR_ILL_FONT_FILE;
	  goto Error;
	}
      } else {
	if (pk_read_n14(fp, dny_f, bw, rs, &(go->bm_table[index]), cc) < 0){
	  vf_error = VF_ERR_ILL_FONT_FILE;
	  goto Error;
	}
      }
      if (go->font_bbx_w < w)
	go->font_bbx_w = w;
      if (go->font_bbx_h < h)
	go->font_bbx_h = h;
      if (go->font_bbx_xoff > -hoff)
	go->font_bbx_xoff = -hoff;
      if (go->font_bbx_yoff > (voff - h))
	go->font_bbx_yoff = (voff - h);
    }
  }
  return go;

Error:
  for (i = 0; i < nchars; i++){
    if (go->bm_table[i].bitmap != NULL)
      vf_free(go->bm_table[i].bitmap);
  }
  vf_free(go->bm_table);
  vf_free(go);
  return NULL;
}

Private int
pk_read_14(FILE* fp, int dyn_f, int bw, UINT4 rs, VF_BITMAP bm, long cc)
{
  long           x, y, x8, xm;
  unsigned char  *bm_ptr;
  unsigned long  bit16_buff;
  int            rest_bit16_buff;
  static unsigned int mask_table[] = 
    { 0xdead,   0x80,   0xc0,   0xe0,   0xf0,   0xf8,   0xfc,   0xfe, 0xdead };
  
  if (rs == 0)
    return 0;

  x8 = bm->bbx_width / 8;
  xm = bm->bbx_width % 8;
  bm_ptr = bm->bitmap;

  bit16_buff = READ_UINT1(fp) << 8; 
  rest_bit16_buff = 8;
  --rs;

  for(y = 0; y < bm->bbx_height; y++){
    for(x = 0; x < x8; x++){
      *(bm_ptr++) = bit16_buff >> 8;
      rest_bit16_buff -= 8;
      bit16_buff = (bit16_buff << 8) & 0xffff;
      if (rs > 0){
	bit16_buff |= (READ_UINT1(fp) << (8 - rest_bit16_buff));
	rest_bit16_buff += 8;
	--rs;
      }
    }
    if (xm != 0){
      *(bm_ptr++) = (bit16_buff >> 8) & mask_table[xm];
      rest_bit16_buff -= xm;
      bit16_buff = (bit16_buff << xm) & 0xffff;
      if (rest_bit16_buff < 8){
	if (rs > 0){
	  bit16_buff |= (READ_UINT1(fp) << (8 - rest_bit16_buff));
	  rest_bit16_buff += 8;
	  --rs;
	}
      }
    }
  }

  return 0;
}

Private int
pk_read_n14(FILE* fp, int dyn_f, int bw, UINT4 rs, VF_BITMAP bm, long cc)
{
  long           x, y, xx, yy, repeat;
  int            bits, b_p;
  unsigned char  *p, *p0, *p1;

  pk_read_nyble_init(rs);
  p    = bm->bitmap;
  bw   = 1-bw;
  bits = 0;
  for (y = 0; y < bm->bbx_height; ){
    b_p    = 0;
    repeat = 0;
    p0     = p;
    for (x = 0; x < bm->bbx_width; x++){
      if (bits == 0){
	bw   = 1-bw;
	if ((bits = pk_read_packed_number(&repeat, fp, dyn_f)) < 0)
	  return -1;
      }
      if (bw == 1)
	*p = *p | bit_table[b_p];
      --bits;
      if (++b_p >= 8){
	b_p = 0;
	p++;
      }
    }
    if (b_p != 0)
      p++;
    y++;
    for (yy = 0; yy < repeat; yy++){
      p1 = p0;
      for (xx = 0; xx < bm->raster; xx++)
	*(p++) = *(p1++);
      y++;
    }
  }
  return 0;
}

Private long
pk_read_packed_number(long* repeat, FILE* fp, int dyn_f)
{
  int   d, n;
  long  di;
  
entry:
  d = pk_read_nyble(fp);
  if (d == 0){
    n = 0; 
    do {
      di = pk_read_nyble(fp);
      n++;
    } while (di == 0);
    for ( ; n > 0; n--)
      di = di*16 + pk_read_nyble(fp); 
    return di - 15 + (13 - dyn_f)*16 + dyn_f;
  } 
  if (d <= dyn_f)
    return d;
  if (d <= 13)
    return (d - dyn_f - 1)*16 + pk_read_nyble(fp) + dyn_f + 1;
  *repeat = 1;
  if (d == 14)
    *repeat = pk_read_packed_number(repeat, fp, dyn_f);
  goto entry;
}

Private int  pk_read_nyble_rest_cnt; 
Private int  pk_read_nyble_max_bytes; 

Private void
pk_read_nyble_init(int max)
{
  pk_read_nyble_rest_cnt  = 0;
  pk_read_nyble_max_bytes = max;
}

Private int
pk_read_nyble(FILE* fp)
{
  static UINT1  d; 
  int           v;

  switch (pk_read_nyble_rest_cnt){
  case 0:
    d = READ_UINT1(fp);
    if (--pk_read_nyble_max_bytes < 0)
      return -1L;
    v = d / 0x10;
    d = d % 0x10;
    pk_read_nyble_rest_cnt = 1;    
    break;
  case 1:
  default:
    v = d;
    pk_read_nyble_rest_cnt = 0;
    break;
  }

  return v;
}

/*EOF*/
