/*
 * zeit.c - 'Syotai Kurabu' font interface
 * by Hirotsugu Kakugawa
 *
 *  3 Dec 1996  First version.
 * 10 Dec 1996  Changed for VFlib version 3.1
 * 12 Dec 1996  Eliminated "do" capability.
 * 26 Feb 1997  Added 'query_font_type'.
 *  7 Aug 1997  VFlib 3.3  Changed API.
 * 27 Jan 1998  VFlib 3.4  Changed API.
 * 18 Oct 2001    Fixed memory leaks.
 *
 */
/*
 * Copyright (C) 1996-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */

#include  "zeit.h"

static  VF_TABLE  zeit_table    = NULL;



Private int
ZEIT_Init(void)
{
  ZEIT_GetZEIT(-1);

  if ((zeit_table = vf_table_create()) == NULL){
    vf_error = VF_ERR_NO_MEMORY;
    return -1;
  }

  return 0;
}


Private void      zeit_release(ZEIT zeit);
Private int       zeit_make_header(char*,int,char**,long**,long**,int);


Private int
ZEIT_Open(char *font_name)
{
  ZEIT  zeit;
  int   zeit_id;

  if ((zeit_id = (zeit_table->get_id_by_key)(zeit_table, font_name, 
					     strlen(font_name)+1)) >= 0){
    (zeit_table->link_by_id)(zeit_table, zeit_id);
    return zeit_id;
  }

  ALLOC_IF_ERR(zeit, struct s_zeit){
    vf_error = VF_ERR_NO_MEMORY;
    return -1;
  }
  zeit->path_name1 = NULL;
  zeit->ol_size1   = NULL;
  zeit->ol_offset1 = NULL;
  zeit->path_name2 = NULL;
  zeit->ol_size2   = NULL;
  zeit->ol_offset2 = NULL;

  if (zeit_make_header(font_name, 1, &zeit->path_name1,
		       &zeit->ol_size1, &zeit->ol_offset1, 1) < 0)
    goto Error;
  if (zeit_make_header(font_name, 2, &zeit->path_name2,
		       &zeit->ol_size2, &zeit->ol_offset2, 2) < 0)
    goto Error;
  
  if ((zeit_id = (zeit_table->put)(zeit_table, zeit, 
				   font_name, strlen(font_name)+1)) < 0){
    vf_error = VF_ERR_NO_MEMORY;
    goto Error;
  }

  ZEIT_SetZEIT(zeit_id, zeit);

  return zeit_id;

Error:
  zeit_release(zeit);
  return -1;
}


Private void
ZEIT_Close(int zeit_id)
{
  ZEIT  zeit;

  if ((zeit = ZEIT_GetZEIT(zeit_id)) == NULL){
    fprintf(stderr, "VFlib internal error: ZEIT_Close()\n");
    vf_error = VF_ERR_INTERNAL;
    return;
  }
  if ((zeit_table->unlink_by_id)(zeit_table, zeit_id) > 0)
    return;
  zeit_release(zeit);
}


Private void
zeit_release(ZEIT zeit)
{
  if (zeit != NULL){
    vf_free(zeit->path_name1);
    vf_free(zeit->path_name2);
    vf_free(zeit->ol_offset1);
    vf_free(zeit->ol_offset2);
    vf_free(zeit->ol_size1);
    vf_free(zeit->ol_size2);
    vf_free(zeit);
  }
}


Private int   zeit_read_header(long**,long**,char*);

Private int            zeit_correct_size(long*,long*,FILE*,int);
Private int            zeit_code2c(int);
Private int            zeit_read_1byte(FILE*);
Private unsigned long  zeit_read_4bytes(FILE*);
Private void           zeit_init_bit_stream(FILE*);
Private int            zeit_read_10bits(void);
Private void           zeit_seek(FILE*,long);
Private long           zeit_current_pos(FILE*);


