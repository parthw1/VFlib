/* 
 * vflserver.c - a font server using VFlib
 * by Hirotsugu Kakugawa
 *
 *   4 Nov 1996   First implementation (for VFlib 3.0.0)
 *  10 Dec 1996   For VFlib 3.1.0.  Enhanced HELP.
 *  13 Dec 1996   Added PROPERTY command.
 *  20 Dec 1996   Added MINIMIZE-BBX command.
 *   7 Mar 1997   Added COPYING command
 *  22 Mar 1997   Upgraded for VFlib 3.2
 *   4 Aug 1997   Upgraded for VFlib 3.3 (VFlib API is changed.)
 *  22 Jan 1998   Upgraded for VFlib 3.4 (Protocol 2.0.)
 *   1 Jun 1998   Changed program names ('vfserver' => 'vflserver')
 *  22 Jul 1998   Added ! syntax for specifying font id. (e.g., '!1', '!!')
 *   8 Dec 1999   Added VFLIB-VERSION command (Protocol 2.1.)
 */
/*
 * Copyright (C) 1996-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */


/*
 * Usage:  vflserver [-v vflibcap] [-s shrink] [cmd-file ... ] 
 *
 * Example:   ./vflserver -v `pwd`/vflibcap 
 *
 * Commands:
 * (Command names are case insensitive; font names are case sensitive.)
 *      OPEN1 font [ point_size [ mag_x mag_y [ dpi_x dpi_y ]]]
 *      OPEN2 font [ pixel_size [ mag_x mag_y ]]
 *      CLOSE font-id
 *      BITMAP1 font-id code_point [ mag_x mag_y ]
 *      BITMAP2 font-id code_point [ mag_x mag_y ]
 *      METRIC1 font-id code_point [ mag_x mag_y ]
 *      METRIC2 font-id code_point [ mag_x mag_y ]
 *      FONTBBX1 font-id [ mag_x mag_y ]
 *      FONTBBX2 font-id [ mag_x mag_y ]
 *      PROPERTY font-id property
 *      MINIMIZE-BBX [flag]
 *      PROTOCOL
 *      QUIT
 *      VERSION
 *      VFLIB-VERSION
 *      DEBUG [category]
 *      SLEEP [sec]
 *      HELP
 *
 */


#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#if defined(HAVE_STRING_H)
#  include  <string.h>
#else
#  include  <strings.h>
#endif
#include "VFlib-3_7.h"

#define PROTOCOL_VERSION  "VFSERVER/2.0"

#ifndef VERSION
#  define VERSION  "2.1"
#endif

#define MIN_SHRINK         0.05
#define MAX_ARGS           8
#define MAX_ARG_LEN        80
#define FONTID_TABLE_SIZE  4096


static int      minimize_bbx_mode = 0;
static int      dump_bitmap_mode = 0;
static int      fontid_table[FONTID_TABLE_SIZE];
static int      fontid_seq;

static void   startup_license(FILE*);
static void   run_init(char*,double);
static void   cmd_loop(FILE*,int,double);
static int    exec_cmd(char*,double);
static void   quit(void);

static int    parse_args(char**,char*);
static void   str_toupper(char *s);
static void   dump_bitmap_hex(VF_BITMAP);
static void   dump_bitmap_char(VF_BITMAP,double);
extern void   vf_dump_bitmap(VF_BITMAP);

static int   cmd_open1(char**,int);
static int   cmd_open2(char**,int);
static int   cmd_close(char**,int);
static int   cmd_bitmap1(char**,int,double);
static int   cmd_bitmap2(char**,int,double);
static int     cmd_bitmap_output(VF_BITMAP,long,double);
static int   cmd_metric1(char**,int);
static int   cmd_metric2(char**,int);
static int   cmd_fontbbx1(char**,int);
static int   cmd_fontbbx2(char**,int);
static int   cmd_prop(char**,int);
static int   cmd_minbbx(char**,int);
static int   cmd_proto(char**,int);
static int   cmd_help(char**,int);
static int   cmd_debug(char**,int);
static int   cmd_sleep(char**,int);
static int   cmd_version(char**,int);
static int   cmd_vflib_version(char**,int);
static void  put_fontid(int);
static int   get_fontid(char*);




