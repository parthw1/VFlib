/*
 * pcf.c - a low level interface for PCF format fonts
 * by Hirotsugu Kakugawa
 *
 * 25 Apr 1997  Added multiple file extension feature.
 * 23 Jan 1998  VFlib 3.4  Changed API.
 * 21 Apr 1998  Deleted multiple file extension feature.
 * 17 Jun 1998  Support for 'font-directory' capability in font definition. 
 */
/*
 * Copyright (C) 1996-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */


Private VF_TABLE  pcf_table       = NULL;


Private int
PCF_Init(void)
{
  PCF_GetPCF(-1);

  if ((pcf_table = vf_table_create()) == NULL){
    vf_error = VF_ERR_NO_MEMORY;
    return -1;
  }

  return 0;
}


Private void   pcf_release(PCF);
Private int    pcf_char_index(PCF,long);
Private int    pcf_load_file(PCF);
Private PCF_TABLE  pcf_read_toc(FILE*,int*);
Private int          pcf_read_props(PCF,FILE*,PCF_TABLE,int);
Private int          pcf_read_metrics(PCF,FILE*,PCF_TABLE,int);
Private void           pcf_read_metric_item(FILE*,INT4,PCF_CHAR);
Private void           pcf_read_compressed_metric_item(FILE*,INT4,PCF_CHAR);
Private int          pcf_read_bitmaps(PCF,FILE*,PCF_TABLE,int);
Private int          pcf_read_ink_metrics(PCF,FILE*,PCF_TABLE,int);
Private int          pcf_read_encodings(PCF,FILE*,PCF_TABLE,int);
Private int          pcf_read_accel(PCF,FILE*,PCF_TABLE,int,INT4);
Private PCF_TABLE  pcf_seek_to_type(FILE*,PCF_TABLE,int,int);
Private int        pcf_type_index(PCF_TABLE,int,int);
Private void       pcf_bit_order_invert(unsigned char*,int);
Private void       pcf_swap_2byte(unsigned char*,int);
Private void       pcf_swap_4byte(unsigned char*,int);
Private int    pcf_skip_file(FILE*,long);
Private INT4   pcf_read_lsb4(FILE*);
Private INT4   pcf_read_int4(FILE*,INT4);
Private INT4   pcf_read_int2(FILE*,INT4);
Private INT4   pcf_read_int1(FILE*,INT4);
Private int    pcf_read_nbyte(FILE*,unsigned char*,int);

Private int
PCF_Open(char *font_file, SEXP fontdirs)
{
  char   *path_name, *uncomp_prog;
  int     pcf_id;
  PCF     pcf;

  path_name = vf_search_file(font_file, -1, NULL, FALSE, -1, fontdirs, 
			     default_compressed_ext, &uncomp_prog);
  if (path_name == NULL){
    vf_error = VF_ERR_NO_FONT_FILE;
    return -1;
  }

  if (pcf_debug('F')){
    printf("PCF Font File: %s ==> %s\n", font_file, path_name);
  }

  /* Check the cache here. (Never forget that the fontdir is 
   * not always the same. */
  if ((pcf_id = (pcf_table->get_id_by_key)(pcf_table, path_name, 
					   strlen(path_name)+1)) >= 0){
    vf_free(path_name);
    if ((pcf = PCF_GetPCF(pcf_id)) == NULL){
      fprintf(stderr, "VFlib internal error: in PCF_Open()\n");
      abort();
    }
    (pcf_table->link_by_id)(pcf_table, pcf_id);
    return pcf_id;
  }

  ALLOC_IF_ERR(pcf, struct s_pcf){
    vf_free(path_name);
    vf_error = VF_ERR_NO_MEMORY;
    return -1;
  }

  pcf->point_size   = -1;
  pcf->pixel_size   = -1;
  pcf->size         = -1;
  pcf->ascent       = -1;
  pcf->descent      = -1;
  pcf->dpi_x        = -1;
  pcf->dpi_y        = -1;
  pcf->nchars       = 0;
  pcf->char_table   = NULL;
  pcf->bitmap_block = NULL;
  pcf->encoding     = NULL;
  pcf->font_bbx_width  = 0;
  pcf->font_bbx_height = 0;
  pcf->font_bbx_xoff   = 0;
  pcf->font_bbx_yoff   = 0;
  pcf->path_name    = path_name;
  pcf->uncompress   = NULL;
  pcf->props        = NULL;

  if ((uncomp_prog != NULL) &&
      ((pcf->uncompress = vf_strdup(uncomp_prog)) == NULL)){
    vf_error = VF_ERR_NO_MEMORY;
    goto Error;
  }
  if ((pcf->props = vf_sexp_empty_list()) == NULL){
    vf_error = VF_ERR_NO_MEMORY;
    goto Error;
  }

  if (pcf_load_file(pcf) < 0)
    goto Error;

  if ((pcf_id = (pcf_table->put)(pcf_table, pcf, 
				 path_name, strlen(path_name)+1)) < 0){
    vf_error = VF_ERR_NO_MEMORY;
    goto Error;
  }

  PCF_SetPCF(pcf_id, pcf);

  return pcf_id;

Error:
  pcf_release(pcf);
  return -1;
}


