/*
 * vfx11 - Display characters on a X11R6 window
 *  by Hirotsugu Kakugawa (h.kakugawa@computer.org)
 *
 * Edition History
 *  24 Jan 1997  Simple version.
 *  25 Jan 1997  Enhanced parsing of cmd line args.
 *               Added key operations: '<', '>', 'm', 'g', and '?'.
 *  28 Jan 1997  Added '+', '-', and 'r' operations. 
 *  22 Mar 1997  Upgrade for VFlib 3.2
 *  20 May 1997  Bug fixed. (Window Clear)
 *   1 Sep 1998  Upgrade for VFlib 3.5
 *  21 Apr 2010  A fixed to avoid core-dump in _XrmInternalStringToQuark().
 */
/*
 * Copyright (C) 1997-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */


#include "../../src/config.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#if defined(HAVE_STRING_H) || defined(STDC_HEADERS)
#  include  <string.h>
#else
#  include  <strings.h>
#endif
#include "../../src/VFlib-3_7.h"

#define VFLX11_VERSION  "2.0.0"

#define CMD_NOP         0
#define CMD_NEXT_PAGE   1
#define CMD_PREV_PAGE   2
#define CMD_EXIT        3
#define CMD_RESIZE      4
#define CMD_FIRST_PAGE  5
#define CMD_LAST_PAGE   6
#define CMD_SET_MARK    7
#define CMD_GOTO_MARK   8
#define CMD_HELP        9
#define CMD_P4PAGES    10
#define CMD_N4PAGES    11
#define CMD_P16PAGES   12
#define CMD_N16PAGES   13
#define CMD_P64PAGES   14
#define CMD_N64PAGES   15
#define CMD_ENLARGE    20
#define CMD_SHRINK     21
#define CMD_REDRAW     22
#define CMD_REOPEN     23

#define POLL_NOTHING    0
#define POLL_EVENT      1

#define ACT_NONE     0
#define ACT_BREAK    1
#define ACT_WINCH    2
#define ACT_REDRAW   3


#define DEFAULT_PIXEL_SIZE    24
#define CHAR_BORDER_FACTOR    1.2

int     Mode = 2;
char   *FontName;
double  ArgFontSize = -1;
double  FontSize = -1;
double  Dpi_X = 1, Dpi_Y = -1;
int     OutlineMode = 0;
int     Verbose = 0;
int     Mode94x94 = 0;
int     PageSize = 256;
double  CharBorderX = CHAR_BORDER_FACTOR;
double  CharBorderY = CHAR_BORDER_FACTOR;

#define CHARS_PER_LINE  16
int  Font_ID;
int  Page;
int  PageMin, PageMax;
int  StartInPage, EndInPage;
int  SkipBeginInPage, SkipEndInPage;

int  CharWidth, CharHeight, CharLines;
int  CharWidth0, CharHeight0;
double  FMagX, FMagY;
int     MagX,  MagY; 
#define MAG_SCALE 100.0
#define MAGX()  ((double)MagX/MAG_SCALE)
#define MAGY()  ((double)MagY/MAG_SCALE)

int   usage(), key_usage();
int   OpenFont(), Loop(), Cmd(), CodePoint();
void  change_win_size();


void  Win_Init(), Win_Clear(), Win_PutBitmap(), Win_Beep(), Win_ChangeSize();
int   Win_UserCmd(), Win_PollUserCmd();


extern  double atof();


