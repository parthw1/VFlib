/* 
 * vflmkt1.c 
 * by Hirotsugu Kakugawa
 * - a vflibcap entry generator for PostScript Type 1 fonts
 * - This program read "font map" file (e.g. psfonts.map) of dvips,
 *   and prints vflibcap entries to standard output.
 * - Useful for generating vflibcap for TeX DVI drivers
 *
 *   2 May 2001   
 *   3 May 2001  Support for kpathsea. 
 *   9 May 2001  Support for font file substitution by Ghostscript fonts
 *  10 May 2001  Support for font class geneeration, afm and encoding
 *               vector directories.
 *  29 Mar 2010  Fixed the format name for kpathsea search.
 */
/*
 * Copyright (C) 2001-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */


#include "config.h"
#include "with.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif
#ifdef HAVE_STRING_H
#  include <string.h>
#endif
#include <ctype.h>
#ifdef HAVE_SYS_PARAM_H
#  include <sys/param.h>
#endif
#ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
#endif

#ifdef WITH_KPATHSEA
# include  "kpathsea/kpathsea.h"
#endif

#include  "VFlib-3_7.h"
#include  "VFsys.h"
#include  "vflibcap.h"
#include  "texfonts.h"
#include  "t1.h"
#include  "fsearch.h"
#include  "vflmklib.h"



void    gen_class_deafult(void);
void    translate(FILE*,FILE*,char*);
int     get_token(char *s, char *b, int *x);
void    emit(void);

int     parse_pscmd(char *cmds, double *val);

void    read_gs_fontmap(void);
char   *find_gs_font(char *f);
int     query_gs_db(char *s);

double  tfm_read_ds(char *name);




#define MAXPSCODE   4
char  texfont[BUFSIZ];
char  psfont[BUFSIZ];
char  t1font[BUFSIZ];
char  encfile[BUFSIZ];
char  pscode[MAXPSCODE][BUFSIZ];

char  *mode  = DEFAULT_KPS_MODE;
char  *dpi   = NULL;
int    dpi_i = DEFAULT_KPS_DPI;

#define NDIRS    64
int   n_t1f; 
char  *t1_fontdirs[NDIRS];
int   n_t1e; 
char  *t1_encdirs[NDIRS];
int   n_t1a; 
char  *t1_afmdirs[NDIRS];
int   n_gsf; 
char  *gs_fontdirs[NDIRS];
char  *gs_fontmap;

#define NGSFONTS   1024
char *gs_db_f[NGSFONTS];
char *gs_db_d[NGSFONTS];
int   i_db; 

char *cmdline = NULL; 
char *font_suffix = "";
int   gen_font = 0;
int   with_tfm = 0;
int   exist_only = 0; 




