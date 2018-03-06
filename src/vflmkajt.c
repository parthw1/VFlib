/* 
 * vflmkajt.c 
 * by Hirotsugu Kakugawa
 * - a vflibcap entry generator for TrueType fonts
 * - This program prints vflibcap entries to standard output.
 *
 *  15 May 2001
 */
/*
 * Copyright (C) 1996-2017 Hirotsugu Kakugawa. 
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
# include <unistd.h>
#endif
#ifdef HAVE_STRING_H
#  include <string.h>
#endif
#include <ctype.h>
#ifdef HAVE_SYS_PARAM_H
#  include  <sys/param.h>
#endif
#ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
#endif

#include  "VFlib-3_7.h"
#include  "VFsys.h"
#include  "vflibcap.h"
#include  "texfonts.h"
#include  "jtex.h"
#include  "fsearch.h"
#include  "vflmklib.h"



void  gen_class_deafult(void);


char  *dpi   = NULL;
int    dpi_i = DEFAULT_KPS_DPI;

#define NDIRS    64
int   n_dirs; 
char  *fontdirs[NDIRS];

char  *cs_name[2] = { "JISX0208", "JISX0212" };
char  *font_name[2][5] = { { "min",   "goth",  "tmin",  "tgoth",  NULL },
                           { "minh",  "gothh", "tminh", "tgothh", NULL } };
int   font_type[] =  {           0,        1,       2,        3,    -1 };
char  *font_class = "pcf";

char  *suffix = ".pk";
char  *ttf_ff_min[2] = { "sazanami-mincho.ttf", "sazanami-mincho_u.ttf" };
char  *ttf_ff_got[2] = { "sazanami-gothic.ttf", "sazanami-gothic_u.ttf" };
char  *ttf_ad_min[2] = { "sazanami-mincho",     "sazanami-mincho_u" };
char  *ttf_ad_got[2] = { "sazanami-gothic",     "sazanami-gothic_u" };
#if 0
char  *ttf_ff_min[2] = { "dfmimp3.ttc", "dfmin3_u.ttc" };
char  *ttf_ff_got[2] = { "dfgotp5.ttc", "dfgot5_u.ttc" };
char  *ttf_ad_min[2] = { "dfmimp3",     "dfmin3_u" };
char  *ttf_ad_got[2] = { "dfgotp5",     "dfgot5_u" };
#endif

#define NTTFS    1024
int   n_ttfs; 
char  *ext_texf[NTTFS];
char  *ext_ttff[NTTFS];
#define JTTF  ".jttf"

int  use_jis0212 = 0;

char *cmdline = NULL; 


void  gen_fonts(void);
void  gen_fonts_jtex(int cs);
void  gen_fonts_pcf(int cs);
void  gen_fonts_ekan(int cs);
void  gen_fonts_ttf(int cs);
void  gen_fonts_ttf_opt(int cs);

void  read_tex_ttf(char *f);
void  read_tex_ttf2(FILE *fp, char *f);



int 
main(int argc, char **argv)
{
  int     i;
  int    xargc;
  char **xargv;

  dpi = malloc(256);
  sprintf(dpi, "%d", dpi_i);

  cmdline = copy_cmdline(argc, argv);

  n_dirs = 0;
  for (i = 0; i < NDIRS; i++){
    fontdirs[i] = NULL;
  }

  n_ttfs = 0;
  for (i = 0; i < NTTFS; i++){
    ext_texf[i] = NULL;
    ext_ttff[i] = NULL;
  }

  xargc = argc; 
  xargv = argv;

  for (xargc--,xargv++; xargc > 0; xargc--,xargv++){
    if ((strcmp(*xargv, "--help") == 0)
	|| (strcmp(*xargv, "-help") == 0)){
      printf("vflmkajt: generates vflibcap entriy for ASCII Japanese TeX\n");
      printf("Usage: vflmkajt [options]\n");
      printf("Options\n");
      printf("  -d DIR    : TFM file directory\n");
      printf("  -c CLASS  : Font class of Kanji font file\n");
      printf("              Supported class: 'pcf', 'ttf', or ekanji'\n");
      printf("              Default is 'pcf' class.\n");
      printf("  -x EXT    : A suffix added for font name mapping\n");
      printf("              Default: '.pk'\n");
      printf("  -jisx0212 : Generate vflibcap entries for JISX0212 fonts\n");
      printf("Options (when 'ttf' class is selected for Kanji font file)\n");
      printf("  -tm TTF      : Use TrueType file TTF for JISX0208 'min'fonts\n");
      printf("  -tg TTF      : Use TrueType file TTF for JISX0208 'goth' fonts\n");
      printf("  -tm0212 TTF  : Use TrueType file TTF for JISX0212 'minh' fonts\n");
      printf("  -tg0212 TTF  : Use TrueType file TTF for JISX0212 'gothh' fonts\n");
      printf("  -tf FILE     : A database file for TeX font and TrueType font file");
      printf("  -tx FONT TTF : A TeX font entry with TrueType file\n");
      printf("  The format of a file for -tf option is a sequence of");
      printf("  lines, each of which contains TeX font name and TrueType");
      printf("  font file name.");

      printf("Example: vflmkajt -d TEXMF -c pcf\n");
      exit(0);

    } else if (strcmp(*xargv, "-d") == 0){
      /* font dir */
      if (n_dirs == NDIRS){
	fprintf(stderr, "Too many TFM font directories\n");
	exit(1);
      }
      xargv++; xargc--;
      check_argc(xargc);
      fontdirs[n_dirs++] = x_strdup(*xargv);

    } else if (strcmp(*xargv, "-c") == 0){
      xargv++; xargc--;
      check_argc(xargc);
      font_class = strdup(*xargv);
      if ((strcmp(font_class, "pcf") != 0) 
	  && (strcmp(font_class, "ekanji") != 0)
	  && (strcmp(font_class, "ttf") != 0)){
	fprintf(stderr, 
		"Unknown name. ('ttf', 'pcf', or 'ekanji' for -c option)\n");
	exit(1);
      }

    } else if (strcmp(*xargv, "-x") == 0){
      xargv++; xargc--;
      check_argc(xargc);
      if (strlen(*xargv) == 0){
	fprintf(stderr, "Suffix for -x option should not be zero length\n");
	exit(1);
      }
      suffix = strdup(*xargv);

    } else if (strcmp(*xargv, "-jisx0212") == 0){
      use_jis0212 = 1;

    } else if ((strcmp(*xargv, "-tm") == 0)
	       || (strcmp(*xargv, "-tm0208") == 0)){
      xargv++; xargc--;
      check_argc(xargc);
      ttf_ff_min[0] = strdup(*xargv);

    } else if ((strcmp(*xargv, "-tg") == 0)
	       || (strcmp(*xargv, "-tg0208") == 0)){
      xargv++; xargc--;
      check_argc(xargc);
      ttf_ff_got[0] = strdup(*xargv);

    } else if (strcmp(*xargv, "-tm0212") == 0){
      xargv++; xargc--;
      check_argc(xargc);
      ttf_ff_min[1] = strdup(*xargv);

    } else if (strcmp(*xargv, "-tg0212") == 0){
      xargv++; xargc--;
      check_argc(xargc);
      ttf_ff_got[1] = strdup(*xargv);

    } else if (strcmp(*xargv, "-tx") == 0){
      xargv++; xargc--;
      check_argc(xargc);
      if (n_ttfs == NTTFS){
	printf("Too many extra TrueType fonts\n"); 
	exit(1);
      }
      ext_texf[n_ttfs] = strdup(*xargv);
      xargv++; xargc--;
      check_argc(xargc);
      ext_ttff[n_ttfs] = strdup(*xargv);
      n_ttfs++;

    } else if (strcmp(*xargv, "-tf") == 0){
      xargv++; xargc--;
      check_argc(xargc);
      read_tex_ttf(*xargv);

    } else {
      if (*xargv[0] == '-'){
	fprintf(stderr, "vflmkajt: unknown option %s\n", *xargv);
	exit(1);
      }
      break;

    }
  }

  banner("Japanese TeX (ASCII Co. version)", "vflmkajt", cmdline);

  gen_class_deafult();    
  gen_fonts();

  return 0;
}