Private void
PCF_Close(int pcf_id)
{
  PCF  pcf;

  if ((pcf = PCF_GetPCF(pcf_id)) == NULL){
    fprintf(stderr, "VFlib Internal error: PCF_Close()\n");
    vf_error = VF_ERR_INTERNAL;
    return;
  }
  if ((pcf_table->unlink_by_id)(pcf_table, pcf_id) > 0)
    return;

  pcf_release(pcf);
}


Private void
pcf_release(PCF pcf)
{
  if (pcf != NULL){
    vf_free(pcf->path_name);
    vf_free(pcf->uncompress);
    vf_free(pcf->char_table);
    vf_free(pcf->bitmap_block);
    vf_free(pcf->encoding);
    vf_sexp_free(&pcf->props);
    vf_free(pcf);
  }
  PCF_GetPCF(-1);
}



static int    pcf_file_pos;

Private int
pcf_load_file(PCF pcf)
{
  FILE      *fp;
  PCF_TABLE  tbl;
  int        has_bdf_accel, ntbl, val;

  pcf_file_pos = 0;
  if (pcf->uncompress == NULL){
    if ((fp = vf_fm_OpenBinaryFileStream(pcf->path_name)) == NULL){
      vf_error = VF_ERR_NO_FONT_FILE;
      return -1;
    } 
  } else {
#if 0
    printf("** PCF \"%s\", \"%s\"\n", pcf->path_name, pcf->uncompress);
#endif
    if ((fp = vf_open_uncompress_stream(pcf->path_name, 
					pcf->uncompress)) == NULL){
      vf_error = VF_ERR_UNCOMPRESS;
      return -1;
    }
  }

  val = -1;
  if ((tbl = pcf_read_toc(fp, &ntbl)) == NULL)
    goto Error;
  if (pcf_read_props(pcf, fp, tbl, ntbl) < 0)
    goto Error;
  if ((has_bdf_accel = pcf_type_index(tbl, ntbl, PCF_BDF_ACCELERATORS)) >= 0)
    if (pcf_read_accel(pcf, fp, tbl, ntbl, PCF_ACCELERATORS) < 0)
      goto Error;
  if (pcf_read_metrics(pcf, fp, tbl, ntbl) < 0)
    goto Error;
  if (pcf_read_bitmaps(pcf, fp, tbl, ntbl) < 0)
    goto Error;
  if (pcf_read_ink_metrics(pcf, fp, tbl, ntbl) < 0)
    goto Error;
  if (pcf_read_encodings(pcf, fp, tbl, ntbl) < 0)
    goto Error;
  if (has_bdf_accel > 0)
    if (pcf_read_accel(pcf, fp, tbl, ntbl, PCF_BDF_ACCELERATORS) < 0)
      goto Error;
  val = 0;
  
Error:
  vf_free(tbl);
  if (pcf->uncompress != NULL)
    vf_close_uncompress_stream(fp);
  fp = NULL;

  return val;
}


