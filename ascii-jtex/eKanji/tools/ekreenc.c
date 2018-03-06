/* ekreenc.c  - reencode an eKanji font.
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

int   pixel_size = PIXEL_SIZE;
int   ifd, ofd;

void   usage(void);
char  *x_index(char *p, char ch);
void   x_memclr(unsigned char *buff, int dsize);



int 
main(int argc, char** argv)
{
  FILE  *efp;
  unsigned char  buff[24*(24/8)];
  char   line[1024];
  char  *efile, *ifile, *ofile, *s;
  int   dsize, i_need_close, o_need_close;
  long  ich, och, och_last;

  efile = ifile = ofile = NULL;

  argc--; argv++;

  while ((argc > 0) && (argv[0][0] == '-')){
    if (strcmp(argv[0], "-24") == 0){
      pixel_size = 24;
    } else if (strcmp(argv[0], "-16") == 0){
      pixel_size = 16;
    } else if (strcmp(argv[0], "-i") == 0){
      argc--; argv++;
      ifile = argv[0];
    } else if (strcmp(argv[0], "-o") == 0){
      argc--; argv++;
      ofile = argv[0];
    } else if (strcmp(argv[0], "-e") == 0){
      argc--; argv++;
      efile = argv[0];
    } else if ((strcmp(argv[0], "-h") == 0)
	       || (strcmp(argv[0], "-help") == 0)
	       || (strcmp(argv[0], "--help") == 0) ){
      usage();
    }
    argc--; argv++;
  }
  
  dsize = pixel_size * ((pixel_size + 7) / 8);

  efp = NULL;
  if (efile != NULL){
    if ((efp = fopen(efile, "r")) < 0){
      fprintf(stderr, "Can't open reeocoding table file: %s\n", ifile);
      exit(0);
    }
  } else {
    fprintf(stderr, "ekreenc: No reencoding table file.\n");
    usage();
  }
  
  if (ifile != NULL){
    i_need_close = 1;
    if ((ifd = open(ifile, O_RDONLY)) < 0){
      fprintf(stderr, "Can't open input font file: %s\n", ifile);
      exit(0);
    }
  } else {
    i_need_close = 0;
    ifd = 0;
  }
  
  if (ofile != NULL){
    o_need_close = 1;
    if ((ofd = creat(ofile, 0644)) < 0){
      fprintf(stderr, "Can't open output font file: %s\n", ofile);
      exit(0);
    }
  } else {
    o_need_close = 0;
    ofd = 1;
  }
  
  och_last = 0;
  while (fgets(line, sizeof(line), efp) != NULL){
    if ((s = x_index(buff, '#')) != NULL)
      *s = '\0';
    if (sscanf(line, "%li%li", &och, &ich) != 2)
      continue;
    if (och_last < (och-1)){
      x_memclr(buff, dsize);
      while (och_last < (och-1)){
	write(ofd, buff, dsize); 
	och_last++;
      }
    }
    if (ich > 0){
      lseek(ifd, (ich-1) * dsize, SEEK_SET);
      if (read(ifd, buff, dsize) > 0){
	write(ofd, buff, dsize);
      } else {
	fprintf(stderr, "No such character: %s\n", argv[0]);
      }
    } else {
      x_memclr(buff, dsize);
      write(ofd, buff, dsize);      
    }
    och_last = och;
    och++;
  }

  if (i_need_close == 1){
    close(ifd);
  }
  if (o_need_close == 1){
    close(ofd);
  }
  fclose(efp);

  return 0;
}


void 
usage(void)
{
  fprintf(stderr, "ekreenc  - reencode an eKanji font file.\n");
  fprintf(stderr, "Usage: ekreenc -e FILE [option]\n");
  fprintf(stderr, " options:\n");
  fprintf(stderr, "  -e FILE  Reencoding table file\n");
  fprintf(stderr, "  -i FILE  Input font file (stdin if not given)\n");
  fprintf(stderr, "  -o FILE  Output font file (stdout if not given)\n");
  fprintf(stderr, "  -24      Dot size of the font file 24-dot (default)\n");
  fprintf(stderr, "  -16      Dot size of the font file 16-dot\n");
  fprintf(stderr, "  -help    Print help.\n");
  exit(0);
}


char*
x_index(char *p, char ch)
{
  while (*p != '\0'){
    if (*p == ch)
      return p;
    p++;
  }
  return NULL;
}

void
x_memclr(unsigned char *buff, int dsize)
{
  int  i;
  
  for (i = dsize; i > 0; i--){
    *buff = 0;
    buff++;
  }
}


/*EOF*/
