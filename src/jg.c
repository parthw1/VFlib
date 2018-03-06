/*
 * jg.c - JG font interface
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

#include  "jg.h"


Private  VF_TABLE  jg_table    = NULL;


Private int
JG_Init(void)
{
  JG_GetJG(-1);

  if ((jg_table = vf_table_create()) == NULL){
    vf_error = VF_ERR_NO_MEMORY;
    return -1;
  }

  return 0;
}


Private void  jg_release(JG jg);
Private int   jg_make_header(char*,JG,int);
Private int   jg_correct_size(int seq, FILE *fp, JG_HEADER header);

Private int
JG_Open(char *font_name)
{
  JG   jg;
  int  jg_id, i;

  if ((jg_id = (jg_table->get_id_by_key)(jg_table, font_name, 
					 strlen(font_name)+1)) >= 0){
    (jg_table->link_by_id)(jg_table, jg_id);
    return jg_id;
  }

  ALLOC_IF_ERR(jg, struct s_jg){
    vf_error = VF_ERR_NO_MEMORY;
    return -1;
  }

  jg->nfiles = 3;

  ALLOCN_IF_ERR(jg->headers, JG_HEADER, jg->nfiles){
    vf_error = VF_ERR_NO_MEMORY;
    goto Error;
  }
  for (i = 0; i < jg->nfiles; i++)
    jg->headers[i] = NULL;

  for (i = 0; i < jg->nfiles; i++){
    ALLOC_IF_ERR(jg->headers[i], struct s_jg_header){
      vf_error = VF_ERR_NO_MEMORY;
      goto Error;
    }
  }
  jg->headers[0]->nchars = JG_CODE_SIZE0;
  jg->headers[1]->nchars = JG_CODE_SIZE1;
  jg->headers[2]->nchars = JG_CODE_SIZE2;

  for (i = 0; i < jg->nfiles; i++){
    if (jg_make_header(font_name, jg, i) < 0)
      goto Error;
  }    

  if ((jg_id = (jg_table->put)(jg_table, jg, 
			       font_name, strlen(font_name)+1)) < 0){
    vf_error = VF_ERR_NO_MEMORY;
    goto Error;
  }

  JG_SetJG(jg_id, jg);

  return jg_id;

Error:
  jg_release(jg);
  return -1;
}


Private void
JG_Close(int jg_id)
{
  JG   jg;

  if ((jg = JG_GetJG(jg_id)) == NULL){
    fprintf(stderr, "VFlib internal error: JG_Close()\n");
    vf_error = VF_ERR_INTERNAL;
    return;
  }
  if ((jg_table->unlink_by_id)(jg_table, jg_id) > 0)
    return;

  jg_release(jg);
}

Private void
jg_release(JG jg)
{
  int  i;

  if (jg != NULL){ 
    for (i = 0; i < jg->nfiles; i++){
      if (jg->headers[i] != NULL){
	vf_free(jg->headers[i]->font_path);
	vf_free(jg->headers[i]->ol_offset);
	vf_free(jg->headers[i]->ol_size);
	vf_free(jg->headers[i]);
      }
    }
    vf_free(jg->headers);
    vf_free(jg);
  }
}


Private int   jg_read_header(char*,JG_HEADER);

Private int            jg_read_1byte(FILE*);
Private unsigned long  jg_read_4bytes(FILE*);
Private void           jg_init_bit_stream(FILE*);
Private unsigned int   jg_read_12bits(void);
Private int            jg_read_xy(int*,int*);


Private int
jg_make_header(char *font_name, JG jg, int fseq)
{
  char   *font_path, *e;
  char   fname[MAXPATHLEN];
  int    i;
  SEXP   s;

  font_path = NULL;
  for (s = default_extensions; vf_sexp_consp(s); s = vf_sexp_cdr(s)){
    e = vf_sexp_get_cstring(vf_sexp_car(s));
    sprintf(fname, "%s%s%d", font_name, e, fseq);
    if ((font_path
	 = vf_search_file(fname, -1, NULL, FALSE, -1,
			  default_fontdirs, NULL, NULL)) != NULL)
      break;
  }
  
  if (font_path == NULL){
    if (fseq == 0){
      vf_error = VF_ERR_NO_FONT_FILE;
      return -1;
    } else {
      jg->headers[fseq]->font_path = NULL;
      jg->headers[fseq]->ol_offset = NULL;
      jg->headers[fseq]->ol_size   = NULL;
    }
    return 0;
  }

  if (debug_on('f'))
    printf("VFlib JG: font path: %s\n", font_path);

  jg->headers[fseq]->font_path = font_path;
  ALLOCN_IF_ERR(jg->headers[fseq]->ol_offset, long, jg->headers[fseq]->nchars)
    goto Error;
  ALLOCN_IF_ERR(jg->headers[fseq]->ol_size, long, jg->headers[fseq]->nchars)
    goto Error;

  for (i = 0; i < jg->headers[fseq]->nchars; i++){
    jg->headers[fseq]->ol_offset[i] = -1;
    jg->headers[fseq]->ol_size[i]   = 0;
  }

  if (jg_read_header(font_path, jg->headers[fseq]) < 0)
    goto Error;

  return 0;

Error:
  vf_error = VF_ERR_NO_MEMORY;
  vf_free(jg->headers[fseq]->font_path); jg->headers[fseq]->font_path = NULL;
  vf_free(jg->headers[fseq]->ol_offset); jg->headers[fseq]->ol_offset = NULL;
  vf_free(jg->headers[fseq]->ol_size);   jg->headers[fseq]->ol_size   = NULL;
  return -1;
}

Private int 
jg_read_header(char *font_path, JG_HEADER header)
{
  int   prefix, i, j;
  FILE  *fp;

  if ((fp = vf_fm_OpenBinaryFileStream(font_path)) == NULL){
    fprintf(stderr, "VFlib Error. File not found: %s\n", font_path);
    return -1;
  }

  fseek(fp, 8L, SEEK_SET);
  prefix  = (unsigned int) jg_read_1byte(fp);
  prefix += (unsigned int) jg_read_1byte(fp) * 0x100;
  header->base = 10 + prefix + 4*header->nchars;

  if (debug_on('i'))
    printf("VFlib JG: PREFIX %04x \n", prefix); 

  fseek(fp, (long)(prefix+0x0a), SEEK_SET);
  for (i = 0; i < header->nchars; i++){
    header->ol_offset[i] = jg_read_4bytes(fp);
    if (debug_on('i'))
      printf("VFlib JG: Index: %04x,  offset: %04lx\n",
	     i, header->ol_offset[i]);
    if (header->ol_offset[i] != EMPTY_PTR)
      header->ol_offset[i] += header->base;
  }

  for (i = 0; i < header->nchars-1; i++){
    if (header->ol_offset[i] == EMPTY_PTR){
      header->ol_size[i] = 0;
    } else {
      for (j = i+1; ; j++){
	if (j >= header->nchars){
	  header->ol_size[i] = -(THRESHOLD_SIZE+1);
	  break;
	}
	if (header->ol_offset[j] != EMPTY_PTR){
	  header->ol_size[i] = -(header->ol_offset[j] - header->ol_offset[i]);
	  break;
	}
      }
    }
    if (debug_on('i'))
      printf("VFlib JG:  index: %04x,  ol size: %04lx\n",
	     i, -header->ol_size[i]);
    if (-(header->ol_size[i]) > THRESHOLD_SIZE)
      header->ol_size[i] = EMPTY_PTR;
  }

  /* when i == header->nchars-1: */
  if (header->ol_offset[header->nchars-1] == EMPTY_PTR){
    header->ol_size[header->nchars-1] = 0;
  } else {
    header->ol_size[header->nchars-1] = -(THRESHOLD_SIZE+1);
    jg_correct_size(header->nchars-1, fp, header);
  }

  return 0;
}