Private PCF_TABLE
pcf_read_toc(FILE *fp, int *ntbl)
{
  PCF_TABLE  tbl;
  INT4       pcf_version;
  int        i;

  pcf_version = pcf_read_lsb4(fp);
  if (pcf_version != PCF_FILE_VERSION){
    *ntbl = 0;
    return NULL;
  }
  if ((*ntbl = pcf_read_lsb4(fp)) < 0)
    return NULL;
  ALLOCN_IF_ERR(tbl, struct s_pcf_table, *ntbl)
    return NULL;
  for (i = 0; i < *ntbl; i++){
    tbl[i].type   = pcf_read_lsb4(fp);
    tbl[i].format = pcf_read_lsb4(fp);
    tbl[i].size   = pcf_read_lsb4(fp);
    tbl[i].offset = pcf_read_lsb4(fp);
  }
  return tbl;
}

Private int
pcf_read_props(PCF pcf, FILE *fp, PCF_TABLE tbl, int ntbl)
{
  int    i, pad, val;
  INT4   format, nprops;
  INT4  *prop_name  = NULL;
  char  *prop_isstr = NULL;
  INT4  *prop_value = NULL;
  char  *propstr    = NULL;
  INT4   propstr_size;
  char  *prop, *value, *p;
  char   charset_name[256], charset_enc[64], value_str[256];
  
  if (pcf_seek_to_type(fp, tbl, ntbl, PCF_PROPERTIES) == NULL)
    return -1;
  format = pcf_read_lsb4(fp);
  if (!PCF_FORMAT_MATCH(format, PCF_DEFAULT_FORMAT))
    return -1;

  val = -1;

  nprops = pcf_read_int4(fp, format);
  ALLOCN_IF_ERR(prop_name,  INT4, nprops)
    goto Error;
  ALLOCN_IF_ERR(prop_isstr, char, nprops)
    goto Error;
  ALLOCN_IF_ERR(prop_value, INT4, nprops)
    goto Error;    
  for (i = 0; i < nprops; i++){
    prop_name[i]  = pcf_read_int4(fp, format);
    prop_isstr[i] = pcf_read_int1(fp, format);
    prop_value[i] = pcf_read_int4(fp, format);
  }
  if ((i = (nprops % 4)) != 0){
    pad = 4 - i;
    pcf_skip_file(fp, (long)pad);
  }
  
  propstr_size = pcf_read_int4(fp, format);
  ALLOCN_IF_ERR(propstr, char, propstr_size+1)
    goto Error;    
  pcf_read_nbyte(fp, (unsigned char*)propstr, propstr_size);

  strcpy(charset_name, "");
  strcpy(charset_enc,  "");
  for (i = 0; i < nprops; i++){
    prop  = &propstr[prop_name[i]];
    value = &propstr[prop_value[i]];
    if (prop_isstr[i]){
      pcf->props = vf_sexp_alist_put(prop, value, pcf->props);
    } else {
      sprintf(value_str, "%ld", (long)prop_value[i]);
      pcf->props = vf_sexp_alist_put(prop, value_str, pcf->props);
    }
    if (pcf_debug('P')){
      if (prop_isstr[i])
	printf("PCF Prop %s: \"%s\"\n", prop, value);
      else  
	printf("PCF Prop %s: %ld\n", prop, (long)prop_value[i]);
    }
    if (STRCMP(prop, "CHARSET_REGISTRY") == 0){
      strncpy(charset_name, value, sizeof(charset_name)-sizeof(charset_enc));
    } else if (STRCMP(prop, "CHARSET_ENCODING") == 0){
      strncpy(charset_enc, value, sizeof(charset_enc));
    } else if (STRCMP(prop, "POINT_SIZE") == 0){
      if (prop_isstr[i])  sscanf(value, "%i", &pcf->pixel_size);
      else                pcf->point_size = (double)prop_value[i] / 10.0;
    } else if (STRCMP(prop, "PIXEL_SIZE") == 0){
      if (prop_isstr[i])  sscanf(value, "%i", &pcf->pixel_size);
      else                pcf->pixel_size = prop_value[i];
    } else if (STRCMP(prop, "FONT_ASCENT") == 0){
      if (prop_isstr[i])  sscanf(value, "%i", &pcf->ascent);
      else                pcf->ascent = prop_value[i];
    } else if (STRCMP(prop, "FONT_DESCENT") == 0){
      if (prop_isstr[i])  sscanf(value, "%i", &pcf->descent);
      else                pcf->descent = prop_value[i];
    } else if (STRCMP(prop, "RESOLUTION_X") == 0){
      if (prop_isstr[i])  sscanf(value, "%lf", &pcf->dpi_x);
      else                pcf->dpi_x = prop_value[i];
    } else if (STRCMP(prop, "RESOLUTION_Y") == 0){
      if (prop_isstr[i])  sscanf(value, "%lf", &pcf->dpi_y);
      else	          pcf->dpi_y = prop_value[i];
    } else if (STRCMP(prop, "SLANT") == 0){
      for (p = value; *p != '\0'; p++)
	*p = toupper(*p);
      pcf->slant = 0.0;
      if ((strcmp(value, "I") == 0) || (strcmp(value, "O") == 0)){
	pcf->slant = 0.17;
      } else if ((strcmp(value, "RI") == 0) || (strcmp(value, "RO") == 0)){
	pcf->slant = -0.17;
      }
    }
  }

  if ((strcmp(charset_enc, "") != 0) && ((strcmp(charset_enc, "0") != 0))){
    strcat(charset_name, "-");
    strcat(charset_name, charset_enc); 
  }

  if ((pcf->size = pcf->point_size) < 0)
    pcf->size = pcf->ascent + pcf->descent;
  if (pcf->dpi_x < 0)
    pcf->dpi_x = DEFAULT_DPI;
  if (pcf->dpi_y < 0)
    pcf->dpi_y = DEFAULT_DPI;

  val = 0;

Error:
  vf_free(prop_name);
  vf_free(prop_isstr);
  vf_free(prop_value);
  vf_free(propstr);

  return val;
}