int
main(int argc, char **argv)
{
  char    *vfcap, **drv_list;
  double  shrink;
  int     i, w;

  vfcap = NULL;
  shrink = -1;
  minimize_bbx_mode = 0;

  --argc; argv++;
  while (argc > 0){
    if ((argc >= 1)
	&& ((strcmp(argv[0], "-h") == 0) || (strcmp(argv[0], "-help") == 0))){
      printf("VFLSERVER - a VFlib server\n");
      printf("Usage: vflserver [-v vflibcap] [cmd-file ... ]\n"); 
      printf("  Example:   ./vflserver -v /usr/local/lib/VFlib3/vflibcap\n");
      exit(0);
    } else if ((argc >= 2) && (strcmp(argv[0], "-v") == 0)){
      --argc; argv++;
      vfcap = argv[0];
      --argc; argv++;
    } else if ((argc >= 2) && (strcmp(argv[0], "-s") == 0)){
      --argc; argv++;
      if ((shrink = atof(argv[0])) < MIN_SHRINK)
	shrink = MIN_SHRINK;
      --argc; argv++;
    } else {
      break;
    }
  }

  if ((VF_Init(vfcap, NULL) < 0) 
      || ((drv_list = VF_FontDriverList()) == NULL)){
    printf("(550 \"vflserver version %s. ", VERSION);
    printf("based on VFlib %s ", VF_GetVersion());
    printf("VFlib error %d)\n", vf_error);
    switch (vf_error){
    case VF_ERR_INTERNAL:
      printf(";  Internal error.\n"); break;
    case VF_ERR_NO_MEMORY:
      printf(";  Server runs out of memory.\n"); break;
    case VF_ERR_NO_VFLIBCAP:
      printf(";  No vflibcap.\n"); break;
    }
    fflush(stdout);
    exit(1);
  }

  printf("; This is a font server VFLSERVER Version %s\n", VERSION);
  printf("; (based on VFlib version %s)\n", VF_GetVersion());
  printf("; Installed font drivers:");
  w = 9999;
  for (i = 0; drv_list[i] != NULL; i++){
    if (w > 50){
      printf("\n;       ");
      w = 0;
    }
    printf("%s ", drv_list[i]);
    w += strlen(drv_list[i]) + 1;
  }
  printf("\n");

  startup_license(stdout);

  printf("; Type `HELP' for description of the protocol.\n\n");
  printf("(100 \"vflserver ready.\")\n");
  fflush(stdout);
  free(drv_list);
  
  fontid_seq = 0;
  for (i = 0; i < FONTID_TABLE_SIZE; i++)
    fontid_table[i] = -1;

  while (argc > 0){
    run_init(argv[0], shrink);
    --argc; argv++;
  }

  cmd_loop(stdin, 0, shrink);

  quit();

  return 0;
}

static void
startup_license(FILE *fp)
{
  if (fp == NULL)
    fp = stdout;
  fprintf(fp,
    ";  Copyright (C) 1996-1999 by Hirotsugu Kakugawa\n"
    ";  All rights reserved.\n"
    ";  This program is distributed in the hope that it will be useful,\n"
    ";  but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
    ";  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
    ";  GNU General Public License for more details.\n");
}


static void
run_init(char *f, double shrink)
{
  FILE  *fp; 

  if ((fp = fopen(f, "r")) == NULL){
    fprintf(stderr, "No such file: %s\n", f);
  } else {
    cmd_loop(fp, 1, shrink);
    fclose(fp);
  }
}

static void 
quit(void)
{
  printf("(100 \"Happy hacking.\")\n");
  fflush(stdout);
}

static void
cmd_loop(FILE *fp, int echo, double shrink)
{
  char  cmdline[1024];
  int   val;

  val = 0;
  for (;;){
    if (fgets(cmdline, sizeof(cmdline), fp) == NULL)
      break;
    if (echo == 1)
      printf("%s", cmdline);
    if ((val = exec_cmd(cmdline, shrink)) < 0){
      quit();
      exit(0);
    }
    fflush(stdout);
  }
}

static int
exec_cmd(char *cmdline, double shrink)
{
  char  cmd[32], *args[MAX_ARGS];
  int   nargs, i, j, iarg, ret;
  
  ret = 0;
  iarg = 0;
  while ((cmdline[iarg] != '\0') && isspace((int)cmdline[iarg]))
    iarg++;
  if (cmdline[iarg] == '\0')
    return ret;
  j = 0;
  while ((cmdline[iarg] != '\0') && !isspace((int)cmdline[iarg]))
    cmd[j++] = cmdline[iarg++];
  cmd[j] = '\0';
  
  for (i = 0; cmd[i] != '\0'; i++)
    cmd[i] = toupper(cmd[i]);
  nargs = parse_args(args, &cmdline[iarg]);
  
  if (strcmp(cmd, "QUIT") == 0){
    ret = -1;
  } else if (strcmp(cmd, "OPEN1") == 0){
    cmd_open1(args, nargs);
  } else if (strcmp(cmd, "OPEN2") == 0){
    cmd_open2(args, nargs);
  } else if (strcmp(cmd, "CLOSE") == 0){
    cmd_close(args, nargs);
  } else if (strcmp(cmd, "BITMAP1") == 0){
    cmd_bitmap1(args, nargs, shrink);
  } else if (strcmp(cmd, "BITMAP2") == 0){
    cmd_bitmap2(args, nargs, shrink);
  } else if (strcmp(cmd, "METRIC1") == 0){
    cmd_metric1(args, nargs);
  } else if (strcmp(cmd, "METRIC2") == 0){
    cmd_metric2(args, nargs);
  } else if (strcmp(cmd, "FONTBBX1") == 0){
    cmd_fontbbx1(args, nargs);
  } else if (strcmp(cmd, "FONTBBX2") == 0){
    cmd_fontbbx2(args, nargs);
  } else if (strcmp(cmd, "PROPERTY") == 0){
    cmd_prop(args, nargs);
  } else if (strcmp(cmd, "PROTOCOL") == 0){
    cmd_proto(args, nargs);
  } else if (strcmp(cmd, "MINIMIZE-BBX") == 0){
    cmd_minbbx(args, nargs);
  } else if (strcmp(cmd, "COMMENT") == 0){
    printf("100 Ok.\n");
  } else if (strcmp(cmd, "DEBUG") == 0){
    cmd_debug(args, nargs);
  } else if (strcmp(cmd, "HELP") == 0){
    cmd_help(args, nargs);
  } else if (strcmp(cmd, "SLEEP") == 0){
    cmd_sleep(args, nargs);
  } else if (strcmp(cmd, "VERSION") == 0){
    cmd_version(args, nargs);
  } else if (strcmp(cmd, "VFLIB-VERSION") == 0){
    cmd_vflib_version(args, nargs);
  } else {
    printf("(500 \"Error. Command unknown. (`HELP' for help.)\")\n");
  }
  return ret;
}

