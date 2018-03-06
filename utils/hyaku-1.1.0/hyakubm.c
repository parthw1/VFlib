/* hyakubm.c 
 * hyakubm  --- Make a bitmap of "Ogura Hyakunin Issyu" in PGM-ASCII format.
 * hyakux11 --- Display "Ogura Hyakunin Issyu" on an X11 window.
 * by Hirotsugu Kakugawa
 *
 * Copyright (C) 1998-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 * License: GPLv3 and FreeType Project License (FTL)
 */
/*
 *    Oct 1998  Version 1.0: only mode 1 fonts.
 * 10 Dec 1998  Version 1.1: supports mode1 and 2 switch. 
 *                 Uses "jiskan16.pcf" in mode 2 by default; this works
 *                 (possibly) all X11 environment.
 */

#include "../../src/config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#if defined(HAVE_STRING_H) || defined(STDC_HEADERS)
#  include  <string.h>
#else
#  include  <strings.h>
#endif
#include <VFlib-3_7.h>
#include "hyakubm.h"

#ifdef HYAKUX11
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#endif 

#define PARAM_NAME_DPI      "TeX_DPI"
#define PARAM_NAME_MODE     "TeX_KPATHSEA_MODE"
#define PARAM_NAME_PROG     "TeX_KPATHSEA_PROGRAM"

#define PARAM_DEFAULT_VFLIBCAP      "vflibcap"
#define PARAM_DEFAULT_DPI           300
#define PARAM_DEFAULT_KMODE         "cx"
#define PARAM_DEFAULT_PROG          "/usr/local/bin/hyakubm"
#define PARAM_DEFAULT_FONT_MODE     2
#define PARAM_DEFAULT_FONT_SIZE     -1
#define PARAM_DEFAULT_FONT_NAME     "jiskan16.pcf"
#define PARAM_DEFAULT_BASELINESKIP  1.4
#define PARAM_DEFAULT_WAIT          300
#define PARAM_DEFAULT_SHRINK        1
#define PARAM_DEFAULT_MARGIN_Y      5
#define PARAM_DEFAULT_MARGIN_X      5

static char*   param_vflibcap     = PARAM_DEFAULT_VFLIBCAP;
static int     param_dpi          = PARAM_DEFAULT_DPI;
static char*   param_kmode        = PARAM_DEFAULT_KMODE;
static char*   param_prog         = PARAM_DEFAULT_PROG;
static int     param_font_mode    = PARAM_DEFAULT_FONT_MODE;
static double  param_font_size    = PARAM_DEFAULT_FONT_SIZE;
static char*   param_font_name    = PARAM_DEFAULT_FONT_NAME;
static double  param_baselineskip = PARAM_DEFAULT_BASELINESKIP;
static int     param_shrink       = PARAM_DEFAULT_SHRINK;
static int     param_margin_t     = PARAM_DEFAULT_MARGIN_Y;
static int     param_margin_b     = PARAM_DEFAULT_MARGIN_Y;
static int     param_margin_l     = PARAM_DEFAULT_MARGIN_X;
static int     param_margin_r     = PARAM_DEFAULT_MARGIN_X;
static double  param_indent1 =  0;
static double  param_indent2 =  4;
static double  param_indent3 =  1;

#ifdef HYAKUX11
static double  param_wait         = PARAM_DEFAULT_WAIT;
static char*   param_geometry     = NULL;
#endif

void   usage(void);
void   hyakubm(int);
void   typeset(int,int,VF_BITMAPLIST);
void   typeset_phase(long*,int,VF_BITMAPLIST,long*,long*);
void   typeset_length(long*,int,long*,long*);
void   shipout(VF_BITMAP,FILE*,int);
#ifdef HYAKUX11
int    shipoutx11(VF_BITMAP,FILE*,int,int);
#endif 

#define PR1(s1)         fprintf(stderr, s1);
#define PR2(s1,s2)      fprintf(stderr, s1, s2);
#define PR3(s1,s2,s3)   fprintf(stderr, s1, s2, s3);