int
main(argc, argv)
  int  argc;
  char **argv;
{
  char    *vflibcap;
  int     na;

  Mode        = 2;
  FontName    = NULL;
  ArgFontSize = -1;
  FontSize    = -1;
  FMagX       = 1.0;
  FMagY       = 1.0;
  MagX        = 1 * MAG_SCALE;
  MagY        = 1 * MAG_SCALE;
  CharBorderX = CHAR_BORDER_FACTOR;
  CharBorderY = CHAR_BORDER_FACTOR;
  OutlineMode = 0;

  vflibcap  = NULL;
  for (argv++, argc--; argc > 0; argc -= na, argv += na){
    na = 1;
    if (argv[0][0] != '-'){
      FontName = argv[0];      
      continue;
    } else if (strcmp(argv[0], "-verbose") == 0){
      Verbose = 1;
      continue;
    } else if (strcmp(argv[0], "-94x94") == 0){
      Mode94x94 = 1;
      continue;
    } else if (strcmp(argv[0], "-mode1") == 0){
      Mode = 1;
      continue;
    } else if (strcmp(argv[0], "-mode2") == 0){
      Mode = 2;
      continue;
    } else if (strcmp(argv[0], "-ol") == 0){
      OutlineMode = 1;
      continue;
    } else if (strcmp(argv[0], "-help") == 0){
      usage();
      exit(0);
    } else if (strcmp(argv[0], "--help") == 0){
      usage();
      exit(0);
    }
    na = 2;
    if (argc < 2){
      usage();
      break;
    }
    if (strcmp(argv[0], "-f") == 0){
      FontName = argv[1];
    } else if (strcmp(argv[0], "-v") == 0){
      vflibcap = argv[1];
    } else if (strcmp(argv[0], "-s") == 0){
      PageSize = atoi(argv[1]);
    } else if (strcmp(argv[0], "-p") == 0){
      ArgFontSize = atof(argv[1]);
    } else if (strcmp(argv[0], "-m") == 0){
      FMagX = FMagY = atof(argv[1]);
    } else if (strcmp(argv[0], "-mx") == 0){
      FMagX = atof(argv[1]);
    } else if (strcmp(argv[0], "-my") == 0){
      FMagY = atof(argv[1]);
    } else if (strcmp(argv[0], "-b") == 0){
      CharBorderX = CharBorderY = atof(argv[1]);
      if (CharBorderX < 0)
	CharBorderX = CharBorderY = CHAR_BORDER_FACTOR;
    } else if (strcmp(argv[0], "-bx") == 0){
      CharBorderX = atof(argv[1]);
      if (CharBorderX < 0)
	CharBorderX = CHAR_BORDER_FACTOR;
    } else if (strcmp(argv[0], "-by") == 0){
      CharBorderY = atof(argv[1]);
      if (CharBorderY < 0)
	CharBorderY = CHAR_BORDER_FACTOR;
    } else {
      usage();
    }
  }

  if (FontName == NULL)
    usage();

  if ((Mode == 2) && (OutlineMode == 1)){
    fprintf(stderr, "Warning: Outline mode is not supported in mode 2.\n");
  }

  if (VF_Init(vflibcap, NULL) < 0){
    fprintf(stderr, "Initializing VFlib: failed\n");
    if (vf_error == VF_ERR_NO_VFLIBCAP){
      if (vflibcap == NULL){
	fprintf(stderr, "Could not read default vflibcap file.\n");
      } else {
	fprintf(stderr, "Could not read vflibcap file: %s.\n", vflibcap);
      }
    }
    exit(1);
  }

  PageMin     = 0;
  PageMax     = 0xffff;
  StartInPage = 0x00;
  EndInPage   = 0xff;
  if (Mode94x94 != 0){
    StartInPage = 0x20;
    EndInPage   = 0x7f;
  }
  SkipBeginInPage = -1;
  SkipEndInPage   = -1;
  Font_ID = -1;

  if (Verbose == 1)
    printf("Opening font: %s\n", FontName);

  if (OpenFont() < 0){
    fprintf(stderr, "Failed to open a font: %s\n", FontName);
    exit(1);
  }

  Loop();

  return 0;
}

