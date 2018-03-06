/*
 * vfldisol.c - Disassemble Vector Font Data
 * by Hirotsugu Kakugawa
 *
 *  31 Dec 1993  
 *  20 Jan 1994  New output format
 *  10 Jan 1997  for VFlib version 3
 *  22 Mar 1997  Upgraded for VFlib 3.2
 *
 */
/*
 * Copyright (C) 1996-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */


#include  "config.h"
#include <stdio.h>
#include <stdlib.h>
#if HAVE_STRING_H
# include <string.h>
#endif
#if HAVE_STRINGS_H
# include <strings.h>
#endif
#if HAVE_STDARG_H
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include "VFlib-3_7.h"

#define CommentLine printf

void  usage(void);
void  DisVFData(VF_OUTLINE,int);
void  EmptyLine(void);
void  PrintCCode(long);
void  PrintToken(void);
void  PrintHeaderI(char*,long);
void  PrintHeaderR(char*,long);
void  Print(int*,char*);
void  Newline(void);
void  PrintEnd(void);
void  PrintXY(int,int);


int
main(int argc, char **argv)
{
  int         Fd, i;
  int         Ch, HexDump;
  long        CharCode;
  double      MagX, MagY, Point, Dpi;
  char        *FontName, *Vfcap;
  VF_OUTLINE  VFData;

  Vfcap    = NULL;
  HexDump  = 0;
  Point    = -1;
  MagX     = 1;
  MagY     = 1;
  Dpi      = -1;
  FontName = NULL;

  Ch = argc + 2001;
  for (i = 1; i < argc; i++){
    if (argv[i][0] == '-'){
      switch (argv[i][1]){
      case 'v':  Vfcap = argv[++i];       break;
      case 'x':  HexDump = 1;             break;
      case 'd':  Dpi = atof(argv[++i]);   break;
      case 'p':  Point = atof(argv[++i]); break;
      case 'm': 
	if (strcmp(argv[i], "-mx") == 0)
	  MagX = atof(argv[++i]);
	else if (strcmp(argv[i], "-my") == 0)
	  MagY = atof(argv[++i]);
	else
	  MagX = MagY = atof(argv[++i]);
	break;
      case 'h':
      default:
	usage();
      }
    } else {
      FontName = argv[i++];
      Ch = i;
      break;
    }
  }

  if ((FontName == NULL) || (Ch >= argc))
    usage();  /* no char codes */

  /* Init VFlib */
  if (VF_Init(Vfcap, NULL) < 0){
    fprintf(stderr, "VFlib init error.\n");    
    exit(1);
  }

  /* OPEN THE FONT */
  if ((Fd = VF_OpenFont1(FontName, Dpi, Dpi, Point, MagX, MagY)) < 0){
    fprintf(stderr, "Open Error: %s\n", FontName);
    fprintf(stderr, "VFlib error code: %d\n", vf_error);
    exit(1);
  }

  printf(";; OUTLINES OF FONT ENTRY %s\n\n", FontName);
  
  while (Ch < argc){
    sscanf(argv[Ch], "%li", &CharCode);
    Ch++;

    /* GET VECTOR FONT DATA */
    if ((VFData = VF_GetOutline(Fd, CharCode, 1, 1)) == NULL){
      printf(";; CAN'T GET OUTLINE FOR CHARACTER 0x%lX OF FONT %s\n",
	     CharCode, FontName);
      printf(";; VFlib error code: %d\n", vf_error);
      continue;
    }

    PrintCCode(CharCode);
    if (HexDump == 0){
      /* DISASSEMBLE IT */
      DisVFData(VFData, Fd);
    } else {
      /* Hex Dump */
      for (i = 0; ; i++){
	printf("%04x   %08lx\n", i, (long)VFData[i]);
	if ((i >= VF_OL_OUTLINE_HEADER_SIZE_TYPE0) && (VFData[i] == 0))
	  break;
      }
    }

    /* RELEASE OUTLINE */
    VF_FreeOutline(VFData);
  }

  printf("END\n");

  /* CLOSE THE FONT */
  VF_CloseFont(Fd);

  return 0;
}