static int
parse_args(char **args, char *cmdline)
{
  int  argc, idx;

  idx = 0;
  for (argc = 0; argc < MAX_ARGS-1;){
    while (isspace((int)cmdline[idx]))
      idx++;
    if (cmdline[idx] == '\0')
      break;
    args[argc] = &cmdline[idx];
    argc++;
    while (!isspace((int)cmdline[idx]) && (cmdline[idx] != '\0'))
      idx++;
    if (cmdline[idx] == '\0')
      break;
    cmdline[idx++] = '\0';
  }
  args[argc] = NULL;

  return argc;
}


/* ------------------------------------------------------------- */

/*
 * OPEN1 font [ point_size [ mag_x mag_y [ dpi_x dpi_y ]]] 
 */
static int
cmd_open1(char **args, int nargs)
{
  int     font_id;
  char    *font;
  double  point_size, mag_x, mag_y, dpi_x, dpi_y;
  
  if ((nargs != 1) && (nargs != 2) && (nargs != 4) && (nargs != 6)){ 
    printf("500 What?  %s\n", 
	   "OPEN1 font [ point_size [ mag_x mag_y [ dpi_x dpi_y ]]]");
    return -1;
  }

  font       = NULL;
  point_size = -1;
  mag_x      = 1;
  mag_y      = 1;
  dpi_x      = -1;
  dpi_y      = -1;

  font = args[0];
  if (nargs >= 2)
    point_size = atof(args[1]);
  if (nargs >= 4){
    mag_x = atof(args[2]);
    mag_y = atof(args[3]);
  }
  if (nargs >= 6){
    dpi_x = atof(args[4]);
    dpi_y = atof(args[5]);
  }

  font_id = VF_OpenFont1(font, dpi_x, dpi_y, point_size, mag_x, mag_y);
  put_fontid(font_id);
  
  if (font_id < 0){
    printf("(551 \"Error. Can't open %s.\")\n", font);
    return -1;
  }

  printf("(100 %d \"%s\")\n", font_id, font);

  return 0;
}

/*
 * OPEN2 font [ pixel_size [ mag_x mag_y ]]
 */
static int
cmd_open2(char **args, int nargs)
{
  int     font_id;
  char    *font;
  int     pixel_size; 
  double  mag_x, mag_y;
  
  if ((nargs != 1) && (nargs != 2) && (nargs != 4) ){
    printf("500 What?  %s\n", 
	   "OPEN2 font [ pixel_size [ mag_x mag_y ]]");
    return -1;
  }

  font = NULL;
  pixel_size = -1;
  mag_x = 1;
  mag_y = 1;

  font = args[0];
  if (nargs >= 2)
    sscanf(args[1], "%i", &pixel_size);
  if (nargs >= 4){
    mag_x = atof(args[2]);
    mag_y = atof(args[3]);
  }

  font_id = VF_OpenFont2(font, pixel_size, mag_x, mag_y);
  put_fontid(font_id);

  if (font_id < 0){
    printf("(551 \"Error. Can't open %s.\")\n", font);
    return -1;
  }

  printf("(100 %d \"%s\")\n", font_id, font);

  return 0;
}

/*
 * CLOSE font-id
 */
static int
cmd_close(char **args, int nargs)
{
  int     font_id;
  
  if (nargs != 1){
    printf("500 What?  %s\n", "CLOSE font-id");
    return -1;
  }

  font_id = get_fontid(args[0]);
  VF_CloseFont(font_id);

  printf("(100 %d)\n", font_id);
  return 0;
}

/*
 * BITMAP1 font-id code_point [ mag_x mag_y ]
 */