int 
main(int argc, char **argv)
{
  int     nmaps, i;
  FILE   *fp;
  int    xargc;
  char **xargv;
#ifdef WITH_KPATHSEA
  int    f;
  char   *progname;
#endif
  char	*s;

  dpi = malloc(256);
  sprintf(dpi, "%d", dpi_i);

  cmdline = copy_cmdline(argc, argv);

  n_t1f = n_t1a = n_t1e = 0;
  for (i = 0; i < NDIRS; i++){
    t1_fontdirs[i] = NULL;
    t1_afmdirs[i]  = NULL;
    t1_encdirs[i]  = NULL;
    gs_fontdirs[i] = NULL;
  }
  gs_fontmap = NULL;

  i_db = 0;
  for (i = 0; i < NGSFONTS; i++){
    gs_db_f[i] = NULL;
    gs_db_d[i] = NULL;
  }

  xargc = argc; 
  xargv = argv;

  nmaps = 0;
  for (xargc--,xargv++; xargc > 0; xargc--,xargv++){
    if ((strcmp(*xargv, "--help") == 0)
	|| (strcmp(*xargv, "-help") == 0)){
      printf("vflmkt1: generates vflibcap entries for Type1 fonts from\n");
      printf("         a 'font map' file of dvips\n");
      printf("Usage: vflmkt1 [options] [map-file ...]\n");
      printf("Options\n");
      printf("  -d DIR   : Type 1 font file directory\n");
      printf("  -a DIR   : AFM file directory\n");
      printf("  -e DIR   : T1Lib encoding vector directory\n");
      printf("  -gf DIR  : Ghostscript font file directory\n");
      printf("  -gm FILE : Ghostscript font map file 'Fontmap' path\n");
      printf("  -r DPI   : Default device resolution\n");
      printf("  -n MODE  : Device mode name for kpathsea\n");
      printf("  -f       : Generate font entries\n");
      printf("  -z       : Generate font entries whose font file exists\n");
      printf("  -t       : Generate font entries with TeX TFM file\n");
      printf("  -x STR   : String added to the end of font name\n");

#ifdef WITH_KPATHSEA
      printf("Example: vflmkt1 -d TEXMF -f psfonts.map\n");
#else
      printf("Example: vflmkt1 -d TEXMF -f /usr/local/share/texmf/dvips/psfonts.map\n");
#endif
      exit(0);

    } else if (strcmp(*xargv, "-r") == 0){
      xargv++; xargc--;
      check_argc(xargc);
      dpi = strdup(*xargv);

    } else if (strcmp(*xargv, "-n") == 0){
      /* mode */
      xargv++; xargc--;
      check_argc(xargc);
      mode = x_strdup(*xargv);

    } else if (strcmp(*xargv, "-d") == 0){
      /* type 1 font dir */
      if (n_t1f == NDIRS){
	fprintf(stderr, "Too many Type 1 font directories\n");
	exit(1);
      }
      xargv++; xargc--;
      check_argc(xargc);
      t1_fontdirs[n_t1f++] = x_strdup(*xargv);

    } else if (strcmp(*xargv, "-a") == 0){
      /* afm dir */
      if (n_t1a == NDIRS){
	fprintf(stderr, "Too many AFM directories\n");
	exit(1);
      }
      xargv++; xargc--;
      check_argc(xargc);
      t1_afmdirs[n_t1a++] = x_strdup(*xargv);

    } else if (strcmp(*xargv, "-e") == 0){
      /* encoding vector dir */
      if (n_t1e == NDIRS){
	fprintf(stderr, "Too many T1Lib Encoding Vector directories\n");
	exit(1);
      }
      xargv++; xargc--;
      check_argc(xargc);
      t1_encdirs[n_t1e++] = x_strdup(*xargv);

    } else if (strcmp(*xargv, "-gf") == 0){
      /* gs font dir */
      if (n_gsf == NDIRS){
	fprintf(stderr, "Too many Ghostscript font directories\n");
	exit(1);
      }
      xargv++; xargc--;
      check_argc(xargc);
      gs_fontdirs[n_gsf++] = x_strdup(*xargv);

    } else if (strcmp(*xargv, "-gm") == 0){
      /* gs font map */
      xargv++; xargc--;
      check_argc(xargc);
      gs_fontmap = x_strdup(*xargv);

    } else if (strcmp(*xargv, "-f") == 0){
      gen_font = 1;

    } else if (strcmp(*xargv, "-z") == 0){
      exist_only = 1;

    } else if (strcmp(*xargv, "-t") == 0){
      with_tfm = 1;

    } else if (strcmp(*xargv, "-x") == 0){
      xargv++; xargc--;
      check_argc(xargc);
      font_suffix = strdup(*xargv);

    } else {
      if (*xargv[0] == '-'){
	fprintf(stderr, "vflmkt1: unknown option %s\n", *xargv);
	exit(1);
      }
      break;

    }
  }

#ifdef WITH_KPATHSEA
  progname = NULL;
  kpse_set_program_name (argv[0], progname);
  kpse_init_prog (uppercasify(kpse_program_name), atoi(dpi), mode, NULL);
  for (f = 0; f < kpse_last_format; f++) {
    kpse_init_format(f);
  }
#endif

  banner("Type 1", "vflmkt1", cmdline);

  read_gs_fontmap();

  gen_class_deafult();    

  for ( ; xargc > 0; xargc--,xargv++){
    nmaps++;
    
    s = *xargv;
    if ((fp = fopen(s, "r")) == NULL){
#ifdef WITH_KPATHSEA
      /*XXX s = kpse_find_file(*xargv, kpse_dvips_config_format, 0); */
      s = kpse_find_file(*xargv, kpse_fontmap_format, 0);
      if (s == NULL){ 
	fprintf(stderr, "Cannot find: %s\n", *xargv);
	exit(1);
      }
      if (s != NULL){ 
	if ((fp = fopen(s, "r")) == NULL){
#endif
	  fprintf(stderr, "Cannot open: %s\n", *xargv);
	  exit(1);
#ifdef WITH_KPATHSEA
	}
      }
#endif
    }
    translate(fp, stdout, s);
    fclose(fp);
  }
  
  if (nmaps == 0){
    translate(stdin, stdout, "<stdard input>");
  }

  printf("\n");

  return 0;
}