int usage()
{
  printf("vflx11 - Display VFlib font version %s, based on VFlib %s.\n",
	 VFLX11_VERSION, VF_GetVersion());
  printf("Usage: vfx11 [Options] FONT\n");
  printf("Options: \n");
  printf("   -v VFLIBCAP : vflibcap file\n");
  printf("   -p PIXEL    : pixel size (or point size)\n");
  printf("   -m MAG      : magnification\n");
  printf("   -mx MAG_X   : horizontal magnification\n");
  printf("   -my MAG_Y   : vertical magnification\n");
  printf("   -f FONT     : font name\n");
  printf("   -mode1      : open font in mode 1\n");
  printf("   -mode2      : open font in mode 2\n");
  printf("   -ol         : outline mode for mode 1\n");

  key_usage();
  printf("Example: vflx11 -v vflibcap-pcf -m 0.5 timR24.pcf\n");
  exit(0);
}

int
OpenFont(void)
{
  int  fd;
  char  *p;

  fd = -1;
  if (Mode == 1){
    if (ArgFontSize < 0)
      fd = VF_OpenFont1(FontName, -1, -1,       -1, FMagX, FMagY);
    else
      fd = VF_OpenFont1(FontName, -1, -1, ArgFontSize, FMagX, FMagY);
  } else if (Mode == 2){
    if (ArgFontSize < 0)
      fd = VF_OpenFont2(FontName,          -1, FMagX, FMagY);
    else
      fd = VF_OpenFont2(FontName, ArgFontSize, FMagX, FMagY);
  }
  
  if (fd < 0)
    return -1;
  
  if ((p = VF_GetFontProp(fd, "PIXEL_SIZE")) != NULL)
    FontSize = atof(p);
  else
    FontSize = DEFAULT_PIXEL_SIZE;
  
  if (Font_ID >= 0){
    VF_CloseFont(Font_ID);
    Font_ID = -1;
  }

  Font_ID = fd;
  
  if (Verbose == 1)
    printf("Pixel size: %d\n", (int)FontSize);

  return 0;
}

int key_usage()
{
  printf("Key and mouse operations:\n");
  printf("   q (or mouse middle button)   : exit\n");
  printf("   b (or mouse left button)     : go to previous page\n");
  printf("   SPC (or mouse right button)  : go to next page\n");
  printf("   m        : set mark on current page\n");
  printf("   g        : go to marked page\n");
  printf("   +        : enlarge window\n");
  printf("   -        : shrink window\n");
  printf("   d        : redraw window\n");
  printf("   ?        : help\n");

  return 0;

}


int Loop()
{
  VF_BITMAP  bm = NULL, bm2;
  VF_OUTLINE ol;
  int        act, nl0, nl1, c;

  if (MagX < 0)
    MagX = 1 * MAG_SCALE;
  if (MagY < 0)
    MagY = 1 * MAG_SCALE;

  nl0 = StartInPage/CHARS_PER_LINE;
  nl1 = (EndInPage+CHARS_PER_LINE-1)/CHARS_PER_LINE;
  CharLines  = nl1 - nl0;

  CharWidth  = FontSize * CharBorderX;
  CharHeight = FontSize * CharBorderY;

  Win_Init(CharWidth * CHARS_PER_LINE, CharHeight * CharLines);

  Page = PageMin;

  for (;;){
    Win_Clear();

AGAIN:    
    for (c = StartInPage; c <= EndInPage; c++){

      if ((SkipBeginInPage <= c) && (c <= SkipEndInPage))
	continue;

      if (Win_PollUserCmd() == POLL_EVENT){
	if ((act = Cmd()) != ACT_NONE){
	  if (act == ACT_WINCH) 
	    change_win_size();
	  Win_Clear();
	  goto AGAIN;
	}
      }

      bm = NULL;
      if (Mode == 1){
	if (OutlineMode == 0){
	  bm = VF_GetBitmap1(Font_ID, CodePoint(c), MAGX(), MAGY());
	} else {
	  bm = NULL;
	  if ((ol = VF_GetOutline(Font_ID, CodePoint(c),
				  MAGX(), MAGY())) != NULL){
	    bm = VF_OutlineToBitmap(ol, -1, -1, -1, 1.0, 1.0);
	    VF_FreeOutline(ol);
	  }
	}
      } else if (Mode == 2){
	bm = VF_GetBitmap2(Font_ID, CodePoint(c), MAGX(), MAGY());
      }

      if (bm != NULL){
	bm2 = VF_MinimizeBitmap(bm);
	if (bm2 != NULL){
	  Win_PutBitmap(bm2, c%CHARS_PER_LINE, c/CHARS_PER_LINE - nl0, 
			CharWidth, CharHeight);
	  VF_FreeBitmap(bm2);
	} else {
	  Win_PutBitmap(bm, c%CHARS_PER_LINE, c/CHARS_PER_LINE - nl0, 
			CharWidth, CharHeight);
	}
	VF_FreeBitmap(bm);
      }
    }

    while ((act = Cmd()) == ACT_NONE)
      ;
    if (act == ACT_WINCH)
      change_win_size();
    
  }

  return 0;
}