Private int
pcf_read_metrics(PCF pcf, FILE *fp, PCF_TABLE tbl, int ntbl)
{
  INT4   format, nmetrics;
  int    i;

  if (pcf_seek_to_type(fp, tbl, ntbl, PCF_METRICS) == NULL)
    return -1;
  format = pcf_read_lsb4(fp);
  if (!PCF_FORMAT_MATCH(format, PCF_DEFAULT_FORMAT)
      && !PCF_FORMAT_MATCH(format, PCF_COMPRESSED_METRICS))
    return -1;
  if (PCF_FORMAT_MATCH(format, PCF_DEFAULT_FORMAT))
    nmetrics = pcf_read_int4(fp, format);
  else
    nmetrics = pcf_read_int2(fp, format);
  ALLOCN_IF_ERR(pcf->char_table, struct s_pcf_char, nmetrics)
    goto Error;
  for (i = 0; i < nmetrics; i++){
    PCF_CHAR  pch;
    pch = &pcf->char_table[i];
    if (PCF_FORMAT_MATCH(format, PCF_DEFAULT_FORMAT))
      pcf_read_metric_item(fp, format, pch);
    else
      pcf_read_compressed_metric_item(fp, format, pch);
    if (pcf_debug('M')){
      printf("PCF  rightSideBearing: %d\n", pch->rightSideBearing);
      printf("PCF  leftSideBearing: %d\n", pch->leftSideBearing);
      printf("PCF  ascent: %d\n", pch->ascent);
      printf("PCF  descent: %d\n", pch->descent);
      printf("PCF  characterWidth: %d\n", pch->characterWidth);
    }
    pch->bbx_width  = pch->rightSideBearing - pch->leftSideBearing;
    pch->bbx_height = pch->ascent + pch->descent;
    pch->off_x = pch->leftSideBearing;
    pch->off_y = -pch->descent;
    pch->mv_x  = pch->characterWidth;
    pch->mv_y  = 0;

    if (pch->bbx_width  > pcf->font_bbx_width)
      pcf->font_bbx_width  = pch->bbx_width;
    if (pch->bbx_height > pcf->font_bbx_height)
      pcf->font_bbx_height = pch->bbx_height;
    if (pch->off_x < pcf->font_bbx_xoff)
      pcf->font_bbx_xoff = pch->off_x;
    if (pch->off_y < pcf->font_bbx_yoff)
      pcf->font_bbx_yoff = pch->off_y;
  }
  if (pcf_debug('B')){
    printf("PCF FONT BOUNDINGBOX %d %d %d %d\n", 
	   pcf->font_bbx_width, pcf->font_bbx_height, 
	   pcf->font_bbx_xoff, pcf->font_bbx_yoff); 
  }
  pcf->nchars = nmetrics;
  return 0;

Error:  
  pcf->nchars = 0;
  return -1;
}
Private void
pcf_read_metric_item(FILE *fp, INT4 format, PCF_CHAR pch)
{
  pch->leftSideBearing  = pcf_read_int2(fp, format);
  pch->rightSideBearing = pcf_read_int2(fp, format);
  pch->characterWidth   = pcf_read_int2(fp, format);
  pch->ascent           = pcf_read_int2(fp, format);
  pch->descent          = pcf_read_int2(fp, format);
  pch->attributes       = pcf_read_int2(fp, format);
}
Private void
pcf_read_compressed_metric_item(FILE *fp, INT4 format, PCF_CHAR pch)
{
  pch->leftSideBearing  = pcf_read_int1(fp, format) - 0x80;
  pch->rightSideBearing = pcf_read_int1(fp, format) - 0x80;
  pch->characterWidth   = pcf_read_int1(fp, format) - 0x80;
  pch->ascent           = pcf_read_int1(fp, format) - 0x80;
  pch->descent          = pcf_read_int1(fp, format) - 0x80;
  pch->attributes       = 0;
}

