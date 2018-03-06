/* this is included by drv_vf.c */
/*
 * vf.c - A vf (virtual font) interface
 * by Hirotsugu Kakugawa
 *
 * 30 Jan 1997  First implementation.
 *  7 Aug 1997  VFlib 3.3  Changed API.
 *  2 Feb 1998  VFlib 3.4
 *
 */
/*
 * Copyright (C) 1996-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */


Private VF_TABLE vf_font_table = NULL;
Private VF_CACHE vf_font_cache = NULL;


struct s_vf_dvi_stack {
  long    h, v, w, x, y, z;
  int     f;
  int                    font_id;
  struct s_vf_dvi_stack  *next;
};
typedef struct s_vf_dvi_stack  *VF_DVI_STACK;

#define   STACK(X)     dvi_stack->next->X


Private int        vf_read_info(VF,FILE*,int,VF_FONT);
Private void       vf_do_glyph_style(VF_BITMAP,int);
Private VF_BITMAP  vf_run_dvi_program(VF,VF_CHAR_PACKET,int,double,double);
Private VF_CHAR_PACKET_TBL vf_cache_loader(VF_CACHE,VF_CACHE_KEY,int);
Private void               vf_cache_disposer(VF_CHAR_PACKET_TBL);

Private int   vf_dvi_interp(VF_BITMAPLIST,VF,
			    int,double,double,long,unsigned char*,int);
Private void  vf_dvi_interp_font_select(VF,VF_DVI_STACK,long,double*);
Private void  vf_dvi_interp_put_char(VF_BITMAPLIST,VF,VF_DVI_STACK,long,
				     int,double,double);
Private void  vf_dvi_interp_put_rule(VF_BITMAPLIST,VF,VF_DVI_STACK,long,long,
				     double,double);
Private int   vf_dvi_stack_init(VF,VF_DVI_STACK);
Private int   vf_dvi_stack_deinit(VF,VF_DVI_STACK);
Private int   vf_dvi_stack_push(VF,VF_DVI_STACK);
Private int   vf_dvi_stack_pop(VF,VF_DVI_STACK);



Private int
vf_vf_init(void)
{
  static int init_flag = 0;

  if (init_flag == 1)
    return 0;

  init_flag = 1;
  vf_font_cache
    = vf_cache_create(VF_CACHE_SIZE, VF_HASH_SIZE,
		      (void*(*)(VF_CACHE,void*,int))vf_cache_loader, 
		      (void(*)(void*))vf_cache_disposer);
  vf_font_table = vf_table_create();
  vf_vf_get_vf(-1);

  return 0;
}