/* correct the size of font data */
Private int
jg_correct_size(int seq, FILE *fp, JG_HEADER header) 
{
  int   x, y;

  if (debug_on('i'))
    printf("VFlib JG:  Too Large for %04x\n   Size:%04lx ",
	   seq, -header->ol_size[seq]); 

  fseek(fp, header->ol_offset[seq], SEEK_SET);
  jg_init_bit_stream(fp);
  for (;;){
    if (jg_read_xy(&x, &y) == -1)
      break;
    while (jg_read_xy(&x, &y) != -1)
      ;
  }
  header->ol_size[seq] = -(ftell(fp) - header->ol_offset[seq]);

  if (debug_on('i'))
    printf("VFlib JG:  ==>  %04lx\n", -header->ol_size[seq]);

  return header->ol_size[seq];
}


Private int   jg_correct_size(int,FILE*,JG_HEADER);
Private int   jg_charcode2c(int);

Private VF_OUTLINE
JG_ReadOutline(int jg_id, int code_point, 
	       double mag_x, double mag_y)
{
  FILE       *fp;
  JG         jg;
  int        fseq, max_code, space2121;
  int        token_idx, idx, cnt, cmd, x, y, cmdp, xp, yp, cmd0, x0, y0;
  long          size, offs;
  unsigned int  scode;
  VF_OUTLINE        outline;
  VF_OUTLINE_ELEM   *sizep, chd;
  double        mx, my;
#define CONV_COORD_X(p)  \
   ((unsigned int)(VF_OL_COORD_OFFSET + ((p)*mx*VF_OL_COORD_RANGE)/JG_MAX_XY))
#define CONV_COORD_Y(p)  \
   ((unsigned int)(VF_OL_COORD_OFFSET + ((p)*my*VF_OL_COORD_RANGE)/JG_MAX_XY))

  mx = mag_x;
  my = mag_y;
  if (mx > 1){
    my /= mx;
    mx = 1.0;
  }
  if (my > 1){
    mx /= my;
    my = 1.0;
  }

  if ((jg = JG_GetJG(jg_id)) == NULL){
    fprintf(stderr, "VFlib internal error: JG_ReadOutline()\n");
    vf_error = VF_ERR_INTERNAL;
    return NULL;
  }
  
  /* Assume JISX0208-1990 & JIS encoding */
  max_code = 0x7426;
  space2121 = 1;

  if ((code_point < 0x2121) || (max_code < code_point)
      || (code_point%256 < 0x21) || (0x7e < code_point%256) ){
    vf_error = VF_ERR_ILL_CODE_POINT;
    return NULL;
  }

  if ((space2121 == 1) && (code_point == 0x2121)){
    ALLOCN_IF_ERR(outline, VF_OUTLINE_ELEM, 
		  VF_OL_OUTLINE_HEADER_SIZE_TYPE0 + 1)
      return NULL;
    outline[VF_OL_OUTLINE_HEADER_SIZE_TYPE0] = 0L;
    return outline;
  }

  if (code_point < 0x3000){
    fseq = 0;
  } else if (code_point < 0x5000){
    fseq = 1;
  } else {
    fseq = 2;
  }
  scode = jg_charcode2c((int)code_point);

  if ((fp = vf_fm_OpenBinaryFileStream(jg->headers[fseq]->font_path)) == NULL){
    fprintf(stderr, "VFlib Error. File not found: %s\n", 
	    jg->headers[fseq]->font_path);
    return NULL;
  }

  offs  = jg->headers[fseq]->ol_offset[scode];
  sizep = &jg->headers[fseq]->ol_size[scode];
  if (*sizep == EMPTY_PTR)
    jg_correct_size(scode, fp, jg->headers[fseq]);
  
  if (*sizep == 0)
    return NULL;
  if (*sizep < 0)
    size = -3*(*sizep) + 1;
  else
    size = *sizep;

  ALLOCN_IF_ERR(outline, VF_OUTLINE_ELEM, 
		VF_OL_OUTLINE_HEADER_SIZE_TYPE0 + size)
    return NULL;
  
  fseek(fp, offs, SEEK_SET);
  jg_init_bit_stream(fp);

  cnt = 0;
  idx = VF_OL_OUTLINE_HEADER_SIZE_TYPE0;
  chd = VF_OL_INSTR_CHAR;
  for (;;){
NEXT_CIRCLE:
    if ((cmd = jg_read_xy(&x, &y)) == -1)
      break;
    cmd0 = cmd; x0 = x; y0 = y;
    cmdp = cmd; xp = x; yp = y; 
    if (debug_on('o'))
      printf("VFlib JG:  CMD0=%d, X0=%d, Y0=%d\n", cmd0, x0, y0);
    token_idx = idx;
    outline[idx++] = (chd | VF_OL_INSTR_CWCURV | VF_OL_INSTR_TOKEN);
    cnt++;
    chd = 0L;

    for (;;){ 
      cmd = jg_read_xy(&x, &y);
      switch (cmd){
      case -1:
      END_OF_CCURV:
	if (cmdp != cmd0){
	  outline[idx++] 
	    = ((cmd0==0) ? VF_OL_INSTR_LINE : VF_OL_INSTR_BEZ) 
	      | VF_OL_INSTR_TOKEN;
	  cnt++;
	}
	outline[idx++] = VF_OL_MAKE_XY(CONV_COORD_X(xp), CONV_COORD_Y(yp));
	cnt++;
	goto NEXT_CIRCLE;
      case 0:
      LINE:
	outline[token_idx] |= (VF_OL_INSTR_LINE | VF_OL_INSTR_TOKEN);
	outline[idx++] = VF_OL_MAKE_XY(CONV_COORD_X(xp), CONV_COORD_Y(yp));
	cnt++;
	cmdp = cmd;  xp = x; yp = y;
	cmd = jg_read_xy(&x, &y);
	while (cmd == 0){
	  outline[idx++] = VF_OL_MAKE_XY(CONV_COORD_X(xp), CONV_COORD_Y(yp));
	  cnt++;
	  cmdp = cmd;  xp = x; yp = y; 
	  cmd = jg_read_xy(&x, &y);
	}
	if (cmd == -1)
	  goto END_OF_CCURV; 
	token_idx = idx;
	outline[idx++] = 0L;
	cnt++;
	goto BEZ;
      case 1:
      BEZ: 
	outline[token_idx] |= (VF_OL_INSTR_BEZ | VF_OL_INSTR_TOKEN);
	outline[idx++] = VF_OL_MAKE_XY(CONV_COORD_X(xp), CONV_COORD_Y(yp));
	cnt++;
	cmdp = cmd;  xp = x; yp = y; 
	cmd = jg_read_xy(&x, &y);
	while (cmd == 1){
	  outline[idx++] = VF_OL_MAKE_XY(CONV_COORD_X(xp), CONV_COORD_Y(yp));
	  cnt++;
	  cmdp = cmd;  xp = x; yp = y; 
	  cmd = jg_read_xy(&x, &y);
	}
	if (cmd == -1)
	  goto END_OF_CCURV; 
	token_idx = idx;
	outline[idx++] = 0L;
	cnt++;
	goto LINE;
      }
    }
  }
  outline[idx++] = 0L;
  cnt++;

  if (*sizep < 0)
    *sizep = (long) cnt;

  if (debug_on('o'))
    printf("VFlib JG:  SIZE %5d\n", *sizep);

  return outline;
}