Private int
pcf_read_bitmaps(PCF pcf, FILE *fp, PCF_TABLE tbl, int ntbl)
{
  INT4     format, nbitmaps;
  CARD4   *offsets = NULL;
  CARD4    bitmap_sizes[PCF_GLYPHPADOPTIONS];
  int      i, xsize;
  int      bitmap_block_size;

  if (pcf_seek_to_type(fp, tbl, ntbl, PCF_BITMAPS) == NULL)
    return -1;
  format = pcf_read_lsb4(fp);
  if (!PCF_FORMAT_MATCH(format, PCF_DEFAULT_FORMAT))
    return -1;
  if ((nbitmaps = pcf_read_int4(fp, format)) != pcf->nchars)
    return -1;
  if (pcf_debug('B')){
    printf("PCF %ld bitmaps\n", (long)nbitmaps);
  }

  ALLOCN_IF_ERR(offsets, CARD4, nbitmaps)
    goto Error;
  for (i = 0; i < nbitmaps; i++)
    offsets[i] = pcf_read_int4(fp, format);

  for (i = 0; i < PCF_GLYPHPADOPTIONS; i++)
    bitmap_sizes[i] = pcf_read_int4(fp, format);
  bitmap_block_size = bitmap_sizes[PCF_GLYPH_PAD_INDEX(format)];
  xsize = (bitmap_block_size > 0) ? bitmap_block_size : 1;
  if (pcf_debug('B')){
    printf("PCF Bitmaps: %d bytes\n", xsize);
  }
  ALLOCN_IF_ERR(pcf->bitmap_block, unsigned char, xsize)
    goto Error;
  pcf_read_nbyte(fp, pcf->bitmap_block, bitmap_block_size);
  
  if (PCF_BIT_ORDER(format) != PCF_MSB_FIRST)
    pcf_bit_order_invert(pcf->bitmap_block, bitmap_block_size);
  if (PCF_BYTE_ORDER(format) != PCF_MSB_FIRST){
    switch (PCF_SCAN_UNIT(format)){
    case 1:
      break;
    case 2:
      pcf_swap_2byte(pcf->bitmap_block, bitmap_block_size);
      break;
    case 4:
      pcf_swap_4byte(pcf->bitmap_block, bitmap_block_size);
      break;
    }
  }
  
  for (i = 0; i < nbitmaps; i++){
    PCF_CHAR     pch;
    int          w, h, pad;

    pch = &pcf->char_table[i];
    w = pch->rightSideBearing - pch->leftSideBearing;
    h = pch->ascent + pch->descent;
    pad = PCF_GLYPH_PAD(format);
    pch->bitmap = &pcf->bitmap_block[offsets[i]];
    pch->raster = ((w + 8*pad - 1)/(8 * pad)) * pad;

    if (pcf_debug('D')){
      struct   vf_s_bitmap bm;
      printf("PCF Bitmap #%d\n", i);
      bm.bbx_width = w;
      bm.bbx_height = h;
      bm.bitmap =  pch->bitmap;
      bm.raster =  pch->raster;
      bm.off_x = bm.off_y = bm.mv_x = bm.mv_y = 0;
      VF_DumpBitmap(&bm);
    }      
  }

  vf_free(offsets);
  return 0;

Error:
  vf_free(offsets);
  vf_free(pcf->char_table);
  pcf->char_table = NULL;
  vf_free(pcf->bitmap_block);
  pcf->bitmap_block = NULL;
  return -1;
}