int Cmd()
{
  static int  markedPage = -100;
  int         temp;

  switch (Win_UserCmd()){
  default:
  case CMD_NOP:
    return ACT_NONE;
  case CMD_EXIT:
    exit(0);
  case CMD_PREV_PAGE:
    if (Page > PageMin){
      Page = Page - 1;
      return ACT_BREAK;
    } else 
      Win_Beep();
    break;
  case CMD_NEXT_PAGE:
    if (Page < PageMax){
      Page = Page + 1;
      return ACT_BREAK;
    } else
      Win_Beep();
    break;
  case CMD_N64PAGES:
    Page += 64;
    if (Page > PageMax)
      Page = PageMax;
    return ACT_BREAK;
  case CMD_P64PAGES:
    Page -= 64;
    if (Page < PageMin)
      Page = PageMin;
    return ACT_BREAK;
  case CMD_N16PAGES:
    Page += 16;
    if (Page > PageMax)
      Page = PageMax;
    return ACT_BREAK;
  case CMD_P16PAGES:
    Page -= 16;
    if (Page < PageMin)
      Page = PageMin;
    return ACT_BREAK;
  case CMD_N4PAGES:
    Page += 4;
    if (Page > PageMax)
      Page = PageMax;
    return ACT_BREAK;
  case CMD_P4PAGES:
    Page -= 4;
    if (Page < PageMin)
      Page = PageMin;
    return ACT_BREAK;
  case CMD_FIRST_PAGE:
    Page = PageMin;
    return ACT_BREAK;
  case CMD_LAST_PAGE:
    Page = PageMax;
    return ACT_BREAK;
  case CMD_SET_MARK:
    markedPage = Page;
    break;
  case CMD_GOTO_MARK:
    if (markedPage >= PageMin){
      temp = Page;
      Page = markedPage;
      markedPage = temp;
      return ACT_BREAK;
    } else
      Win_Beep();
    break;
  case CMD_HELP:
    key_usage();
    break;
  case CMD_RESIZE:
    return ACT_WINCH;
  case CMD_REDRAW:
    return ACT_REDRAW;
  case CMD_ENLARGE:
    MagX = MagX + 20;
    MagY = MagY + 20;
    return ACT_WINCH;
  case CMD_SHRINK:
    if ((MagX > 30) && (MagY > 30)){
      MagX = MagX - 20;
      MagY = MagY - 20;
    }
    return ACT_WINCH;
  case CMD_REOPEN:
    if (Verbose == 1)
      printf("Reopening font: %s\n", FontName);
    if (OpenFont() < 0){
      fprintf(stderr, "Failed to reopen a font: %s\n", FontName);
      exit(1);
    }
    return ACT_REDRAW;
  }

  return ACT_BREAK;
}

int CodePoint(c)
     int c;
{
  return Page * PageSize + c;
}

void
change_win_size()
{

  CharWidth  = FontSize * CharBorderX * MAGX();
  CharHeight = FontSize * CharBorderY * MAGY();

  Win_ChangeSize(CharWidth * CHARS_PER_LINE, CharHeight * CharLines);
}