Private int
jg_charcode2c(int code_point)
{
  int  jgcode;
  
  if (code_point < 0x3000)
    jgcode = (code_point/256 - 0x21)*0x5e + (code_point%256) - 0x21;
  else if (code_point < 0x5000)
    jgcode = (code_point/256 - 0x30)*0x5e + (code_point%256) - 0x21;
  else 
    jgcode = (code_point/256 - 0x50)*0x5e + (code_point%256) - 0x21;

  return jgcode;
}



Private int
jg_read_1byte(FILE *fp)
{
  return fgetc(fp);
}

Private unsigned long
jg_read_4bytes(FILE *fp)
{
  unsigned long i1, i2, i3, i4;
  
  i1 = (unsigned long)jg_read_1byte(fp);
  i2 = (unsigned long)jg_read_1byte(fp);
  i3 = (unsigned long)jg_read_1byte(fp);
  i4 = (unsigned long)jg_read_1byte(fp);

  return i1 + i2*0x100 + i3*0x10000 + i4*0x1000000;
}


Private FILE           *jg_bitstream_fp = NULL;
Private unsigned int    jg_left_bits  = 0;
Private unsigned long   jg_bit_stream = 0;

Private void
jg_init_bit_stream(FILE *fp)
{
  jg_bitstream_fp = fp;
  jg_bit_stream   = 0;
  jg_left_bits    = 0;
}

