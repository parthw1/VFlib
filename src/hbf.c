/*
 * hbf.c - a low lebel interface for HBF format fonts
 * by Hirotsugu Kakugawa
 *
 * 26 Mar 1997  Fixed bugs.
 * 26 Jan 1998  VFlib 3.4  Changed API.
 * 21 Apr 1998  Deleted multiple file extension feature.
 * 17 Jun 1998  Support for 'font-directory' capability in font definition. 
 */
/*
 * Copyright (C) 1997-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */

Private VF_TABLE  hbf_table       = NULL;


Private int
HBF_Init(void)
{
  HBF_GetHBF(-1);

  if ((hbf_table = vf_table_create()) == NULL){
    vf_error = VF_ERR_NO_MEMORY;
    return -1;
  }

  return 0;
}


Private int  hbf_load_hbf_file(HBF, SEXP);
Private int  hbf_load_bitmap(HBF,int);
Private void hbf_release(HBF);
Private int  hbf_subindex(HBF,long,long);
Private unsigned char* hbf_find_bitmap(HBF,long);

Private int
HBF_Open(char *font_file, SEXP fontdirs)
{
  char   *path_name, *uncomp_prog;
  int     hbf_id;
  HBF     hbf;

  path_name = vf_search_file(font_file, -1, NULL, FALSE, -1, fontdirs, 
			     default_compressed_ext, &uncomp_prog);
  if (path_name == NULL){
    vf_error = VF_ERR_NO_FONT_FILE;
    return -1;
  }
#if 0
  printf("** HBF Font File: %s ==> %s\n", font_file, path_name);
#endif

  if ((hbf_id = (hbf_table->get_id_by_key)(hbf_table, path_name, 
					   strlen(path_name)+1)) >= 0){
    (hbf_table->link_by_id)(hbf_table, hbf_id);
    return hbf_id;
  }

  ALLOC_IF_ERR(hbf, struct s_hbf){
    vf_error = VF_ERR_NO_MEMORY;
    return -1;
  }

  hbf->nchars     = 0;
  hbf->point_size = -1;
  hbf->pixel_size = -1;
  hbf->size       = -1;
  hbf->dpi_x      = -1;
  hbf->dpi_y      = -1;
  hbf->slant      = 0;
  hbf->ascent     = 0;
  hbf->descent    = 0;
  hbf->font_bbx_width  = 0;
  hbf->font_bbx_height = 0; 
  hbf->font_bbx_xoff   = 0;
  hbf->font_bbx_yoff   = 0;
  hbf->path_name  = NULL;
  hbf->props      = NULL;
  hbf->uncompress = NULL;

  if ((hbf->path_name = vf_strdup(path_name)) == NULL){
    vf_error = VF_ERR_NO_MEMORY;
    goto Error;
  }
  if ((uncomp_prog != NULL) &&
      ((hbf->uncompress = vf_strdup(uncomp_prog)) == NULL)){
    vf_error = VF_ERR_NO_MEMORY;
    goto Error;
  }
  if ((hbf->props = vf_sexp_empty_list()) == NULL){
    vf_error = VF_ERR_NO_MEMORY;
    goto Error;
  }

  if (hbf_load_hbf_file(hbf, fontdirs) < 0)
    goto Error;

#if 0
  printf("** HBF: charset:%d, %fpt, %fdpi(x) %fdpi(y) (%d chars)\n", 
	 hbf->charset, hbf->point_size, hbf->dpi_x, hbf->dpi_y, hbf->nchars);
#endif

  if ((hbf_id = (hbf_table->put)(hbf_table, hbf, 
				 path_name, strlen(path_name)+1)) < 0){
    vf_error = VF_ERR_NO_MEMORY;
    goto Error;
  }

  HBF_SetHBF(hbf_id, hbf);

  return hbf_id;

Error:
  hbf_release(hbf);
  return -1;
}

