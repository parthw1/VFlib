/* ekdump.c  - dump eKanji font in ASCII art form.
 * by Hirotsugu Kakugawa
 * 
 *  Copyright (C) 1999  Hirotsugu Kakugawa. All rights reserved. 
 *  See "COPYING" for distribution of this software. 
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#define PIXEL_SIZE      24
#define DOT_FOREGROUND  '*'
#define DOT_BACKGROUND  '.'

int   pixel_size = PIXEL_SIZE;
char  dot_fg = DOT_FOREGROUND;
char  dot_bg = DOT_BACKGROUND;


void  usage(void);
void  dump_char(long ch, unsigned char *buff);


int 
main(int argc, char** argv)
{
  unsigned char  buff[24*(24/8)];
  int   fd, dsize, need_close;
  long  ch;

  argc--; argv++;

  while ((argc > 0) && (argv[0][0] == '-')){
    if (strcmp(argv[0], "-24") == 0){
      pixel_size = 24;
    } else if (strcmp(argv[0], "-16") == 0){
      pixel_size = 16;
    } else if (strcmp(argv[0], "-fg") == 0){
      argc--; argv++;
      dot_fg = argv[0][0];
    } else if (strcmp(argv[0], "-bg") == 0){
      argc--; argv++;
      dot_bg = argv[0][0];
    } else if ((strcmp(argv[0], "-h") == 0)
	       || (strcmp(argv[0], "-help") == 0)
	       || (strcmp(argv[0], "--help") == 0) ){
      usage();
    }
    argc--; argv++;
  }
  
  dsize = pixel_size * ((pixel_size + 7) / 8);

  if (argc > 0){
    need_close = 1;
    if ((fd = open(argv[0], O_RDONLY)) < 0){
      fprintf(stderr, "Can't open %s\n", argv[0]);
      exit(0);
    }
    argc--; argv++;
  } else {
    need_close = 0;
    fd = 0;
  }

  if (argc > 0){
    while (argc > 0){
      sscanf(argv[0], "%li", &ch);
      if (ch < 1){
	fprintf(stderr, "No such character: %s\n", argv[0]);
	continue;
      }
      lseek(fd, (ch-1) * dsize, SEEK_SET);
      if (read(fd, buff, dsize) > 0){
	dump_char(ch, buff);
      } else {
	fprintf(stderr, "No such character: %s\n", argv[0]);
      }
      argc--; argv++;
    }
  } else {
    ch = 1;
    while (read(fd, buff, dsize) > 0){
      dump_char(ch, buff);
      ch++;
    }
  }

  if (need_close == 1){
    close(fd);
  }

  return 0;
}


void
usage(void)
{

  fprintf(stderr, "ekdump  - dump eKanji font in ASCII art form.\n");
  fprintf(stderr, "Usage: ekdump [option] [font_file [code...]]\n");
  fprintf(stderr, " options:\n");
  fprintf(stderr, "  -fg C  Foreground character [default: %c]\n",
	  DOT_FOREGROUND);
  fprintf(stderr, "  -bg C  Background character [default: %c]\n",
	  DOT_BACKGROUND);
  fprintf(stderr, "  -24    Dot size of the font file 24-dot (default)\n");
  fprintf(stderr, "  -16    Dot size of the font file 16-dot\n");
  fprintf(stderr, "  -help  Print help.\n");
  fprintf(stderr, "If character codes are not given, all characters\n");
  fprintf(stderr, "are dumped in a font file. If font file name is\n");
  fprintf(stderr, "not given, ekdump reads font file data from\n");
  fprintf(stderr, "standard input and all characters are dumped\n");
  fprintf(stderr, "in the input stream.\n");
  exit(0);
}

void
dump_char(long ch, unsigned char *buff)
{
  int  wb, i, j;
  static unsigned char  bits[] = { 0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01 };

  printf("Code %06d (0x%06x, Row %d Cel %d)\n", 
	 ch, ch, (ch-1)/94+1, (ch-1)%94 + 1);

  wb = (pixel_size + 7) / 8; 
  for (i = 0; i < pixel_size; i++){
    for (j = 0; j < pixel_size; j++){
      if ((buff[i*wb + (j/8)] & bits[j%8]) != 0){
	putchar(dot_fg);
      } else {
	putchar(dot_bg);
      }
    }
    putchar('\n');
  }
  putchar('\n');
}


/*EOF*/