Private unsigned int 
jg_read_12bits(void)
{
  if (jg_left_bits < 12){
    jg_bit_stream  = jg_bit_stream * 0x10000L;
    jg_bit_stream  += (unsigned int) jg_read_1byte(jg_bitstream_fp);
    jg_bit_stream  += (unsigned int) jg_read_1byte(jg_bitstream_fp) * 0x0100L;
    jg_left_bits   += 16;
  }
  jg_left_bits -= 12;

  return (jg_bit_stream >> jg_left_bits) & JG_MAX_VALUE;
}

Private int
jg_read_xy(int *xp, int *yp)
{
  int   x, y, fx, fy;

  x = jg_read_12bits();
  y = jg_read_12bits();
  if ((x == JG_MAX_VALUE) && (y == JG_MAX_VALUE))
    return -1;

  *xp  = x & JG_XY_MASK;
  *yp  = y & JG_XY_MASK;
  fx   = x & JG_CMD_MASK;
  fy   = y & JG_CMD_MASK;

  if (*xp > (JG_MAX_XY+1)/2)
    *xp = (JG_MAX_XY+1) - *xp;
  else 
    *xp += (JG_MAX_XY+1)/2;
  if (*yp < (JG_MAX_XY+1)/2)
    *yp  = (JG_MAX_XY+1)/2 - *yp;

  *xp -= (JG_MAX_XY+1)/4;
  *xp = (*xp < 0) ? 0: *xp * 2;
  *yp -= (JG_MAX_XY+1)*5/16;
  *yp = (*yp < 0) ? 0: *yp * 2;
  if (fx != 0)
    return 1;

  return 0;
}



/*EOF*/