void
gen_class_deafult(void)
{
  int   i;

  printf("(%s %s", 
	 VF_CAPE_VFLIBCAP_CLASS_DEFAULT_DEFINITION, FONTCLASS_NAME);
  printf("\n  (%s", VF_CAPE_FONT_DIRECTORIES);
  for (i = 0; i < n_t1f; i++)
    printf("\n       \"%s\"", t1_fontdirs[i]);
  for (i = 0; i < n_gsf; i++)
    printf("\n       \"%s\"", gs_fontdirs[i]);
  printf(")");
  printf("\n  (%s", VF_CAPE_TYPE1_AFM_DIRECTORIES);
  for (i = 0; i < n_t1a; i++)
    printf("\n       \"%s\"", t1_afmdirs[i]);
  printf(")");
  printf("\n  (%s", VF_CAPE_TYPE1_ENC_DIRECTORIES);
  for (i = 0; i < n_t1e; i++)
    printf("\n       \"%s\"", t1_encdirs[i]);
  printf(")");
  printf("\n  (%s \"none\")", VF_CAPE_TYPE1_LOG_LEVEL);
  printf("\n  (%s %s)", VF_CAPE_DPI, dpi);
  printf(")\n");
  printf("\n");
}


void
translate(FILE *in, FILE *out, char *map)
{
  int   lno, x, v, i;
  char  buff[BUFSIZ];
  char  opt[BUFSIZ];
  int   npscode;

  if (gen_font == 0)
    return;

  printf(";; dvips mapfile: %s\n", map);

  lno = 0;
  while (fgets(buff, sizeof(buff), in) != NULL){
    lno++;
    if (isspace(buff[0]))
      continue;
    switch (buff[0]){
    case '\0':
    case ' ':
    case '\t':
    case '%':
    case '*':
    case ';':
    case '#':
      continue;
    default:
      break;
    }
    x = 0;
    npscode = 0;
    *t1font = *encfile = *opt = '\0';
    for (i = 0; i < MAXPSCODE; i++)
      *pscode[i] = '\0';
    v = get_token(texfont, buff, &x); 
    v = get_token(psfont,  buff, &x); 
    while (v >= 0){
      v = get_token(opt, buff, &x);
      if (strncmp(opt, "<<", 2) == 0){
	if (*t1font != '\0') 
	  fprintf(stderr, "Error: line %d: %s\n", lno, buff);
	strcpy(t1font, &opt[1]);
      } else if (strncmp(opt, "<[", 2) == 0){
	if (*encfile != '\0') 
	  fprintf(stderr, "Error: line %d: %s\n", lno, buff);
	strcpy(encfile, &opt[1]);
      } else if (strncmp(opt, "<", 1) == 0){
	if ((strlen(opt) >= 4) && (strcmp(".enc", &opt[strlen(opt)-4]) == 0)){
	  if (*encfile != '\0') 
	    fprintf(stderr, "Error: line %d: %s\n", lno, buff);
	  strcpy(encfile, &opt[1]);
	} else {
	  if (*t1font != '\0') 
	    fprintf(stderr, "Error: line %d: %s\n", lno, buff);
	  strcpy(t1font, &opt[1]);
	}
      } else if (strncmp(opt, "\"", 1) == 0){
	strcpy(pscode[npscode], &opt[1]);
	pscode[npscode][strlen(pscode[npscode])-1] = '\0';
	npscode++;
	if (npscode == MAXPSCODE){
	  fprintf(stderr, "Error (Too many PS code): line %d: %s\n",
		  lno, buff);
	  exit(0);
	}
      }
    }

    emit(); 
  }

  printf(";; end of %s\n", map);
}