/*------------------------*/
Display               *Disp;
Window                Win;
XEvent                *xev = NULL;
GC                    Gc;
XGCValues             GcVal;
XSetWindowAttributes  Att;
unsigned int          WinX, WinY, WinBorder, WinDepth;
char                  *DisplayName; 
char                  WindowName[256];
#define  WIN_BORDER  3


void Win_Init(w, h)
     int  w, h;
{
  if ((w > 2000) || (h > 2000)){
    fprintf(stderr, "Window is too large: (%dx%d)\n", w, h);
    exit(0);
  }

  WinX = w + WIN_BORDER * 2;
  WinY = h + WIN_BORDER * 2;
  DisplayName = NULL;
  Disp = XOpenDisplay(DisplayName);
  if (Disp == NULL){
    fprintf(stderr, "Can't open display\n"); 
    exit(1);
  }
  Win = XCreateSimpleWindow(Disp, RootWindow(Disp, 0), 
			    0, 0, WinX, WinY, 2,
			    BlackPixel(Disp, 0), WhitePixel(Disp, 0));
  Gc     = XCreateGC(Disp, Win, 0, 0);
  XSetForeground(Disp, Gc, WhitePixel(Disp, 0));

/*
  Att.override_redirect = True;
  Att.backing_store     = Always;
  XChangeWindowAttributes(Disp, Win, CWOverrideRedirect, &Att);
  XChangeWindowAttributes(Disp, Win, CWBackingStore, &Att);
*/
  XSelectInput(Disp, Win, 
	       (KeyPressMask|ButtonPressMask|ExposureMask|ConfigureNotify)); 
  XMapWindow(Disp, Win);
  Win_Clear();
  XFlush(Disp);
}

void Win_Clear()
{
  sprintf(WindowName, "VFlib font: %s (0x%X-0x%X / %d-%d)",
	  FontName, CodePoint(StartInPage), CodePoint(EndInPage),
	  CodePoint(StartInPage), CodePoint(EndInPage));
  XStoreName(Disp, Win, WindowName);
  XFillRectangle(Disp, Win, Gc, 0, 0, WinX, WinY);
}

void Win_Beep()
{
  XBell(Disp, 50);
}

void Win_PutBitmap(bm, x, y, cw, ch)
     VF_BITMAP  bm;
     int        x, y, cw, ch;
{
  int     w, h, ix, iy, i, n;
  unsigned char  *xbitmap, c;
  Pixmap         pix;
  unsigned long  fg, bg;
  unsigned int   depth;
  static unsigned char bit_rev[] = {
    /* 0000 0001 0010 0011 0100 0101 0110 0111 */
      0x0, 0x8, 0x4, 0xc, 0x2, 0xa, 0x6, 0xe,
    /* 1000 1001 1010 1011 1100 1101 1110 1111 */
      0x1, 0x9, 0x5, 0xd, 0x3, 0xb, 0x7, 0xf };

  if (bm == NULL)
    return;

  depth = DefaultDepth(Disp, DefaultScreen(Disp));
  fg = BlackPixel(Disp, DefaultScreen(Disp));
  bg = WhitePixel(Disp, DefaultScreen(Disp));

  if ((w = bm->bbx_width) == 0)
    w = 1;
  if ((h = bm->bbx_height) == 0)
    h = 1;

  n = h * ((w+7)/8);
  if ((xbitmap = (unsigned char*)malloc(n)) == NULL){
    fprintf(stderr, "No Memory.\n");
    exit(1);
  }
  for (i = 0; i < n; i++)
    xbitmap[i] = 0;

  if ((bm->bbx_width != 0) && (bm->bbx_height != 0)){
    for (iy = 0; iy < bm->bbx_height; iy++){
      for (ix = 0; ix < (bm->bbx_width+7)/8; ix++){
	c = bm->bitmap[ix+iy*bm->raster];
	xbitmap[ix+iy*((bm->bbx_width+7)/8)]
	  = bit_rev[(c%0x10)]*0x10 + bit_rev[(c/0x10)];
      }
    }
  }

  pix = XCreatePixmapFromBitmapData(Disp, Win, (char*)xbitmap, 
				    w, h, fg, bg, depth);
  XCopyArea(Disp, pix, Win, Gc, 0, 0, w, h, 
	    x*cw + bm->off_x + WIN_BORDER, 
	    y*ch - bm->off_y + ch*0.8 + WIN_BORDER);
  XFreePixmap(Disp, pix);
  free(xbitmap);
}