int
main(int argc, char **argv)
{
  int    poem_no, m;

  poem_no = -1;
  
  for (--argc, argv++; argc > 0; --argc, argv++){
    if (isdigit((int)*argv[0])){
      poem_no = atoi(*argv);
      if ((poem_no < 1) || (100 < poem_no)){
	usage();
      }
    } else {
      if ((strcmp(*argv, "-v") == 0) && (argc > 1)){
	param_vflibcap = argv[1];
	argc--; argv++;
      } else if (strcmp(*argv, "-cx") == 0){
	param_dpi = 300;  param_kmode = "cx";
      } else if (strcmp(*argv, "-sparcptr") == 0){
	param_dpi = 400;  param_kmode = "sparcptr";
      } else if (strcmp(*argv, "-ljfour") == 0){
	param_dpi = 600;  param_kmode = "ljfour";
      } else if ((strcmp(*argv, "-dpi") == 0) && (argc > 1)){
	param_dpi = atoi(argv[1]);
	argc--; argv++;
      } else if ((strcmp(*argv, "-mode") == 0) && (argc > 1)){
	param_kmode = argv[1];
	argc--; argv++;
      } else if ((strcmp(*argv, "-s") == 0) && (argc > 1)){
	param_shrink = atoi(argv[1]);
	argc--; argv++;
      } else if ((strcmp(*argv, "-f") == 0) && (argc > 1)){
	param_font_name = argv[1];
	argc--; argv++;
      } else if (strcmp(*argv, "-mode1") == 0){
	param_font_mode = 1;
      } else if (strcmp(*argv, "-mode2") == 0){
	param_font_mode = 2;
      } else if ((strcmp(*argv, "-p") == 0) && (argc > 1)){
	param_font_size = atof(argv[1]);
	argc--; argv++;
      } else if ((strcmp(*argv, "-f") == 0) && (argc > 1)){
	param_font_name = argv[1];
	argc--; argv++;
      } else if ((strcmp(*argv, "-b") == 0) && (argc > 1)){
	param_baselineskip = atof(argv[1]);
	argc--; argv++;
      } else if ((strcmp(*argv, "-g") == 0) && (argc > 1)){
	m = atoi(argv[1]);
	param_margin_t = param_margin_b = m;
	param_margin_l = param_margin_r = m;
	argc--; argv++;
      } else if ((strcmp(*argv, "-gy") == 0) && (argc > 1)){
	param_margin_t = param_margin_b = atoi(argv[1]);
	argc--; argv++;
      } else if ((strcmp(*argv, "-gx") == 0) && (argc > 1)){
	param_margin_l = param_margin_r = atoi(argv[1]);
	argc--; argv++;
#ifdef HYAKUX11
      } else if ((strcmp(*argv, "-w") == 0) && (argc > 1)){
	param_wait = atoi(argv[1]);
	argc--; argv++;
      } else if ((strcmp(*argv, "-geometry") == 0) && (argc > 1)){
	param_geometry = argv[1];
	argc--; argv++;
#endif
      } else if ((strcmp(*argv, "-i1") == 0) && (argc > 1)){
	param_indent1 = atof(argv[1]);
	argc--; argv++;
      } else if ((strcmp(*argv, "-i2") == 0) && (argc > 1)){
	param_indent2 = atof(argv[1]);
	argc--; argv++;
      } else if ((strcmp(*argv, "-i3") == 0) && (argc > 1)){
	param_indent3 = atof(argv[1]);
	argc--; argv++;
      } else if ((strcmp(*argv, "-h") == 0) || (strcmp(*argv, "--help") == 0)){
	usage();
      } else {
	usage();
      }
    }
  }

  hyakubm(poem_no);

  return 0;
}