static int
cmd_bitmap1(char **args, int nargs, double shrink)
{
  int        font_id, val;
  long       code_point;
  double     mag_x, mag_y;
  VF_BITMAP  bm, bm1;
  
  if ((nargs != 2) && (nargs != 3) && (nargs != 4)){
    printf("(500 \"Error.  Usage:%s\")\n", 
	   "BITMAP1 font-id char-code [ mag_x mag_y ]");
    return -1;
  }

  font_id    = -1;
  code_point = 0;
  mag_x      = 1;
  mag_y      = 1;

  font_id = get_fontid(args[0]);
  sscanf(args[1], "%li", &code_point);
  if (nargs == 3){
    mag_x = mag_y = atof(args[2]);
  } else if (nargs == 4){
    mag_x = atof(args[2]);
    mag_y = atof(args[3]);
  }
  
  bm = VF_GetBitmap1(font_id, code_point, mag_x, mag_y);

  if (bm != NULL){
    if (minimize_bbx_mode == 1){
      bm1 = VF_MinimizeBitmap(bm);
      val = cmd_bitmap_output(bm1, code_point, shrink);
      VF_FreeBitmap(bm1);
      VF_FreeBitmap(bm);
    } else {
      val = cmd_bitmap_output(bm, code_point, shrink);
      VF_FreeBitmap(bm);
    }
  } else {
    val = cmd_bitmap_output(bm, code_point, shrink);
  }

  return val;
}

/*
 * BITMAP2 font-id code_point [ mag_x mag_y ]
 */
static int
cmd_bitmap2(char **args, int nargs, double shrink)
{
  int        font_id, val;
  long       code_point;
  double     mag_x, mag_y;
  VF_BITMAP  bm, bm1;
  
  if ((nargs != 2) && (nargs != 3) && (nargs != 4)){
    printf("(500 \"Error.  Usage: %s\")\n", 
	   "BITMAP2 font-id char-code [ mag_x mag_y ]");
    return -1;
  }

  font_id    = -1;
  code_point = 0; 
  mag_x      = 1;
  mag_y      = 1;

  font_id = get_fontid(args[0]);
  sscanf(args[1], "%li", &code_point);
  if (nargs == 3){
    mag_x = mag_y = atof(args[2]);
  } else if (nargs == 4){
    mag_x = atof(args[2]);
    mag_y = atof(args[3]);
  }

  bm = VF_GetBitmap2(font_id, code_point, mag_x, mag_y);
  if (bm != NULL){
    if (minimize_bbx_mode == 1){
      bm1 = VF_MinimizeBitmap(bm);
      val = cmd_bitmap_output(bm1, code_point, shrink);
      VF_FreeBitmap(bm1);
      VF_FreeBitmap(bm);
    } else {
      val = cmd_bitmap_output(bm, code_point, shrink);
      VF_FreeBitmap(bm);
    }
  } else {
    val = cmd_bitmap_output(bm, code_point, shrink);
  }

  return val;
}

static int
cmd_bitmap_output(VF_BITMAP bm, long code_point, double shrink)
{
  if (bm == NULL){
    switch (vf_error){
    case VF_ERR_ILL_FONTID:
      printf("(550 \"Error. Illegal font-id.\")\n"); 
      break;
    case VF_ERR_INTERNAL:
      printf("(550 \"Error. Internal error.\")\n"); 
      break;
    case VF_ERR_NO_MEMORY:
      printf("(550 \"Error. No memory.\")\n"); 
      break;
    case VF_ERR_ILL_CODE_POINT:
      printf("(550 \"Error. No such character %ld (0x%04lx).\")\n", 
	     code_point, code_point);
      break;
    default:
      printf("(550 \"Error. VFlib error code = %d.\")\n", vf_error);
    }
    return -1;
  }
  
  printf("(100 %d %d %d %d %d %d\n ",
	 bm->bbx_width, bm->bbx_height, 
	 bm->off_x, bm->off_y, bm->mv_x, bm->mv_y);
  dump_bitmap_hex(bm);
  if (dump_bitmap_mode == 1)
    dump_bitmap_char(bm, shrink);
  printf(")\n");
  return 0;
}

/*
 * METRIC1 font-id code_point [ mag_x mag_y ]
 */
static int
cmd_metric1(char **args, int nargs)
{
  int         font_id;
  long        code_point;
  double      mag_x, mag_y;
  VF_METRIC1  met;

  if ((nargs != 2) || (nargs > 4)){
    printf("(500 \"Error.  Usage: %s\")\n", 
	   "METRIC1 font-id code_point [ mag_x mag_y ]");
    return -1;
  }

  font_id    = -1;
  code_point = 0; 
  mag_x      = 1;
  mag_y      = 1;

  font_id = get_fontid(args[0]);
  sscanf(args[1], "%li", &code_point);
  if (nargs == 3){
    mag_x = mag_y = atof(args[2]);
  } else if (nargs == 4){
    mag_x = atof(args[2]);
    mag_y = atof(args[3]);
  }

  if ((met = VF_GetMetric1(font_id, code_point, NULL, mag_x, mag_y)) == NULL){
    printf("(550 \"Error. No metric.\")\n");
    return -1;
  }
  printf("(100 %f %f %f %f %f %f)\n",
	 met->bbx_width, met->bbx_height, 
	 met->off_x, met->off_y, met->mv_x, met->mv_y);
  VF_FreeMetric1(met);

  return 0;
}