Private int
vf_vf_open(VF_FONT font, FONT_VF font_vf, int implicit)
{
  int     vf_id;
  char    *vf_path, *tfm_path;
  double  dpi_x, dpi_y = 0.0;
  VF      vf;
  FILE    *fp;
  char    key[2*1024];

  vf_path = NULL;
  tfm_path = NULL;

  sprintf(key, "%s %.6f %.6f %.6f %.6f %.6f %.6f %.6f", 
	  font_vf->font_file, 
	  font->dpi_x, font->dpi_y, font->mag_x, font->mag_y,
	  font_vf->dpi_x, font_vf->dpi_y, font_vf->mag);

  vf_id = (vf_font_table->get_id_by_key)(vf_font_table, key, strlen(key)+1);
  if (vf_id >= 0){
    (vf_font_table->link_by_id)(vf_font_table, vf_id);
    return vf_id;
  }

  vf_path = vf_tex_search_file_misc(font_vf->font_file, implicit,
				    FSEARCH_FORMAT_TYPE_VF,
				    default_fontdirs, default_extensions);
  if (vf_path == NULL)
    return -1;


  if (vf_debug('f'))
    printf("VFlib Virtual Font: font file %s\n   ==> %s\n", 
	   font_vf->font_file, vf_path);


  tfm_path = vf_tex_search_file_tfm(font_vf->font_file, 
				    default_tfm_dirs, default_tfm_extensions);
  if (tfm_path == NULL){
    vf_free(vf_path);
    return -1;
  }

  if (vf_debug('f'))
    printf("VFlib Virtual Font: TFM font file %s\n   ==> %s\n", 
	   font_vf->font_file, tfm_path);

  if ((vf_path == NULL) || (tfm_path == NULL)){
    vf_free(vf_path);
    vf_free(tfm_path);
    return -1;
  }

  if (((dpi_x = font->dpi_x) < 0) || ((dpi_y = font->dpi_y) < 0)){
    if (((dpi_x = font_vf->dpi_x) < 0) || (dpi_x = font_vf->dpi_x)){
      dpi_x = dpi_y = vf_tex_default_dpi();
    }
  }

  ALLOC_IF_ERR(vf, struct s_vf){
    vf_error = VF_ERR_NO_MEMORY;
    return -1; 
  }
  vf->vf_path          = vf_path;
  vf->cs               = 0;
  vf->ds               = 0;
  vf->design_size      = 0;
  vf->dpi_x            = dpi_x;
  vf->dpi_y            = dpi_y;
  vf->mag_x            = font->mag_x * font_vf->mag;
  vf->mag_y            = font->mag_y * font_vf->mag;
  vf->tfm_path         = tfm_path;
  vf->tfm              = NULL;
  vf->subfonts         = NULL;
  vf->subfonts_opened  = 1;
  vf->offs_char_packet = 0;

  if ((vf->tfm = vf_tfm_open(vf->tfm_path)) == NULL)
    goto Error;

  if ((fp = vf_fm_OpenBinaryFileStream(vf->vf_path)) == NULL){
    if (vf_error != VF_ERR_NO_MEMORY)
      vf_error = VF_ERR_NO_FONT_FILE;
    goto Error;
  }

  if (vf_read_info(vf, fp, font_vf->open_style, font) < 0){
    vf_error = VF_ERR_ILL_FONT_FILE;
    goto Error;
  }

  if ((vf_id = (vf_font_table->put)(vf_font_table, vf,
				    key, strlen(key)+1)) < 0){
    vf_error = VF_ERR_NO_MEMORY;
    goto Error;
  }

  return vf_id;


Error:
  if (vf != NULL){
    vf_free(vf->vf_path);
    vf_free(vf->tfm_path);
    vf_tfm_free(vf->tfm);
    vf_free(vf);
  }
  return -1;
}