void
gen_class_deafult(void)
{
  int   i;

  printf("(%s %s", 
	 VF_CAPE_VFLIBCAP_CLASS_DEFAULT_DEFINITION, FONTCLASS_NAME_JTEX);
  printf("\n  (%s", VF_CAPE_TEX_TFM_DIRECTORIES);
  for (i = 0; i < n_dirs; i++)
    printf("\n       \"%s\"", fontdirs[i]);
  printf(")");
  printf("\n  (%s \".tfm\")", VF_CAPE_TEX_TFM_EXTENSIONS);
  printf("\n  (%s \"%s\")", VF_CAPE_JTEX_MAP_SUFIX, suffix);
  printf(")");
  printf("\n");
}


void
gen_fonts(void)
{
  if (strcmp(font_class, "pcf") == 0){
    printf("\n; %s", cs_name[0]);
    gen_fonts_pcf(0);
    gen_fonts_jtex(0);
    if (use_jis0212 == 1){
      printf("\n; %s", cs_name[1]);
      gen_fonts_pcf(1);
      gen_fonts_jtex(1);
    }
  } else if (strcmp(font_class, "ekanji") == 0){
    printf("\n; %s", cs_name[0]);
    gen_fonts_ekan(0);
    gen_fonts_jtex(0);
    if (use_jis0212 == 1){
      printf("\n; %s", cs_name[1]);
      gen_fonts_ekan(1);
      gen_fonts_jtex(1);
    }
  } else if (strcmp(font_class, "ttf") == 0){
    printf("\n; %s", cs_name[0]);
    gen_fonts_ttf(0);
    gen_fonts_jtex(0);
    gen_fonts_ttf_opt(0);
    if (use_jis0212 == 1){
      printf("\n; %s", cs_name[1]);
      gen_fonts_ttf(1);
      gen_fonts_jtex(1);
    }
  }
  printf("\n");
}


