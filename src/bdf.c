/*
 * bdf.c - low level bdf file interface 
 * by Hirotsugu Kakugawa
 *
 *  25 Apr 1997  Added multiple file extension feature.
 *  20 Jan 1998  VFlib 3.4  Changed API.
 *  21 Apr 1998  Deleted multiple file extension feature.
 *  17 Jun 1998  Support for 'font-directory' capability in font definition. 
 */
/*
 * Copyright (C) 1996-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */


Private VF_TABLE       bdf_table       = NULL;


Private int
BDF_Init(void)
{
  static int init_flag = 0;

  if (init_flag == 0){
    init_flag = 1;
    BDF_GetBDF(-1);
    if ((bdf_table = vf_table_create()) == NULL){
      vf_error = VF_ERR_NO_MEMORY;
      return -1;
    }
  }

  return 0;
}


Private void      bdf_release(BDF bdf);
Private int       bdf_char_index(BDF,long);
Private int       bdf_load_file(BDF);
Private BDF_CHAR  read_bitmap(BDF_CHAR,FILE*);
Private int       bdf_sort_index(BDF,int,int);
Private int       bdf_partition(BDF,int,int);

Private int
BDF_Open(char *font_file, SEXP fontdirs)
{
  char   *path_name, *uncomp_prog;
  int     bdf_id;
  BDF     bdf;

  path_name = vf_search_file(font_file, -1, NULL, FALSE, -1, fontdirs, 
			     default_compressed_ext, &uncomp_prog);
  if (path_name == NULL){
    vf_error = VF_ERR_NO_FONT_FILE;
    return -1;
  }

  if (bdf_debug('F')){
    printf("BDF Font File: %s ==> %s\n", font_file, path_name);
  }

  /* Check the cache here. (Never forget that the fontdir is 
   * not always the same. */
  if ((bdf_id = (bdf_table->get_id_by_key)(bdf_table, path_name, 
					   strlen(path_name)+1)) >= 0){
    (bdf_table->link_by_id)(bdf_table, bdf_id);
    vf_free(path_name);
    return bdf_id;
  }


  ALLOC_IF_ERR(bdf, struct s_bdf){
    vf_error = VF_ERR_NO_MEMORY;
    vf_free(path_name);
    return -1;
  }

  bdf->point_size   = -1;
  bdf->pixel_size   = -1;
  bdf->size         = -1;
  bdf->dpi_x        = -1;
  bdf->dpi_y        = -1;
  bdf->nchars       = 0;
  bdf->char_table   = NULL;
  bdf->char_table_x = NULL;
  bdf->path_name    = path_name;
  bdf->uncompress   = NULL;
  bdf->props        = NULL;

  if ((uncomp_prog != NULL) &&
      ((bdf->uncompress = vf_strdup(uncomp_prog)) == NULL)){
    vf_error = VF_ERR_NO_MEMORY;
    goto Error;
  }
  if ((bdf->props = vf_sexp_empty_list()) == NULL){
    vf_error = VF_ERR_NO_MEMORY;
    goto Error;
  }

  if (bdf_load_file(bdf) < 0)
    goto Error;

  if ((bdf_id = (bdf_table->put)(bdf_table, bdf,
				 path_name, strlen(path_name)+1)) < 0){
    vf_error = VF_ERR_NO_MEMORY;
    goto Error;
  }

  BDF_SetBDF(bdf_id, bdf);

  return bdf_id;

Error:
  bdf_release(bdf);
  return -1;
}


Private void
BDF_Close(int bdf_id)
{
  BDF  bdf;

  if ((bdf = BDF_GetBDF(bdf_id)) == NULL){
    fprintf(stderr, "VFlib internal error: BDF_Close()\n");
    vf_error = VF_ERR_INTERNAL;
    return;
  }
  if ((bdf_table->unlink_by_id)(bdf_table, bdf_id) > 0){
    return;
  }

  bdf_release(bdf);
}