void 
usage(void)
{
#ifndef HYAKUX11
  PR1("hyakubm ---  Make bitmap of Hyakunin-Issyu.\n");
#else
  PR1("hyakux11 ---  Print Hyakunin-Issyu on an X11 Window.\n");
#endif
  PR1("Usage: hyakubm [OPTIONS] [POEM_NO]\n");
  PR1("POEM_NO is the poem number, from 1 to 100\n");
  PR1("Options:\n");  
  PR2("  -v VFLIBCAP  vflibcap file [%s]\n", PARAM_DEFAULT_VFLIBCAP);  
  PR2("  -f FONT      font name [%s]\n", PARAM_DEFAULT_FONT_NAME);  
  PR1("  -mode1       font is opened in VFlib mode 1\n");
  PR1("  -mode2       font is opened in VFlib mode 2  (defualt mode)\n");
  PR1("  -p           font size in pixel (mode 1) or point (mode 2)\n");
  PR2("  -dpi         device resolution (mode 1) [%d]\n",
      PARAM_DEFAULT_DPI);
  PR2("  -mode        device mode for font search in kpathsea [%s]\n",
      PARAM_DEFAULT_KMODE);
  PR1("  -cx          same as '-dpi 300 -mode cx'\n");
  PR1("  -sparcptr    same as '-dpi 400 -mode sparcptr'\n");
  PR1("  -ljfour      same as '-dpi 600 -mode ljfour'\n");
  PR2("  -b B         baseline skip factor [%.2f]\n", 
      PARAM_DEFAULT_BASELINESKIP);
  PR2("  -gx MX       horizontal margin [%d]\n", PARAM_DEFAULT_MARGIN_X);
  PR2("  -gy MY       vertical margin [%d]\n", PARAM_DEFAULT_MARGIN_Y);
  PR1("  -g M         same as '-gx M -gy M'\n");  
  PR2("  -s S         shrink factor of an image [%d]\n", PARAM_DEFAULT_SHRINK);
#ifdef HYAKUX11
  PR2("  -w S         wait for S second for each poem [%d]\n",
      PARAM_DEFAULT_WAIT);
  PR1("  -geometry G  window geometry\n");
#endif
  exit(0);
}


void
hyakubm(int poem_no)
{
  int            fid, p;
  VF_BITMAP      bm;
  char           vflib_param[1024];
  struct vf_s_bitmaplist  bmlist;

  sprintf(vflib_param, "%s=%d, %s=%s, %s=%s",
	  PARAM_NAME_DPI,  param_dpi,
	  PARAM_NAME_MODE, param_kmode, 
	  PARAM_NAME_PROG, param_prog);
  if (VF_Init(param_vflibcap, vflib_param) < 0){
    PR1("Failed to initialize VFlib\n");
    return;
  }

  if (param_font_mode == 1){
    fid = VF_OpenFont1(param_font_name, param_dpi, param_dpi, param_font_size, 
		       1.0, 1.0);
  } else {
    fid = VF_OpenFont2(param_font_name, param_font_size, 1.0, 1.0);
  }
  if (fid < 0){
    PR2("Failed to open a font: %s\n", param_font_name);
    return;
  }

  p = poem_no;
  if (p < 1)
    p = 1;

#ifndef HYAKUX11

  VF_BitmapListInit(&bmlist);
  typeset(p, fid, &bmlist);
  bm = VF_BitmapListCompose(&bmlist);
  VF_BitmapListFinish(&bmlist);
  shipout(bm, stdout, p);

#else

  for (;;){
    VF_BitmapListInit(&bmlist);
    typeset(p, fid, &bmlist);
    bm = VF_BitmapListCompose(&bmlist);
    VF_BitmapListFinish(&bmlist);
    p = shipoutx11(bm, stdout, p, param_wait);
    if (p > 100)
      p = 100;
    if (p < 1)
      p = 1;
  }

#endif

  VF_CloseFont(fid);
}