/*
 * METRIC2 font-id code_point [ mag_x mag_y ]
 */
static int
cmd_metric2(char **args, int nargs)
{
  int         font_id;
  long        code_point;
  double      mag_x, mag_y;
  VF_METRIC2  met;

  if ((nargs != 2) || (nargs > 4)){
    printf("500 What?  %s\n",
	   "METRIC2 font-id char-code [ mag_x mag_y ]");
    return -1;
  }

  font_id    = -1;
  code_point = 0; 
  mag_x      = 1;
  mag_y      = 1;

  font_id = get_fontid(args[0]);
  sscanf(args[1], "%li", &code_point);
  if (nargs == 3){
    mag_x = mag_y = atof(args[2]);
  } else if (nargs == 4){
    mag_x = atof(args[2]);
    mag_y = atof(args[3]);
  }

  if ((met = VF_GetMetric2(font_id, code_point, NULL, mag_x, mag_y)) == NULL){
    printf("(550 \"Error.  No metric.\")\n");
    return -1;
  }
  printf("(100 %d %d %d %d %d %d)\n",
	 met->bbx_width, met->bbx_height, 
	 met->off_x, met->off_y, met->mv_x, met->mv_y);
  VF_FreeMetric2(met);

  return 0;
}

/*
 * FONTBBX1 font-id [ mag_x mag_y ]
 */
static int
cmd_fontbbx1(char **args, int nargs)
{
  int      font_id;
  double   mag_x, mag_y;
  double   w, h, xoff, yoff;

  if ((nargs != 1) || (nargs > 3)){
    printf("(500 \"Error.  Usage: %s\")\n", 
	   "FONTBBX1 font-id [ mag_x mag_y ]");
    return -1;
  }

  font_id = -1;
  mag_x   = 1;
  mag_y   = 1;

  font_id = get_fontid(args[0]);
  if (nargs == 2){
    mag_x = mag_y = atof(args[1]);
  } else if (nargs == 3){
    mag_x = atof(args[1]);
    mag_y = atof(args[2]);
  }

  if (VF_GetFontBoundingBox1(font_id, mag_x, mag_y,
			     &w, &h, &xoff, &yoff) < 0){
    printf("(550 \"Error. No font bounding box information.\")\n");
    return -1;
  }

  printf("(100 %f %f %f %f)\n",  w, h, xoff, yoff);
  return 0;
}

/*
 * FONTBBX2 font-id [ mag_x mag_y ]
 */
static int
cmd_fontbbx2(char **args, int nargs)
{
  int      font_id;
  double   mag_x, mag_y;
  int      w, h, xoff, yoff;

  if ((nargs != 1) || (nargs > 3)){
    printf("(500 \"Error.  Usage: %s\")\n", 
	   "FONTBBX2 font-id [ mag_x mag_y ]");
    return -1;
  }

  font_id = -1;
  mag_x   = 1;
  mag_y   = 1;

  font_id = get_fontid(args[0]);
  if (nargs == 2){
    mag_x = mag_y = atof(args[1]);
  } else if (nargs == 3){
    mag_x = atof(args[1]);
    mag_y = atof(args[2]);
  }

  if (VF_GetFontBoundingBox2(font_id, mag_x, mag_y,
			     &w, &h, &xoff, &yoff) < 0){
    printf("(550 \"Error. No font bounding box information.\")\n");
    return -1;
  }

  printf("(100 %d %d %d %d)\n",  w, h, xoff, yoff);
  return 0;
}

/*
 * PROPERTY font-id property
 */
static int
cmd_prop(char **args, int nargs)
{
  int    font_id;
  char  *prop, *value;

  if (nargs != 2){
    printf("(500 \"Error.  Usage: %s\")\n",
	   "PROPERTY font-id property");
    return -1;
  }

  font_id = get_fontid(args[0]);
  prop = args[1];

  if ((value = VF_GetFontProp(font_id, prop)) == NULL){
    printf("(550 \"Error.  No such property: %s\")\n", prop);
    return -1;
  }

  printf("(100 \"%s\" \"%s\")\n", prop, value);
  free(value);

  return 0;
}

/*
 * MINIMIZE-BBX [flag]
 */