Private int
zeit_make_header(char *font_name, int ext, char **path_namep,
		 long **ol_sizep, long **ol_offsetp, int type)
{
  char    *e;
  char    fname[MAXPATHLEN];
  int     i;
  SEXP    s;

  *path_namep = NULL;
  *ol_offsetp = NULL;
  *ol_sizep   = NULL;

  for (s = default_extensions; vf_sexp_consp(s); s = vf_sexp_cdr(s)){
    e = vf_sexp_get_cstring(vf_sexp_car(s));
    sprintf(fname, "%s%s%d", font_name, e, ext);
    if ((*path_namep
	 = vf_search_file(fname, -1, NULL, FALSE, -1,
			  default_fontdirs, NULL, NULL)) != NULL)
      break;
  }

  if (*path_namep == NULL){
    if (type == 1){
      vf_error = VF_ERR_NO_FONT_FILE;
      return -1;
    }
    return 0;
  }

  if (debug_on('f'))
    printf("VFlib ZEIT: font path: %s\n", *path_namep);

  ALLOCN_IF_ERR(*ol_offsetp, long, ZEIT_NCHARS){
    vf_error = VF_ERR_NO_MEMORY;
    goto Error;
  }
  ALLOCN_IF_ERR(*ol_sizep, long, ZEIT_NCHARS){
    vf_error = VF_ERR_NO_MEMORY;
    goto Error;
  }

  for (i = 0; i < ZEIT_NCHARS; i++){
    (*ol_offsetp)[i] = -1;
    (*ol_sizep)[i]   = 0;
  }

  if (zeit_read_header(ol_sizep, ol_offsetp, *path_namep) < 0){
    goto Error;
  }

  return 0;

Error:
  vf_free(*path_namep);  *path_namep = NULL;
  vf_free(*ol_offsetp);  *ol_offsetp = NULL;
  vf_free(*ol_sizep);    *ol_sizep   = NULL;
  return -1;
}


Private int 
zeit_read_header(long **ol_sizep, long **ol_offsetp, char *font_path)
{
  int    i, j;
  FILE   *fp;

  if ((fp = vf_fm_OpenBinaryFileStream(font_path)) == NULL){
    fprintf(stderr, "VFlib Error. File not found: %s\n", font_path);
    return -1;
  }

  (void) zeit_read_1byte(fp);
  (void) zeit_read_1byte(fp);

  for (i = 0; i < ZEIT_NCHARS; i++){
    (*ol_offsetp)[i] = zeit_read_4bytes(fp);
    if (debug_on('i'))
      printf("VFlib ZEIT: Header  %04d: %08lx\n", i, (*ol_offsetp)[i]);
  }

  for (i = 0; i < ZEIT_NCHARS-1; i++){
    if ((*ol_offsetp)[i] == 0xffffffff){
      (*ol_sizep)[i] = 0;
      continue;
    }
    for (j = i+1; ; j++){
      if (j >= ZEIT_NCHARS){
	(*ol_sizep)[i] = -(THRESHOLD_SIZE+1);
	break;
      }
      if ((*ol_offsetp)[j] != 0xffffffff){
	(*ol_sizep)[i] = -((*ol_offsetp)[j] - (*ol_offsetp)[i]); /* A MAGIC */
	break;
      }
    }
    if (-((*ol_sizep)[i]) > THRESHOLD_SIZE)  /* Large... check size. */
      zeit_correct_size(&((*ol_sizep)[i]), &((*ol_offsetp)[i]), fp, i); 
  }
  if ((*ol_offsetp)[ZEIT_NCHARS-1] == 0xffffffff){
    (*ol_sizep)[ZEIT_NCHARS-1] = 0;
  } else {
    (*ol_sizep)[ZEIT_NCHARS-1] = -(THRESHOLD_SIZE+1);           /* A MAGIC */
    zeit_correct_size(&((*ol_sizep)[i]), &((*ol_offsetp)[i]), 
		      fp, ZEIT_NCHARS-1);
  }

  return 0;
}

