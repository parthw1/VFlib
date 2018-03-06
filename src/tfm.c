/*
 * tfm.c - TFM files interface
 *
 * 28 Sep 1996
 * 25 Mar 1997  Added setting a program name for kpathsea by variable.
 * 02 Apr 1997  Added support for .ofm files (Omega metrics file) (WL)
 *  3 Jul 1997  Added Virtual Font support.
 *  8 Aug 1997  for VFlib 3.3  
 *  1 Feb 1998  for VFlib 3.4
 */
/*
 * Copyright (C) 1996-2017 Hirotsugu Kakugawa. 
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
#include  "vflibcap.h"
#include  "consts.h"
#include  "metric.h"
#include  "cache.h"
#include  "sexp.h"
#include  "texfonts.h"
#include  "tfm.h"


#define RDS2PT(rds)    (tfm->design_size * ((double)(rds)/(double)(1<<20)))


Glocal SEXP_LIST     vf_tex_tfm_fontdirs;
Glocal SEXP_LIST     vf_tex_tfm_extensions;

Private VF_TABLE  tfm_table = NULL;



Glocal int
vf_tfm_init(void)
{
  static int   inited = 0;

  if (inited == 0){
    inited = 1;
    if ((tfm_table = vf_table_create()) == NULL){
      vf_error = VF_ERR_NO_MEMORY;
      return -1;
    }
  }

  return 0;
}


Private TFM   read_tfm(FILE* fp);


Glocal TFM
vf_tfm_open(char *tfm_path)
{
  int    tfm_id;
  TFM    tfm;
  FILE   *fp;

  if ((tfm_id = (tfm_table->get_id_by_key)(tfm_table, tfm_path, 
					   strlen(tfm_path)+1)) >= 0){
    (tfm_table->link_by_id)(tfm_table, tfm_id);
    return (tfm_table->get_obj_by_id)(tfm_table, tfm_id);
  }

#if 0
  printf("* TFM Open: %s\n", tfm_path);
#endif

  if ((fp = vf_fm_OpenBinaryFileStream(tfm_path)) == NULL)
    goto Error;

  if ((tfm = read_tfm(fp)) == NULL)
    goto Error;

  if ((tfm_table->put)(tfm_table, tfm, tfm_path, strlen(tfm_path)+1) < 0)
    return NULL;

  return tfm;


Error:
  vf_error = VF_ERR_NO_METRIC_FILE;
  return NULL;
}




Glocal int
vf_tfm_jfm_chartype(TFM tfm, UINT4 code)
{
  int  i;

  tfm->ct_kcode[tfm->nt] = code; 
  i = 0;
  while (tfm->ct_kcode[i] != code)
    i++;
  if (i == tfm->nt)
    return 0;
  return tfm->ct_ctype[i];
}


Glocal VF_METRIC1
vf_tfm_metric(TFM tfm, UINT4 code, VF_METRIC1 metric)
{
  int      dir_h, index;
  double   w, h, d;

  if (metric == NULL)
    if ((metric = vf_alloc_metric1()) == NULL)
      return NULL;

  if ((tfm->type == METRIC_TYPE_TFM) || (tfm->type == METRIC_TYPE_OFM)){
    dir_h = 1;
    index = (int)code;
  } else {   /* == METRIC_TYPE_JFM */
    dir_h = (tfm->type_aux == METRIC_TYPE_JFM_AUX_H) ? 1 : 0;
    if (code == 0)
      index = 0;
    else
      index = vf_tfm_jfm_chartype(tfm, code);
  }
  if ((index < tfm->begin_char) || (tfm->end_char < index)){
    vf_error = VF_ERR_ILL_CODE_POINT;
    return NULL;
  }

  w = RDS2PT(tfm->width[index - tfm->begin_char]);
  h = RDS2PT(tfm->height[index - tfm->begin_char]);
  d = RDS2PT(tfm->depth[index - tfm->begin_char]);
#if 0
  printf("* %d: W=%f H=%f D=%f\n", index, w, h, d);
#endif

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
  return metric;
}