int 
get_token(char *s, char *b, int *x)
{
  int  d;

  if (b[*x] == '\0')
    return -1;
  while (isspace(b[*x])){
    (*x)++;
    if (b[*x] == '\0')
      return -1;
  }

  d = 0;
  if (b[*x] == '"')
    d = 1;
  for (;;){
    *s = b[*x];
    if (b[*x] == '\0')
      return 0;
    s++;
    (*x)++;
    if ((d == 0) && isspace(b[*x])){
      *s = '\0';
      break;
    }
    if ((d == 1) && (b[*x] == '"'))
      d = 0;
  }

  while (isspace(b[*x])){
    (*x)++;
    if (b[*x] == '\0')
      return -1;
  }

  return 0;
}

int
nonlowers_only(char *f)
{
  while (*f) {
    if (islower(*f))
      return 0;
    f++;
  }
  return 1;
}

char *make_lower(char *f)
{
  char *lf, *p;

  if (f == NULL) {
    fprintf(stderr, "Cannot happen: f == NULL\n");
    exit(1);
  }
  if ((lf = malloc(strlen(f)+1)) == NULL) {
    fprintf(stderr, "No memory\n");
    exit(1);
  }
  p = lf;
  while (*f) {
    *p = tolower(*f);
    p++;
    f++;
  }
  *p = '\0';
  return lf;
}

void 
emit(void)
{
  char   *f, *ff;
  int     i, n, type, l;
  double  val, ds;

  if (*t1font != '\0'){
    ff = t1font;
  } else {
    ff = psfont;
  }

  if (exist_only == 1){
#ifdef WITH_KPATHSEA
    f = check_font_exist(ff, t1_fontdirs, n_t1f, kpse_type1_format, NULL);
#else
    f = check_font_exist(ff, t1_fontdirs, n_t1f, 0, NULL);
#endif
    ds = tfm_read_ds(texfont);
    if (ds < 0){
      free(f);
      f = NULL;
    }      
    if (f == NULL){
      /* search substitute font in Ghostscript font directory */
      f = find_gs_font(ff);    /* f is NULL if not found in gs Fontmap */
      if (f == NULL){
	return;
      } else {                 /* ignore .gsf font files */
	l = strlen(ff);
	if ((l >= strlen(".gsf")) && (strcmp(&f[l-4], ".gsf") == 0)){
	  free(f); 
	  f = NULL;
	  return;
	}
      }
    }
    printf("(%s %s%s", VF_CAPE_VFLIBCAP_FONT_ENTRY_DEFINITION, 
	   texfont, font_suffix); 
    printf("\t(%s %s)", VF_CAPE_FONT_CLASS, FONTCLASS_NAME); 
    if ((with_tfm == 1) && (ds > 0)){
      printf("\n  (%s %.2f)", VF_CAPE_POINT_SIZE, ds);
      printf(" (%s \"%s\")", VF_CAPE_TYPE1_TFM, texfont);
    }
    if (!nonlowers_only(f)){
      printf(" (%s \"%s\")", VF_CAPE_FONT_FILE, f);
    } else {
      char *lf = make_lower(f);
      printf(" (%s \"%s\" \"%s\")", VF_CAPE_FONT_FILE, f, lf);
      free(lf);
    }

  } else {

    /* search substitute font in Ghostscript font directory */
    f = find_gs_font(ff);    /* f is NULL if not found in gs Fontmap */
    ds = tfm_read_ds(texfont);
    if (f != NULL){
      /* ignore .gsf font files */
      l = strlen(ff);
      if ((l >= strlen(".gsf")) && (strcmp(&f[l-4], ".gsf") == 0)){
	free(f); f = NULL;
      }
    }
    printf("(%s %s%s", VF_CAPE_VFLIBCAP_FONT_ENTRY_DEFINITION, 
	   texfont, font_suffix); 
    printf("\t(%s %s)", VF_CAPE_FONT_CLASS, FONTCLASS_NAME); 
    if ((with_tfm == 1) && (ds > 0)){
      printf("\n  (%s %.2f)", VF_CAPE_POINT_SIZE, ds);
      printf(" (%s \"%s\")", VF_CAPE_TYPE1_TFM, texfont);
    }
    if (f != NULL){
      printf("\n  (%s \"%s\" \"%s\")", VF_CAPE_FONT_FILE, ff, f);
    } else {
      if (!nonlowers_only(ff)){
	printf(" (%s \"%s\")", VF_CAPE_FONT_FILE, ff);
      } else {
	char *lf = make_lower(ff);
	printf(" (%s \"%s\" \"%s\")", VF_CAPE_FONT_FILE, ff, lf);
	free(lf);
      }
    }
  }

  n = 0;
  for (i = 0; i < MAXPSCODE; i++){
    if (pscode[i][0] == '\0')
      continue;
    if ((type = parse_pscmd(pscode[i], &val)) < 0)
      continue;
    if (n == 0){
      printf("\n  "); 
    } else {
      printf(" ");
    }
    switch (type){
    case 1:
      if (*encfile != '\0')
	printf("(%s \"%s\")", VF_CAPE_TYPE1_ENC_VECT, encfile);
      break;
    case 2:
      printf("(%s %.3f)", VF_CAPE_SLANT_FACTOR, val);
      break;
    case 3:
      printf("(%s %.3f)", VF_CAPE_ASPECT_RATIO, val);
      break;
    }
    n++;
  }
  printf(")\n");

  if (f != NULL)
    free(f);
}


