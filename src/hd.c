/* 
 * hd.c - a hexadecimal dump 
 * by Hirotsugu Kakugawa
 *
 */
/*
 * Copyright (C) 1996-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * License: GPLv3 and FreeType Project License (FTL)
 *
 */


#include <stdio.h>
#include <stdlib.h>

#define SEPARETER  "  "
int hd(FILE*);


int
main(argc, argv)
     int argc;
     char **argv;
{
  if (argc > 1){
    if (strncmp(argv[1], "-h", 2) == 0){
      printf("hd --- hex dump\n");
      printf("Usage: hd [file ... ]\n");
      exit(0);
    }
    hd(fopen(argv[1], "rb"));
  } else 
    hd(stdin);

  return 0;
}
 
int hd(FILE *fp)
{
  int  c, addr, oaddr, i;
  char cstr[17];

  if (fp == NULL)
    exit(1);

  addr = 0;
  while ((c = getc(fp)) >= 0){
    if (addr % 256 == 0)
      printf("          +0 +1 +2 +3 +4 +5 +6 +7 +8 +9 +A +B +C +D +E +F\n");
    if (addr % 16 == 0)
      printf("%08x: ", addr);

    printf("%02x ", c);
    cstr[addr%16] = ((c <= 0x1f)||(0x7f <= c)) ? '.': (char)c;

    addr++;
    if (addr % 16 == 0){
      printf("%s", SEPARETER);
      for (i = 0; i < 16; i++)
	printf("%c", cstr[i]);
      putchar('\n');
    }
    if (addr % 256 == 0)
      putchar('\n');
  }

  addr = addr % 16;
  if (addr != 0){
    for (oaddr = addr; addr < 16; addr++)
      printf("%s", "   ");
    printf("%s", SEPARETER);
    for (i = 0; i < oaddr; i++)
      printf("%c", cstr[i]);
    putchar('\n');
  }

  return 0;
}

/*EOF*/