Private TFM
read_tfm(FILE* fp)
{
  TFM    tfm;
  UINT4  lf, lh, nc, nci, err;
  UINT4  offset_header, offset_char_info, offset_param;
  UINT4  nw,  nh,  nd,  ni, nl, nk, neng, np, dir;
  INT4   *w,  *h,  *d;
  UINT4  *ci, v;
  UINT4  i;
  INT4   bbxw, bbxh, xoff, yoff;
  
  ALLOC_IF_ERR(tfm, struct s_tfm){
    vf_error = VF_ERR_NO_MEMORY;
    return NULL;
  }

  tfm->width  = NULL;
  tfm->height = NULL;
  tfm->depth  = NULL;
  tfm->ct_kcode = NULL;
  tfm->ct_ctype = NULL;

  tfm->font_bbx_w = 0.0;
  tfm->font_bbx_h = 0.0;
  tfm->font_bbx_xoff = 0.0;
  tfm->font_bbx_yoff = 0.0;
  
  err = 0;
  rewind(fp);
  lf = (UINT4)READ_UINT2(fp);
  if ((lf == 11) || (lf == 9)){
    /* JFM file of Japanese TeX by ASCII Coop. */
    tfm->type        = METRIC_TYPE_JFM;
    tfm->type_aux    = (lf == 11)?METRIC_TYPE_JFM_AUX_H:METRIC_TYPE_JFM_AUX_V;
    tfm->nt          = (UINT4)READ_UINT2(fp);
    lf               = (UINT4)READ_UINT2(fp);
    lh               = (UINT4)READ_UINT2(fp);    
    offset_header    = 4*7;
    offset_char_info = 4*(7+tfm->nt+lh);
  } else if (lf == 0){
    /* Omega Metric File */
    tfm->type        = METRIC_TYPE_OFM;
    tfm->type_aux    = READ_INT2(fp);    /* ofm_level */
    if ((tfm->type_aux < 0) || (1 < tfm->type_aux))
      tfm->type_aux = 0;  /* broken, maybe */
    lf               = READ_UINT4(fp);
    lh               = READ_UINT4(fp);
    if (tfm->type_aux == 0){   /* level 0 OFM */
      offset_header    = 4*14;
      offset_char_info = 4*(14+lh);
    } else {                   /* level 1 OFM: *** NOT SUPPORTED YET *** */
      offset_header    = 4*29;
      offset_char_info = 4*(29+lh);
    }
  } else {
    /* Traditional TeX Metric File */
    tfm->type        = METRIC_TYPE_TFM;
    tfm->type_aux    = 0;
    lh               = (int)READ_UINT2(fp);    
    offset_header    = 4*6;
    offset_char_info = 4*(6+lh);
  }

  if (tfm->type == METRIC_TYPE_OFM){
    tfm->begin_char  = READ_UINT4(fp);
    tfm->end_char    = READ_UINT4(fp);
    nw   = READ_UINT4(fp);
    nh   = READ_UINT4(fp);
    nd   = READ_UINT4(fp);

    ni   = READ_UINT4(fp);
    nl   = READ_UINT4(fp);
    nk   = READ_UINT4(fp);
    neng = READ_UINT4(fp);
    np   = READ_UINT4(fp);
    dir  = READ_UINT4(fp); 

    if (((signed)(tfm->begin_char-1) > (signed)tfm->end_char) ||
        (tfm->end_char > 65535)){
      vf_error = VF_ERR_INVALID_METRIC;
      return NULL;
    }
  } else {
    tfm->begin_char  = (int)READ_UINT2(fp); 
    tfm->end_char    = (int)READ_UINT2(fp);
    nw   = (UINT4)READ_UINT2(fp);
    nh   = (UINT4)READ_UINT2(fp);
    nd   = (UINT4)READ_UINT2(fp);

    ni   = (UINT4)READ_UINT2(fp);
    nl   = (UINT4)READ_UINT2(fp);
    nk   = (UINT4)READ_UINT2(fp);
    neng = (UINT4)READ_UINT2(fp);
    np   = (UINT4)READ_UINT2(fp);

    if (tfm->type == METRIC_TYPE_TFM){
      if (((signed)(tfm->begin_char-1) > (signed)tfm->end_char) ||
          (tfm->end_char > 255)){
        vf_error = VF_ERR_INVALID_METRIC;
        return NULL;
      }
    }
  }

  fseek(fp, offset_header, SEEK_SET);
  tfm->cs          = READ_UINT4(fp); 
  tfm->ds          = READ_UINT4(fp); 
  tfm->design_size = (double)(tfm->ds)/(double)(1<<20);
  
  nc  = tfm->end_char - tfm->begin_char + 1;
  nci = nc;
  if (tfm->type == METRIC_TYPE_OFM)
    nci *= 2;
  ci = (UINT4*)calloc(nci, sizeof(UINT4));
  w  = (INT4*)calloc(nw,  sizeof(UINT4));
  h  = (INT4*)calloc(nh,  sizeof(UINT4));
  d  = (INT4*)calloc(nd,  sizeof(UINT4));
  if ((ci == NULL) || (w == NULL) || (h == NULL) || (d == NULL)){
    err = VF_ERR_NO_MEMORY;
    goto Exit;
  }
  fseek(fp, offset_char_info, SEEK_SET);
  for (i = 0; i < nci; i++)
    ci[i] = READ_UINT4(fp);
  offset_param = ftell(fp) + 4*(nw + nh + nd + ni + nl + nk + neng);
  for (i = 0; i < nw; i++)
    w[i] = READ_INT4(fp);
  for (i = 0; i < nh; i++)
    h[i] = READ_INT4(fp);
  for (i = 0; i < nd; i++)
    d[i] = READ_INT4(fp);

  tfm->width  = (INT4*)calloc(nc, sizeof(INT4));
  tfm->height = (INT4*)calloc(nc, sizeof(INT4));
  tfm->depth  = (INT4*)calloc(nc, sizeof(INT4));
  if ((tfm->width == NULL) || (tfm->height == NULL) || (tfm->depth == NULL)){
    err = VF_ERR_NO_MEMORY;
    goto Exit;
  }
  bbxw = 0;
  bbxh = 0;
  xoff = 0;
  yoff = 0;
  if (tfm->type == METRIC_TYPE_OFM){
    for (i = 0; i < nc; i++){
      v = ci[2*i];
      tfm->depth[i]  = d[v & 0xff]; v >>= 8;
      tfm->height[i] = h[v & 0xff]; v >>= 8;
      tfm->width[i]  = w[v & 0xffff];
      if (bbxw < tfm->width[i])
	bbxw = tfm->width[i];
      if (bbxh < (tfm->height[i] + tfm->depth[i]))
	bbxh = tfm->height[i] + tfm->depth[i];
      if (yoff > -tfm->depth[i])
	yoff = -tfm->depth[i];
#if 0
      printf("** %.3f %.3f %.3f\n",
	     (double)tfm->width[i]/(double)(1<<20), 
	     (double)tfm->height[i]/(double)(1<<20), 
	     (double)tfm->depth[i]/(double)(1<<20));
#endif
    }
  } else {
    for (i = 0; i < nc; i++){
      v = ci[i] / 0x10000L;
      tfm->depth[i]  = d[v & 0xf];  v >>= 4;
      tfm->height[i] = h[v & 0xf];  v >>= 4;
      tfm->width[i]  = w[v & 0xff];
      if (bbxw < tfm->width[i])
	bbxw = tfm->width[i];
      if (bbxh < (tfm->height[i] + tfm->depth[i]))
	bbxh = tfm->height[i] + tfm->depth[i];
      if (yoff > -tfm->depth[i])
	yoff = -tfm->depth[i];
#if 0
      printf("** %.3f %.3f\n",
	     (double)tfm->height[i]/(double)(1<<20), 
	     (double)tfm->depth[i]/(double)(1<<20));
#endif
    }
  }
  tfm->font_bbx_w = tfm->design_size * ((double)bbxw / (double)(1<<20));
  tfm->font_bbx_h = tfm->design_size * ((double)bbxh / (double)(1<<20));
  tfm->font_bbx_xoff = tfm->design_size * ((double)xoff / (double)(1<<20));
  tfm->font_bbx_yoff = tfm->design_size * ((double)yoff / (double)(1<<20));

  if (tfm->type == METRIC_TYPE_JFM){
    fseek(fp, 4*(7+lh), SEEK_SET);
    tfm->ct_kcode = (unsigned int*)calloc(tfm->nt+1, sizeof(unsigned int));
    tfm->ct_ctype = (unsigned int*)calloc(tfm->nt+1, sizeof(unsigned int));
    if ((tfm->ct_kcode == NULL) || (tfm->ct_ctype == NULL)){
      err = VF_ERR_NO_MEMORY;
      goto Exit;
    }
    for (i = 0; i < tfm->nt; i++){
      v = READ_UINT4(fp);
      tfm->ct_kcode[i] = v/0x10000L;
      tfm->ct_ctype[i] = v%0x10000L;
    }
    tfm->ct_kcode[tfm->nt] = 0; /* sentinel */
    tfm->ct_ctype[tfm->nt] = 0;
  }

  fseek(fp, offset_param, SEEK_SET);
  tfm->slant = (double)READ_INT4(fp)/(double)(1<<20);

Exit:
  vf_free(ci);
  vf_free(w);
  vf_free(h);
  vf_free(d);

  if (err != 0){
    vf_tfm_free(tfm);
    vf_error = err;
    return NULL;
  }
  return tfm;
}


Glocal void
vf_tfm_free(TFM tfm)
{
  int  tfm_id;

  if (tfm == NULL)
    return;

  tfm_id = (tfm_table->get_id_by_obj)(tfm_table, tfm);
  if (tfm_id < 0)
    return;

  if ((tfm_table->unlink_by_id)(tfm_table, tfm_id) <= 0){
    vf_free(tfm->width);    
    vf_free(tfm->height);
    vf_free(tfm->depth);    
    vf_free(tfm->ct_kcode);
    vf_free(tfm->ct_ctype);
    vf_free(tfm);
  }
}


/*EOF*/