Private int
hbf_load_hbf_file(HBF hbf, SEXP fontdirs)
{
  FILE   *fp;
  char   linebuf[160], prop_string[160], *name, *file_path, *p;
  char   charset_name[80], charset_name2[80], charset_enc2[5], charset[90];
  char   *uncomp_prog;
  int    index, r, i, n;

  hbf->byte2_range_start = NULL;
  hbf->byte2_range_end = NULL;
  hbf->byte2_ranges = 0;
  hbf->code_range_start = NULL;
  hbf->code_range_end = NULL;
  hbf->code_range_offset = NULL;
  hbf->code_range_bitmap_file_paths = NULL;
  hbf->code_range_bitmaps = NULL;
  hbf->code_range_offset = NULL;
  hbf->code_range_bitmap_uncompresser = NULL;

  if (hbf->uncompress == NULL){
    if ((fp = vf_fm_OpenTextFileStream(hbf->path_name)) == NULL){
      vf_error = VF_ERR_NO_FONT_FILE;
      return -1;
    } 
  } else {
    if ((fp = vf_open_uncompress_stream(hbf->path_name, 
					hbf->uncompress)) == NULL){
      vf_error = VF_ERR_UNCOMPRESS;
      return -1;
    } 
  }

  /* Perse: Char set, pixel size, ... */
  strcpy(charset_name, "");
  strcpy(charset_name2, "");
  strcpy(charset_enc2,  "");
  for (;;){
    if (fgets(linebuf, sizeof(linebuf), fp) == NULL){
      vf_error = VF_ERR_ILL_FONT_FILE;
      goto Unexpected_Error;
    }
    if (STRCMP(linebuf, "HBF_STARTFONT") == 0)
      continue;
    if (STRCMP(linebuf, "COMMENT") == 0)
      continue;
    if (STRCMP(linebuf, "ENDPROPERTIES") == 0)
      break;

    { int  x;
      for (x = strlen(linebuf)-1; x >= 0; x--){
	switch (linebuf[x]){
	case '\n':
	case '\r':
	  linebuf[x] = '\0';
	}
      }
    }

    { char  *prop_name, *prop_value, *p, *p0, c0;

      prop_name = linebuf;
      for (p = linebuf; (c0 = *p) != '\0'; p++)
	if (isspace((int)c0))
	  break;
      p0 = p;
      *p = '\0';
      if (c0 != '\0'){
	p++;
	while (isspace((int)*p))
	  p++;
      }
      if (*p == '\0'){
	prop_value = NULL;
      } else {
	prop_value = p;
	if (prop_value[0] == '"'){
	  prop_value = &prop_value[1];
	  prop_value[strlen(prop_value)-1] = '\0';
	}
	hbf->props = vf_sexp_alist_put(prop_name, prop_value, hbf->props);
	if (hbf_debug('P'))
	  printf("HBF Prop \"%s\" = \"%s\"\n", prop_name, prop_value);
      }
      *p0 = c0;
    }

    if (STRCMP(linebuf, "SIZE") == 0){
      sscanf(linebuf, "%*s%lf%lf%lf",
	     &hbf->point_size, &hbf->dpi_x, &hbf->dpi_y);
      hbf->size = hbf->point_size;
    } else if ((STRCMP(linebuf, "HBF_BITMAP_BOUNDING_BOX") == 0)
	       || (STRCMP(linebuf, "FONTBOUNDINGBOX") == 0)){
      sscanf(linebuf, "%*s%d%d%d%d", 
	     &hbf->font_bbx_width, &hbf->font_bbx_height, 
	     &hbf->font_bbx_xoff, &hbf->font_bbx_yoff);
      hbf->ascent = hbf->font_bbx_height + hbf->font_bbx_yoff;
      hbf->descent = -hbf->font_bbx_yoff;
    } else if (STRCMP(linebuf, "HBF_CODE_SCHEME") == 0){
      p = &linebuf[strlen("HBF_CODE_SCHEME")];
      while (isspace((int)*p))
	p++;
      for (i = 0;  p[i] != '\0'; i++)
	if (!isprint((int)p[i]))
	  break;
      p[i] = '\0';
      strcpy(charset_name, p);
    } else if (STRCMP(linebuf, "CHARSET_REGISTRY") == 0){
      sscanf(linebuf, "%*s%s", prop_string);
      name = &prop_string[1];          /* ignore `"' */
      name[strlen(name)-1] = '\0';
      strncpy(charset_name2, name, sizeof(charset_name));
    } else if (STRCMP(linebuf, "CHARSET_ENCODING") == 0){
      sscanf(linebuf, "%*s%s", prop_string);
      name = &prop_string[1];          /* ignore `"' */
      name[strlen(name)-1] = '\0';
      strncpy(charset_enc2, name, sizeof(charset_enc2));
    } else if (STRCMP(linebuf, "RESOLUTION_X") == 0){
      sscanf(linebuf, "%*s%lf", &hbf->dpi_x);
    } else if (STRCMP(linebuf, "RESOLUTION_Y") == 0){
      sscanf(linebuf, "%*s%lf", &hbf->dpi_y);
    } else if (STRCMP(linebuf, "PIXEL_SIZE") == 0){
      sscanf(linebuf, "%*s%d", &hbf->pixel_size);
    } else if (STRCMP(linebuf, "POINT_SIZE") == 0){
      sscanf(linebuf, "%*s%lf", &hbf->point_size);
      hbf->point_size = hbf->point_size / 10.0;
    } else if (STRCMP(linebuf, "SLANT") == 0){
      sscanf(linebuf, "%*s%s", prop_string);
      name = &prop_string[1];          /* ignore `"' */
      name[strlen(name)-1] = '\0';
      for (p = name; *p != '\0'; p++)
	*p = toupper(*p);
      hbf->slant = 0.0;
      if ((strcmp(name, "I") == 0) || (strcmp(name, "O") == 0)){
	hbf->slant = 0.17;
      } else if ((strcmp(name, "RI") == 0) || (strcmp(name, "RO") == 0)){
	hbf->slant = -0.17;
      }
    }
  }
  if (strcmp(charset_name, "") != 0){
    sprintf(charset, "%s", charset_name); 
  } else {
    if ((strcmp(charset_enc2, "") != 0) && ((strcmp(charset_enc2, "0") != 0)))
      sprintf(charset, "%s-%s", charset_name2, charset_enc2); 
    else
      sprintf(charset, "%s", charset_name2); 
  }
#if 0
  printf("** HBF font file charset (ID=%d) '%s'\n", hbf->charset, charset);
  printf("*1 %f %f %f\n",
	 hbf->point_size, hbf->dpi_x, hbf->dpi_y);
  printf("*2 %d %d %d %d\n",
	 hbf->font_bbx_width, hbf->font_bbx_height,
	 hbf->font_bbx_xoff, hbf->font_bbx_yoff);
#endif

  if (hbf->point_size < 0)
    hbf->point_size = DEFAULT_POINT_SIZE;
  if (hbf->pixel_size < 0)
    hbf->pixel_size = DEFAULT_PIXEL_SIZE;
  if (hbf->dpi_x < 0)
    hbf->dpi_x = DEFAULT_DPI;
  if (hbf->dpi_y < 0)
    hbf->dpi_y = DEFAULT_DPI;

  /* parse: CHARS */
  for (;;){
    if (fgets(linebuf, sizeof(linebuf), fp) == NULL){
      vf_error = VF_ERR_ILL_FONT_FILE;
      goto Unexpected_Error;
    }
    if (STRCMP(linebuf, "CHARS") == 0){
      sscanf(linebuf, "%*s%d", &hbf->nchars);
      if (hbf->nchars < 0){
	vf_error = VF_ERR_ILL_FONT_FILE;
	goto Unexpected_Error;
      }
      break;
    }
  }

  /* Parse:  HBF_START_BYTE_2_RANES ... HBF_END_BYTE_2_RANES */
  hbf->byte2_ranges = 0;
  for (;;){
    if (fgets(linebuf, sizeof(linebuf), fp) == NULL){
      vf_error = VF_ERR_ILL_FONT_FILE;
      goto Unexpected_Error;
    }
    if (STRCMP(linebuf, "HBF_START_BYTE_2_RANGES") == 0){
      sscanf(linebuf, "%*s%d", &hbf->byte2_ranges);
      if (hbf->byte2_ranges <= 0){
	vf_error = VF_ERR_ILL_FONT_FILE;
	goto Unexpected_Error;
      }
      ALLOCN_IF_ERR(hbf->byte2_range_start, int, hbf->byte2_ranges){
	vf_error = VF_ERR_NO_MEMORY;
	goto Unexpected_Error;
      }
      ALLOCN_IF_ERR(hbf->byte2_range_end, int, hbf->byte2_ranges){
	vf_error = VF_ERR_NO_MEMORY;
	goto Unexpected_Error;
      }
      break;
    } 
  }
  hbf->n_byte2 = 0;
  for (r = 0; r < hbf->byte2_ranges; r++){
    if (fgets(linebuf, sizeof(linebuf), fp) == NULL){
      vf_error = VF_ERR_ILL_FONT_FILE;
      goto Unexpected_Error;
    }
    if (STRCMP(linebuf, "HBF_END_BYTE_2_RANGES") == 0){
      vf_error = VF_ERR_ILL_FONT_FILE;
      goto Unexpected_Error;
    } else if (STRCMP(linebuf, "HBF_BYTE_2_RANGE") == 0){
      sscanf(linebuf, "%*s%i-%i",
	     &(hbf->byte2_range_start[r]), 
	     &(hbf->byte2_range_end[r]));
      hbf->n_byte2 
	+= (hbf->byte2_range_end[r] - hbf->byte2_range_start[r] + 1);
    } 
  }

  for (i = 0; i < 256; i++)
    hbf->byte2_index[i] = -1;
  index = 0;
  for (r = 0; r < hbf->byte2_ranges; r++){
    for (i = hbf->byte2_range_start[r]; i <= hbf->byte2_range_end[r]; i++)
      hbf->byte2_index[i] = index++;
  }

  /* Parse:  HBF_START_CODE_RANES ... HBF_END_CODE_RANES */
  for (;;){
    if (fgets(linebuf, sizeof(linebuf), fp) == NULL){
      vf_error = VF_ERR_ILL_FONT_FILE;
      goto Unexpected_Error;
    }
    if (STRCMP(linebuf, "HBF_START_CODE_RANGES") == 0){
      sscanf(linebuf, "%*s%d", &hbf->code_ranges);
      if (hbf->code_ranges <= 0){
	vf_error = VF_ERR_ILL_FONT_FILE;
	goto Unexpected_Error;
      }
      ALLOCN_IF_ERR(hbf->code_range_start, long, hbf->code_ranges){
	vf_error = VF_ERR_NO_MEMORY;
	goto Unexpected_Error;
      }
      ALLOCN_IF_ERR(hbf->code_range_end, long, hbf->code_ranges){
	vf_error = VF_ERR_NO_MEMORY;
	goto Unexpected_Error;
      }
      ALLOCN_IF_ERR(hbf->code_range_bitmaps, 
		    unsigned char**, hbf->code_ranges){
	vf_error = VF_ERR_NO_MEMORY;
	goto Unexpected_Error;
      }
      ALLOCN_IF_ERR(hbf->code_range_offset, long, hbf->code_ranges){
	vf_error = VF_ERR_NO_MEMORY;
	goto Unexpected_Error;
      }
      ALLOCN_IF_ERR(hbf->code_range_bitmap_file_paths, 
		    char*, hbf->code_ranges){
	vf_error = VF_ERR_NO_MEMORY;
	goto Unexpected_Error;
      }
      ALLOCN_IF_ERR(hbf->code_range_bitmap_uncompresser, 
		    char*, hbf->code_ranges){
	vf_error = VF_ERR_NO_MEMORY;
	goto Unexpected_Error;
      }
      break;
    } 
  }

  for (r = 0; r < hbf->code_ranges; ){
    if (fgets(linebuf, sizeof(linebuf), fp) == NULL){
      vf_error = VF_ERR_ILL_FONT_FILE;
      goto Unexpected_Error;
    }
    if (STRCMP(linebuf, "HBF_END_CODE_RANGES") == 0){
      vf_error = VF_ERR_ILL_FONT_FILE;
      goto Unexpected_Error;
    } else if (STRCMP(linebuf, "HBF_CODE_RANGE") == 0){
      sscanf(linebuf, "%*s%li-%li%s%li",
	     &(hbf->code_range_start[r]),
	     &(hbf->code_range_end[r]),
	     prop_string,
	     &(hbf->code_range_offset[r]));
#if 0
      printf(">>%s", linebuf);
      printf("   %lx -- %lx,  %s,  %ld\n", 
	     hbf->code_range_start[r], hbf->code_range_end[r],
	     prop_string, hbf->code_range_offset[r]);
#endif
      file_path = NULL;
      if (fontdirs != NULL){
	file_path 
	  = vf_search_file(prop_string, -1, NULL, FALSE, -1, fontdirs, 
			   default_compressed_ext, &uncomp_prog);
      }
      if (file_path == NULL){
	vf_error = VF_ERR_NO_FONT_FILE;
	goto Unexpected_Error;
      }
      if ((hbf->code_range_bitmap_file_paths[r] 
	   = vf_strdup(file_path)) == NULL){
	vf_error = VF_ERR_NO_MEMORY;
	goto Unexpected_Error;
      } 
#if 0
      printf("** HBF Bitmap File: %s\n", hbf->code_range_bitmap_file_paths[r]);
#endif
      n = hbf_subindex(hbf, hbf->code_range_end[r], 
		       hbf->code_range_start[r]) + 1;
      ALLOCN_IF_ERR(hbf->code_range_bitmaps[r], unsigned char*, n){
	vf_error = VF_ERR_NO_MEMORY;
	goto Unexpected_Error;
      }
      for (i = 0; i < n; i++)
	hbf->code_range_bitmaps[r][i] = NULL;

      if (uncomp_prog == NULL){
	hbf->code_range_bitmap_uncompresser[r] = NULL;
      } else {
	if ((hbf->code_range_bitmap_uncompresser[r] 
	     = vf_strdup(uncomp_prog)) == NULL){
	  vf_error = VF_ERR_NO_MEMORY;
	  goto Unexpected_Error;
	}
	if (hbf_load_bitmap(hbf, r) < 0){
	  vf_error = VF_ERR_ILL_FONT_FILE;
	  goto Unexpected_Error;
	}
      }

      r++;
    } 
  }

  if ((uncomp_prog != NULL) && (fp != NULL))
    vf_close_uncompress_stream(fp);
  return 0; 

Unexpected_Error:
  if (uncomp_prog != NULL)
    vf_close_uncompress_stream(fp);
  hbf_release(hbf);
  return -1;
}