Private int
pcf_read_ink_metrics(PCF pcf, FILE *fp, PCF_TABLE tbl, int ntbl)
{
  return 0;  /* ignore */
}

Private int
pcf_read_encodings(PCF pcf, FILE *fp, PCF_TABLE tbl, int ntbl)
{
  INT4     format;
  INT2     i, jth, ne;

  if (pcf_seek_to_type(fp, tbl, ntbl, PCF_BDF_ENCODINGS) == NULL)
    goto Error;
  format = pcf_read_lsb4(fp);
  if (!PCF_FORMAT_MATCH(format, PCF_DEFAULT_FORMAT))
    goto Error;

  pcf->firstCol    = pcf_read_int2(fp, format);
  pcf->lastCol     = pcf_read_int2(fp, format);
  pcf->firstRow    = pcf_read_int2(fp, format);
  pcf->lastRow     = pcf_read_int2(fp, format);
  pcf->defaultCh   = pcf_read_int2(fp, format);
  pcf->nencodings
    = (pcf->lastCol - pcf->firstCol + 1) * (pcf->lastRow - pcf->firstRow + 1);
  ne = (pcf->nencodings > 0) ? pcf->nencodings : 1;
  ALLOCN_IF_ERR(pcf->encoding, INT2, ne)
    goto Error;
  for (i = 0; i < pcf->nencodings; i++){
    if ((jth = pcf_read_int2(fp, format)) == 0xffff)
      pcf->encoding[i] = -1;
    else 
      pcf->encoding[i] = jth;
  }
  return 0;

Error:
  vf_free(pcf->encoding);
  pcf->encoding   = NULL;
  pcf->nencodings = 0;
  return -1;
}

