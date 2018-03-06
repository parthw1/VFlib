/*
 * mkcscv2x.c - make code conversion table
 * by Hirotsugu Kakugawa
 *
 *  28 Jul 1997
 */
/*
 * Copyright (C) 1997 Hirotsugu Kakugawa. 
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  
 */


#include  <stdio.h>
#include  <stdlib.h>

#define  UNDEF  -1L
#define  BASE    128

#define TABLE_SIZE      20000
struct conv_table {
  long  code1;
  long  code2;
};
int    block_size = 256;
int    compact = 0;

void conv(struct conv_table*, FILE*, char **);

int
main(int argc, char **argv)
{
  struct conv_table *cvtbl;

  argv++;
  argc--;

  compact = 0;
  if ((argc >= 1) && (strcmp(argv[0], "-c") == 0)){
    compact = 1;
    argc--;
    argv++;
  }

  if (argc < 4){
    fprintf(stderr, 
	    "Usage: mkcscv [-c] %s [block size]\n", 
	    "CS1-NAME CS1-ENC CS2-NAME CS2-ENC");
    exit(1);
  }

  fprintf(stderr, "*** Making CCV file: (%s, %s) => (%s, %s)\n", 
	  argv[0], argv[1], argv[2], argv[3]); 

  if (argc >= 5)
    block_size = atoi(argv[5]);

  cvtbl = (struct conv_table*)calloc(TABLE_SIZE+1, sizeof(struct conv_table));
  if (cvtbl == NULL){
    fprintf(stderr, "No memory\n");
    exit(1);
  }
  
  conv(cvtbl, stdin, argv);

  return 0;
}

void conv(struct conv_table* cvtbl, FILE* fp, char **argv)
{
  int   c1min, c1max, c2min, c2max, c1, c2, blocks;
  int   index, print_block, i, t;
  char  line[BUFSIZ];
  
  for (i = 0; i < TABLE_SIZE; i++){ 
    cvtbl[i].code1 = UNDEF;
    cvtbl[i].code2 = UNDEF;
  }

  /* Input must be in code order. */
  c1min = 0xffff;        c1max = 0x00;
  c2min = block_size-1;  c2max = 0x00;
  for (i = 0; ; i++){
    if (fgets(line, sizeof(line), fp) == NULL)
      break;
    sscanf(line, "%li%li", &cvtbl[i].code1, &cvtbl[i].code2);
    c1 = cvtbl[i].code1 / block_size;
    c2 = cvtbl[i].code1 % block_size;
    if (c1min > c1)
      c1min = c1;
    if (c1max < c1)
      c1max = c1;
    if (c2min > c2)
      c2min = c2;
    if (c2max < c2)
      c2max = c2;
  }

  printf("; Conversion table: %s ==> %s\n", argv[0], argv[2]);
  printf("(charset-external-name %s)\n", argv[0]);
  printf("(charset-external-encoding %s)\n", argv[1]);
  printf("(charset-internal-name %s)\n", argv[2]);
  printf("(charset-internal-encoding %s)\n", argv[3]);
  if (compact == 0)
    printf("(table-type array)\n");
  else
    printf("(table-type random-arrays)\n");
  printf("; Code point C is converted to C' by the following formula:\n");
  printf(";   C' = Table[(c1 - c1min)*M + (c2 - c2min)],\n");
  printf(";   where c1 = C/B and c2 = C%%B, and M = c2max - c2min + 1.\n");
  printf(";   B is a block size given by the 'block-size:' parameter.\n");
  printf("(c1-min 0x%x)\n", c1min);
  printf("(c1-max 0x%x)\n", c1max);
  printf("(c2-min 0x%x)\n", c2min);
  printf("(c2-max 0x%x)\n", c2max);
  printf("(block-size %d)\n", block_size);

  if (compact == 0){
    printf("(nblocks %d)\n", c1max-c1min+1);
  } else {
    blocks = 0;
    index = 0;
    for (c1 = c1min; c1 <= c1max; c1++){
      print_block = 0;
      for (c2 = c2min; c2 <= c2max; c2++){
	if (cvtbl[index].code1 == c1*block_size+c2){
	  if (print_block == 0)
	    blocks++;
	  print_block = 1;
	  index++;
	}
      }
    }
    printf("(nblocks %d)\n", blocks);
  }

  index = 0;
  for (c1 = c1min; c1 <= c1max; c1++){
    if (compact == 0){
      print_block = 1;
    } else {
      print_block = 0;
      for (c2 = c2min; c2 <= c2max; c2++){
	if (cvtbl[index].code1 == c1*block_size+c2){
	  print_block = 1;
	  break;
	}
      }
    }
    if (print_block == 1){
      printf("; 0x%04x ... 0x%04x\n", 
	     c1*block_size+c2min, c1*block_size+c2max);
      printf("(block %d", c1-c1min);
      t = 0;
      for (c2 = c2min; c2 <= c2max; c2++){
	if ((t % 8) == 0)
	  printf("\n    ");
	if (cvtbl[index].code1 == c1*block_size+c2)
	  printf("0x%04lx ", cvtbl[index++].code2);
	else 
	  printf("-1     ");
	t++;
      }
      printf(")\n");
    }
  }
}

/*EOF*/