Private void
hbf_release(HBF hbf)
{
  int  i, n, r;

  if (hbf != NULL){
    vf_free(hbf->path_name);
    vf_free(hbf->uncompress);
    vf_free(hbf->byte2_range_start);
    vf_free(hbf->byte2_range_end);
    vf_free(hbf->code_range_start);
    vf_free(hbf->code_range_end);
    if (hbf->code_range_bitmap_file_paths != NULL)
      for (r = 0; r < hbf->byte2_ranges; r++)
	vf_free(hbf->code_range_bitmap_file_paths[r]);
    vf_free(hbf->code_range_bitmap_file_paths);
    if (hbf->code_range_bitmap_uncompresser != NULL)
      for (r = 0; r < hbf->byte2_ranges; r++)
	vf_free(hbf->code_range_bitmap_uncompresser[r]);
    vf_free(hbf->code_range_bitmap_uncompresser);
    if (hbf->code_range_bitmaps != NULL){
      for (r = 0; r < hbf->byte2_ranges; r++){
	if (hbf->code_range_bitmaps[r] == NULL)
	  continue;
	n = hbf_subindex(hbf, hbf->code_range_end[r], hbf->code_range_end[r]);
	for (i = 0; i <= n; i++)
	  vf_free(hbf->code_range_bitmaps[r][i]);
	vf_free(hbf->code_range_bitmaps[r]);
      }
    }
    vf_free(hbf->code_range_offset);
    vf_sexp_free(&hbf->props);
    vf_free(hbf);
  }
}