void Win_ChangeSize(w, h)
{
  WinX = w + WIN_BORDER * 2;
  WinY = h + WIN_BORDER * 2;
  XResizeWindow(Disp, Win, w, h);
}


int Win_UserCmd()
{
  XEvent xevent;
  char   str[2014];
  KeySym k;

  if (xev == NULL)
    XNextEvent(Disp, &xevent);
  xev = NULL;

  switch(xevent.type){
  case ButtonPress:
    switch (xevent.xbutton.button){
    case 1:
      return CMD_PREV_PAGE;
    case 2:
      return CMD_EXIT;
    case 3:
      return CMD_NEXT_PAGE;
    }
  case Expose:
    return CMD_REDRAW;
#if 0
  case ConfigureNotify:
    x = xevent.xconfigure.width;
    y = xevent.xconfigure.height;
    if ((WinX != x) || (y != WinY)){
      WinX = x;
      WinY = y;
      return CMD_RESIZE;
    }
#endif
    break;
  case KeyPress:
    if (XLookupString(&xevent.xkey, str, sizeof(str), &k, NULL) == 1){
      switch (k){
      case XK_b: case XK_B: case XK_BackSpace: case XK_Delete:
      case XK_j:
	return CMD_PREV_PAGE;
      case XK_space: case XK_Return: case XK_Linefeed:
      case XK_k:
	return CMD_NEXT_PAGE;
      case XK_bracketleft:  case XK_h:
	return CMD_P4PAGES;
      case XK_bracketright: case XK_l:
	return CMD_N4PAGES;
      case XK_braceleft:
	return CMD_P16PAGES;
      case XK_braceright:
	return CMD_N16PAGES;
      case XK_parenleft:
	return CMD_P64PAGES;
      case XK_parenright:
	return CMD_N64PAGES;
      case XK_less: 
	return CMD_FIRST_PAGE;
      case XK_greater:
	return CMD_LAST_PAGE;
      case XK_m: case XK_M:
	return CMD_SET_MARK;
      case XK_g: case XK_G:
	return CMD_GOTO_MARK;
      case XK_question: 
	return CMD_HELP;
      case XK_q: case XK_Q:
	return CMD_EXIT;
      case XK_plus:
	return CMD_ENLARGE;
      case XK_minus:
	return CMD_SHRINK;
      case XK_r:
	return CMD_REOPEN;
      case XK_d:
	return CMD_REDRAW;
      }
      break;
    }
  default:
    return CMD_NOP;
  }
  return CMD_NOP;
}

int Win_PollUserCmd()
{
  int    val;
  XEvent xevent;
  char   str[1024];
  KeySym k;

  val = POLL_NOTHING;
  xev = NULL;

  if (XCheckWindowEvent(Disp, Win, 
			(KeyPressMask | ButtonPressMask
			 | ExposureMask | ConfigureNotify), 
			&xevent) 
      == False)
    return POLL_NOTHING;

  switch(xevent.type){
  case Expose:
  case ButtonPress:
    val = POLL_EVENT;
    break;
#if 0
  case ConfigureNotify:
    if ((WinX != xevent.xconfigure.width)
	|| (WinY != xevent.xconfigure.height))
      val = POLL_EVENT;
    break;    
#endif
  case KeyPress:
    if (XLookupString(&xevent.xkey, str, sizeof(str), &k, NULL) == 1)
      val = POLL_EVENT;
    break;
  }
  if (val == POLL_EVENT)
    xev = &xevent;
  return val;
}

/*EOF*/