static int
cmd_minbbx(char **args, int nargs)
{
  char  *res;
  int   val;

  if (nargs != 1){
    printf("(500 \"Error.  Usage: %s\")\n",
	   "MINIMIZE-BBX [flag]");
    return -1;
  }

  res = "100";
  val = 0;
  
  str_toupper(args[0]);
  if (   (strcmp(args[0], "0") == 0)
      || (strcmp(args[0], "OFF") == 0)
      || (strcmp(args[0], "NO") == 0)){
    minimize_bbx_mode = 0;
  } else if (   (strcmp(args[0], "1") == 0)
	     || (strcmp(args[0], "ON") == 0)
	     || (strcmp(args[0], "YES") == 0)){
    minimize_bbx_mode = 1;
  } else {
    res = "500";
    val = -1;
  }
  printf("(%s \"%s\")\n", res, minimize_bbx_mode?"ON":"OFF");

  return val;
}


static void
put_fontid(int fontid)
{
  if (fontid_seq < FONTID_TABLE_SIZE)
    fontid_table[fontid_seq++] = fontid;
}

static int
get_fontid(char  *s)
{
  int   fontid, h;

  fontid = -1;
  if (s[0] == '!'){   /* font id by history */
    if (strcmp(s, "!!") == 0){ /* "!!" : the latest font id */
      if ((fontid_seq > 0) && (fontid_seq < FONTID_TABLE_SIZE))
	fontid = fontid_table[fontid_seq-1];
    } else {                   /* "!i" : font id for i-th VF_OpenFontX() */
      sscanf(&s[1], "%i", &h);
      if ((h >= 0) && (h < FONTID_TABLE_SIZE))
	fontid = fontid_table[h];
    }
  } else {            /* font id by number */
    sscanf(s, "%i",  &fontid);
  }

  return fontid;
}



/*
 *      PROTOCOL
 */
static int
cmd_proto(char **args, int nargs)
{
  printf("(100 \"%s\")\n", PROTOCOL_VERSION);
  return 0;
}

/*
 *      SLEEP [sec]
 */
static int
cmd_sleep(char **args, int nargs)
{
  int   t;

  t = 1;
  if (args[0] != NULL){
    if ((t = atoi(args[0])) < 1)
      t = 1;
  }
  sleep(t);
  printf("(100 %d)\n", t);
  return 0;
}

/*
 *      VERSION
 */
static int
cmd_version(char **args, int nargs)
{
  printf("(100 \"%s\")\n", VERSION);
  return 0;
}

/*
 *      VFlib VERSION
 */
static int
cmd_vflib_version(char **args, int nargs)
{
  printf("(100 \"%s\")\n", VF_GetVersion());
  return 0;
}

extern void  VF_DumpFontTable(void);
/*
 *      DEBUG [category]
 */
static int
cmd_debug(char **args, int nargs)
{
  if (args[0] != NULL){
    str_toupper(args[0]);
    if (strcmp(args[0], "BITMAP") == 0){
      dump_bitmap_mode = 1 - dump_bitmap_mode;
      printf("(100 \"Ascii-art bitmap %s.\")\n",
	     (dump_bitmap_mode==1)?"on":"off");
    } else if (strcmp(args[0], "DUMPFONTS") == 0){
      VF_DumpFontTable();
    } else {
      printf("(500 \"Error.\")\n");
      return -1;
    }
  } else {
    printf("(500 \"Error. Debug what? (Type `HELP' for help.)\")\n");
  }
  return 0;
}

/*
 *      HELP 
 */

static void  cmd_help_open1(void), cmd_help_open2(void);
static void  cmd_help_close(void);
static void  cmd_help_bitmap1(void), cmd_help_bitmap2(void);
static void  cmd_help_metric1(void), cmd_help_metric2(void);
static void  cmd_help_fontbbx1(void), cmd_help_fontbbx2(void); 
static void  cmd_help_property(void); 
static void  cmd_help_minbbx(void), cmd_help_protocol(void); 
static void  cmd_help_quit(void), cmd_help_version(void); 
static void  cmd_help_debug(void), cmd_help_sleep(void); 
static void  cmd_help_help(void);

struct s_help {
  char   *cmd_name;
  void  (*help_func)(void);
};
static struct s_help helpers[] = {
  {"OPEN1", cmd_help_open1}, {"OPEN2", cmd_help_open2}, 
  {"CLOSE", cmd_help_close},
  {"BITMAP1", cmd_help_bitmap1}, {"BITMAP2", cmd_help_bitmap2},
  {"METRIC1", cmd_help_metric1}, {"METRIC2", cmd_help_metric2},
  {"FONTBBX1", cmd_help_fontbbx1}, {"FONTBBX2", cmd_help_fontbbx2},
  {"PROPERTY", cmd_help_property},  {"MINIMIZE-BBX", cmd_help_minbbx},
  {"PROTOCOL", cmd_help_protocol},  {"QUIT", cmd_help_quit},
  {"VERSION", cmd_help_version},  {"DEBUG", cmd_help_debug},
  {"HELP", cmd_help_help},  {"SLEEP", cmd_help_sleep}, {"?", cmd_help_help},  
  {NULL, NULL} };

