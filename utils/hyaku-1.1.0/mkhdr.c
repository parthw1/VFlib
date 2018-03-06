/* mkhdr.c
 *
 * Copyright (C) 1998-2017 Hirotsugu Kakugawa. 
 * All rights reserved.
 * License: GPLv3 and FreeType Project License (FTL)
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>


long  txtbuff[7][BUFSIZ];

void  mk_hdr(void);
int   get_line(void);
void  parseit(unsigned char*);
void  print_poem_i(int n);
void  dumpit(void);


int
main(int argc, char **argv)
{
  mk_hdr();

  return 0;
}

void
mk_hdr(void)
{
  int  n, i;

  printf("
struct s_poem {
  long phase1[8];
  long phase2[10];
  long phase3[8];
  long phase4[10];
  long phase5[10];
  long auth[32];
};
typedef  struct s_poem  *POEM;\n\n");

  printf("struct s_poem poem_table[] = {\n");

  for (i = 1; i <= 100; i++){
    while ((n = get_line()) != 7){
      if (n < 0) 
	break;
    }
    printf("  /* %d */\n", i);
    printf("  {");
    print_poem_i(2);    printf("\n   ");
    print_poem_i(3);    printf("\n   ");
    print_poem_i(4);    printf("\n   ");
    print_poem_i(5);    printf("\n   ");
    print_poem_i(6);    printf("\n   ");
    print_poem_i(1);
    printf("},\n");
    while ((n = get_line()) != 7){
      if (n < 0) 
	break;
    }
  }

  printf("};\n");
}

int
get_line(void)
{
  int  i, j, x;
  int  ch;
  long c;

  for (i = 0; i < 7; i++)
    txtbuff[i][0] = 0;

  for (i = 0; i < 7; i++){
    for (;;){
      if ((ch = getc(stdin)) < 0){
	txtbuff[i][0] = 0;
	return i-1;
      }
      if (!isspace(ch) || (ch == '\n')){
	ungetc(ch, stdin);
	break;
      }
    }
    j = 0;
    for (;;){
      if ((ch = getc(stdin)) < 0)
	return i;
      if (ch == '\n'){
	txtbuff[i][j] = 0;
	return i+1;
      }
      if (isspace(ch)){
	txtbuff[i][j] = 0;
	break;
      }
      if ((c = ch) > 0x80){
	ch = getc(stdin); 
	c = (c & 0x7f) * 256 + (ch & 0x7f);
      }
      if (c != '#'){
	txtbuff[i][j] = c;
      } else {
	txtbuff[i][j] = 0;
	for (;;){
	  if (((ch = getc(stdin)) < 0) || (ch == '\n'))
	    return i;
	}
      }
      j++;
    }
    txtbuff[i][j] = 0;
  }
  return i;
}

void
print_poem_i(int n)
{
  int  i, j;
  
  printf("{");
  for (j = 0; txtbuff[n][j] != 0; j++){
    printf("%d,", txtbuff[n][j]);
  }
  printf("0},"); 
}

void 
dumpit(void)
{
  int  i, j;
  
  for (i = 0; i < 7; i++){
    for (j = 0; txtbuff[i][j] != 0; j++){
      if (txtbuff[i][j] < 0x80){
	printf("%c", txtbuff[i][j]);
      } else {
	printf("%c%c", 0x80 | txtbuff[i][j]/256, 0x80 | txtbuff[i][j]);
      }
    }
    printf(" "); 
  }
  printf("\n"); 
}