Private void
HBF_Close(int hbf_id)
{
  HBF  hbf;

  if ((hbf = HBF_GetHBF(hbf_id)) == NULL){
    fprintf(stderr, "VFlib Internal error: HBF_Close()\n");
    abort();
  }

  if ((hbf_table->unlink_by_id)(hbf_table, hbf_id) <= 0)
    hbf_release(hbf);
}


Private HBF_CHAR
HBF_GetBitmap(int hbf_id, long code_point)
{
  HBF         hbf;

  hbf = HBF_GetHBF(hbf_id);
  return HBF_GetHBFChar(hbf, code_point);
}

Private unsigned char*
hbf_find_bitmap(HBF hbf, long code_point)
{
  int            subindex, bmsize, r, i;
  long           offset;
  FILE           *fp;
  unsigned char  *p;

  subindex = 0;
  for (r = 0; r < hbf->code_ranges; r++){
    if ((hbf->code_range_start[r] <= code_point) 
	&& (code_point <= hbf->code_range_end[r])){
      break;
    }
  }
  if (r == hbf->code_ranges){
    vf_error = VF_ERR_ILL_CODE_POINT;
    return NULL;
  }

  if ((subindex = hbf_subindex(hbf, code_point,
			       hbf->code_range_start[r])) < 0){
    vf_error = VF_ERR_ILL_CODE_POINT;
    return NULL;
  }
  if (hbf->code_range_bitmaps[r][subindex] != NULL)
    return hbf->code_range_bitmaps[r][subindex];

#if 0
  printf("  Opening bitmap file: %s\n", 
	 hbf->code_range_bitmap_file_paths[r]);
#endif
  if (hbf->code_range_bitmap_uncompresser[r] != NULL){
    fprintf(stderr, "VFlib Internal error: HBF_GetBitmap()\n");
    abort();
  }

  if ((fp = vf_fm_OpenBinaryFileStream(hbf->code_range_bitmap_file_paths[r])) 
      == NULL){
    vf_error = VF_ERR_NO_FONT_FILE;
    return NULL;
  } 
  bmsize = ((hbf->font_bbx_width+7)/8) * hbf->font_bbx_height; 
  offset = hbf->code_range_offset[r] + subindex * bmsize;
  fseek(fp, offset, SEEK_SET); 
#if 0
  printf("  code point: 0x%04x, index: %d, offset: %d\n",
	 code_point, subindex, offset);
#endif

  ALLOCN_IF_ERR(hbf->code_range_bitmaps[r][subindex], unsigned char, bmsize)
    return NULL;
  p = &hbf->code_range_bitmaps[r][subindex][0];
  for (i = 0; i < bmsize; i++)
    *(p++) = getc(fp);

  return hbf->code_range_bitmaps[r][subindex];
}