Private int
zeit_correct_size(long *sizep, long *offsetp, FILE *fp, int i)
{
  int   x, y;

  if (debug_on('i'))
    printf("VFlib ZEIT: Correct Size  %04x:  size %d, offs %ld\n", 
	   i, (int)-(*sizep), *offsetp);

  zeit_seek(fp, *offsetp);
  zeit_init_bit_stream(fp);
  for (;;){
    x = zeit_read_10bits(); 
    y = zeit_read_10bits();
    if ((x == 1023) && (y == 1023))
      break;
    for (;;){
      x = zeit_read_10bits();
      y = zeit_read_10bits();
      if ((x == 1023) && (y == 1023))
	break;
    }
  }
  *sizep = -(zeit_current_pos(fp) - *offsetp);

  if (debug_on('i'))
    printf("VFlib ZEIT:   ==>  %04x\n", (int)-(*sizep));

  return *sizep;
}

Private VF_OUTLINE
ZEIT_ReadOutline(int zeit_id, int code_point, 
		 double mag_x, double mag_y)
{
  FILE       *fp;
  ZEIT       zeit;
  int        idx, x, y, xx, yy, max_code, space2121;
  char       *font_file;
  VF_OUTLINE      outline;
  VF_OUTLINE_ELEM *sizep;
  long          size, offs;
  unsigned int  scode;
  double        mx, my;

  mx = mag_x;
  my = mag_y;
  if (mx > 1){
    mx = 1.0;
    my = 1.0 / mx;
  }

  if ((zeit = ZEIT_GetZEIT(zeit_id)) == NULL){
    fprintf(stderr, "VFlib internal error: ZEIT_ReadOutline()\n");
    vf_error = VF_ERR_INTERNAL;
    return NULL;
  }
  
  /* Assume JISX0208-1990 & KUTEN encoding */
  max_code = 0x7426;
  space2121 = 1;

  if ((code_point < 0x2121) || (max_code < code_point)
      || (code_point%256 < 0x21) || (0x7e < code_point%256)){
    vf_error = VF_ERR_ILL_CODE_POINT;
    return NULL;
  }

  if ((space2121 == 1) && (code_point == 0x2121)){
    size = VF_OL_OUTLINE_HEADER_SIZE_TYPE0 + 1;
    ALLOCN_IF_ERR(outline, VF_OUTLINE_ELEM, size)
      return NULL;
    outline[VF_OL_HEADER_INDEX_DATA_SIZE] = size;
    return outline;
  }

  scode = zeit_code2c(code_point);
  if (code_point < 0x5000){
    font_file = zeit->path_name1;
    offs  = zeit->ol_offset1[scode];
    sizep = &zeit->ol_size1[scode];
  } else {
    font_file = zeit->path_name2;
    offs  = zeit->ol_offset2[scode];
    sizep = &zeit->ol_size2[scode];
  }
  if (*sizep == 0)
    return NULL;
  if (*sizep < 0)                 /* A MAGIC */
    size = 2*(-*sizep) + 1; 
  else
    size = *sizep + 1;

  ALLOCN_IF_ERR(outline, VF_OUTLINE_ELEM, 
		VF_OL_OUTLINE_HEADER_SIZE_TYPE0 + size)
    return NULL;
  
  if ((fp = vf_fm_OpenBinaryFileStream(font_file)) == NULL){
    fprintf(stderr, "VFlib Error. File not found: %s\n", font_file);
    return NULL;
  }

  zeit_seek(fp, offs);
  zeit_init_bit_stream(fp);
  for (idx = 0; ; ){
    x = zeit_read_10bits();
    y = zeit_read_10bits();
    if ((x == 1023) && (y == 1023))
      break;
    xx = VF_OL_COORD_OFFSET + mx * (x*VF_OL_COORD_RANGE)/ZEIT_MAX_VALUE;
    yy = VF_OL_COORD_OFFSET + my * (y*VF_OL_COORD_RANGE)/ZEIT_MAX_VALUE;
    outline[VF_OL_OUTLINE_HEADER_SIZE_TYPE0 + idx++] 
      = VF_OL_INSTR_TOKEN | VF_OL_INSTR_CWCURV | VF_OL_INSTR_LINE;
    outline[VF_OL_OUTLINE_HEADER_SIZE_TYPE0 + idx++] 
      = VF_OL_MAKE_XY(xx, yy);
    for (;;){
      x = zeit_read_10bits();
      y = zeit_read_10bits();
      if ((x == 1023) && (y == 1023))
	break;
      xx = VF_OL_COORD_OFFSET + mx * (x*VF_OL_COORD_RANGE)/ZEIT_MAX_VALUE;
      yy = VF_OL_COORD_OFFSET + my * (y*VF_OL_COORD_RANGE)/ZEIT_MAX_VALUE;
      outline[VF_OL_OUTLINE_HEADER_SIZE_TYPE0 + idx++] = VF_OL_MAKE_XY(xx, yy);
    }
  }
  outline[VF_OL_OUTLINE_HEADER_SIZE_TYPE0 + idx++] = 0;

  if (outline[VF_OL_OUTLINE_HEADER_SIZE_TYPE0] != 0)
    outline[VF_OL_OUTLINE_HEADER_SIZE_TYPE0] |= VF_OL_INSTR_CHAR;

  if (*sizep < 0)
    *sizep = (long)idx;

  if (debug_on('i'))
    printf("VFlib ZEIT:      SIZE %5d\n", *sizep);

  outline[VF_OL_HEADER_INDEX_DATA_SIZE] 
    = *sizep + VF_OL_OUTLINE_HEADER_SIZE_TYPE0;

  return outline;
}