Private int
vf_read_info(VF vf, FILE *fp, int open_style, VF_FONT font)
{
  UINT1                id, a, l;
  UINT4                k, c, s, d;
  VF_SUBFONT           sf, sf0, sf_next;
  struct s_vf_subfont  subfont;
  double               scale;
  int                  fid, name_len, i;
  char                 subfont_name[1024];

  if (READ_UINT1(fp) != VFINST_PRE)
    return -1;
  id = READ_UINT1(fp);
  switch (id){
  case VFINST_ID_BYTE:
    break;
  default:
    return -1;
  }

  k = READ_UINT1(fp);  
  SKIP_N(fp,k);

  vf->cs = READ_UINT4(fp);
  vf->ds = READ_UINT4(fp);
  if ((vf->cs != vf->tfm->cs) || (vf->ds != vf->tfm->ds)){
    vf_error = VF_ERR_ILL_FONT_FILE;
    return -1;
  }

  vf->design_size     = (double)(vf->ds)/(double)(1<<20);
  vf->subfonts_opened = 1;
  vf->default_subfont = -1;

  subfont.next = NULL;
  for (sf0 = &subfont; ; sf0 = sf){
    ALLOC_IF_ERR(sf, struct s_vf_subfont){
      vf_error = VF_ERR_NO_MEMORY;
      goto error_exit;
    }
    sf0->next = sf;
    switch (READ_UINT1(fp)){
    default:
      vf->offs_char_packet = ftell(fp)-1;
      sf0->next = NULL;
      vf_free(sf);
      goto end_fontdef;
    case VFINST_FNTDEF1:
      k = (UINT4)READ_UINT1(fp);
      c = READ_UINT4(fp); s = READ_UINT4(fp); d = READ_UINT4(fp);
      a = READ_UINT1(fp); l = READ_UINT1(fp);
      break;
    case VFINST_FNTDEF2:
      k = (UINT4)READ_UINT2(fp);
      c = READ_UINT4(fp); s = READ_UINT4(fp); d = READ_UINT4(fp);
      a = READ_UINT1(fp); l = READ_UINT1(fp);
      break;
    case VFINST_FNTDEF3:
      k = (UINT4)READ_UINT3(fp);
      c = READ_UINT4(fp); s = READ_UINT4(fp); d = READ_UINT4(fp);
      a = READ_UINT1(fp); l = READ_UINT1(fp);
      break;
    case VFINST_FNTDEF4:
      k = (UINT4)READ_UINT4(fp);
      c = READ_UINT4(fp); s = READ_UINT4(fp); d = READ_UINT4(fp);
      a = READ_UINT1(fp); l = READ_UINT1(fp);
      break;
    }

    name_len = a + l;
    sf->k       = k;
    sf->s       = s;
    sf->d       = d;
    sf->a       = a;
    sf->l       = l;
    sf->next    = NULL;

    scale = (double)sf->s/(double)(1<<20);

    if ((sf->n = (char*)malloc(name_len + 1)) == NULL){
      vf_error = VF_ERR_NO_MEMORY;
      goto error_exit;
    }
    for (i = 0; i < name_len; i++)
      sf->n[i] = (char)READ_UINT1(fp);
    sf->n[i] = '\0';


    sprintf(subfont_name, "%s", &sf->n[sf->a]);

    if (vf_debug('s')){
      printf("VFlib Virtual Font: subfont %d: %s, scaled %f\n", 
	     (int)sf->k, subfont_name, scale);
    }
    
    if (open_style != TEX_OPEN_STYLE_NONE){
      fid = vf_tex_try_map_and_open_font(font, subfont_name, 
					 default_font_mapping,
					 vf->tfm->design_size, NULL, NULL, 
					 scale);
      if (fid >= 0){
	sf->font_id = fid;
	if (vf->default_subfont < 0)
	  vf->default_subfont = sf->k;
	if (vf_debug('s'))
	  printf("VFlib Virtual Font: subfont is opened: font id %d\n", fid);
      } else {
	sf->font_id = -1;
	vf->subfonts_opened = 0;
	if (vf_debug('s'))
	  printf("VFlib Virtual Font: subfont %d is not opened\n", (int)sf->k);
      }
    } else {
      if (vf_debug('s'))
	printf("VFlib Virtual Font: subfont %d is not requested to open\n", 
	       (int)sf->k);
    }
  }

end_fontdef:
  if (vf->subfonts_opened == 0){
    if (open_style == TEX_OPEN_STYLE_REQUIRE){
      if (vf_debug('s'))
	printf("VFlib Virtual Font: all subfonts are required but failed\n");
      goto error_exit;
    } else {
      if (vf_debug('s'))
	printf("VFlib Virtual Font: not all fonts are opened; continue.\n");
    }
  }

  vf->subfonts = subfont.next;
  return 0;  


error_exit:
  for (sf = subfont.next; sf != NULL; sf = sf_next){
    sf_next = sf->next;
    vf_free(sf->n);
    vf_free(sf);
  }
  vf->subfonts = NULL;
  return -1;
}


Private void
vf_vf_close(int vf_id)
{
  VF          vf;
  VF_SUBFONT  sf, sf_next;

  vf = vf_vf_get_vf(vf_id);

  if ((vf_font_table->unlink_by_id)(vf_font_table, vf_id) > 0)
    return;
  
  if (vf != NULL){
    vf_free(vf->vf_path);
    vf_free(vf->tfm_path);
    vf_tfm_free(vf->tfm);
    for (sf = vf->subfonts; sf != NULL; sf = sf_next){
      sf_next = sf->next;
      vf_free(sf->n);
      vf_free(sf);
    }
  }

  vf_vf_get_vf(-1);
  vf_free(vf);
}


Private double
vf_vf_get_design_size(int vf_id)
{
  VF   vf;

  vf = vf_vf_get_vf(vf_id);
  if ((vf == NULL) || (vf->tfm == NULL)){
    fprintf(stderr, "VFlib internal error in vf_vf_get_design_size()\n");
    abort();
  }
  return vf->tfm->design_size;
}


#if 0
static VF         last_vf      = NULL;  
static int        last_vf_id   = -1;  
#endif