int
parse_pscmd(char *cmds, double *val)
{
  char  psc[2][256];

  if (sscanf(cmds, "%s %s", psc[0], psc[1]) == 2){
    if (strcmp(psc[1], "ReEncodeFont") == 0){
      return 1; 
    }
    if (strcmp(psc[1], "SlantFont") == 0){
      *val = atof(psc[0]);
      return 2; 
    }
    if (strcmp(psc[1], "ExtendFont") == 0){
      *val = atof(psc[0]);
      return 3; 
    }
    return -1;
  }

  return -1;
}





void
read_gs_fontmap(void)
{
  int   lno, i, j;
  char  f[BUFSIZ], d[BUFSIZ];
  char  buff[BUFSIZ];
  char  *p;
  FILE *fp; 

  if (n_gsf == 0)
    return;

  if ((fp = fopen(gs_fontmap, "r")) == NULL){
    fprintf(stderr, "Cannot open Ghostscript Fontmap file\n");
    exit(1);
  }

  lno = 0;
  while (fgets(buff, sizeof(buff), fp) != NULL){
    lno++;
    /* skip empty line */
    if ((p = strchr(buff, '%')) != NULL)
      *p = '\0';
    for (i = 0; buff[i] != '\0'; i++){
      if (!isspace(buff[i]))
	break;
    }
    if (buff[i] == '\0')
      continue;
    /* font name */
    for (j = 0; (buff[i] != '\0') && !isspace(buff[i]); i++, j++)
      f[j] = buff[i];
    f[j] = '\0';
    if (buff[i] == '\0'){
      fprintf(stderr, "Unexpected file format: gs_fontmap\n");
      continue;
    }

    /* skip space */
    for ( ; buff[i] != '\0'; i++){
      if (!isspace(buff[i]))
	break;
    }
    if (buff[i] == '\0'){
      fprintf(stderr, "Unexpected file format: Ghostscript Fontmap\n");
      continue;
    }
    /* font file or alias */
    for (j = 0; (buff[i] != '\0') && !isspace(buff[i]); i++, j++)
      d[j] = buff[i];
    d[j] = '\0';
    
    /* add to DB */
    if (i_db == NGSFONTS){
      fprintf(stderr, "Too many fonts in Ghostscript Fontmap\n");
      exit(1);
    }
#if 0
    printf("*** GS Fontmap db %d: %s\t%s\n", i_db, f, d);
#endif
    gs_db_f[i_db] = x_strdup(f);
    gs_db_d[i_db] = x_strdup(d);
    i_db++;
  }

  fclose(fp);
  fp = NULL;
}