Private int
zeit_code2c(int code)
{
  if (code < 0x5000)
    return ((code/0x100) - 0x21)*0x5e + (code%0x100) - 0x21;

  return ((code/0x100) - 0x50)*0x5e + (code%0x100) - 0x21;
}



Private int
zeit_read_1byte(FILE *fp)
{
  return getc(fp);
}

Private unsigned long
zeit_read_4bytes(FILE *fp)
{
  unsigned long i1, i2, i3, i4;
  
  i1 = (unsigned long)zeit_read_1byte(fp);
  i2 = (unsigned long)zeit_read_1byte(fp);
  i3 = (unsigned long)zeit_read_1byte(fp);
  i4 = (unsigned long)zeit_read_1byte(fp);

  return i1 + i2*0x100 + i3*0x10000 + i4*0x1000000;
}


Private FILE           *zeit_bitstream_fp = NULL;
Private unsigned int   zeit_left_bits  = 0;
Private unsigned long  zeit_bit_stream = 0;
Private unsigned long  zeit_power2tbl[] = {
  0x000001,0x000002,0x000004,0x000008,0x000010,0x000020,0x000040,0x000080,
  0x000100,0x000200,0x000400,0x000800,0x001000,0x002000,0x004000,0x008000,
  0x010000,0x020000,0x040000,0x080000,0x100000,0x200000,0x400000,0x800000
}; 

Private void
zeit_init_bit_stream(FILE *fp)
{
  zeit_bitstream_fp = fp;
  zeit_bit_stream   = 0;
  zeit_left_bits    = 0;
}

Private int 
zeit_read_10bits(void)
{
  if (zeit_left_bits < 10){
    zeit_bit_stream  *= 0x10000;
    zeit_bit_stream  += (unsigned int)zeit_read_1byte(zeit_bitstream_fp);
    zeit_bit_stream  += (unsigned int)zeit_read_1byte(zeit_bitstream_fp)*0x100;
    zeit_left_bits   += 16;
  }
  zeit_left_bits -= 10;

  return (zeit_bit_stream/zeit_power2tbl[zeit_left_bits]) % ZEIT_MAX_VALUE;
}

Private void
zeit_seek(FILE *fp, long pos)
{
  fseek(fp, pos + ZEIT_HEADER_SIZE, SEEK_SET);
}

Private long
zeit_current_pos(FILE *fp)
{
  return ftell(fp) - ZEIT_HEADER_SIZE;
}


/*EOF*/