void 
typeset(int poem_no, int fid, VF_BITMAPLIST bmlist)
{
  VF_BITMAP   bm3121;
  int         dir;
  long        refp_x, refp_y;
  long        dx, dy, w, h;
  long        ref1, ref2, ref_max, x3, y3;

  if (param_font_mode == 1){
    bm3121 = VF_GetBitmap1(fid, 0x3121L, 1.0, 1.0);
  } else {
    bm3121 = VF_GetBitmap2(fid, 0x3121L, 1.0, 1.0);
  }
  dx = bm3121->mv_x;
  dy = bm3121->mv_y;
  w  = bm3121->bbx_width;
  h  = bm3121->bbx_height;

  if (dy == 0)
    dir = 0;  /* horizontal */
  else
    dir = 1;  /* vertical */

  if (dir == 0){

    refp_x = param_indent1 * dx;
    refp_y = 0;
    typeset_phase(poem_table[poem_no-1].phase1, fid, bmlist, &refp_x, &refp_y);
    typeset_phase(poem_table[poem_no-1].phase2, fid, bmlist, &refp_x, &refp_y);
    typeset_phase(poem_table[poem_no-1].phase3, fid, bmlist, &refp_x, &refp_y);
    ref1 = refp_x;
    
    refp_x = param_indent2 * dx;
    refp_y -= param_baselineskip * h;
    typeset_phase(poem_table[poem_no-1].phase4, fid, bmlist, &refp_x, &refp_y);
    typeset_phase(poem_table[poem_no-1].phase5, fid, bmlist, &refp_x, &refp_y);
    ref2 = refp_x;
    
    refp_y -= param_baselineskip * h;
    if ((ref_max = ref1) < ref2)
      ref_max = ref2;
    typeset_length(poem_table[poem_no-1].auth, fid, &x3, &y3);
    refp_x = ref_max - x3 + param_indent3 * dx;
    typeset_phase(poem_table[poem_no-1].auth, fid, bmlist, &refp_x, &refp_y);

  } else /* vertical */ {  

    refp_x = 0;
    refp_y = param_indent1 * dy;
    typeset_phase(poem_table[poem_no-1].phase1, fid, bmlist, &refp_x, &refp_y);
    typeset_phase(poem_table[poem_no-1].phase2, fid, bmlist, &refp_x, &refp_y);
    typeset_phase(poem_table[poem_no-1].phase3, fid, bmlist, &refp_x, &refp_y);
    ref1 = refp_y;
    
    refp_x -= param_baselineskip * w;
    refp_y = param_indent2 * dy;
    typeset_phase(poem_table[poem_no-1].phase4, fid, bmlist, &refp_x, &refp_y);
    typeset_phase(poem_table[poem_no-1].phase5, fid, bmlist, &refp_x, &refp_y);
    ref2 = refp_y;
    
    refp_x -= param_baselineskip * w;
    if ((ref_max = ref1) > ref2)        /* Note: ref1, ref2 < 0 */
      ref_max = ref2;
    typeset_length(poem_table[poem_no-1].auth, fid, &x3, &y3);
    refp_y = ref_max - y3 + param_indent3 * dy;
    typeset_phase(poem_table[poem_no-1].auth, fid, bmlist, &refp_x, &refp_y);
  }
}

void
typeset_phase(long *s, int fid, VF_BITMAPLIST bmlist, 
	      long *refp_x, long *refp_y)
{
  int   i;
  VF_BITMAP  bm;
  
  for (i = 0; s[i] != 0L; i++){
    if (param_font_mode == 1){
      bm = VF_GetBitmap1(fid, s[i], 1.0, 1.0);
    } else {
      bm = VF_GetBitmap2(fid, s[i], 1.0, 1.0);
    }
    VF_BitmapListPut(bmlist, bm, *refp_x, *refp_y);
    *refp_x += bm->mv_x; 
    *refp_y += bm->mv_y;
  }
}


void
typeset_length(long *s, int fid, long *x, long *y)
{
  int   i;
  long  junkx, junky;
  VF_BITMAP  bm;
  
  if (x == NULL)
    x = &junkx;
  if (y == NULL)
    y = &junky;
  *x = 0;
  *y = 0;
  for (i = 0; s[i] != 0L; i++){
    if (param_font_mode == 1){
      bm = VF_GetBitmap1(fid, s[i], 1.0, 1.0);
    } else {
      bm = VF_GetBitmap2(fid, s[i], 1.0, 1.0);
    }
    *x += bm->mv_x; 
    *y += bm->mv_y;
    VF_FreeBitmap(bm);
  }
}