Private VF
vf_vf_get_vf(int vf_id)
{
  VF   vf;

#if 1
  vf = NULL;
  if (vf_id >= 0)
    vf = (vf_font_table->get_obj_by_id)(vf_font_table, vf_id);
#else
  if (vf_id < 0){
    last_vf = NULL;
    last_vf_id = -1;
    return NULL;
  }

  if (vf_id == last_vf_id){
    vf = last_vf;
  } else {
    vf = (vf_font_table->get_obj_by_id)(vf_font_table, vf_id);
    if (vf != NULL){
      last_vf = vf;
      last_vf_id = vf_id;
    } else {
      last_vf = NULL;
      last_vf_id = -1;
    }
  }
#endif

  return vf;
}




Private VF_CHAR_PACKET_TBL
vf_cache_loader(VF_CACHE c, VF_CACHE_KEY ck, int l)
{
  VF_CHAR_PACKET_TBL  vf_char_packets;
  VF_CHAR_PACKET      packets;
  int            npackets, ch;
  FILE           *fp;
  int            b;
  long           n;
  unsigned char  *cp;

#if 0
  printf("* VF cache_load %s\n", ck->font_path);
#endif
  npackets = ck->tfm->end_char - ck->tfm->begin_char + 1;

  if (ck->font_path == NULL)
    return NULL;
  if ((fp = vf_fm_OpenBinaryFileStream(ck->font_path)) == NULL)
    return NULL;
  
  ALLOC_IF_ERR(vf_char_packets, struct s_vf_char_packet_tbl){
    vf_error = VF_ERR_NO_MEMORY;
    return NULL; 
  }
  ALLOCN_IF_ERR(packets, struct s_vf_char_packet, 
		npackets+1){  /* one more element for sentinel. 
				 See vf_get_bitmap(). */
    vf_free(vf_char_packets);
    vf_error = VF_ERR_NO_MEMORY;
    return NULL; 
  }
  vf_char_packets->npackets = npackets;
  vf_char_packets->packets  = packets;
  for (ch = 0; ch < npackets; ch++){
    vf_char_packets->packets[ch].pl  = 0;
    vf_char_packets->packets[ch].cc  = 0;
    vf_char_packets->packets[ch].tfm = 0;
    vf_char_packets->packets[ch].dvi = NULL;
  }

  fseek(fp, ck->offs_char_packet, SEEK_SET);
  for (ch = 0; ch < npackets; ch++){
    b = (int)READ_UINT1(fp);
    if (((int)VFINST_CP_SHORT_CHAR0 <= b)
	&& (b <= (int)VFINST_CP_SHORT_CHAR241)){
      vf_char_packets->packets[ch].pl  = (UINT4)b;
      vf_char_packets->packets[ch].cc  = (UINT4)READ_UINT1(fp);
      vf_char_packets->packets[ch].tfm = (UINT4)READ_UINT3(fp);
    } else if (b == (int)VFINST_CP_LONG_CHAR){
      vf_char_packets->packets[ch].pl  = READ_UINT4(fp);
      vf_char_packets->packets[ch].cc  = READ_UINT4(fp);
      vf_char_packets->packets[ch].tfm = READ_UINT4(fp);
    } else if (b == VFINST_POST){
      break;
    } else {
      fprintf(stderr, "VFlib warning: Broken VF file: %s\n", ck->font_path);
      break;
    }
#if 0
    printf("  0x%02x: pl 0x%04lx: cc 0x%04lx\n", b,
	   vf_char_packets->packets[ch].pl, 
	   vf_char_packets->packets[ch].cc);
#endif

    if (vf_char_packets->packets[ch].pl > 0){
      vf_char_packets->packets[ch].dvi 
	= (unsigned char*)malloc(vf_char_packets->packets[ch].pl);
      if (vf_char_packets->packets[ch].dvi == NULL){
	vf_free(vf_char_packets->packets);
	vf_free(vf_char_packets);
	vf_error = VF_ERR_NO_MEMORY;
	return NULL; 
      }
      n = vf_char_packets->packets[ch].pl;
      for (cp = vf_char_packets->packets[ch].dvi; n > 0; cp++, n--)
	*cp = READ_UINT1(fp);
    }
  }

  return vf_char_packets;
}


Private void
vf_cache_disposer(VF_CHAR_PACKET_TBL vf_char_packets)
{
  int   ch;

#if 0
  printf("* VF cache_disposer\n");
#endif
  if (vf_char_packets != NULL){
    if (vf_char_packets->packets != NULL){
      for (ch = vf_char_packets->npackets-1; ch >= 0; ch--)
	vf_free(vf_char_packets->packets[ch].dvi);
    }
    vf_free(vf_char_packets->packets);
  }
  vf_free(vf_char_packets);
}