Private void
bdf_release(BDF bdf)
{
  int  ch;

  if (bdf != NULL){
    vf_free(bdf->path_name);
    vf_free(bdf->uncompress);
    if (bdf->char_table != NULL){
      for (ch = 0; ch < bdf->nchars; ch++)
	vf_free(bdf->char_table[ch].bitmap);
    }
    vf_free(bdf->char_table);
    vf_free(bdf->char_table_x);
    vf_sexp_free(&bdf->props);
    vf_free(bdf);
  }
  BDF_GetBDF(-1);
}


Private int
bdf_load_file(BDF bdf)
{
  FILE   *fp;
  char   linebuf[BUFSIZ], prop_string[160], *name;
  char   charset_name[256], charset_enc[256], charset[256], *p;
  long   code_point, last_ch;
  int    ch_index, need_sorting, nchars, i;
  int    have_fontboundingbox;

  if (bdf->uncompress == NULL){
#ifdef WIN32
    if ((fp = vf_fm_OpenBinaryFileStream(bdf->path_name)) == NULL){
# else
    if ((fp = vf_fm_OpenTextFileStream(bdf->path_name)) == NULL){
#endif
      vf_error = VF_ERR_NO_FONT_FILE;
      return -1;
    } 
  } else {
    if ((fp = vf_open_uncompress_stream(bdf->path_name, 
					bdf->uncompress)) == NULL){
      vf_error = VF_ERR_UNCOMPRESS;
      return -1;
    } 
  }

  strcpy(charset_name, "");
  strcpy(charset_enc,  "");
  bdf->char_table   = NULL;
  bdf->char_table_x = NULL;
  have_fontboundingbox = 0;
  bdf->font_bbx_width  = 0;
  bdf->font_bbx_height = 0;
  bdf->font_bbx_xoff   = 0;
  bdf->font_bbx_yoff   = 0;
  bdf->ascent  = 0;
  bdf->descent = 0;

  if (bdf_debug('R'))
    printf(">> BDF reading header\n");

  for (;;){
    if (fgets(linebuf, sizeof(linebuf), fp) == NULL){
      vf_error = VF_ERR_ILL_FONT_FILE;
      goto Unexpected_Error;
    }
    { int l = strlen(linebuf);
      if ((l > 0) && (linebuf[l-1] == '\n'))
	linebuf[l-1] = '\0';
    }
    if (strncmp(linebuf, "ENDPROPERTIES", 13) == 0)
      break;
    if (strncmp(linebuf, "STARTFONT", 9) == 0)
      continue;
    if (strncmp(linebuf, "COMMENT", 7) == 0)
      continue;

#if 0
    { int  x;
      for (x = strlen(linebuf)-1; x >= 0; x--){
	switch (linebuf[x]){
	case '\n':
	case '\r':
	  linebuf[x] = '\0';
	}
      }
    }
#endif

    {
      char *prop_name, *prop_value, *p, *p0, c0;

      prop_name = linebuf;
      for (p = linebuf; (c0 = *p) != '\0'; p++)
	if (isspace((int)c0))
	  break;
      p0 = p;
      *p = '\0';
      if (c0 != '\0'){
	p++;
	while (isspace((int)(*p)))
	  p++;
      }
      if (*p == '\0'){
	prop_value = "";
      } else {
	prop_value = p;
	if (prop_value[0] == '"'){
	  prop_value = &prop_value[1];
	  prop_value[strlen(prop_value)-1] = '\0';
	}
	bdf->props = vf_sexp_alist_put(prop_name, prop_value, bdf->props);
	if (bdf_debug('P'))
	  printf(">> BDF Prop \"%s\" = \"%s\"\n", prop_name, prop_value);
      }
      *p0 = c0;
    }

#if 0
    printf("*** %s\n", linebuf);
#endif
    if (strncmp(linebuf, "SIZE", 4) == 0){
      sscanf(linebuf, "%*s%lf%lf%lf",
	     &bdf->point_size, &bdf->dpi_x, &bdf->dpi_y);
      bdf->size = bdf->point_size;
    } else if (strncmp(linebuf, "FONTBOUNDINGBOX", 15) == 0){
      sscanf(linebuf, "%*s%d%d%d%d",
	     &bdf->font_bbx_width, &bdf->font_bbx_height,
	     &bdf->font_bbx_xoff, &bdf->font_bbx_yoff);
	have_fontboundingbox = 1;
    } else if (strncmp(linebuf, "CHARSET_REGISTRY", 16) == 0){
      sscanf(linebuf, "%*s%s", prop_string);
      if (prop_string[0] == '"'){ /* ignore `"' */
	prop_string[strlen(prop_string)] = '\0'; 
	name = &prop_string[1];
      } else
	name = prop_string;
      strncpy(charset_name, name, sizeof(charset_name));
    } else if (strncmp(linebuf, "CHARSET_ENCODING", 16) == 0){
      sscanf(linebuf, "%*s%s", prop_string);
      if (prop_string[0] == '"'){/* ignore `"' */
	prop_string[strlen(prop_string)] = '\0'; 
	name = &prop_string[1];          
      } else
	name = prop_string;
      strncpy(charset_enc, name, sizeof(charset_enc));
    } else if (strncmp(linebuf, "PIXEL_SIZE", 10) == 0){
      sscanf(linebuf, "%*s%d", &bdf->pixel_size);
    } else if (strncmp(linebuf, "POINT_SIZE", 10) == 0){
      sscanf(linebuf, "%*s%lf", &bdf->point_size);
      bdf->point_size = bdf->point_size / 10.0;
    } else if (strncmp(linebuf, "RESOLUTION_X", 12) == 0){
      sscanf(linebuf, "%*s%lf", &bdf->dpi_x);
    } else if (strncmp(linebuf, "RESOLUTION_Y", 12) == 0){
      sscanf(linebuf, "%*s%lf", &bdf->dpi_y);
    } else if (strncmp(linebuf, "FONT_ASCENT", 11) == 0){
      sscanf(linebuf, "%*s%d", &bdf->ascent);
    } else if (strncmp(linebuf, "FONT_DESCENT", 12) == 0){
      sscanf(linebuf, "%*s%d", &bdf->descent);
    } else if (strncmp(linebuf, "SLANT", 5) == 0){
      sscanf(linebuf, "%*s%s", prop_string);
      if (prop_string[0] == '"'){  /* ignore `"' */
	prop_string[strlen(prop_string)] = '\0'; 
	name = &prop_string[1];          
      } else
	name = prop_string;
      for (p = name; *p != '\0'; p++)
	*p = toupper(*p);
      bdf->slant = 0.0;
      if ((strcmp(name, "I") == 0) || (strcmp(name, "O") == 0)){
	bdf->slant = 0.17;
      } else if ((strcmp(name, "RI") == 0) || (strcmp(name, "RO") == 0)){
	bdf->slant = -0.17;
      }
    }
  }
  if ((strcmp(charset_enc, "") != 0) && ((strcmp(charset_enc, "0") != 0)))
    sprintf(charset, "%s-%s", charset_name, charset_enc); 
  else
    sprintf(charset, "%s", charset_name); 
  if (bdf_debug('C'))
    printf(">> BDF Charset (ID=%d) %s\n", bdf->charset, charset);

  if (bdf->dpi_x < 0)
    bdf->dpi_x = DEFAULT_DPI;
  if (bdf->dpi_y < 0)
    bdf->dpi_y = DEFAULT_DPI;

  for (;;){
    if (fgets(linebuf, sizeof(linebuf), fp) == NULL){
      vf_error = VF_ERR_ILL_FONT_FILE;
      goto Unexpected_Error;
    }
    if (strncmp(linebuf, "CHARS", 5) == 0){
      sscanf(linebuf, "%*s%d", &bdf->nchars);
      if (bdf->nchars < 0){
	vf_error = VF_ERR_ILL_FONT_FILE;
	goto Unexpected_Error;
      }
      vf_free(bdf->char_table);
      ALLOCN_IF_ERR(bdf->char_table, struct s_bdf_char, bdf->nchars){
	vf_error = VF_ERR_NO_MEMORY;
	goto Unexpected_Error;
      }
      vf_free(bdf->char_table_x);
      ALLOCN_IF_ERR(bdf->char_table_x, long, bdf->nchars){
	vf_error = VF_ERR_NO_MEMORY;
	goto Unexpected_Error;
      }
      for (ch_index = 0; ch_index < bdf->nchars; ch_index++)
	bdf->char_table_x[ch_index] = ch_index;
      break;
    }
  }

  if (bdf_debug('R'))
    printf(">> BDF reading chars\n");

  last_ch = -1L;
  nchars = 0;
  need_sorting = 0;
  for (ch_index = 0; ch_index < bdf->nchars; ch_index++){
NextChar:
    for (;;){
      if (fgets(linebuf, sizeof(linebuf), fp) == NULL){
	vf_error = VF_ERR_ILL_FONT_FILE;
	goto Unexpected_Error;
      }
      if (strncmp(linebuf, "ENDFONT", 7) == 0)
	goto EndFont;
      if (strncmp(linebuf, "STARTCHAR", 9) == 0)
	break;
    }
    bdf->char_table[ch_index].f_offset   = -1;
    bdf->char_table[ch_index].bbx_width  = -1;
    bdf->char_table[ch_index].bbx_height = -1;
    bdf->char_table[ch_index].off_x      = 0;
    bdf->char_table[ch_index].off_y      = 0;
    bdf->char_table[ch_index].mv_x       = 0;
    bdf->char_table[ch_index].mv_y       = 0;
    bdf->char_table[ch_index].bitmap     = NULL;
    for (;;){
      if (fgets(linebuf, sizeof(linebuf), fp) == NULL){
	vf_error = VF_ERR_ILL_FONT_FILE;
	goto Unexpected_Error;
      }
      if (strncmp(linebuf, "ENDCHAR", 7) == 0)
	break;
      if (strncmp(linebuf, "ENCODING", 8) == 0){
	sscanf(linebuf, "%*s%ld", &code_point);
	if (code_point < 0L)
	  goto NextChar;
	bdf->char_table[ch_index].code_point = code_point;
#if 0
	if ((code_point % 0x21) == 0)
	  printf("BDF Reading Char: Encoding=0x%x\n", code_point);
#endif
      } else if (STRCMP(linebuf, "BBX") == 0){
	sscanf(linebuf, "%*s%d%d%d%d", 
	       &bdf->char_table[ch_index].bbx_width,
	       &bdf->char_table[ch_index].bbx_height,
	       &bdf->char_table[ch_index].off_x, 
	       &bdf->char_table[ch_index].off_y);
	if (have_fontboundingbox == 0){
	  if (bdf->font_bbx_width < bdf->char_table[ch_index].bbx_width)
	    bdf->font_bbx_width = bdf->char_table[ch_index].bbx_width;
	  if (bdf->font_bbx_height < bdf->char_table[ch_index].bbx_height)
	    bdf->font_bbx_height = bdf->char_table[ch_index].bbx_height;
	}
      } else if (strncmp(linebuf, "DWIDTH", 6) == 0){
	sscanf(linebuf, "%*s%d%d",
	       &bdf->char_table[ch_index].mv_x,
	       &bdf->char_table[ch_index].mv_y);
      } else if (strncmp(linebuf, "BITMAP", 6) == 0){
	if ((bdf->uncompress != NULL) 
	    || (bdf->nchars < 512)){  /* LOAD BITMAP */
	  bdf->char_table[ch_index].f_offset = 0L;
	  if (read_bitmap(&bdf->char_table[ch_index], fp) == NULL)
	    goto Unexpected_Error;
	} else {                       /* LAZY BITMAP LOADING */
	  bdf->char_table[ch_index].f_offset = (long)ftell(fp);
	  bdf->char_table[ch_index].bitmap   = NULL;  
	  for (i = 1; i <= bdf->char_table[ch_index].bbx_height; i++){
	    if (fgets(linebuf, sizeof(linebuf), fp) == NULL){
	      vf_error = VF_ERR_ILL_FONT_FILE;
	      goto Unexpected_Error;
	    }
	  }
	}
      } else {
	;  /* ignore other keywords */
      }
    } /* end char */
    if (   (bdf->char_table[ch_index].code_point < 0L)
	|| (bdf->char_table[ch_index].f_offset   < 0)
	|| (bdf->char_table[ch_index].bbx_width  < 0)
	|| (bdf->char_table[ch_index].bbx_height < 0) ){
      vf_error = VF_ERR_ILL_FONT_FILE;
      break;
    }
    nchars++;
    if (bdf->char_table[ch_index].code_point < last_ch)
      need_sorting = 1;
    last_ch = bdf->char_table[ch_index].code_point;
  }

EndFont:
  bdf->nchars = nchars;
  if (need_sorting == 1){ /* for binary search */
    if (bdf_debug('R'))
      printf(">> BDF sorting\n");
    bdf_sort_index(bdf, 0, ch_index-1);
  } else {
    if (bdf_debug('R'))
      printf(">> BDF need not sorting\n");
  }
#if 0
  for (i = 0; i <= bdf->nchars; i++){
    printf("** %d 0x%x\n",  
	   i, bdf->char_table[bdf->char_table_x[i]].code_point);
  }
#endif

  if ((bdf->uncompress != NULL) && (fp != NULL))
    vf_close_uncompress_stream(fp);

  if (bdf_debug('R'))
    printf(">> BDF done\n");

  return 0; 


Unexpected_Error:
  if (bdf->uncompress != NULL)
    vf_close_uncompress_stream(fp);
  if (bdf->char_table != NULL){
    for (ch_index = 0; ch_index < bdf->nchars; ch_index++)
      vf_free(bdf->char_table[ch_index].bitmap);
  }
  vf_free(bdf->char_table);
  vf_free(bdf->char_table_x);
  bdf->char_table = NULL;
  bdf->char_table_x = NULL;
  
  return -1;
}

#if 0

/* Shell sort */
Private int
bdf_sort_index(BDF bdf, int x, int y)
{
  int    gap, i, j, temp, len;
  long     *chx;
  BDF_CHAR  cht;

  cht = bdf->char_table;
  chx = bdf->char_table_x;

  len = bdf->nchars;
  for (gap = len/2; gap > 0; gap = gap / 2){
    for (i = gap; i < len; i++){
      for (j = i - gap; 
	   (j >= 0) && (cht[chx[j]].code_point > cht[chx[j+gap]].code_point);
	   j -= gap){
	temp = chx[j];
	chx[j] = chx[j+gap];
	chx[j+gap] = temp;
      }
    }
  }

  return 0;
}

#else

/* Quick sort */ 
Private int
bdf_sort_index(BDF bdf, int x, int y)
{
  int      z;

Loop:

  if (x < y){
    z = bdf_partition(bdf, x, y);

#if 0
    printf("** %d(%d) %d(%d) %d(%d)\n", 
	   x, bdf->char_table[bdf->char_table_x[x]].code_point,
	   z, bdf->char_table[bdf->char_table_x[z]].code_point,
	   y, bdf->char_table[bdf->char_table_x[y]].code_point);
#endif

    if (x < z-1){
      (void) bdf_sort_index(bdf, x,   z-1);
    }

    if (z+1 < y){
      /*(void) bdf_sort_index(bdf, z+1, y);*/
      x = z+1; 
      goto Loop;
    }

  }
  return 0;
}

Private int
bdf_partition(BDF bdf, int x, int y)
{
  long      t;
  int       i, p, tmp;
  BDF_CHAR  cht;
  long     *chx;

  cht = bdf->char_table;
  chx = bdf->char_table_x;

  t = cht[chx[x]].code_point;
  i = x+1; 
  p = x;

  for (;;){
    if (y < i)
      return p;
    if (cht[chx[i]].code_point < t){
      /* swap( d[i], d[p+1]) */
      tmp = chx[i];
      chx[i] = chx[p+1];
      chx[p+1] = tmp;
      i++; p++;
    } else {
      i++;
    }
  }
}
#endif

Private BDF_CHAR
BDF_GetBitmap(int bdf_id, long code_point)
{
  int            index;
  FILE           *fp;
  BDF            bdf;
  BDF_CHAR       bdf_char;
 
  if ((bdf = BDF_GetBDF(bdf_id)) == NULL){
    fprintf(stderr, "VFlib internal error: BDF_GetBitmap()\n");
    vf_error = VF_ERR_INTERNAL;
    return NULL;
  }

  if ((index = bdf_char_index(bdf, code_point)) < 0){
    vf_error = VF_ERR_ILL_CODE_POINT;
    return NULL;
  }
  bdf_char = &bdf->char_table[index];

  if (bdf_char->bitmap != NULL)
    return bdf_char;

#ifdef WIN32
  if ((fp = vf_fm_OpenBinaryFileStream(bdf->path_name)) == NULL){
#else
  if ((fp = vf_fm_OpenTextFileStream(bdf->path_name)) == NULL){
#endif
    /* --- font file is lost (maybe) */
    vf_error = VF_ERR_NO_FONT_FILE;
    return NULL;
  }
  fseek(fp, bdf_char->f_offset, SEEK_SET);
  return read_bitmap(bdf_char, fp);
}

#define X_TO_D(c)     ((isxdigit((int)(c)))?(Xc_To_Dec_Tbl[c-0x30]):16)
Private int  Xc_To_Dec_Tbl[] = { /* (BDF files are encoded by ASCII) */
       /* +0 +1 +2 +3 +4 +5 +6 +7 +8 +9 +A +B +C +D +E +F */
  /*30*/   0, 1, 2, 3, 4, 5, 6, 7, 8, 9,-1,-1,-1,-1,-1,-1, /* 0,1,2,3,... */
  /*40*/  -1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1, /* @,a,b,c,... */
  /*50*/  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 
  /*60*/  -1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1};/* `,A,B,C,... */

Private BDF_CHAR
read_bitmap(BDF_CHAR bdf_char, FILE *fp)
{
  int            bm_size, h, i;
  char           linebuf[(2*2048)/8];       /* Is this really enough? */
  unsigned char  ch1, ch2, *bmp, *lbp;

  bdf_char->raster = (bdf_char->bbx_width+7)/8;
  bm_size          = bdf_char->raster * bdf_char->bbx_height;
  if ((bdf_char->bitmap = (unsigned char*)calloc(1, bm_size)) == NULL){
    vf_error = VF_ERR_NO_MEMORY;
    return NULL;
  }
  bmp = bdf_char->bitmap;
  for (h = 0; h < bdf_char->bbx_height; h++){
    if (fgets(linebuf, sizeof(linebuf), fp) == NULL){
      vf_free(bdf_char->bitmap);
      bdf_char->bitmap = NULL;
      vf_error = VF_ERR_ILL_FONT_FILE;
      return NULL;
    }
    for (i = 0, lbp = (unsigned char *)linebuf; i < bdf_char->raster; i++){
      ch1 = *(lbp++);
      ch2 = *(lbp++);
      *(bmp++) = X_TO_D(ch1)*16 + X_TO_D(ch2);
    }
  }
  return bdf_char;
}

Private int
bdf_char_index(BDF bdf, long code_point)
{
  int   hi, lo, m, x1, x2;

  x1 = bdf->char_table_x[0];
  x2 = bdf->char_table_x[bdf->nchars-1];
  if ((code_point < bdf->char_table[x1].code_point)
      || (bdf->char_table[x2].code_point < code_point))
    return -1;
  
  /* binary search */
  lo = 0;
  hi = bdf->nchars;
  if (lo >= hi)
    return -1;
  while (lo < hi){
    m = (lo+hi)/2;   /*printf("lo=%d  hi=%d  m=%d\n", lo, hi, m);*/
    if (bdf->char_table[bdf->char_table_x[m]].code_point < code_point)
      lo = m+1;
    else 
      hi = m;
  }
  if (bdf->char_table[bdf->char_table_x[hi]].code_point != code_point)
    return -1;

  return bdf->char_table_x[hi];
}


Private char*
BDF_GetProp(BDF bdf, char *name)
{
  SEXP  v;
  char  *r;

  if ((v = vf_sexp_assoc(name, bdf->props)) == NULL)
    return NULL;
  if ((r = vf_strdup(vf_sexp_get_cstring(vf_sexp_cadr(v)))) == NULL){
    vf_error = VF_ERR_NO_MEMORY;
    return NULL;
  }

  return r;     /* CALLER MUST RELEASE THIS STRING LATER */
}


Private BDF_CHAR
BDF_GetBDFChar(BDF bdf, long code_point)
{
  int  index;

  if ((index = bdf_char_index(bdf, code_point)) < 0){
    vf_error = VF_ERR_ILL_CODE_POINT;
    return NULL;
  }
  return &bdf->char_table[index];
}


/*EOF*/