Private int
pcf_read_accel(PCF pcf, FILE *fp, PCF_TABLE tbl, int ntbl, INT4 type)
{
  INT4     format;

  if (pcf_seek_to_type(fp, tbl, ntbl, type) == NULL)
    goto Error;
  format = pcf_read_lsb4(fp);
  if (!PCF_FORMAT_MATCH(format, PCF_DEFAULT_FORMAT)
      && !PCF_FORMAT_MATCH(format, PCF_ACCEL_W_INKBOUNDS))
    goto Error;

  pcf_read_int1(fp, format); /* noOverlap */
  pcf_read_int1(fp, format); /* constantMetrics */
  pcf_read_int1(fp, format); /* terminalFont */
  pcf_read_int1(fp, format); /* constantWidth */
  pcf_read_int1(fp, format); /* inkInside */
  pcf_read_int1(fp, format); /* inkMetrics */
  pcf_read_int1(fp, format); /* drawDirection */
                                          /* anamorphic */
                                          /* cachable */
  pcf_read_int1(fp, format); /* (alignment) */
  pcf->ascent  = pcf_read_int4(fp, format); /* fontAscent */
  pcf->descent = pcf_read_int4(fp, format); /* fontDescent */
  pcf_read_int4(fp, format); /* maxOverlap */
  /* Metrics minbounds, maxbounds, (ink_minbounds, and ink_maxbounds)
     come here */ 
  return 0;

Error:
  return -1;
}
 
Private PCF_TABLE
pcf_seek_to_type(FILE *fp, PCF_TABLE tbl, int ntbl, int type)
{
  int  i;
  
  if ((i = pcf_type_index(tbl, ntbl, type)) < 0)
    return NULL;

  if (pcf_file_pos > tbl[i].offset)
    return NULL;
  if (pcf_skip_file(fp, tbl[i].offset - pcf_file_pos) < 0)
    return NULL;

  return &tbl[i];
}

Private int
pcf_type_index(PCF_TABLE tbl, int ntbl, int type)
{
  int   i;

  for (i = 0; i < ntbl; i++){
    if (tbl[i].type == type)
      return i;
  }
  return -1;
}


Private void
pcf_bit_order_invert(unsigned char *bitmap, int size)
{
  unsigned char  c1, c2, *p;
  static unsigned char  inv_tbl[] = {
    0x0, /*0000=>0000*/   0x8, /*0001=>1000*/
    0x4, /*0010=>0100*/   0xc, /*0011=>1100*/
    0x2, /*0100=>0010*/   0xa, /*0101=>1010*/
    0x6, /*0110=>0110*/   0xe, /*0111=>1110*/
    0x1, /*1000=>0001*/   0x9, /*1001=>1001*/
    0x5, /*1010=>0101*/   0xd, /*1011=>1101*/
    0x3, /*1100=>0011*/   0xb, /*1101=>1011*/
    0x7, /*1110=>0111*/   0xf, /*1111=>1111*/
  };

  for (p = bitmap; size > 0; --size, p++){
    c1 = inv_tbl[(*p&0xf0) >> 4];
    c2 = inv_tbl[(*p&0x0f)];
    *p = (c2<<4)|c1;
  }
}

Private void
pcf_swap_2byte(unsigned char *bitmap, int size)
{
  unsigned char  *p, p0;

  for (p = bitmap; size > 0; p += 2, size -= 2){
    p0 = *(p+0);
    *(p+0) = *(p+1);
    *(p+1) = p0;
  }
}

Private void
pcf_swap_4byte(unsigned char *bitmap, int size)
{
  unsigned char *p, p0, p1;

  for (p = bitmap; size > 0; p += 4, size -= 4){
    p0 = *(p+0);
    p1 = *(p+1);
    *(p+0) = *(p+3);
    *(p+1) = *(p+2);
    *(p+2) = p1;
    *(p+3) = p0;
  }
}


Private int
pcf_skip_file(FILE *fp, long nskip)
{
  for ( ; nskip > 0; nskip--){
    if (getc(fp) < 0)
      return -1;
    pcf_file_pos++;
  }
  return 0;
}

Private INT4
pcf_read_lsb4(FILE *fp)
{
  INT4  n;
  
  n  = (INT4)getc(fp);
  n += (INT4)getc(fp) * 0x00000100;
  n += (INT4)getc(fp) * 0x00010000;
  n += (INT4)getc(fp) * 0x01000000;
  pcf_file_pos += 4;
  return n;
}