Private int
hbf_load_bitmap(HBF hbf, int r)
{
  int            bmsize, index, i, j;
  unsigned char  *p;
  FILE           *fp;

  if (hbf->code_range_bitmap_uncompresser[r] == NULL){
    fprintf(stderr, "VFlib fatal: hbf_load_bitmap()\n");
    abort();
  }

  fp = vf_open_uncompress_stream(hbf->code_range_bitmap_file_paths[r], 
				 hbf->code_range_bitmap_uncompresser[r]);
  if (fp == NULL){
    vf_error = VF_ERR_UNCOMPRESS;
    return -1;
  }

  if ((index = hbf_subindex(hbf, hbf->code_range_end[r],
			    hbf->code_range_start[r])) < 0){
    fprintf(stderr, "VFlib fatal: hbf_load_bitmap()\n");
    abort();
  }

  fseek(fp, hbf->code_range_offset[r], SEEK_SET);
  for (i = 0; i <= index; i++){
    bmsize = ((hbf->font_bbx_width+7)/8) * hbf->font_bbx_height; 
    ALLOCN_IF_ERR(hbf->code_range_bitmaps[r][i], unsigned char, bmsize)
      return -1;
    p =  &hbf->code_range_bitmaps[r][i][0];
    for (j = 0; j < bmsize; j++)
      *(p++) = getc(fp);
  }

  vf_close_uncompress_stream(fp);
  return 0;
}