static int
cmd_help(char **args, int nargs)
{

  if (nargs == 0){
    printf("; Help keywords:\n");
    printf(";    OPEN1, OPEN2, CLOSE, BITMAP1, BITMAP2, METRIC1, METRIC2\n");
    printf(";    FONTBBX1 FONTBBX2 PROPERTY, MINIMIZE-BBX, PROTOCOL, QUIT\n");
    printf(";    VERSION, DEBUG, HELP, SLEEP\n");
    printf("; HELP ?  --- for more information on command list.\n");
    printf("; HELP KEY  --- help on KEY listed above.\n");
    printf("; Each command returns result with status value (3 digits) \n");
    printf("; followed by a command-specific result sequence.\n");
    printf("(100 \"Ok.\")\n");
  } else {
    int  i, c;
    int  found;

    for (i = 0; args[0][i] != '\0'; i++)
      args[0][i] = toupper(args[0][i]);
    found = 0;
    c = 0;
    while (helpers[c].cmd_name != NULL){
      if (strcmp(helpers[c].cmd_name, args[0]) == 0){
	found = 1;
	break;
      }
      c++;
    }
    if (found == 0){
      printf("; Unknown help for \"%s\"\n", args[0]);
      cmd_help(NULL, 0);
    } else {
      (*helpers[c].help_func)();
      printf("(100 \"Ok.\")\n");
    }
  }



  return 0;
}

static void 
cmd_help_help(void)
{
    printf("; The following commands are recognized.\n");
    printf(";   OPEN1 font [ point-size [ mag_x mag_y [ dpi_x dpi_y ]]]\n");
    printf(";   OPEN2 font [ pixel-size [ mag_x mag_y ]]\n");
    printf(";   CLOSE font-id\n");
    printf(";   BITMAP1 font-id code_point [ mag_x mag_y ]\n");
    printf(";   BITMAP2 font-id code_point [ mag_x mag_y ]\n");
    printf(";   METRIC1 font-id code_point [ mag_x mag_y ]\n");
    printf(";   METRIC2 font-id code_point [ mag_x mag_y ]\n");
    printf(";   FONTBBX1 font-id [ mag_x mag_y ]\n");
    printf(";   FONTBBX2 font-id [ mag_x mag_y ]\n");
    printf(";   PROPERTY font-id property\n");
    printf(";   MINIMIZE-BBX [flag]\n");
    printf(";   PROTOCOL\n");
    printf(";   QUIT\n");
    printf(";   VERSION\n");
    printf(";   DEBUG [category]\n");
    printf(";   HELP [category]\n");
    printf(";   SLEEP [sec]\n");
    printf("; HELP CMDS  --- help on CMDS\n");
    printf("; Each command returns result with status value (3 digits) \n");
    printf("; followed by a command-specific result sequence.\n");
}