Private INT4
pcf_read_int4(FILE *fp, INT4 format)
{
  INT4  n;

  if (PCF_BYTE_ORDER(format) == PCF_MSB_FIRST){
    n  = (INT4)getc(fp) * 0x01000000;
    n += (INT4)getc(fp) * 0x00010000;
    n += (INT4)getc(fp) * 0x00000100;
    n += (INT4)getc(fp);
  } else { 
    n  = (INT4)getc(fp);
    n += (INT4)getc(fp) * 0x00000100;
    n += (INT4)getc(fp) * 0x00010000;
    n += (INT4)getc(fp) * 0x01000000;
  }
  pcf_file_pos += 4;
  return n;
}

Private INT4
pcf_read_int2(FILE *fp, INT4 format)
{
  INT4  n;

  if (PCF_BYTE_ORDER(format) == PCF_MSB_FIRST){
    n  = (INT4)getc(fp) * 0x100;
    n += (INT4)getc(fp);
  } else { 
    n  = (INT4)getc(fp);
    n += (INT4)getc(fp) * 0x100;
  }
  pcf_file_pos += 2;
  return n;
}

Private INT4
pcf_read_int1(FILE *fp, INT4 format)
{
  pcf_file_pos++;
  return (INT4)getc(fp);
}

Private int
pcf_read_nbyte(FILE *fp, unsigned char *buff, int size)
{
  unsigned char *p;
  int  c;

  for (p = buff; size > 0; size--, p++){
    if ((c = getc(fp)) < 0)
      return -1;
    *p = (unsigned char) c;
    pcf_file_pos++;
  }
  return 0;
}


Private PCF_CHAR
PCF_GetBitmap(int pcf_id, long code_point)
{
  int            index;
  PCF            pcf;
 
  if ((pcf = PCF_GetPCF(pcf_id)) == NULL){
    fprintf(stderr, "VFlib internal error: BDF_GetBitmap()\n");
    vf_error = VF_ERR_INTERNAL;
    return NULL;
  }

  if ((index = pcf_char_index(pcf, code_point)) < 0){
    vf_error = VF_ERR_ILL_CODE_POINT;
    return NULL;
  }
  return &pcf->char_table[index];
}

Private int
pcf_char_index(PCF pcf, long code_point)
{
  int  char_row, char_col, ncol, i;
  
  char_row = code_point / 0x100;
  char_col = code_point % 0x100;
  if ((char_row < pcf->firstRow) || (pcf->lastRow < char_row)
      || (char_col < pcf->firstCol) || (pcf->lastCol < char_col))
    return -1;

  ncol = (pcf->lastCol - pcf->firstCol + 1);
  i = (char_col - pcf->firstCol) + ncol * (char_row - pcf->firstRow);
  return pcf->encoding[i];
}


Private char*
PCF_GetProp(PCF pcf, char *name)
{
  SEXP  v;
  char  *r;

#if 0
  char  *prop_value, *val, str[160];

  if (strcmp(name, "DEFAULT_CHAR") == 0){
    sprintf(str, "%d", pcf->defaultCh);
    return vf_strdup(str);
  } else if (strcmp(name, "FONT_ASCENT") == 0){
    sprintf(str, "%d", pcf->ascent);
    return vf_strdup(str);
  } else if (strcmp(name, "FONT_DESCENT") == 0){
    sprintf(str, "%d", pcf->descent);
    return vf_strdup(str);
  }
#endif

  if ((v = vf_sexp_assoc(name, pcf->props)) == NULL)
    return NULL;
  if ((r = vf_strdup(vf_sexp_get_cstring(vf_sexp_cadr(v)))) == NULL){
    vf_error = VF_ERR_NO_MEMORY;
    return NULL;
  }

  return r;     /* CALLER MUST RELEASE THIS STRING LATER */
}


Private PCF_CHAR
PCF_GetPCFChar(PCF pcf, long code_point)
{
  int  index;

  if ((index = pcf_char_index(pcf, code_point)) < 0){
    vf_error = VF_ERR_ILL_CODE_POINT;
    return NULL;
  }
  return &pcf->char_table[index];
}


/*EOF*/