Private int
hbf_subindex(HBF hbf, long code_point, long base)
{
  int  code_hi, code_lo, base_hi, base_lo;
  int  subindex, r;

  code_hi = code_point / 256;  code_lo = code_point % 256;
  base_hi = base / 256;        base_lo = base % 256;

  /**
  printf(" code hi:%x lo:%02x,  base hi:%d lo:%d,  %d %d\n",
	 code_hi, code_lo, base_hi, base_lo, 
	 hbf->byte2_index[code_lo], hbf->byte2_index[base_lo]);
  **/
  if (base_hi == code_hi){
    if ((hbf->byte2_index[code_lo] < 0) || (hbf->byte2_index[base_lo] < 0))
      return -1;
    subindex = hbf->byte2_index[code_lo] - hbf->byte2_index[base_lo];
  } else {   /* base_hi != code_hi */
    /* in base_hi */
    subindex = hbf->n_byte2 - hbf->byte2_index[base_lo];
    /* in base_hi+1, ..., code_hi-1 */
    for (r = base_hi+1; r <= code_hi-1; r++)
      subindex += hbf->n_byte2;
    /* in code_hi */
    subindex += hbf->byte2_index[code_lo];
  }
#if 0
  printf("hbf_subindex: index:%d, base:0x%lx, code:0x%x\n",
    subindex, base, code_point);
#endif
  return subindex;
}


Private char*
HBF_GetProp(HBF hbf, char *name)
{
  SEXP  v;
  char  *r;

  if ((v = vf_sexp_assoc(name, hbf->props)) == NULL)
    return NULL;
  if ((r = vf_strdup(vf_sexp_get_cstring(vf_sexp_cadr(v)))) == NULL){
    vf_error = VF_ERR_NO_MEMORY;
    return NULL;
  }

  return r;     /* CALLER MUST RELEASE THIS STRING LATER */
}


Private HBF_CHAR
HBF_GetHBFChar(HBF hbf, long code_point)
{
  static struct s_hbf_char  hbf_char;

  hbf_char.bbx_width  = hbf->font_bbx_width;
  hbf_char.bbx_height = hbf->font_bbx_height;
  hbf_char.off_x = hbf->font_bbx_xoff;
  hbf_char.off_y = hbf->font_bbx_yoff;
  hbf_char.mv_x = hbf->font_bbx_width;
  hbf_char.mv_y = 0;
  hbf_char.bitmap = hbf_find_bitmap(hbf, code_point);
  hbf_char.raster = (hbf->font_bbx_width+7)/8;
  return &hbf_char;
}


/*EOF*/