void
usage()
{
  printf("vfldisol  --- disassemble outline data\n");
  printf("Usage vfldisol [options] font code1 code2 ...\n");
  printf("Options: \n");
  printf("  -v VFLIBCAP    : vflibcap absolute path.\n");	
  printf("  -d DPI         : device resolution in dpi.\n");	
  printf("  -p POINT       : character point size.\n");	
  printf("  -x             : hex dump instead of disas.\n");	
  printf("Example 1: vfldisol -f timR24.pcf 0x67 0x68 0x69\n");
  printf("Example 2: vfldisol -f goth 0x2124\n");
  exit(0);
}



/*
 * Disassemble Vector Font Data returned by VF_GetOutline()
 */ 

void
DisVFData(VF_OUTLINE vfdata, int fd)
{
  long          cmd, *ptr;
  unsigned int  x, y;
  int           m;

  if (vfdata == NULL)
    return;
  PrintHeaderI("FORMAT    ", vfdata[VF_OL_HEADER_INDEX_HEADER_TYPE]);
  PrintHeaderI("DATA_SIZE ", vfdata[VF_OL_HEADER_INDEX_DATA_SIZE]);
  PrintHeaderR("DPI_X     ", vfdata[VF_OL_HEADER_INDEX_DPI_X]);
  PrintHeaderR("DPI_Y     ", vfdata[VF_OL_HEADER_INDEX_DPI_Y]);
  PrintHeaderR("POINT_SIZE", vfdata[VF_OL_HEADER_INDEX_POINT_SIZE]);
  PrintHeaderI("EM        ", vfdata[VF_OL_HEADER_INDEX_EM]);
  PrintHeaderI("MAX_X     ", vfdata[VF_OL_HEADER_INDEX_MAX_X]);
  PrintHeaderI("MAX_Y     ", vfdata[VF_OL_HEADER_INDEX_MAX_Y]);
  PrintHeaderI("REF_X     ", vfdata[VF_OL_HEADER_INDEX_REF_X]);
  PrintHeaderI("REF_Y     ", vfdata[VF_OL_HEADER_INDEX_REF_Y]);
  PrintHeaderI("MV_X      ", vfdata[VF_OL_HEADER_INDEX_MV_X]);
  PrintHeaderI("MV_Y      ", vfdata[VF_OL_HEADER_INDEX_MV_Y]);

  ptr = &vfdata[VF_OL_OUTLINE_HEADER_SIZE_TYPE0];
  do {
    m = 0;
    cmd = *ptr;
    ptr++; 
    if (cmd == 0L){
      PrintEnd();
      EmptyLine();
    } else if ((cmd & VF_OL_INSTR_TOKEN) != 0){
      PrintToken();
      if ((cmd & VF_OL_INSTR_CHAR) != 0)
	Print(&m, "CH");
      if ((cmd & VF_OL_INSTR_CWCURV) != 0)
	Print(&m, "C1");
      if ((cmd & VF_OL_INSTR_CCWCURV) != 0)
	Print(&m, "C2");
      if ((cmd & VF_OL_INSTR_LINE) != 0)
	Print(&m, "LI");
      if ((cmd & VF_OL_INSTR_ARC) != 0)
	Print(&m, "AR");
      if ((cmd & VF_OL_INSTR_BEZ) != 0)
	Print(&m, "BE");
      Newline();
    } else {
      x = VF_OL_GET_X(cmd);
      y = VF_OL_GET_Y(cmd);
      PrintXY(x, y);
    }
  } while (cmd != 0L);
}


void
EmptyLine(void)
{
  printf("\n");
}

void
PrintCCode(long n)
{
  printf("CHAR    ");
  printf("0x%lX\n", (long)n);
}

void
PrintToken(void)
{
  printf("        ");
  printf("TOKEN  ");
}

void
PrintHeaderI(char *label, long val)
{
  printf("        ");
  printf("%s  %ld\n", label, val);
}

void
PrintHeaderR(char *label, long val)
{
  printf("        ");
  printf("%s  %f\n", label, (double)VF_OL_HEADER_DECODE(val));
}

void
Print(int *mp, char *str)
{
  if (*mp == 0)
    printf("[");
  if (*mp > 0)
    printf(",");
  printf("%s", str);
  (*mp)++;
}

void
Newline(void)
{
  printf("]\n");
}

void
PrintEnd(void)
{
  printf("        ");
  printf("END     \n");
}

void
PrintXY(int x, int y)
{
  printf("        ");
  printf("XY  %d %d\n", x,y);
}

/*EOF*/