Private int
vf_vf_get_metric(int vf_id, long code_point, VF_METRIC1 met,
		 double *ret_design_size)
{
  VF    vf;
  struct vf_s_metric1 metric1;

  if (   ((vf = vf_vf_get_vf(vf_id)) == NULL) 
      || (vf->tfm == NULL) ){
    fprintf(stderr, "VFlib internal error in vf_vf_get_metric()\n");
    abort();
  }

  if (ret_design_size != NULL)
    *ret_design_size = vf->tfm->design_size;

  if (met == NULL)
    met = &metric1;

  if (vf_tfm_metric(vf->tfm, code_point, met) == NULL)
    return -1;

  return 0;
}

Private VF_BITMAP
vf_vf_get_bitmap(int vf_id, int mode, long code_point,
		 double mag_x, double mag_y,
		 int open_style, int glyph_style)
{
  VF                     vf;
  VF_BITMAP              bm;
  VF_CHAR_PACKET_TBL     cptbl;
  struct vf_s_metric1    met1;
  struct vf_s_metric2    met2;
  struct s_vf_cache_key  ck;
  int                    idx;

  if (   ((vf = vf_vf_get_vf(vf_id)) == NULL) 
      || (vf->tfm == NULL)  ){
    fprintf(stderr, "VFlib internal error in vf_vf_get_bitmap()\n");
    abort();
  }

  switch (open_style){
  default:
  case TEX_OPEN_STYLE_NONE:
    if (vf_tfm_metric(vf->tfm, code_point, &met1) == NULL)
      return NULL;
#if 0 /*XXX*/
    bm = vf_alloc_bitmap_with_metric1(&met1, 
				      vf->dpi_x * vf->mag_x * mag_x, 
				      vf->dpi_y * vf->mag_y * mag_y);
#else
    bm = vf_alloc_bitmap_with_metric1(&met1, 
				      vf->dpi_x * mag_x, 
				      vf->dpi_y * mag_y);
#endif
    vf_do_glyph_style(bm, glyph_style);
    return bm;
  case TEX_OPEN_STYLE_TRY:
    if (vf->subfonts_opened == 0){
      if (vf_tfm_metric(vf->tfm, code_point, &met1) == NULL)
	return NULL;
#if 0 /*XXX*/
      bm = vf_alloc_bitmap_with_metric1(&met1, 
					vf->dpi_x * vf->mag_x * mag_x, 
					vf->dpi_y * vf->mag_y * mag_y);
#else
      bm = vf_alloc_bitmap_with_metric1(&met1, 
					vf->dpi_x * mag_x, 
					vf->dpi_y * mag_y);
#endif
      vf_do_glyph_style(bm, glyph_style);
      return bm;
    }
    break;
  case TEX_OPEN_STYLE_REQUIRE:
    if (vf->subfonts_opened == 0)
      return NULL;
    break;
  }

  ck.font_path        = vf->vf_path;
  ck.tfm              = vf->tfm;
  ck.offs_char_packet = vf->offs_char_packet;
  if ((cptbl = (vf_font_cache->get)(vf_font_cache, &ck, sizeof(ck))) == NULL)
    return NULL;

  idx = 0; 
  cptbl->packets[cptbl->npackets].cc = code_point;   /* sentinel */
  while ((long)cptbl->packets[idx].cc != code_point)
    idx++;

  if (idx == cptbl->npackets){
    vf_error = VF_ERR_ILL_CODE_POINT;
    return NULL;
  }

  bm = vf_run_dvi_program(vf, &cptbl->packets[idx], mode, mag_x, mag_y);
  if (bm != NULL){
    if (vf_tfm_metric(vf->tfm, code_point, &met1) == NULL){
      VF_FreeBitmap(bm);
      return NULL;
    }
#if 0 /*XXX*/
    vf_metric1_to_metric2(&met1, (double)vf->dpi_y * vf->mag_y * mag_y, &met2);
#else
    vf_metric1_to_metric2(&met1, (double)vf->dpi_y * mag_y, &met2);
#endif
    bm->mv_x = met2.mv_x;
    bm->mv_y = met2.mv_y;
  } else {
    if (vf_tfm_metric(vf->tfm, code_point, &met1) == NULL)
      return NULL;
#if 0 /*XXX*/
    bm = vf_alloc_bitmap_with_metric1(&met1, 
				      vf->dpi_x * vf->mag_x * mag_x, 
				      vf->dpi_y * vf->mag_y * mag_y);
#else
    bm = vf_alloc_bitmap_with_metric1(&met1, 
				      vf->dpi_x * mag_x, 
				      vf->dpi_y * mag_y);
#endif
    vf_do_glyph_style(bm, glyph_style);
  }

  return bm;
}