char*
find_gs_font(char *f)
{
  int   j;
  char  *s;

  if ((j = query_gs_db(f)) < 0)
    return NULL;

  while (gs_db_d[j][0] == '/'){    /* resolve alias */
    if ((j = query_gs_db(&gs_db_d[j][1])) < 0)
      return NULL;
  } 

  s = x_strdup(&gs_db_d[j][1]);
  s[strlen(s)-1] = '\0';

  return s;
}

int
query_gs_db(char *s)
{
  int  i;

  for (i = 0; i < i_db; i++){
    if (strcmp(&gs_db_f[i][1], s) == 0)
      return i;
  }
  return -1;  /* font not in db. */
}




double
tfm_read_ds(char *name)
{
  FILE  *fp;
  char  tfm[1024];
  UINT4  lf, cs, ds, offset_header;
  UINT2  aux;
  int    type;
  char  *s;

  sprintf(tfm, "%s.%s", name, DEFAULT_EXTENSIONS_TFM);
#ifdef WITH_KPATHSEA
  s = kpse_find_file(tfm, kpse_tfm_format, 0);
#else
  s = name;
#endif
  if (s == NULL)
    return -1;
  if ((fp = fopen(s, "r")) == NULL)
    return -1;

  lf = (UINT4)READ_UINT2(fp);
  if ((lf == 11) || (lf == 9)){
    /* JFM file of Japanese TeX by ASCII Coop. */
    type = METRIC_TYPE_JFM;
    (UINT4)READ_UINT2(fp);
    (UINT4)READ_UINT2(fp);
    (UINT4)READ_UINT2(fp);    
    offset_header = 4*7;
  } else if (lf == 0){
    /* Omega Metric File */
    type = METRIC_TYPE_OFM;
    aux = READ_INT2(fp);    /* ofm_level */
    READ_UINT4(fp);
    READ_UINT4(fp);
    if (aux == 0){   /* level 0 OFM */
      offset_header = 4*14;
    } else {                   /* level 1 OFM: *** NOT SUPPORTED YET *** */
      offset_header = 4*29;
    }
  } else {
    /* Traditional TeX Metric File */
    type = METRIC_TYPE_TFM;
    aux = 0;
    READ_UINT2(fp);    
    offset_header    = 4*6;
  }

  if (type == METRIC_TYPE_OFM){
    READ_UINT4(fp);
    READ_UINT4(fp);
    READ_UINT4(fp);
    READ_UINT4(fp);
    READ_UINT4(fp);
    READ_UINT4(fp);
    READ_UINT4(fp);
    READ_UINT4(fp);
    READ_UINT4(fp);
    READ_UINT4(fp);
    READ_UINT4(fp); 
  } else {
    READ_UINT2(fp); 
    READ_UINT2(fp);
    READ_UINT2(fp);
    READ_UINT2(fp);
    READ_UINT2(fp);
    READ_UINT2(fp);
    READ_UINT2(fp);
    READ_UINT2(fp);
    READ_UINT2(fp);
    READ_UINT2(fp);
  }

  fseek(fp, offset_header, SEEK_SET);
  cs = READ_UINT4(fp); 
  ds = READ_UINT4(fp); 

  fclose(fp);

  return (double)(ds)/(double)(1<<20);
}

unsigned long
vf_tex_read_uintn(FILE* fp, int size)
{
  unsigned long  v;

  v = 0L;
  while (size >= 1){
    v = v*256L + (unsigned long)getc(fp);
    --size;
  }
  return v;
}

long
vf_tex_read_intn(FILE* fp, int size)
{
  long           v;

  v = (long)getc(fp) & 0xffL;
  if (v & 0x80L)
    v = v - 256L;
  --size;
  while (size >= 1){
    v = v*256L + (unsigned long)getc(fp);
    --size;
  }

  return v;
}

void
vf_tex_skip_n(FILE* fp, int size)
{
  while (size > 0){
    (void)getc(fp);
    --size;
  }
}