void
gen_fonts_jtex(int cs)
{
  int   p, i;
  static int   pt[] = { 5, 6, 7, 8, 9, 10, 12, -1};

  for (i = 0; font_name[cs][i] != NULL; i++){
    for (p = 0; pt[p] > 0; p++){
      printf("\n(%s   %s%d%s", 
	     VF_CAPE_VFLIBCAP_FONT_ENTRY_DEFINITION, 
	     font_name[cs][i], pt[p], suffix);
      printf("    \t(%s %2d)", VF_CAPE_JTEX_KF_POINT_SIZE, pt[p]);
      printf(" %s-def)", font_name[cs][i]);
    }
  }
  printf("\n");
}



void
gen_fonts_pcf(int cs)
{
  char  *f, *dr;
  int  i;
  char *adj[2]  = { "jiskan24",  
		    "jiskan24" };
  char *font[2] = { "\"jiskan24.pcf\" \"jiskan16.pcf\" \"k14.pcf\"",
		    "\"jisksp40.pcf\"" };
  
  for (i = 0; font_name[cs][i] != NULL; i++){
    f = font_name[cs][i];
    dr = ((font_type[i] & 0x02) == 0) ? "" : "t";
    printf("\n");
    printf("(define-macro %s-def", f);
    printf("\n  (font-class ascii-jtex-kanji) (kanji-font jtex-%s)", f);
    printf("\n  (kanji-font-magnification %.2f)", 0.85);
    printf("  (metric-adjustment-file \"%s%s.adj\"))", dr, adj[cs]);
    printf("\n");
    printf("(define-font jtex-%s", f);
    printf("\t(font-class pcf)");
    printf("\n");
    printf("  (font-file %s))", font[cs]);
  }
}



void
gen_fonts_ekan(int cs)
{
  char  *f, *dr;
  int  i;
  char *adj[2]  = { "ekanji",  
		    "ekanji" };
  char *font[2] = { "\"jisx9052.d24\" ",
		    "\"ekan0010.d24\"" };
  
  for (i = 0; font_name[cs][i] != NULL; i++){
    f = font_name[cs][i];
    dr = ((font_type[i] & 0x02) == 0) ? "" : "t";
    printf("\n");
    printf("(define-macro %s-def", f);
    printf("\n  (font-class ascii-jtex-kanji) (kanji-font jtex-%s)", f);
    printf("\n  (kanji-font-magnification %.2f)", 0.85);
    printf("  (metric-adjustment-file \"%s%s.adj\"))", dr, adj[cs]);
    printf("\n");
    printf("(define-font jtex-%s", f);
    printf("\t(font-class ekanji)");
    printf("\n");
    printf("  (font-file %s)", font[cs]);
    if ((font_type[i] & 0x02) == 0){
      printf(" (writing-direction \"horizontal\")");
    } else {
      printf(" (writing-direction \"vertical\")");
    }
    if (cs == 0){ /* jisx0208 */
      printf("\n  (character-set \"eKanji\")");
      printf(" (encoding \"ISO2022\")");
      printf("\n  (font-character-set \"eKanji\")");
      printf(" (font-encoding \"SEQUENTIAL2-1\")");
    } else {      /* jisx0212 */
      printf("\n  (mock-font-encoding with-offset -0x4dff)");
      printf("\n  (character-set \"JISX0212\")");
      printf(" (encoding \"ISO2022\")");
      printf("\n  (font-character-set \"Unicode\")");
      printf(" (font-encoding \"Unicode\")");
    }
    printf(")");
  }
}