#ifndef HYAKUX11

void 
shipout(VF_BITMAP bm, FILE *fp, int poem_no)
{
  char  title[512];

  sprintf(title, "Hyakunin Issyu #%d", poem_no);
  VF_ImageOut_PGMAscii(bm, fp, -1, -1, 
		       VF_IMAGEOUT_POSITION_NONE, VF_IMAGEOUT_POSITION_NONE,
		       param_margin_l, param_margin_r, 
		       param_margin_t, param_margin_b, 
		       0, param_shrink,  "hyakubm", title);
}

#else

static int              x_initialized = 0;
static int              x_mapped = 0;
static Display         *x_disp;
static Window           x_win;
static GC               x_gc_win;
static int              x_w, x_h;
static int              x_aa;
static unsigned long    x_pix_fg;
static unsigned long    x_pix_bg;
static unsigned long   *x_pix_table  = NULL;

static char *param_fg = "black";
static char *param_bg = "white";
static unsigned char  bits[] = {
  0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };

static void  x_create_window(VF_BITMAP);
static void  x_update_window(VF_BITMAP,int);


int
shipoutx11(VF_BITMAP bm, FILE *fp, int p, int t)
{
  int     waitsec;
  XEvent  xev;
  KeySym  ks;
  char   keyin[16];

  if (x_initialized == 0){
    x_create_window(bm);
    x_initialized = 1;
  }

  x_update_window(bm, p);

  if ((waitsec = t) < 0)
    waitsec = 1;

  while ((t < 0) || (waitsec > 0)){
    while ((waitsec > 0) && (XPending(x_disp) == 0)){
      sleep(1);
      waitsec--; 
    }
    if (XPending(x_disp) > 0){
      XNextEvent(x_disp, &xev);
      switch (xev.type){
      case Expose:
	while (XCheckWindowEvent(x_disp, x_win, ExposureMask, &xev) == True)
	  ;
	x_update_window(bm, p);
	break;
      case KeyPress:
	if (XLookupString(&xev.xkey, keyin, sizeof(keyin), &ks, NULL) != 1){
	  switch (ks){
	  case XK_space:      return (p) % 100 + 1;
	  }
	} else {
	  switch (keyin[0]){
	  case ' ':           return (p) % 100 + 1;
	  case 'b': case 'B': return (p-2+100) % 100 + 1;
	  case '<':           return 1;
	  case '>':           return 100;
	  case 'q': case 'Q': exit(0);
	  }
	}
      }
    }
  }

  return (p) % 100 + 1;
}