static void 
cmd_help_open1(void)
{
  printf("; OPEN1 font [ point-size [ mag_x mag_y [ dpi_x dpi_y ]]]\n");
  printf("; --- Open a font named FONT in mode 1. \n");
  printf(";   It's parameters are point size (POINT_SIZE),\n");
  printf(";   magnification factors (MAG_X, MAG_Y), and device \n");
  printf(";   resolutions in DPI (DPI_X, DPI_Y). If these parameters\n");
  printf(";   are omitted, default values of the font are used.\n");
  printf("; This command returns a font identifier in integer, if\n");
  printf("; a requested font is successfully opened.\n");
}
static void 
cmd_help_open2(void)
{
  printf("; OPEN2 font [ pixel-size [ mag_x mag_y ]]\n");
  printf("; --- Open a font named FONT in mode 1. \n");
  printf(";   It's parameters are pixel size (PIXEL_SIZE),\n");
  printf(";   magnification factors (MAG_X, MAG_Y). If these parameters\n");
  printf(";   are omitted, default values of the font are used.\n");
  printf("; This command returns a font identifier in integer, if\n");
  printf("; a requested font is successfully opened.\n");
}
static void 
cmd_help_close(void)
{
  printf("; CLOSE font-id\n");
  printf("; --- Close an opened font of FONT_ID.\n");
}
static void 
cmd_help_bitmap1(void)
{
  printf("; BITMAP1 font-id code_point [ mag_x mag_y ]\n");
  printf("; --- Get a bitmap of a character CODE_POINT in a FONT_ID.\n");
  printf(";   FONT_ID must be in mode 1.\n");
  printf("; This command returns Bw Bh Rx Ry Mx My BM\n");
  printf("; - Bw (width) and Bh (height) form a bounding-box of a bitmap,\n");
  printf(";   (in pixel).\n");
  printf("; - Rx and Ry  form a vector from the reference point of \n");
  printf(";   a bitmap to the upper left corner of a bitmap (in pixel).\n");
  printf("; - Mx and My form a vector from the reference point to the next\n");
  printf(";   next reference point (in pixel).\n");
  printf("; - BM is the bitmap, starting from top line of a bitmap to \n");
  printf(";   the bottom. Each line starts from left to right, 1 bit per\n");
  printf(";   pixel, padding is 8 pixels. Leftmost pixel in 8 pixel\n");
  printf(";   packet has 0x80 weight, rightmost one has 0x01.\n");
}
static void 
cmd_help_bitmap2(void)
{
  printf("; BITMAP2 font-id code_point [ mag_x mag_y ]\n");
  printf("; --- Get a bitmap of a character CODE_POINT in a FONT_ID.\n");
  printf(";   FONT_ID must be in mode 2.\n");
  printf("; This command returns Bw Bh Rx Ry Mx My BM\n");
  printf("; See help on BITMAP1 for return values.\n");
}
static void 
cmd_help_metric1(void)
{
  printf("; METRIC1 font-id code_point [ mag_x mag_y ]\n");
  printf("; --- Get a metric of a character CODE_POINT in a FONT_ID.\n");
  printf(";   FONT_ID must be in mode 1.\n");
  printf("; This command returns Bw Bh Rx Ry Mx My\n");
  printf("; See help on BITMAP1 for return values, *except* units are\n");
  printf("; points.\n");
}
static void 
cmd_help_metric2(void)
{
  printf("; METRIC2 font-id code_point [ mag_x mag_y ]\n");
  printf("; --- Get a metric of a character CODE_POINT in a font FONT_ID.\n");
  printf(";   FONT_ID must be in mode 2.\n");
  printf("; This command returns Bw Bh Rx Ry Mx My\n");
  printf("; See help on BITMAP1 for return values. (Units are\n");
  printf("; pixel.)\n");
}
static void 
cmd_help_fontbbx1(void)
{
  printf("; FONTBBX1 font-id [ mag_x mag_y ]\n");
  printf("; --- Get font bounding information of a font FONT_ID.\n");
  printf(";   FONT_ID must be in mode 1.\n");
  printf("; This command returns W H XOFF YOFF\n");
  printf(";  - W, H are max width, height, respectively.\n");
  printf(";  - XOFF and YOFF form a max vector from the reference point to\n");
  printf(";    lower left corner of a bounding box.\n");
  printf("; These units are point.\n");
}
static void 
cmd_help_fontbbx2(void)
{
  printf("; FONTBBX2 font-id [ mag_x mag_y ]\n");
  printf("; --- Get font bounding information of a font FONT_ID.\n");
  printf(";   FONT_ID must be in mode 2.\n");
  printf("; This command returns W H XOFF YOFF\n");
  printf(";  - W, H are max width, height, respectively.\n");
  printf(";  - XOFF and YOFF form a max vector from the reference point to\n");
  printf(";    lower left corner of a bounding box.\n");
  printf("; These units are pixel.\n");
}
static void 
cmd_help_property(void)
{
  printf("; PROPERTY font-id property_name\n");
  printf("; --- Get a property value of PROPERTY_NAME of a font FONT_ID.\n");
  printf("; This command returns property value in string.\n");
}
static void 
cmd_help_minbbx(void)
{
  printf("; MINIMIZE-BBX [flag]\n");
  printf("; --- Followed BITAMAP1 and BITMAP2 commands return `minimized'\n");
  printf("; bitmap, in a sense that no smaller bounding box can contain\n");
  printf("; all black pixels.\n");
}
static void 
cmd_help_protocol(void)
{
  printf("; PROTOCOL\n");
  printf("; --- Return protocol version string.\n");
}
static void 
cmd_help_quit(void)
{
  printf("; QUIT\n");
  printf("; --- Quit vflserver.\n");
}
static void 
cmd_help_version(void)
{
  printf("; VERSION\n");
  printf("; --- Print version of vflserver.\n");
}
static void 
cmd_help_debug(void)
{
  printf("; DEBUG [category]\n");
  printf("; --- Set debug mode on CATEGORY.\n");
  printf(";  Categories:\n");
  printf(";   - BITMAP - print obtained bitmaps by BITMAP1 and BITMAP2\n");
  printf(";              in ASCII art form.\n");
  printf(";   Currently, only BITMAP is defined for debug category\n");
}
static void 
cmd_help_sleep(void)
{
  printf("; SLEEP [sec]\n");
  printf("; --- Sleep for SEC seconds.\n");
}



static void
dump_bitmap_hex(VF_BITMAP bm)
{
  int            x, y, b;
  unsigned char  *p;

  printf("\"");
  b = 0;
  for (y = 0; y < bm->bbx_height; y++){
    p = &bm->bitmap[y * bm->raster];
    for (x = 0; x < (bm->bbx_width+7)/8; x++){
      printf("%02x", p[x]);
      b++;
    }
  }
  printf("\"");
}
static void
dump_bitmap_char(VF_BITMAP bm, double shrink)
{
  printf("\n\"\n");
  vf_dump_bitmap(bm);
  printf("\"");
}

static void 
str_toupper(char *s)
{
  while (*s){
    *s = toupper(*s);
    s++;
  }
}

  
/*EOF*/