Private void
vf_do_glyph_style(VF_BITMAP bm, int glyph_style)
{
  switch (glyph_style){
  default:
  case TEX_GLYPH_STYLE_EMPTY:
    break;
  case TEX_GLYPH_STYLE_FILL:
    VF_FillBitmap(bm);
    break;
  }
}


Private VF_BITMAP
vf_run_dvi_program(VF vf, VF_CHAR_PACKET packet, 
		   int mode, double mag_x, double mag_y)
{
  struct vf_s_bitmaplist  the_bmlist;
  VF_BITMAP               bm;

  vf_bitmaplist_init(&the_bmlist);
  vf_dvi_interp(&the_bmlist, vf, mode, mag_x, mag_y, 
		packet->cc, packet->dvi, packet->pl);
  bm = vf_bitmaplist_compose(&the_bmlist);
  vf_bitmaplist_finish(&the_bmlist);

  return bm;
}

Private int
vf_dvi_interp(VF_BITMAPLIST bmlist, VF vf,
	      int mode, double mag_x, double mag_y, 
	      long cc, unsigned char *dvi_prog, int prog_len)
{
  int                    pc, instr, n, ret;
  long                   code_point, h, w, f, length;
  double                 fmag;
  double                 r_mv_x, r_mv_y;
  struct vf_s_metric1    met, *m;
  struct s_vf_dvi_stack  the_dvi_stack, *dvi_stack;
  
  fmag = 1.0;
  dvi_stack = &the_dvi_stack;
  vf_dvi_stack_init(vf, dvi_stack);

  pc = 0;
  ret = 0;
  while (pc < prog_len){
    if (vf_debug('d')){
      printf("VFlib Virtual Font\n   ");
      printf("DVI CODE  PC=0x%04x: INSTR=0x%02x (%d)  H=0x%08x V=0x%08x\n", 
	     pc, (int)dvi_prog[pc], (int)dvi_prog[pc],
	     (int)STACK(h), (int)STACK(v));
    }
    instr = (int)dvi_prog[pc++];
    if (instr <= VFINST_SET4){ /* SETCHAR0 ... SETCHAR127, SET1, ... ,SET4 */
      if ((code_point = instr) > VFINST_SETCHAR127){
	n = instr - VFINST_SET1 + 1;
	code_point = GET_UINTN(&dvi_prog[pc], n);
	pc += n;
      }
      vf_dvi_interp_put_char(bmlist, vf, dvi_stack, code_point,
			     mode, fmag * mag_x, fmag * mag_y);
      m = VF_GetMetric1(STACK(font_id), code_point, &met, 
			fmag * mag_x, fmag * mag_y);
      if (m == NULL)
	continue;
      r_mv_x = (met.mv_x / vf->design_size) * (double)(1<<20);
      r_mv_y = (met.mv_y / vf->design_size) * (double)(1<<20);
      STACK(h) = STACK(h) + toint(r_mv_x);
      STACK(v) = STACK(v) + toint(r_mv_y);
    } else if ((VFINST_FNTNUM0 <= instr) && (instr <= (VFINST_FNTNUM63))){
      f = instr - VFINST_FNTNUM0;
      vf_dvi_interp_font_select(vf, dvi_stack, f, &fmag);
    } else {
      switch (instr){
      case VFINST_PUT1: case VFINST_PUT2: case VFINST_PUT3: case VFINST_PUT4:
	n = instr - VFINST_SET1 + 1;
	code_point = (UINT4)GET_UINTN(&dvi_prog[pc], n); pc += n;
	vf_dvi_interp_put_char(bmlist, vf, dvi_stack, code_point,
			       mode, fmag * mag_x, fmag * mag_y);
	break;	
      case VFINST_SETRULE:
	h = (long)GET_INT4(&dvi_prog[pc]); pc += 4;
	w = (long)GET_INT4(&dvi_prog[pc]); pc += 4;
	vf_dvi_interp_put_rule(bmlist, vf, dvi_stack, w, h, mag_x, mag_y);
	STACK(h) += w;
	break;
      case VFINST_PUTRULE:
	h = (long)GET_INT4(&dvi_prog[pc]); pc += 4;
	w = (long)GET_INT4(&dvi_prog[pc]); pc += 4;
	vf_dvi_interp_put_rule(bmlist, vf, dvi_stack, w, h, mag_x, mag_y);
	break;
      case VFINST_RIGHT1: case VFINST_RIGHT2: 
      case VFINST_RIGHT3: case VFINST_RIGHT4:
	n = instr - VFINST_RIGHT1 + 1;
	STACK(h) += (long)GET_INTN(&dvi_prog[pc], n); pc += n;
	break;
      case VFINST_X1: case VFINST_X2: case VFINST_X3: case VFINST_X4:
	n = instr - VFINST_X0;
	STACK(x) = (long)GET_INTN(&dvi_prog[pc], n); pc += n;
      case VFINST_X0:
	STACK(h) += STACK(x);
	break;
      case VFINST_W1: case VFINST_W2: case VFINST_W3: case VFINST_W4:
	n = instr - VFINST_W0;
	STACK(w) = (long)GET_INTN(&dvi_prog[pc], n); pc += n;
      case VFINST_W0: 
	STACK(h) += STACK(w);
	break;
      case VFINST_Y1: case VFINST_Y2: case VFINST_Y3: case VFINST_Y4:
	n = instr - VFINST_Y0;
	STACK(y) = (long)GET_INTN(&dvi_prog[pc], n); pc += n;
      case VFINST_Y0: 
	STACK(v) += STACK(y);
	break;
      case VFINST_Z1: case VFINST_Z2: case VFINST_Z3: case VFINST_Z4:
	n = instr - VFINST_Z0;
	STACK(z) = (long)GET_INTN(&dvi_prog[pc], n); pc += n;
      case VFINST_Z0: 
	STACK(v) += STACK(z);
	break;
      case VFINST_DOWN1: case VFINST_DOWN2: 
      case VFINST_DOWN3: case VFINST_DOWN4:
	n = instr - VFINST_DOWN1 + 1;
	STACK(v) += (long)GET_INTN(&dvi_prog[pc], n);
	break;
      case VFINST_XXX1: case VFINST_XXX2: case VFINST_XXX3: case VFINST_XXX4:
	n = instr - VFINST_XXX1 + 1;
	length = (long)GET_INTN(&dvi_prog[pc], n); pc += n;
	pc += length;
	break;
      case VFINST_FNT1: case VFINST_FNT2: case VFINST_FNT3: case VFINST_FNT4:
	n = instr - VFINST_FNT1 + 1;
	f = GET_UINTN(&dvi_prog[pc], n); pc += n;
	vf_dvi_interp_font_select(vf, dvi_stack, f, &fmag);
	break;
      case VFINST_PUSH:
	vf_dvi_stack_push(vf, dvi_stack);
	break;
      case VFINST_POP:
	vf_dvi_stack_pop(vf, dvi_stack);
	break;
      case VFINST_NOP:
	break;
      default:
	vf_error = VF_ERR_ILL_FONT_FILE;
	ret = -1;
	goto ExitInterp;
      }
    }
  }

ExitInterp:
  vf_dvi_stack_deinit(vf, dvi_stack);
  return ret;
}