void
gen_fonts_ttf(int cs)
{
  char  *f, *ff, *dr, *ad;
  int  i;

  for (i = 0; font_name[cs][i] != NULL; i++){
    f = font_name[cs][i];
    dr = ((font_type[i] & 0x02) == 0) ? "" : "t";
    ff = ((font_type[i] % 0x02) == 0) ? ttf_ff_min[cs] : ttf_ff_got[cs];
    ad = ((font_type[i] % 0x02) == 0) ? ttf_ad_min[cs] : ttf_ad_got[cs];
    printf("\n");
    printf("(define-macro %s-def", f);
    printf("\t(font-class ascii-jtex-kanji)");
    printf(" (kanji-font jtex-%s)", f);
    printf("\n ");
    printf(" (kanji-font-magnification %.2f)", 0.95);
    printf(" (metric-adjustment-file \"%s%s.adj\")", dr, ad);
    printf(")");
    printf("\n");
    printf("(define-font jtex-%s", f);
    printf("\t(font-class truetype)");
    printf(" (font-file \"%s\")", ff);
    printf("\n ");
    printf(" (character-set \"%s\")", cs_name[cs]);
    printf(" (encoding \"ISO2022\")");
    printf(")");
  }
  printf("\n");
}

void
gen_fonts_ttf_opt(int cs)
{
  int  i;

  if (n_ttfs == 0)
    return;

  printf("\n");
  printf("(define-macro jtex-h-def");
  printf("\t(font-class ascii-jtex-kanji)");
  printf("\n ");
  printf(" (kanji-font-point-size 10)");
  printf(" (metric-adjustment-file \"%s%s\")", "", "f5ajchm3.adj");
  printf(")");
  printf("\n");
  printf("(define-macro jtex-v-def");
  printf("\t(font-class ascii-jtex-kanji)");
  printf("\n ");
  printf(" (kanji-font-point-size 10)");
  printf(" (metric-adjustment-file \"%s%s\")", "t", "f5ajchm3.adj");
  printf(")");
  printf("\n");
  printf("(define-macro jtex-ttf");
  printf("\t\t(font-class truetype)");
  printf(" (dpi $TeX_DPI)");
  printf("\n ");
  printf(" (platform-id \"microsoft\")");
  printf(" (character-set \"%s\")", cs_name[cs]);
  printf(" (encoding \"ISO2022\")");
  printf(")");
  printf("\n");

  for (i = 0; i < n_ttfs; i++){
    printf("\n");
    printf("(define-font  %s%s\tjtex-h-def (kanji-font  %s%s))",
	   ext_texf[i], suffix, ext_texf[i], JTTF);
    printf("\n");
    printf("(define-font  %s%s\tjtex-ttf (font-file \"%s\"))",
	   ext_texf[i], JTTF, ext_ttff[i]);
    printf("\n");
    printf("(define-font t%s%s\tjtex-v-def (kanji-font t%s%s))",
	   ext_texf[i], suffix, ext_texf[i], JTTF);
    printf("\n");
    printf("(define-font t%s%s\tjtex-ttf (font-file \"%s\"))",
	   ext_texf[i], JTTF, ext_ttff[i]);
  }
}



void
read_tex_ttf(char *f)
{
  FILE  *fp;

  if (strcmp(f, "--") == 0){
    read_tex_ttf2(stdin, "standard input"); 
  } else {
    if ((fp = fopen(f, "r")) != NULL){
      read_tex_ttf2(fp, f); 
      fclose(fp);
    }
  }
}

void
read_tex_ttf2(FILE *fp, char *f)
{
  char  lbuf[BUFSIZ];
  char *p, *q[2];
  int  i;

  while (fgets(lbuf, sizeof(lbuf), fp) != NULL){
    if ((p = strchr(lbuf, '#')) != NULL)
      *p = '\0';
    if ((p = strchr(lbuf, '\n')) != NULL)
      *p = '\0';
    if (lbuf[0] == '\0')
      continue;
    q[0] = q[1] = NULL;
    p = lbuf; 
    for (i = 0; i < 2; i++){
      while ((*p != '\0') && (isspace((int)*p)))
	p++; 
      if (*p == '\0')
	break;
      q[i] = p;
      while ((*p != '\0') && (!isspace((int)*p)))
	p++;      
      if (*p == '\0')
	break;
      *p = '\0';
      p++; 
    }
    if (q[0] == NULL)
      continue;
    if (n_ttfs == NTTFS){
      printf("Too many extra TrueType fonts in %s\n", f); 
      exit(1);
    }
    if (q[1] == NULL)
      q[1] = q[0];
    ext_texf[n_ttfs] = x_strdup(q[0]);
    ext_ttff[n_ttfs] = x_strdup(q[1]);
    n_ttfs++;
  }
}