void
x_create_window(VF_BITMAP bm)
{
  XSizeHints   hints;
  XColor       col, xc_fg, xc_bg, xc;
  long         w, h;
  int          geom_x, geom_y;
  unsigned int geom_w, geom_h;
  int          i, gf;

  if (x_initialized == 1)
    return;

  x_aa = param_shrink * param_shrink + 1;
  w = bm->bbx_width;
  h = bm->bbx_height;

  if ((x_disp = XOpenDisplay(NULL)) == NULL){
    PR1("Can't open X display.\n");
    exit(0);
  }

  XAllocNamedColor(x_disp, DefaultColormap(x_disp,0), param_fg, &xc_fg, &xc); 
  XAllocNamedColor(x_disp, DefaultColormap(x_disp,0), param_bg, &xc_bg, &xc); 
  x_pix_fg = xc_fg.pixel;
  x_pix_bg = xc_bg.pixel;

  geom_x = geom_y = geom_w = geom_h = 1;
  gf = XParseGeometry(param_geometry, &geom_x, &geom_y, &geom_w, &geom_h);

  x_win = XCreateSimpleWindow(x_disp, RootWindow(x_disp, 0), geom_x, geom_y, 
			      w, h, 2, XBlackPixel(x_disp,0), x_pix_bg);

  if (((gf & XValue) != 0) || ((gf & YValue) != 0)){
    hints.flags = USPosition;
    hints.x = geom_x;
    hints.y = geom_y;
    XSetStandardProperties(x_disp, x_win, "", "", None, NULL, 0, &hints);
  }

  x_gc_win = XCreateGC(x_disp, x_win, 0, 0);
  x_pix_table = (unsigned long*)malloc(x_aa * sizeof(unsigned long));
  if (x_pix_table == NULL){
    PR1("No memory.\n");
    exit(0);
  }
  for (i = 0; i < x_aa; i++){
    col.flags = DoRed | DoGreen | DoBlue;
    col.red   = xc_bg.red   + ((xc_fg.red   - xc_bg.red)   * i)/(x_aa-1);
    col.green = xc_bg.green + ((xc_fg.green - xc_bg.green) * i)/(x_aa-1);
    col.blue  = xc_bg.blue  + ((xc_fg.blue  - xc_bg.blue)  * i)/(x_aa-1);
    if (XAllocColor(x_disp, DefaultColormap(x_disp, 0), &col) == 0){
      PR1("Can't allocate colors.\n");
      exit(0);
    }
    x_pix_table[i] = col.pixel;
  }
  XSetForeground(x_disp, x_gc_win, x_pix_table[0]);
  XSetBackground(x_disp, x_gc_win, x_pix_table[x_aa-1]);
  XStoreName(x_disp, x_win, "OHI");
  XSelectInput(x_disp, x_win, KeyPressMask|ButtonPressMask|ExposureMask);
}

void
x_update_window(VF_BITMAP bm, int poem_no)
{
  int     x, y, z;
  int    *pgm_buff, *g, pgm_w, pgm_h, max_val;
  char   name[32];
  XImage         *x_image;
  unsigned char  *p;

  if (param_shrink <= 0)
    param_shrink = 1;
  max_val = param_shrink * param_shrink;
  pgm_w = (bm->bbx_width  + param_shrink - 1) / param_shrink;
  pgm_h = (bm->bbx_height + param_shrink - 1) / param_shrink;

  x_w = param_margin_l + pgm_w + param_margin_r;
  x_h = param_margin_t + pgm_h + param_margin_b;

  sprintf(name, "OHI#%d", poem_no);
  XStoreName(x_disp, x_win, name);
  XResizeWindow(x_disp, x_win, x_w, x_h);
  if (x_mapped == 0){
    XMapWindow(x_disp, x_win);
    x_mapped = 1;
  }
  XClearWindow(x_disp, x_win);

  if ((pgm_buff = calloc(pgm_w * pgm_h, sizeof(int))) == NULL){
    PR1("No memory.\n");
    exit(1);
  }

  p = bm->bitmap;
  for (y = 0; y < bm->bbx_height; y++){ 
    g = &pgm_buff[(y/param_shrink)*pgm_w];
    for (x = 0; x < bm->bbx_width; x++){ 
      if ((p[x/8] & bits[x%8]) != 0)
	g[x/param_shrink] += 1;
    }
    p = p + bm->raster;
  }

  if ((x_image = XCreateImage(x_disp, 
			      DefaultVisual(x_disp,0), DefaultDepth(x_disp,0),
			      ZPixmap, 0, NULL, x_w, x_h, 8, 0)) == NULL){
    PR1("No memory.\n");
    exit(0);
  }
  z = x_image->bytes_per_line * x_h;
  if ((x_image->data = (char*)malloc((z!=0)?z:1)) == NULL){
    PR1("No memory.\n");
    exit(0);
  }

  g = pgm_buff;
  for (y = 0; y < pgm_h; y++){
    for (x = 0; x < pgm_w; x++)
      XPutPixel(x_image, x, y, x_pix_table[pgm_buff[pgm_w * y + x]]);
    g = g + pgm_w;
  }
 
  XPutImage(x_disp, x_win, x_gc_win, x_image, 0, 0,
	    param_margin_l, param_margin_t, pgm_w, pgm_h);

  XDestroyImage(x_image);
  free(pgm_buff);
}

#endif

/*EOF*/