Private void
vf_dvi_interp_put_char(VF_BITMAPLIST bmlist, VF vf, VF_DVI_STACK dvi_stack,
		       long code_point, int mode, double mag_x, double mag_y)
{
  VF_BITMAP  bm;
  double     rx, ry, ds;
  long       off_x, off_y;

  if (STACK(font_id) < 0)
    return;
  if (mode == 1){
    bm = VF_GetBitmap1(STACK(font_id), code_point, mag_x, mag_y);
  } else {
    bm = VF_GetBitmap2(STACK(font_id), code_point, mag_x, mag_y);
  }
#if 0
  printf("** VF_GetBitmap(%d, 0x%lx, %.2f %.2f) = %p\n",
	 STACK(font_id), code_point, mag_x, mag_y, bm);
  VF_DumpBitmap(bm);
#endif
  if (bm == NULL)
    return;

  ds = vf->design_size / (double)(1<<20);
#if 1 /*XXX*/
  rx = vf->mag_x * mag_x * (vf->dpi_x/72.27) * ds;
  ry = vf->mag_y * mag_y * (vf->dpi_y/72.27) * ds;
#else
  rx = (vf->dpi_x/72.27) * ds;
  ry = (vf->dpi_y/72.27) * ds;
#endif
  off_x =  rx * (double)STACK(h);
  off_y = -ry * (double)STACK(v);

  vf_bitmaplist_put(bmlist, bm, off_x, off_y);
}

Private void
vf_dvi_interp_put_rule(VF_BITMAPLIST bmlist, VF vf, VF_DVI_STACK dvi_stack,
		       long w, long h, double mag_x, double mag_y)
{
  VF_BITMAP      bm;
  double         rx, ry, ds;
  int            bm_w, bm_h;
  long           off_x, off_y;

  ds = vf->design_size / (double)(1<<20);
  rx = vf->mag_x * mag_x * vf->dpi_x/72.27 * ds;
  ry = vf->mag_y * mag_y * vf->dpi_y/72.27 * ds;

  bm_w = rx * w;
  bm_h = ry * h;
  if (bm_w <= 0)
    bm_w = 1;
  if (bm_h <= 0)
    bm_h = 1;
  
  bm = vf_alloc_bitmap(bm_w, bm_h);
  if (bm == NULL)
    return;
  VF_FillBitmap(bm);

  bm->off_x = 0;
  bm->off_y = bm_h - 1;
  off_x =  rx * (double)STACK(h);
  off_y = -ry * (double)STACK(v);

  vf_bitmaplist_put(bmlist, bm, off_x, off_y);
}

Private void
vf_dvi_interp_font_select(VF vf, VF_DVI_STACK dvi_stack, long f,
			  double *fmag_p)
{
  VF_SUBFONT  sf;
  
  STACK(f) = f;
  STACK(font_id) = -1;
  for (sf = vf->subfonts; sf != NULL; sf = sf->next){
    if (sf->k == f){
      STACK(font_id) = sf->font_id;
      if (fmag_p != NULL)
	*fmag_p = 1;
      break;
    }
  }
#if 0
  printf("FONT %d: fid=%d (%s)\n", f, sf->font_id, sf->n);
#endif
}


Private int
vf_dvi_stack_init(VF vf, VF_DVI_STACK stack)
{
  VF_DVI_STACK  top;

  ALLOC_IF_ERR(top, struct s_vf_dvi_stack){
    vf_error = VF_ERR_NO_MEMORY;
    return -1;
  }
  top->h = top->v = top->w = top->x = top->y = top->z = 0;
  top->f = vf->default_subfont;
  top->font_id = vf->subfonts->font_id;
  top->next    = NULL;
  stack->next = top;
  return 0;
}

Private int
vf_dvi_stack_deinit(VF vf, VF_DVI_STACK stack)
{
  VF_DVI_STACK  elem, elem_next;

  elem = stack->next; 
  while (elem != NULL){
    elem_next = elem->next;
    vf_free(elem);
    elem = elem_next;
  }
  return 0;  
}

Private int
vf_dvi_stack_push(VF vf, VF_DVI_STACK stack)
{
  VF_DVI_STACK  new_elem, top;

  ALLOC_IF_ERR(new_elem, struct s_vf_dvi_stack){
    vf_error = VF_ERR_NO_MEMORY;
    return -1;
  }

  top = stack->next;
  new_elem->h = top->h;
  new_elem->v = top->v;
  new_elem->w = top->w;
  new_elem->x = top->x;
  new_elem->y = top->y;
  new_elem->z = top->z;
  new_elem->f = top->f;
  new_elem->font_id = top->font_id;
  new_elem->next = top;
  stack->next = new_elem;

  return 0;
}

Private int
vf_dvi_stack_pop(VF vf, VF_DVI_STACK stack)
{
  VF_DVI_STACK  top;

  top = stack->next;
  if (top == NULL){
    fprintf(stderr, "VFlib warning: VF DVI stack under flow: %s\n",
	    vf->vf_path);
    return -1;
  }

  stack->next = top->next;
  vf_free(top);

  return 0;
}


/*EOF*/
